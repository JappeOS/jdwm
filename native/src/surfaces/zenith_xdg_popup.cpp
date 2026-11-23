#include "zenith_xdg_popup.hpp"
#include "popup_placement.hpp"
#include "server.hpp"
#include "util/process_diagnostics.hpp"

extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
}

ZenithXdgPopup::ZenithXdgPopup(wlr_xdg_popup* xdg_popup, std::shared_ptr<ZenithXdgSurface> zenith_xdg_surface)
	  : xdg_popup{xdg_popup}, zenith_xdg_surface(std::move(zenith_xdg_surface)) {

	commit.notify = zenith_xdg_popup_commit;
	wl_signal_add(&xdg_popup->base->surface->events.commit, &commit);

	reposition.notify = [](wl_listener* listener, void*) {
		ZenithXdgPopup* popup = wl_container_of(listener, popup, reposition);
		popup->constrain();
	};
	wl_signal_add(&xdg_popup->events.reposition, &reposition);

	// The popup role is destroyed before its xdg_surface. Release its owner
	// here so all listeners are unlinked while the signals are still alive.
	destroy.notify = [](wl_listener* listener, void*) {
		ZenithXdgPopup* popup = wl_container_of(listener, popup, destroy);
		const auto id = popup->zenith_xdg_surface->zenith_surface->id;
		ZenithServer::instance()->xdg_popups.erase(id);
	};
	wl_signal_add(&xdg_popup->events.destroy, &destroy);

	layout_change.notify = [](wl_listener* listener, void*) {
		ZenithXdgPopup* popup = wl_container_of(listener, popup, layout_change);
		popup->update_if_reactive();
	};
	wl_signal_add(&ZenithServer::instance()->output_layout->events.change, &layout_change);
}

ZenithXdgPopup::~ZenithXdgPopup() {
	wl_list_remove(&commit.link);
	wl_list_remove(&destroy.link);
	wl_list_remove(&reposition.link);
	wl_list_remove(&layout_change.link);
}

void ZenithXdgPopup::constrain() {
	auto* server = ZenithServer::instance();
	auto* parent = xdg_popup->parent;
	int dx = 0, dy = 0;

	while (parent) {
		auto* xdg = wlr_xdg_surface_try_from_wlr_surface(parent);
		if (!xdg)
			return;

		if (xdg->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
			// Flutter reports the client geometry relative to the layout extents.
			auto* surface = static_cast<ZenithSurface*>(parent->data);
			auto it = server->toplevel_geometries.find(surface->id);
			if (it == server->toplevel_geometries.end())
				return;

			wlr_box extents{}, geometry{};
			wlr_output_layout_get_box(server->output_layout, nullptr, &extents);
			if (!place_xdg_popup(server->output_layout, extents.x + it->second.x + dx,
			                     extents.y + it->second.y + dy, xdg_popup->scheduled.rules, geometry))
				return;

			placed = true;
			if (!wlr_box_equal(&geometry, &xdg_popup->scheduled.geometry)) {
				xdg_popup->scheduled.geometry = geometry;
				wlr_xdg_surface_schedule_configure(xdg_popup->base);
			}

			return;
		}

		if (xdg->role != WLR_XDG_SURFACE_ROLE_POPUP || !xdg->popup)
			return;

		dx += xdg->popup->current.geometry.x;
		dy += xdg->popup->current.geometry.y;
		parent = xdg->popup->parent;
	}
}

void ZenithXdgPopup::update_if_reactive() {
	if (xdg_popup->base->initialized && (!placed || xdg_popup->scheduled.rules.reactive))
		constrain();
}

// Child positioners are relative to their parent's committed geometry. Revisit
// reactive descendants after the client actually applies a new popup position.
void update_popup_children(wlr_xdg_surface* parent) {
	auto* server = ZenithServer::instance();
	visit_popup_descendants(parent, [server](wlr_xdg_popup* child) {
		auto* surface = static_cast<ZenithSurface*>(child->base->surface->data);
		auto it = server->xdg_popups.find(surface->id);
		if (it != server->xdg_popups.end())
			it->second->update_if_reactive();
	});
}

void zenith_xdg_popup_commit(wl_listener* listener, void* data) {
	ZenithXdgPopup* zenith_xdg_popup = wl_container_of(listener, zenith_xdg_popup, commit);
	wlr_xdg_surface* xdg_surface = zenith_xdg_popup->xdg_popup->base;

	wlr_log(WLR_DEBUG, "zenith: popup commit, initial_commit=%d, initialized=%d",
	        xdg_surface->initial_commit, xdg_surface->initialized);

	if (xdg_surface->initial_commit) {
		zenith_xdg_popup->placed = false;
		zenith_xdg_popup->constrain();
		if (wlr_log_get_verbosity() >= WLR_DEBUG) {
			pid_t pid;
			wl_client_get_credentials(wl_resource_get_client(xdg_surface->resource), &pid, nullptr, nullptr);
			wlr_log(WLR_DEBUG, "zenith:fence op=popup-initial client_pid=%d open_fds=%lld soft_limit=%lld",
			        pid, zenith::diagnostics::open_fd_count(), zenith::diagnostics::fd_soft_limit());
		}
	}

	const auto& geometry = zenith_xdg_popup->xdg_popup->current.geometry;
	if (geometry.x != zenith_xdg_popup->last_x || geometry.y != zenith_xdg_popup->last_y) {
		zenith_xdg_popup->last_x = geometry.x;
		zenith_xdg_popup->last_y = geometry.y;
		update_popup_children(xdg_surface);
	}
	
	if (!xdg_surface->configured) {
		wlr_log(WLR_DEBUG, "zenith: Sending initial configure for popup");
		wlr_xdg_surface_schedule_configure(xdg_surface);
	}
}
