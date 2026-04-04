#include "zenith_xwayland_toplevel.hpp"

#include "server.hpp"
#include "surfaces/zenith_surface.hpp"
#include "output/zenith_output_manager.hpp"
#include "xwayland_debug.hpp"

extern "C" {
#include <wlr/util/log.h>
#define class wlroots_xwayland_class
#include <wlr/xwayland/xwayland.h>
#include <xcb/xproto.h>
#undef class
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
	wl_client* surface_client = wl_resource_get_client(surface->resource);
	wl_client* focused_client = wl_resource_get_client(focused_surface->resource);
	return surface_client != nullptr && surface_client == focused_client;
}

static int managed_x_for(const ZenithXwaylandToplevel* toplevel) {
	if (toplevel == nullptr || toplevel->xwayland_surface == nullptr) {
		return 0;
	}
	return toplevel->has_configure_position ? toplevel->configure_x : toplevel->xwayland_surface->x;
}

static int managed_y_for(const ZenithXwaylandToplevel* toplevel) {
	if (toplevel == nullptr || toplevel->xwayland_surface == nullptr) {
		return 0;
	}
	return toplevel->has_configure_position ? toplevel->configure_y : toplevel->xwayland_surface->y;
}

static void ensure_configure_position(ZenithXwaylandToplevel* toplevel) {
	if (toplevel == nullptr || toplevel->xwayland_surface == nullptr || toplevel->has_configure_position) {
		return;
	}
	toplevel->has_configure_position = true;
	toplevel->configure_x = toplevel->xwayland_surface->x;
	toplevel->configure_y = toplevel->xwayland_surface->y;
}

ZenithXwaylandToplevel::ZenithXwaylandToplevel(wlr_xwayland_surface* xwayland_surface)
	: xwayland_surface(xwayland_surface) {
	destroy.notify = zenith_xwayland_surface_destroy;
	wl_signal_add(&xwayland_surface->events.destroy, &destroy);

	request_configure.notify = zenith_xwayland_surface_request_configure;
	wl_signal_add(&xwayland_surface->events.request_configure, &request_configure);

	request_move.notify = zenith_xwayland_surface_request_move;
	wl_signal_add(&xwayland_surface->events.request_move, &request_move);

	request_resize.notify = zenith_xwayland_surface_request_resize;
	wl_signal_add(&xwayland_surface->events.request_resize, &request_resize);

	request_maximize.notify = zenith_xwayland_surface_request_maximize;
	wl_signal_add(&xwayland_surface->events.request_maximize, &request_maximize);

	request_fullscreen.notify = zenith_xwayland_surface_request_fullscreen;
	wl_signal_add(&xwayland_surface->events.request_fullscreen, &request_fullscreen);

	set_title.notify = zenith_xwayland_surface_set_title;
	wl_signal_add(&xwayland_surface->events.set_title, &set_title);

	set_decorations.notify = zenith_xwayland_surface_set_decorations;
	wl_signal_add(&xwayland_surface->events.set_decorations, &set_decorations);

	associate.notify = zenith_xwayland_surface_associate;
	wl_signal_add(&xwayland_surface->events.associate, &associate);

	dissociate.notify = zenith_xwayland_surface_dissociate;
	wl_signal_add(&xwayland_surface->events.dissociate, &dissociate);

	if (xwayland_surface->surface != nullptr) {
		attach_surface_listeners();
	}
}

ZenithXwaylandToplevel::~ZenithXwaylandToplevel() {
	detach_surface_listeners();
}

bool ZenithXwaylandToplevel::managed() const {
	return !xwayland_surface->override_redirect;
}

bool ZenithXwaylandToplevel::ensure_registered() {
	if (registered) {
		return true;
	}
	if (!managed() || xwayland_surface->surface == nullptr || xwayland_surface->surface->data == nullptr) {
		return false;
	}

	auto* zenith_surface = static_cast<ZenithSurface*>(xwayland_surface->surface->data);
	auto* server = ZenithServer::instance();
	auto holder = server->xwayland_surfaces.find(xwayland_surface);
	if (holder == server->xwayland_surfaces.end()) {
		return false;
	}
	view_id = zenith_surface->id;
	registered = true;
	server->xwayland_toplevels.insert_or_assign(view_id, holder->second);
	server->toplevels.insert_or_assign(view_id, holder->second);
	return true;
}

