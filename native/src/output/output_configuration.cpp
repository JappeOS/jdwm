#include "zenith_output_manager.hpp"
#include "output_management.hpp"
#include "configuration_transaction.hpp"
#include "server.hpp"
#include "output.hpp"
#include "util/egl/gl_context_lock.hpp"
#include <unordered_set>
#include <GLES2/gl2.h>

extern "C" {
#define static
#include <wlr/backend.h>
#include <wlr/render/gles2.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/util/log.h>
#undef static
}

namespace zenith {
namespace {
struct BackendStates {
	std::vector<wlr_backend_output_state> states;

	explicit BackendStates(const OutputConfiguration& config, bool powered = true) : states(config.size()) {
		for (size_t i = 0; i < config.size(); ++i) {
			const auto& h = config[i];
			auto& s = states[i];
			s.output = h.output;
			wlr_output_state_init(&s.base);
			s.base.allow_reconfiguration = true;
			wlr_output_state_set_enabled(&s.base, h.enabled && powered);

			if (!h.enabled || !powered)
				continue;

			if (h.mode)
				wlr_output_state_set_mode(&s.base, h.mode);
			else
				wlr_output_state_set_custom_mode(&s.base, h.width, h.height, h.refresh);

			wlr_output_state_set_scale(&s.base, h.scale);
			wlr_output_state_set_transform(&s.base, static_cast<wl_output_transform>(h.transform));
			wlr_output_state_set_adaptive_sync_enabled(&s.base, h.adaptive_sync);
		}
	}

	~BackendStates() {
		for (auto& state : states)
			wlr_output_state_finish(&state.base);
	}

	BackendStates(const BackendStates&) = delete;
	BackendStates& operator=(const BackendStates&) = delete;

	bool test(wlr_backend* backend) {
		egl::GlContextGuard guard;
		return states.empty() || wlr_backend_test(backend, states.data(), states.size());
	}

	bool commit(wlr_backend* backend) {
		egl::GlContextGuard guard;
		return states.empty() || wlr_backend_commit(backend, states.data(), states.size());
	}
};
}

OutputSettings ZenithOutputManager::capture_output(ZenithOutput* output) const {
	auto* o = output->wlr_output;
	OutputSettings result{o, o->enabled, o->current_mode, o->width, o->height, o->refresh,
		0, 0, o->scale, int(o->transform), o->adaptive_sync_status == WLR_OUTPUT_ADAPTIVE_SYNC_ENABLED};

	if (auto previous = configured_.find(o); previous != configured_.end()) {
		result.x = previous->second.x; result.y = previous->second.y;
	}

	if (auto* layout = wlr_output_layout_get(server_->output_layout, o)) {
		result.x = layout->x; result.y = layout->y;
	}

	return result;
}

OutputConfiguration ZenithOutputManager::configuration() const {
	OutputConfiguration result;
	for (const auto& output : server_->outputs) {
		auto it = configured_.find(output->wlr_output);
		result.push_back(it == configured_.end() ? capture_output(output.get()) : it->second);
	}
	return result;
}

bool ZenithOutputManager::configure(const OutputConfiguration& proposal, bool test_only) const {
	if (applying_ || server_->is_shutting_down() || proposal.size() != server_->outputs.size())
		return false;

	std::unordered_set<wlr_output*> seen;
	for (const auto& h : proposal) {
		if (!h.output || !seen.insert(h.output).second ||
			std::none_of(server_->outputs.begin(), server_->outputs.end(),
				[&](const auto& o) { return o->wlr_output == h.output; })) {
			return false;
		}

		if (h.mode) {
			bool found = false;
			wlr_output_mode* mode;
			wl_list_for_each(mode, &h.output->modes, link)
			if (mode == h.mode)
				found = true;
			if (!found || h.width != h.mode->width || h.height != h.mode->height || h.refresh != h.mode->refresh)
				return false;
		}
	}

	GLint max_dimension = 0;
	{
		egl::GlContextGuard guard;
		wlr_egl_make_current(wlr_gles2_renderer_get_egl(server_->renderer), nullptr);
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_dimension);
		GLint renderbuffer_limit = 0;
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &renderbuffer_limit);
		max_dimension = std::min(max_dimension, renderbuffer_limit);
	}

	DesktopGeometry geometry;
	if (!configuration_geometry(proposal, server_->display_scale, max_dimension, geometry))
		return false;

	// Test the intended configuration even while temporarily blanked.
	BackendStates intended(proposal);
	wlr_output* format = nullptr;
	for (const auto& head : proposal) {
		if (head.enabled && (!format || (server_->output && head.output == server_->output->wlr_output)))
			format = head.output;
	}

	std::shared_ptr<SwapChain<wlr_gles2_buffer>> prepared;

	OutputConfiguration old_actual;
	for (const auto& output : server_->outputs)
		old_actual.push_back(capture_output(output.get()));

	BackendStates rollback(old_actual), desired(proposal, display_enabled_);
	auto next_configured = configured_;
	for (const auto& head : proposal)
		next_configured[head.output] = head;

	const auto result = run_configuration_transaction(test_only,
		[&] { return intended.test(server_->backend); },
		[&] {
			if (format)
				prepared = create_output_swap_chain(format, geometry.pixel_width, geometry.pixel_height);
			return !format || prepared != nullptr;
		},
		[&] {
			applying_ = true;
			return desired.commit(server_->backend);
		},
		[&] { return rollback.commit(server_->backend); });

	if (result == ConfigurationResult::Rejected)
		return false;
	if (result == ConfigurationResult::Tested)
		return true;

	const bool committed = result == ConfigurationResult::Applied;
	if (!committed) {
		const bool restored = result == ConfigurationResult::Restored;
		wlr_log(WLR_ERROR, "zenith: output configuration commit failed; rollback %s", restored ? "succeeded" : "failed");
		if (!restored) {
			for (const auto& output : server_->outputs) {
				configured_[output->wlr_output] = capture_output(output.get());
				wlr_log(WLR_ERROR, "zenith: recovered output '%s' enabled=%d size=%dx%d",
					output->wlr_output->name, output->wlr_output->enabled, output->wlr_output->width, output->wlr_output->height);
			}
		}
	} else {
		// Refresh=0 requests backend selection for a custom mode. Advertise
		// the resulting rate rather than keeping the request's placeholder.
		if (display_enabled_) {
			for (auto& [output, settings] : next_configured) {
				if (settings.enabled)
					settings.refresh = output->refresh;
			}
		}
		configured_.swap(next_configured);
		prepared_target_ = std::move(prepared);
		// Even unchanged framebuffer dimensions may have a new coordinate map.
		composition_mapping_.clear();
	}

	applying_ = false;
	reconcile();
	return committed;
}

