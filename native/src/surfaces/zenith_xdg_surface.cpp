#include "zenith_xdg_surface.hpp"
#include "zenith_xdg_toplevel.hpp"
#include "binary_messenger.hpp"
#include "server.hpp"
#include "assert.hpp"
#include "cursor_debug.hpp"
#include "output/zenith_output_manager.hpp"

extern "C" {
#include <wlr/util/log.h>
}

static bool pointer_focus_belongs_to_same_client(
	ZenithServer* server,
	wlr_surface* surface
) {
	if (server == nullptr || surface == nullptr) {
		return false;
	}
	wlr_surface* focused_surface = server->seat->pointer_state.focused_surface;
	if (focused_surface == nullptr) {
		return false;
	}
	if (surface->resource == nullptr || focused_surface->resource == nullptr) {
		return false;
	}
	wl_client* surface_client = wl_resource_get_client(surface->resource);
	wl_client* focused_client = wl_resource_get_client(focused_surface->resource);
	return surface_client != nullptr && surface_client == focused_client;
}

ZenithXdgSurface::ZenithXdgSurface(wlr_xdg_surface* xdg_surface, std::shared_ptr<ZenithSurface> zenith_surface)
	  : xdg_surface{xdg_surface}, zenith_surface(std::move(zenith_surface)) {
	destroy.notify = zenith_xdg_surface_destroy;
	wl_signal_add(&xdg_surface->events.destroy, &destroy);

	map.notify = zenith_xdg_surface_map;
	wl_signal_add(&xdg_surface->surface->events.map, &map);

	unmap.notify = zenith_xdg_surface_unmap;
	wl_signal_add(&xdg_surface->surface->events.unmap, &unmap);
}

ZenithXdgSurface::~ZenithXdgSurface() {
	// Listener links are detached in zenith_xdg_surface_destroy while wlroots
	// destroy signal is being emitted.
}

static std::shared_ptr<ZenithXdgSurface> register_xdg_surface(ZenithServer* server, wlr_xdg_surface* xdg_surface) {
	auto* zenith_surface = static_cast<ZenithSurface*>(xdg_surface->surface->data);
	const std::shared_ptr<ZenithSurface>& zenith_surface_ref = server->surfaces.at(zenith_surface->id);

	auto* zenith_xdg_surface = new ZenithXdgSurface(xdg_surface, zenith_surface_ref);
	xdg_surface->data = zenith_xdg_surface;
	auto zenith_xdg_surface_ref = std::shared_ptr<ZenithXdgSurface>(zenith_xdg_surface);
	server->xdg_surfaces.insert(std::make_pair(zenith_surface->id, zenith_xdg_surface_ref));
	return zenith_xdg_surface_ref;
}

void zenith_xdg_toplevel_create(wl_listener* listener, void* data) {
	ZenithServer* server = wl_container_of(listener, server, new_xdg_toplevel);
	auto* toplevel = static_cast<wlr_xdg_toplevel*>(data);
	wlr_log(WLR_INFO, "zenith: new_toplevel event received");
	auto zenith_xdg_surface_ref = register_xdg_surface(server, toplevel->base);
	size_t id = zenith_xdg_surface_ref->zenith_surface->id;
	auto zenith_toplevel = std::make_shared<ZenithXdgToplevel>(toplevel, zenith_xdg_surface_ref);
	server->xdg_toplevels.insert(std::make_pair(id, zenith_toplevel));
	server->toplevels.insert(std::make_pair(id, zenith_toplevel));
	wlr_log(WLR_INFO, "zenith: toplevel registered with id=%zu", id);
}

void zenith_xdg_popup_create(wl_listener* listener, void* data) {
	ZenithServer* server = wl_container_of(listener, server, new_xdg_popup);
	auto* popup = static_cast<wlr_xdg_popup*>(data);
	auto zenith_xdg_surface_ref = register_xdg_surface(server, popup->base);
	size_t id = zenith_xdg_surface_ref->zenith_surface->id;
	auto zenith_popup = new ZenithXdgPopup(popup, zenith_xdg_surface_ref);
	server->xdg_popups.insert(std::make_pair(id, zenith_popup));
}

