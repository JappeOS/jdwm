#pragma once

#include <memory>
#include "zenith_surface.hpp"

extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
}

struct ZenithXdgSurface {
	ZenithXdgSurface(wlr_xdg_surface* xdg_surface, std::shared_ptr<ZenithSurface> zenith_surface);
	~ZenithXdgSurface();

	wlr_xdg_surface* xdg_surface;
	std::shared_ptr<ZenithSurface> zenith_surface;

	/* callbacks */
	wl_listener map{};
	wl_listener unmap{};
	wl_listener destroy{};
};

/*
 * These events are raised when wlr_xdg_shell receives a new toplevel or popup.
 * In wlroots 0.19+, toplevel and popup creation are separate events.
 */
void zenith_xdg_toplevel_create(wl_listener* listener, void* data);
void zenith_xdg_popup_create(wl_listener* listener, void* data);

void zenith_xdg_surface_map(wl_listener* listener, void* data);

void zenith_xdg_surface_unmap(wl_listener* listener, void* data);

void zenith_xdg_surface_destroy(wl_listener* listener, void* data);
