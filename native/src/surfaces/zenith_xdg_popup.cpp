#include "zenith_xdg_popup.hpp"

extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
}

ZenithXdgPopup::ZenithXdgPopup(wlr_xdg_popup* xdg_popup, std::shared_ptr<ZenithXdgSurface> zenith_xdg_surface)
	  : xdg_popup{xdg_popup}, zenith_xdg_surface(std::move(zenith_xdg_surface)) {

	commit.notify = zenith_xdg_popup_commit;
	wl_signal_add(&xdg_popup->base->surface->events.commit, &commit);
}

ZenithXdgPopup::~ZenithXdgPopup() {
	wl_list_remove(&commit.link);
}

void zenith_xdg_popup_commit(wl_listener* listener, void* data) {
	ZenithXdgPopup* zenith_xdg_popup = wl_container_of(listener, zenith_xdg_popup, commit);
	wlr_xdg_surface* xdg_surface = zenith_xdg_popup->xdg_popup->base;
	wlr_log(WLR_INFO, "zenith: popup commit, initial_commit=%d, initialized=%d",
	        xdg_surface->initial_commit, xdg_surface->initialized);
	if (!xdg_surface->configured) {
		wlr_log(WLR_INFO, "zenith: Sending initial configure for popup");
		wlr_xdg_surface_schedule_configure(xdg_surface);
	}
}