void zenith_xdg_surface_map(wl_listener* listener, void* data) {
	ZenithXdgSurface* zenith_xdg_surface = wl_container_of(listener, zenith_xdg_surface, map);
	size_t id = zenith_xdg_surface->zenith_surface->id;
	wlr_log(WLR_INFO, "zenith: xdg_surface map, id=%zu, role=%d", id, zenith_xdg_surface->xdg_surface->role);
	auto* server = ZenithServer::instance();

	// Apply deferred maximize (couldn't be done before surface was initialized)
	if (zenith_xdg_surface->xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
		auto it = server->xdg_toplevels.find(id);
		if (it != server->xdg_toplevels.end()) {
			ZenithXdgToplevel* toplevel = it->second.get();
			if (toplevel->pending_maximize) {
				toplevel->pending_maximize = false;
				toplevel->resize(server->max_window_size.width, server->max_window_size.height);
				toplevel->maximize(true);
			}
		}
	}

	server->embedder_state->map_xdg_surface(id, (int) zenith_xdg_surface->xdg_surface->role);
	server->output_manager->schedule_compositor_frame();
}

void zenith_xdg_surface_unmap(wl_listener* listener, void* data) {
	ZenithXdgSurface* zenith_xdg_surface = wl_container_of(listener, zenith_xdg_surface, unmap);
	size_t id = zenith_xdg_surface->zenith_surface->id;
	auto* server = ZenithServer::instance();
	wlr_surface* surface = zenith_xdg_surface->xdg_surface->surface;

	for (auto* text_input : server->text_inputs) {
		if (text_input->wlr_text_input->focused_surface == surface) {
			text_input->leave();
		}
	}

	if (pointer_focus_belongs_to_same_client(server, zenith_xdg_surface->xdg_surface->surface)) {
		if (zenith_cursor_debug_enabled()) {
			wlr_log(WLR_INFO, "zenith:cursor xdg_unmap clear pointer focus by client match id=%zu", id);
		}
		wlr_seat_pointer_notify_clear_focus(server->seat);
	}
	if (server->pointer != nullptr && server->pointer->is_visible()) {
		// Prevent stale client cursor surfaces from a just-unmapped window.
		if (zenith_cursor_debug_enabled()) {
			wlr_log(
				WLR_INFO,
				"zenith:cursor xdg_unmap restore default id=%zu forced_hidden=%d cursor=%s",
				id,
				server->pointer->is_forced_hidden() ? 1 : 0,
				server->pointer->current_cursor_name()
			);
		}
		server->pointer->restore_default_cursor();
	}
	server->embedder_state->unmap_xdg_surface(id, (int) zenith_xdg_surface->xdg_surface->role);
	server->output_manager->schedule_compositor_frame();
}

void zenith_xdg_surface_destroy(wl_listener* listener, void* data) {
	ZenithXdgSurface* zenith_xdg_surface = wl_container_of(listener, zenith_xdg_surface, destroy);
	size_t id = zenith_xdg_surface->zenith_surface->id;

	auto* server = ZenithServer::instance();
	wlr_surface* surface = zenith_xdg_surface->xdg_surface->surface;

	// wlroots emits xdg_surface destroy and then asserts listener lists are empty.
	wl_list_remove(&zenith_xdg_surface->map.link);
	wl_list_remove(&zenith_xdg_surface->unmap.link);
	wl_list_remove(&zenith_xdg_surface->destroy.link);

	for (auto* text_input : server->text_inputs) {
		if (text_input->wlr_text_input->focused_surface == surface) {
			text_input->leave();
		}
	}

	if (pointer_focus_belongs_to_same_client(server, zenith_xdg_surface->xdg_surface->surface)) {
		if (zenith_cursor_debug_enabled()) {
			wlr_log(WLR_INFO, "zenith:cursor xdg_destroy clear pointer focus by client match id=%zu", id);
		}
		wlr_seat_pointer_notify_clear_focus(server->seat);
	}
	if (server->pointer != nullptr && server->pointer->is_visible()) {
		// Prevent stale client cursor surfaces from a just-destroyed window.
		if (zenith_cursor_debug_enabled()) {
			wlr_log(
				WLR_INFO,
				"zenith:cursor xdg_destroy restore default id=%zu forced_hidden=%d cursor=%s",
				id,
				server->pointer->is_forced_hidden() ? 1 : 0,
				server->pointer->current_cursor_name()
			);
		}
		server->pointer->restore_default_cursor();
	}

	if (zenith_xdg_surface->xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
		bool erased = server->xdg_toplevels.erase(id);
		assert(erased);
		erased = server->toplevels.erase(id);
		assert(erased);
	} else if (zenith_xdg_surface->xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
		bool erased = server->xdg_popups.erase(id);
		assert(erased);
	}
	bool erased = server->xdg_surfaces.erase(id);
	assert(erased);
}
