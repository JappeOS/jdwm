#include "output_management.hpp"
#include "zenith_output_manager.hpp"
#include "server.hpp"
#include <memory>
#include <algorithm>
#include <stdexcept>

extern "C" {
#define static
#include <wlr/types/wlr_output_management_v1.h>
#undef static
}

namespace zenith {
OutputManagement::OutputManagement(ZenithServer* server) : server_(server) {
	protocol_ = wlr_output_manager_v1_create(server->display);
	if (!protocol_)
		throw std::runtime_error("Failed to create output management protocol");

	apply_.notify = apply;
	test_.notify = test;
	destroy_.notify = destroyed;
	wl_signal_add(&protocol_->events.apply, &apply_);
	wl_signal_add(&protocol_->events.test, &test_);
	wl_signal_add(&protocol_->events.destroy, &destroy_);
}

OutputManagement::~OutputManagement() {
	if (protocol_) {
		wl_list_remove(&apply_.link);
		wl_list_remove(&test_.link);
		wl_list_remove(&destroy_.link);
	}
}

void OutputManagement::destroyed(wl_listener* listener, void*) {
	OutputManagement* self = wl_container_of(listener, self, destroy_);
	wl_list_remove(&self->apply_.link);
	wl_list_remove(&self->test_.link);
	wl_list_remove(&self->destroy_.link);
	self->protocol_ = nullptr;
}

void OutputManagement::apply(wl_listener* listener, void* data) {
	OutputManagement* self = wl_container_of(listener, self, apply_);
	self->request(static_cast<wlr_output_configuration_v1*>(data), false);
}

void OutputManagement::test(wl_listener* listener, void* data) {
	OutputManagement* self = wl_container_of(listener, self, test_);
	self->request(static_cast<wlr_output_configuration_v1*>(data), true);
}

void OutputManagement::request(wlr_output_configuration_v1* config, bool test_only) {
	std::unique_ptr<wlr_output_configuration_v1, decltype(&wlr_output_configuration_v1_destroy)>
		owned(config, wlr_output_configuration_v1_destroy);
	OutputConfiguration proposal;
	wlr_output_configuration_head_v1* head;

	wl_list_for_each(head, &config->heads, link) {
		const auto& h = head->state;

		// disable_head carries no mode/position/scale. Retain the last settings
		// so a later enable or power-cycle doesn't lose the user's layout.
		if (!h.enabled) {
			const auto current = server_->output_manager->configuration();
			auto previous = std::find_if(current.begin(), current.end(),
				[&](const auto& item) { return item.output == h.output; });
			if (previous != current.end()) {
				auto disabled = *previous;
				disabled.enabled = false;
				proposal.push_back(disabled);
				continue;
			}
		}

		proposal.push_back({h.output, h.enabled, h.mode,
			h.mode ? h.mode->width : h.custom_mode.width,
			h.mode ? h.mode->height : h.custom_mode.height,
			h.mode ? h.mode->refresh : h.custom_mode.refresh,
			h.x, h.y, h.scale, int(h.transform), h.adaptive_sync_enabled});
	}

	if (!server_->is_shutting_down() && server_->output_manager->configure(proposal, test_only))
		wlr_output_configuration_v1_send_succeeded(config);
	else
		wlr_output_configuration_v1_send_failed(config);
}

void OutputManagement::publish() {
	if (!protocol_ || server_->is_shutting_down())
		return;

	auto* config = wlr_output_configuration_v1_create();
	if (!config)
		return;

	for (const auto& settings : server_->output_manager->configuration()) {
		auto* head = wlr_output_configuration_head_v1_create(config, settings.output);
		if (!head) {
			wlr_output_configuration_v1_destroy(config);
			return;
		}

		auto& h = head->state;
		h.enabled = settings.enabled;
		h.mode = settings.mode;
		h.custom_mode = {settings.width, settings.height, settings.refresh};
		h.x = settings.x; h.y = settings.y;
		h.scale = settings.scale;
		h.transform = static_cast<wl_output_transform>(settings.transform);
		h.adaptive_sync_enabled = settings.adaptive_sync;
	}

	wlr_output_manager_v1_set_configuration(protocol_, config);
}
} // namespace zenith