void ZenithXwaylandToplevel::attach_surface_listeners() {
	if (surface_listeners_attached || xwayland_surface->surface == nullptr) {
		return;
	}
	map.notify = zenith_xwayland_surface_map;
	wl_signal_add(&xwayland_surface->surface->events.map, &map);

	unmap.notify = zenith_xwayland_surface_unmap;
	wl_signal_add(&xwayland_surface->surface->events.unmap, &unmap);

	commit.notify = zenith_xwayland_surface_commit;
	wl_signal_add(&xwayland_surface->surface->events.commit, &commit);

	surface_listeners_attached = true;
}

void ZenithXwaylandToplevel::detach_surface_listeners() {
	if (!surface_listeners_attached) {
		return;
	}
	wl_list_remove(&map.link);
	wl_list_remove(&unmap.link);
	wl_list_remove(&commit.link);
	surface_listeners_attached = false;
}

void ZenithXwaylandToplevel::request_close() const {
	wlr_xwayland_surface_close(xwayland_surface);
}

void ZenithXwaylandToplevel::set_visible(bool value) {
	is_visible = value;
}

bool ZenithXwaylandToplevel::visible() const {
	return is_visible;
}

bool ZenithXwaylandToplevel::maximized() const {
	return is_maximized;
}

std::optional<ToplevelDecoration> ZenithXwaylandToplevel::decoration() const {
	if (!managed()) {
		return std::nullopt;
	}
	return xwayland_surface->decorations == WLR_XWAYLAND_SURFACE_DECORATIONS_ALL
		? ToplevelDecoration::SERVER_SIZE
		: ToplevelDecoration::CLIENT_SIZE;
}

const char* ZenithXwaylandToplevel::protocol() const {
	return "xwayland";
}

void ZenithXwaylandToplevel::focus(bool focus_value) const {
	auto* server = ZenithServer::instance();
	auto* seat = server->seat;
	auto* surface = xwayland_surface->surface;
	if (surface == nullptr) {
		return;
	}

	if (focus_value) {
		if (managed()) {
			wlr_xwayland_surface_restack(
				xwayland_surface,
				nullptr,
				XCB_STACK_MODE_ABOVE
			);
			if (zenith_xwayland_input_debug_enabled()) {
				wlr_log(
					WLR_INFO,
					"zenith:xw-restack view=%zu title=\"%s\" mode=above",
					view_id,
					xwayland_surface->title != nullptr ? xwayland_surface->title : ""
				);
			}
		}

		wlr_surface* prev_surface = seat->keyboard_state.focused_surface;
		if (prev_surface == surface) {
			return;
		}

		wlr_xwayland_surface_activate(xwayland_surface, true);
		wlr_keyboard* keyboard = wlr_seat_get_keyboard(seat);
		if (keyboard != nullptr) {
			wlr_seat_keyboard_notify_enter(
				seat,
				surface,
				keyboard->keycodes,
				keyboard->num_keycodes,
				&keyboard->modifiers
			);
		}

		// Xwayland clients do not use text-input-v3 directly; clear prior focus.
		for (auto* text_input : server->text_inputs) {
			if (text_input->wlr_text_input->focused_surface != nullptr) {
				text_input->leave();
			}
		}
		return;
	}

	for (auto* text_input : server->text_inputs) {
		if (text_input->wlr_text_input->focused_surface != nullptr) {
			text_input->leave();
		}
	}
	if (seat->keyboard_state.focused_surface == surface) {
		wlr_seat_keyboard_notify_clear_focus(seat);
	}
	wlr_xwayland_surface_activate(xwayland_surface, false);
}

void ZenithXwaylandToplevel::maximize(bool value) const {
	if (!managed()) {
		return;
	}
	auto* server = ZenithServer::instance();
	if (value) {
		wlr_xwayland_surface_configure(
			xwayland_surface,
			managed_x_for(this),
			managed_y_for(this),
			server->max_window_size.width,
			server->max_window_size.height
		);
	}
	wlr_xwayland_surface_set_maximized(xwayland_surface, value, value);
	auto* mutable_self = const_cast<ZenithXwaylandToplevel*>(this);
	mutable_self->is_maximized = value;
}

