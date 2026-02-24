#include "zenith_xdg_toplevel.hpp"
#include "server.hpp"
#include "zenith_toplevel_decoration.hpp"

extern "C" {
#include <wlr/util/log.h>
}

ZenithXdgToplevel::ZenithXdgToplevel(wlr_xdg_toplevel* xdg_toplevel,
                                     std::shared_ptr<ZenithXdgSurface> zenith_xdg_surface)
	  : xdg_toplevel{xdg_toplevel}, zenith_xdg_surface(std::move(zenith_xdg_surface)) {

	// In wlroots 0.19+, we cannot call set_size/set_maximized before the
	// surface's initial commit. Defer to the commit/map handlers instead.
	auto* server = ZenithServer::instance();
	pending_maximize = server->start_windows_maximized;

	destroy.notify = zenith_xdg_toplevel_destroy;
	wl_signal_add(&xdg_toplevel->events.destroy, &destroy);

	commit.notify = zenith_xdg_toplevel_commit;
	wl_signal_add(&xdg_toplevel->base->surface->events.commit, &commit);

	wlr_log(WLR_INFO, "zenith: Created xdg_toplevel, pending_maximize=%d", pending_maximize);

	request_fullscreen.notify = zenith_xdg_toplevel_request_fullscreen;
	wl_signal_add(&xdg_toplevel->events.request_fullscreen, &request_fullscreen);

	request_maximize.notify = zenith_xdg_toplevel_request_maximize;
	wl_signal_add(&xdg_toplevel->events.request_maximize, &request_maximize);

	request_minimize.notify = zenith_xdg_toplevel_request_minimize;
	wl_signal_add(&xdg_toplevel->events.request_minimize, &request_minimize);

	set_app_id.notify = zenith_xdg_toplevel_set_app_id;
	wl_signal_add(&xdg_toplevel->events.set_app_id, &set_app_id);

	set_title.notify = zenith_xdg_toplevel_set_title;
	wl_signal_add(&xdg_toplevel->events.set_title, &set_title);

	request_move.notify = zenith_xdg_toplevel_request_move;
	wl_signal_add(&xdg_toplevel->events.request_move, &request_move);

	request_resize.notify = zenith_xdg_toplevel_request_resize;
	wl_signal_add(&xdg_toplevel->events.request_resize, &request_resize);
}

ZenithXdgToplevel::~ZenithXdgToplevel() {
	// commit is attached to wlr_surface::events.commit and must be removed
	// before wl_surface destruction assertions run.
	wl_list_remove(&commit.link);
}

void zenith_xdg_toplevel_destroy(wl_listener* listener, void* data) {
	ZenithXdgToplevel* zenith_xdg_toplevel = wl_container_of(listener, zenith_xdg_toplevel, destroy);

	// wlroots emits toplevel->events.destroy and then asserts all listener
	// lists are empty, so detach every toplevel listener here.
	wl_list_remove(&zenith_xdg_toplevel->request_fullscreen.link);
	wl_list_remove(&zenith_xdg_toplevel->request_maximize.link);
	wl_list_remove(&zenith_xdg_toplevel->request_minimize.link);
	wl_list_remove(&zenith_xdg_toplevel->request_move.link);
	wl_list_remove(&zenith_xdg_toplevel->request_resize.link);
	wl_list_remove(&zenith_xdg_toplevel->set_app_id.link);
	wl_list_remove(&zenith_xdg_toplevel->set_title.link);
	wl_list_remove(&zenith_xdg_toplevel->destroy.link);
}