bool ZenithOutputManager::request_state(ZenithOutput* output, const wlr_output_state& state) const {
	auto proposal = configuration();
	for (auto& h : proposal) {
		if (h.output != output->wlr_output)                           continue;
		if (state.committed & WLR_OUTPUT_STATE_ENABLED)               h.enabled = state.enabled;
		if (state.committed & WLR_OUTPUT_STATE_SCALE)                 h.scale = state.scale;
		if (state.committed & WLR_OUTPUT_STATE_TRANSFORM)             h.transform = int(state.transform);
		if (state.committed & WLR_OUTPUT_STATE_ADAPTIVE_SYNC_ENABLED) h.adaptive_sync = state.adaptive_sync_enabled;
		if (state.committed & WLR_OUTPUT_STATE_MODE) {
			h.mode = state.mode_type == WLR_OUTPUT_STATE_MODE_FIXED ? state.mode : nullptr;
			h.width = h.mode ? h.mode->width : state.custom_mode.width;
			h.height = h.mode ? h.mode->height : state.custom_mode.height;
			h.refresh = h.mode ? h.mode->refresh : state.custom_mode.refresh;
		}
	}
	return configure(proposal, false);
}

void ZenithOutputManager::reconcile() const {
	if (applying_ || server_->is_shutting_down())
		return;

	if (!server_->output || !configured_.count(server_->output->wlr_output) ||
		!configured_.at(server_->output->wlr_output).enabled) {
		server_->output.reset();
		for (const auto& output : server_->outputs) {
			if (configured_.count(output->wlr_output) && configured_.at(output->wlr_output).enabled) {
				server_->output = output;
				break;
			}
		}
	}

	for (const auto& output : server_->outputs) {
		auto* o = output->wlr_output;
		auto it = configured_.find(o);

		if (o->enabled && it != configured_.end()) {
			auto* layout = wlr_output_layout_add(server_->output_layout, o, it->second.x, it->second.y);
			if (layout && output->scene_output)
				wlr_scene_output_layout_add_output(server_->scene_output_layout, layout, output->scene_output);
		} else {
			wlr_output_layout_remove(server_->output_layout, o);
			if (output->schedule_frame_timer)
				wl_event_source_timer_update(output->schedule_frame_timer, 0);
			if (output->vsync_timer)
				wl_event_source_timer_update(output->vsync_timer, 0);
		}

		if (output->scene_buffer)
			wlr_scene_node_set_enabled(&output->scene_buffer->node, o->enabled);
		output->last_frame_commit_ns = 0;
	}

	update_scene_node_positions(server_);
	ensure_composition_target();
	send_virtual_desktop_metrics(server_);
	refresh_xwayland_workareas();

	if (server_->embedder_state)
		server_->embedder_state->publish_monitor_layout();

	if (server_->output_management)
		server_->output_management->publish();

	if (server_->pointer && current_render_output()) {
		wlr_cursor_warp_closest(server_->pointer->cursor, nullptr, server_->pointer->cursor->x, server_->pointer->cursor->y);
	}

	schedule_compositor_frame();
}

void ZenithOutputManager::layout_to_flutter(double& x, double& y) const {
	wlr_box box{};
	wlr_output_layout_get_box(server_->output_layout, nullptr, &box);
	x = (x - box.x) * server_->display_scale;
	y = (y - box.y) * server_->display_scale;
}
} // namespace zenith