void ZenithXwaylandToplevel::resize(size_t width, size_t height) const {
	if (!managed()) {
		return;
	}
	wlr_xwayland_surface_configure(
		xwayland_surface,
		managed_x_for(this),
		managed_y_for(this),
		(uint16_t) width,
		(uint16_t) height
	);
}

void ZenithXwaylandToplevel::emit_commit() const {
	if (!registered || !managed()) {
		return;
	}
	auto* server = ZenithServer::instance();
	wlr_surface* surface = xwayland_surface->surface;

	// Generic toplevel commit uses x/y as surface-local visible-bounds offset.
	// For Xwayland, x/y are global layout coordinates, not surface-local values.
	// Report local bounds to avoid offsetting texture content in Flutter.
	int x = 0;
	int y = 0;
	int width = xwayland_surface->width;
	int height = xwayland_surface->height;
	if (surface != nullptr) {
		// For Xwayland, surface-local bounds and input routing must track the
		// committed wl_surface size to avoid stale/offset hit regions.
		width = surface->current.width;
		height = surface->current.height;
	}

	std::optional<std::string> title = std::nullopt;
	if (xwayland_surface->title != nullptr) {
		title = std::string(xwayland_surface->title);
	}
	if (zenith_xwayland_input_debug_enabled()) {
		pixman_box32_t extents = {0, 0, 0, 0};
		int surface_width = 0;
		int surface_height = 0;
		int mapped = 0;
		if (surface != nullptr) {
			extents = surface->input_region.extents;
			surface_width = surface->current.width;
			surface_height = surface->current.height;
			mapped = surface->mapped ? 1 : 0;
		}
		wlr_log(
			WLR_INFO,
			"zenith:xw-commit view=%zu title=\"%s\" xw_xy=(%d,%d) send_xy=(%d,%d) send_wh=(%d,%d) surf_wh=(%d,%d) extents=[%d,%d,%d,%d] mapped=%d",
			view_id,
			xwayland_surface->title != nullptr ? xwayland_surface->title : "",
			xwayland_surface->x,
			xwayland_surface->y,
			x,
			y,
			width,
			height,
			surface_width,
			surface_height,
			extents.x1,
			extents.y1,
			extents.x2,
			extents.y2,
			mapped
		);
	}

	server->embedder_state->commit_toplevel_surface(
		view_id,
		protocol(),
		x,
		y,
		width,
		height,
		decoration(),
		title,
		std::nullopt
	);
}

void ZenithXwaylandToplevel::handle_unmap() {
	auto* server = ZenithServer::instance();
	wlr_surface* surface = xwayland_surface->surface;
	if (surface != nullptr) {
		for (auto* text_input : server->text_inputs) {
			if (text_input->wlr_text_input->focused_surface == surface) {
				text_input->leave();
			}
		}
		if (pointer_focus_belongs_to_same_client(server, surface)) {
			wlr_seat_pointer_notify_clear_focus(server->seat);
		}
	}
	if (server->pointer != nullptr && server->pointer->is_visible()) {
		// Prevent stale client cursor surfaces from a just-unmapped window.
		server->pointer->restore_default_cursor();
	}

	if (is_mapped && registered && managed()) {
		server->embedder_state->unmap_toplevel_surface(view_id, protocol());
		server->output_manager->schedule_compositor_frame();
	}
	is_mapped = false;
}

void zenith_xwayland_surface_create(wl_listener* listener, void* data) {
	ZenithServer* server = wl_container_of(listener, server, new_xwayland_surface);
	auto* surface = static_cast<wlr_xwayland_surface*>(data);
	auto toplevel = std::make_shared<ZenithXwaylandToplevel>(surface);
	server->xwayland_surfaces.insert_or_assign(surface, toplevel);
	if (toplevel->ensure_registered()) {
		toplevel->emit_commit();
	}
}