void zenith_xdg_toplevel_commit(wl_listener* listener, void* data) {
	ZenithXdgToplevel* zenith_xdg_toplevel = wl_container_of(listener, zenith_xdg_toplevel, commit);
	wlr_xdg_surface* xdg_surface = zenith_xdg_toplevel->xdg_toplevel->base;
	wlr_log(WLR_INFO, "zenith: toplevel commit, initial_commit=%d, initialized=%d, configured=%d",
	        xdg_surface->initial_commit, xdg_surface->initialized, xdg_surface->configured);
	if (!xdg_surface->configured) {
		// Apply any pending decoration mode before the initial configure.
		// Clients like Chromium send request_mode before the first commit, so
		// request_mode_handle defers to here to avoid the initialized assertion.
		auto* server = ZenithServer::instance();
		size_t id = zenith_xdg_toplevel->zenith_xdg_surface->zenith_surface->id;
		auto it = server->toplevel_decorations.find(id);
		if (it != server->toplevel_decorations.end()) {
			auto* deco = it->second->wlr_toplevel_decoration;
			wlr_xdg_toplevel_decoration_v1_mode mode = deco->requested_mode;
			wlr_xdg_toplevel_decoration_v1_set_mode(deco,
				mode != WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_NONE
					? mode
					: WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
		}
		// When an xdg_surface performs an initial commit, the compositor must
		// reply with a configure so the client can map the surface.
		wlr_log(WLR_INFO, "zenith: Scheduling initial configure for toplevel");
		wlr_xdg_surface_schedule_configure(xdg_surface);
	}
}

void ZenithXdgToplevel::focus(bool focus) const {
	auto* server = ZenithServer::instance();
	wlr_seat* seat = server->seat;
	wlr_xdg_surface* xdg_surface = xdg_toplevel->base;
	wlr_surface* surface = xdg_surface->surface;
	wl_client* client = wl_resource_get_client(xdg_surface->resource);

	if (focus) {
		wlr_surface* prev_surface = seat->keyboard_state.focused_surface;
		bool is_surface_already_focused = prev_surface == surface;
		if (is_surface_already_focused) {
			return;
		}
		// Activate the new surface.
		if (xdg_surface->initialized) {
			wlr_xdg_toplevel_set_activated(xdg_surface->toplevel, true);
		}
		/*
		 * Tell the seat to have the keyboard enter this surface. wlroots will keep
		 * track of this and automatically send key events to the appropriate
		 * clients without additional work on your part.
		 */
		wlr_keyboard* keyboard = wlr_seat_get_keyboard(seat);
		if (keyboard != nullptr) {
			wlr_seat_keyboard_notify_enter(seat, xdg_surface->surface,
			                               keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);

			for (auto& text_input: server->text_inputs) {
				if (wl_resource_get_client(text_input->wlr_text_input->resource) == client &&
				    text_input->wlr_text_input->focused_surface != xdg_surface->surface) {
					text_input->enter(xdg_surface->surface);
				}
			}
		}
	} else {
		for (auto& text_input: server->text_inputs) {
			if (wl_resource_get_client(text_input->wlr_text_input->resource) == client) {
				text_input->leave();
			}
		}
		wlr_seat_keyboard_notify_clear_focus(seat);
		if (xdg_toplevel->base->initialized) {
			wlr_xdg_toplevel_set_activated(xdg_toplevel, false);
		}
	}
}

void ZenithXdgToplevel::maximize(bool value) const {
	if (xdg_toplevel->base->initialized) {
		if (value) {
			auto* server = ZenithServer::instance();
			wlr_xdg_toplevel_set_size(
				xdg_toplevel,
				server->max_window_size.width,
				server->max_window_size.height
			);
		}
		wlr_xdg_toplevel_set_maximized(xdg_toplevel, value);
	}
}

void ZenithXdgToplevel::resize(size_t width, size_t height) const {
	if (xdg_toplevel->base->initialized) {
		wlr_xdg_toplevel_set_size(xdg_toplevel, width, height);
	}
}

void zenith_xdg_toplevel_request_fullscreen(wl_listener* listener, void* data) {
	ZenithXdgToplevel* zenith_xdg_toplevel = wl_container_of(listener, zenith_xdg_toplevel, request_fullscreen);
	wlr_xdg_toplevel* toplevel = zenith_xdg_toplevel->xdg_toplevel;
	if (toplevel->base->initialized) {
		wlr_xdg_toplevel_set_fullscreen(toplevel, toplevel->requested.fullscreen);
	}
}

void zenith_xdg_toplevel_request_maximize(wl_listener* listener, void* data) {
	(void)data;
	ZenithXdgToplevel* zenith_xdg_toplevel = wl_container_of(listener, zenith_xdg_toplevel, request_maximize);
	wlr_xdg_toplevel* toplevel = zenith_xdg_toplevel->xdg_toplevel;
	size_t id = zenith_xdg_toplevel->zenith_xdg_surface->zenith_surface->id;
	if (toplevel->base->initialized) {
		if (toplevel->requested.maximized) {
			auto* server = ZenithServer::instance();
			wlr_xdg_toplevel_set_size(
				toplevel,
				server->max_window_size.width,
				server->max_window_size.height
			);
		}
		wlr_xdg_toplevel_set_maximized(toplevel, toplevel->requested.maximized);
	}
	ZenithServer::instance()->embedder_state->set_window_state(id, toplevel->requested.maximized, zenith_xdg_toplevel->visible);
}

void zenith_xdg_toplevel_request_minimize(wl_listener* listener, void* data) {
	(void)data;
	ZenithXdgToplevel* zenith_xdg_toplevel = wl_container_of(listener, zenith_xdg_toplevel, request_minimize);
	zenith_xdg_toplevel->visible = false;
	zenith_xdg_toplevel->focus(false);
	size_t id = zenith_xdg_toplevel->zenith_xdg_surface->zenith_surface->id;
	ZenithServer::instance()->embedder_state->set_window_state(
		id,
		zenith_xdg_toplevel->xdg_toplevel->current.maximized,
		false
	);
}

void zenith_xdg_toplevel_set_app_id(wl_listener* listener, void* data) {
	ZenithXdgToplevel* zenith_xdg_toplevel = wl_container_of(listener, zenith_xdg_toplevel, set_app_id);
	size_t id = zenith_xdg_toplevel->zenith_xdg_surface->zenith_surface->id;
	char* app_id = zenith_xdg_toplevel->zenith_xdg_surface->xdg_surface->toplevel->app_id;
	ZenithServer::instance()->embedder_state->set_app_id(id, app_id);
}

void zenith_xdg_toplevel_set_title(wl_listener* listener, void* data) {
	ZenithXdgToplevel* zenith_xdg_toplevel = wl_container_of(listener, zenith_xdg_toplevel, set_title);
	size_t id = zenith_xdg_toplevel->zenith_xdg_surface->zenith_surface->id;
	char* title = zenith_xdg_toplevel->zenith_xdg_surface->xdg_surface->toplevel->title;
	ZenithServer::instance()->embedder_state->set_window_title(id, title);
}

void zenith_xdg_toplevel_request_move(wl_listener* listener, void* data) {
	ZenithXdgToplevel* zenith_xdg_toplevel = wl_container_of(listener, zenith_xdg_toplevel, request_move);
	size_t id = zenith_xdg_toplevel->zenith_xdg_surface->zenith_surface->id;
	ZenithServer::instance()->embedder_state->interactive_move(id);
}

void zenith_xdg_toplevel_request_resize(wl_listener* listener, void* data) {
	ZenithXdgToplevel* zenith_xdg_toplevel = wl_container_of(listener, zenith_xdg_toplevel, request_resize);
	auto* event = static_cast<wlr_xdg_toplevel_resize_event*>(data);
	auto edge = static_cast<xdg_toplevel_resize_edge>(event->edges);
	if (edge == XDG_TOPLEVEL_RESIZE_EDGE_NONE) {
		// I don't know how to interpret this event.
		return;
	}
	size_t id = zenith_xdg_toplevel->zenith_xdg_surface->zenith_surface->id;
	ZenithServer::instance()->embedder_state->interactive_resize(id, edge);
}
