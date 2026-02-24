#pragma once

#include "zenith_xdg_surface.hpp"

extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
}

struct ZenithXdgPopup {
	ZenithXdgPopup(wlr_xdg_popup* xdg_popup, std::shared_ptr<ZenithXdgSurface> zenith_xdg_surface);
	~ZenithXdgPopup();

	wlr_xdg_popup* xdg_popup;
	std::shared_ptr<ZenithXdgSurface> zenith_xdg_surface;

	/* callbacks */
	wl_listener commit = {};
};

void zenith_xdg_popup_commit(wl_listener* listener, void* data);