void zenith_xwayland_surface_destroy(wl_listener* listener, void* data) {
	(void)data;
	ZenithXwaylandToplevel* toplevel = wl_container_of(listener, toplevel, destroy);
	auto* server = ZenithServer::instance();

	toplevel->handle_unmap();
	toplevel->detach_surface_listeners();

	wl_list_remove(&toplevel->request_configure.link);
	wl_list_remove(&toplevel->request_move.link);
	wl_list_remove(&toplevel->request_resize.link);
	wl_list_remove(&toplevel->request_maximize.link);
	wl_list_remove(&toplevel->request_fullscreen.link);
	wl_list_remove(&toplevel->set_title.link);
	wl_list_remove(&toplevel->set_decorations.link);
	wl_list_remove(&toplevel->associate.link);
	wl_list_remove(&toplevel->dissociate.link);
	wl_list_remove(&toplevel->destroy.link);

	if (toplevel->registered) {
		server->xwayland_toplevels.erase(toplevel->view_id);
		server->toplevels.erase(toplevel->view_id);
		toplevel->registered = false;
		toplevel->view_id = 0;
	}
	server->xwayland_surfaces.erase(toplevel->xwayland_surface);
}

void zenith_xwayland_surface_request_configure(wl_listener* listener, void* data) {
	ZenithXwaylandToplevel* toplevel = wl_container_of(listener, toplevel, request_configure);
	auto* event = static_cast<wlr_xwayland_surface_configure_event*>(data);
	int x = event->x;
	int y = event->y;
	// For managed windows, Flutter/compositor owns placement. Accepting
	// client-requested x/y here can desync visual placement and X11 input
	// targeting (especially on multi-monitor layouts).
	if (toplevel->managed()) {
		ensure_configure_position(toplevel);
		x = toplevel->configure_x;
		y = toplevel->configure_y;
	}
	wlr_xwayland_surface_configure(
		toplevel->xwayland_surface,
		x,
		y,
		event->width,
		event->height
	);
	if (zenith_xwayland_input_debug_enabled()) {
		wlr_log(
			WLR_INFO,
			"zenith:xw-request-configure view=%zu title=\"%s\" req_xy=(%d,%d) send_xy=(%d,%d) req_wh=(%d,%d) has_cfg_xy=%d cfg_xy=(%d,%d)",
			toplevel->view_id,
			toplevel->xwayland_surface->title != nullptr ? toplevel->xwayland_surface->title : "",
			event->x,
			event->y,
			x,
			y,
			event->width,
			event->height,
			toplevel->has_configure_position ? 1 : 0,
			toplevel->configure_x,
			toplevel->configure_y
		);
	}
}

void zenith_xwayland_surface_request_move(wl_listener* listener, void* data) {
	(void)data;
	ZenithXwaylandToplevel* toplevel = wl_container_of(listener, toplevel, request_move);
	if (!toplevel->ensure_registered() || !toplevel->managed()) {
		return;
	}
	ZenithServer::instance()->embedder_state->interactive_move(toplevel->view_id);
}

void zenith_xwayland_surface_request_resize(wl_listener* listener, void* data) {
	ZenithXwaylandToplevel* toplevel = wl_container_of(listener, toplevel, request_resize);
	auto* event = static_cast<wlr_xwayland_resize_event*>(data);
	if (!toplevel->ensure_registered() || !toplevel->managed()) {
		return;
	}
	auto edge = static_cast<xdg_toplevel_resize_edge>(event->edges);
	if (edge == XDG_TOPLEVEL_RESIZE_EDGE_NONE) {
		return;
	}
	ZenithServer::instance()->embedder_state->interactive_resize(toplevel->view_id, edge);
}

void zenith_xwayland_surface_request_maximize(wl_listener* listener, void* data) {
	(void)data;
	ZenithXwaylandToplevel* toplevel = wl_container_of(listener, toplevel, request_maximize);
	if (!toplevel->ensure_registered() || !toplevel->managed()) {
		return;
	}
	toplevel->maximize(toplevel->xwayland_surface->maximized_horz || toplevel->xwayland_surface->maximized_vert);
	ZenithServer::instance()->embedder_state->set_window_state(
		toplevel->view_id,
		toplevel->maximized(),
		toplevel->visible(),
		toplevel->protocol()
	);
}

void zenith_xwayland_surface_request_fullscreen(wl_listener* listener, void* data) {
	(void)data;
	ZenithXwaylandToplevel* toplevel = wl_container_of(listener, toplevel, request_fullscreen);
	wlr_xwayland_surface_set_fullscreen(toplevel->xwayland_surface, toplevel->xwayland_surface->fullscreen);
}

