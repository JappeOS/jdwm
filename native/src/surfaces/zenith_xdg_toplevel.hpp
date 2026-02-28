#pragma once

#include "zenith_xdg_surface.hpp"
#include "zenith_toplevel.hpp"

extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
}

struct ZenithXdgToplevel : public ZenithToplevel {
	ZenithXdgToplevel(wlr_xdg_toplevel* xdg_toplevel,
	                  std::shared_ptr<ZenithXdgSurface> zenith_xdg_surface);

	wlr_xdg_toplevel* xdg_toplevel;
	std::shared_ptr<ZenithXdgSurface> zenith_xdg_surface;
	bool is_visible = true;
	bool pending_maximize = false;

	~ZenithXdgToplevel();

	/* callbacks */
	wl_listener destroy = {};
	wl_listener commit = {};
	wl_listener request_fullscreen = {};
	wl_listener request_maximize = {};
	wl_listener request_minimize = {};
	wl_listener request_move = {};
	wl_listener request_resize = {};
	wl_listener set_app_id = {};
	wl_listener set_title = {};

	void focus(bool focus) const override;

	void maximize(bool value) const override;

	void resize(size_t width, size_t height) const override;

	void request_close() const override;

	void set_visible(bool value) override;

	bool visible() const override;

	bool maximized() const override;

	std::optional<ToplevelDecoration> decoration() const override;

	const char* protocol() const override;
};

void zenith_xdg_toplevel_commit(wl_listener* listener, void* data);

void zenith_xdg_toplevel_destroy(wl_listener* listener, void* data);

void zenith_xdg_toplevel_request_fullscreen(wl_listener* listener, void* data);

void zenith_xdg_toplevel_request_maximize(wl_listener* listener, void* data);

void zenith_xdg_toplevel_request_minimize(wl_listener* listener, void* data);

void zenith_xdg_toplevel_set_app_id(wl_listener* listener, void* data);

void zenith_xdg_toplevel_set_title(wl_listener* listener, void* data);

void zenith_xdg_toplevel_request_move(wl_listener* listener, void* data);

void zenith_xdg_toplevel_request_resize(wl_listener* listener, void* data);
