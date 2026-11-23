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

	void constrain();
	void update_if_reactive();
	bool placed = false;
	int last_x = 0, last_y = 0;

	/* callbacks */
	wl_listener commit = {};
	wl_listener reposition = {};
	wl_listener destroy = {};
	wl_listener layout_change = {};
};

void zenith_xdg_popup_commit(wl_listener* listener, void* data);

void update_popup_children(wlr_xdg_surface* parent);