void zenith_xwayland_surface_set_title(wl_listener* listener, void* data) {
	(void)data;
	ZenithXwaylandToplevel* toplevel = wl_container_of(listener, toplevel, set_title);
	if (!toplevel->ensure_registered() || !toplevel->managed()) {
		return;
	}
	const char* title = toplevel->xwayland_surface->title;
	if (title == nullptr) {
		return;
	}
	ZenithServer::instance()->embedder_state->set_window_title(
		toplevel->view_id,
		title,
		toplevel->protocol()
	);
}

void zenith_xwayland_surface_set_decorations(wl_listener* listener, void* data) {
	(void)data;
	ZenithXwaylandToplevel* toplevel = wl_container_of(listener, toplevel, set_decorations);
	if (!toplevel->ensure_registered() || !toplevel->managed() || !toplevel->is_mapped) {
		return;
	}
	toplevel->emit_commit();
	ZenithServer::instance()->output_manager->schedule_compositor_frame();
}

void zenith_xwayland_surface_associate(wl_listener* listener, void* data) {
	(void)data;
	ZenithXwaylandToplevel* toplevel = wl_container_of(listener, toplevel, associate);
	toplevel->attach_surface_listeners();
	if (toplevel->ensure_registered()) {
		toplevel->emit_commit();
	}
}

void zenith_xwayland_surface_dissociate(wl_listener* listener, void* data) {
	(void)data;
	ZenithXwaylandToplevel* toplevel = wl_container_of(listener, toplevel, dissociate);
	auto* server = ZenithServer::instance();

	toplevel->handle_unmap();
	toplevel->detach_surface_listeners();
	if (toplevel->registered) {
		server->xwayland_toplevels.erase(toplevel->view_id);
		server->toplevels.erase(toplevel->view_id);
		toplevel->registered = false;
		toplevel->view_id = 0;
	}
}

void zenith_xwayland_surface_map(wl_listener* listener, void* data) {
	(void)data;
	ZenithXwaylandToplevel* toplevel = wl_container_of(listener, toplevel, map);
	auto* server = ZenithServer::instance();
	if (!toplevel->ensure_registered() || !toplevel->managed()) {
		return;
	}

	toplevel->is_visible = true;
	toplevel->is_mapped = true;
	ensure_configure_position(toplevel);
	if (zenith_xwayland_input_debug_enabled()) {
		wlr_log(
			WLR_INFO,
			"zenith:xw-map view=%zu title=\"%s\" xw_xy=(%d,%d) xw_wh=(%d,%d) has_cfg_xy=%d cfg_xy=(%d,%d)",
			toplevel->view_id,
			toplevel->xwayland_surface->title != nullptr ? toplevel->xwayland_surface->title : "",
			toplevel->xwayland_surface->x,
			toplevel->xwayland_surface->y,
			toplevel->xwayland_surface->width,
			toplevel->xwayland_surface->height,
			toplevel->has_configure_position ? 1 : 0,
			toplevel->configure_x,
			toplevel->configure_y
		);
	}

	if (server->start_windows_maximized) {
		toplevel->maximize(true);
	}

	server->embedder_state->map_toplevel_surface(toplevel->view_id, toplevel->protocol());
	toplevel->emit_commit();
	server->embedder_state->set_window_state(
		toplevel->view_id,
		toplevel->maximized(),
		toplevel->visible(),
		toplevel->protocol()
	);
	server->output_manager->schedule_compositor_frame();
}

void zenith_xwayland_surface_unmap(wl_listener* listener, void* data) {
	(void)data;
	ZenithXwaylandToplevel* toplevel = wl_container_of(listener, toplevel, unmap);
	if (zenith_xwayland_input_debug_enabled()) {
		wlr_log(
			WLR_INFO,
			"zenith:xw-unmap view=%zu title=\"%s\"",
			toplevel->view_id,
			toplevel->xwayland_surface->title != nullptr ? toplevel->xwayland_surface->title : ""
		);
	}
	toplevel->handle_unmap();
}

void zenith_xwayland_surface_commit(wl_listener* listener, void* data) {
	(void)data;
	ZenithXwaylandToplevel* toplevel = wl_container_of(listener, toplevel, commit);
	if (!toplevel->is_mapped) {
		return;
	}
	toplevel->emit_commit();
}
