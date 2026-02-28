#pragma once

#include <optional>
#include <string>

#include "zenith_toplevel.hpp"

extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
struct wlr_xwayland_surface;
}

struct ZenithXwaylandToplevel : public ZenithToplevel {
	explicit ZenithXwaylandToplevel(wlr_xwayland_surface* xwayland_surface);
	~ZenithXwaylandToplevel() override;

	wlr_xwayland_surface* xwayland_surface;
	size_t view_id = 0;
	bool registered = false;
	bool is_visible = true;
	bool is_maximized = false;
	bool is_mapped = false;
	bool surface_listeners_attached = false;
	// Coordinates used when sending X11 configure events.
	// In extend mode these are output-local (not global layout coordinates).
	bool has_configure_position = false;
	int configure_x = 0;
	int configure_y = 0;

	/* xwayland surface callbacks */
	wl_listener destroy{};
	wl_listener request_configure{};
	wl_listener request_move{};
	wl_listener request_resize{};
	wl_listener request_maximize{};
	wl_listener request_fullscreen{};
	wl_listener set_title{};
	wl_listener set_decorations{};
	wl_listener associate{};
	wl_listener dissociate{};

	/* wl_surface callbacks (valid only while associated) */
	wl_listener map{};
	wl_listener unmap{};
	wl_listener commit{};

	bool ensure_registered();
	bool managed() const;
	void attach_surface_listeners();
	void detach_surface_listeners();
	void handle_unmap();
	void emit_commit() const;

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

void zenith_xwayland_surface_create(wl_listener* listener, void* data);

void zenith_xwayland_surface_destroy(wl_listener* listener, void* data);
void zenith_xwayland_surface_request_configure(wl_listener* listener, void* data);
void zenith_xwayland_surface_request_move(wl_listener* listener, void* data);
void zenith_xwayland_surface_request_resize(wl_listener* listener, void* data);
void zenith_xwayland_surface_request_maximize(wl_listener* listener, void* data);
void zenith_xwayland_surface_request_fullscreen(wl_listener* listener, void* data);
void zenith_xwayland_surface_set_title(wl_listener* listener, void* data);
void zenith_xwayland_surface_set_decorations(wl_listener* listener, void* data);
void zenith_xwayland_surface_associate(wl_listener* listener, void* data);
void zenith_xwayland_surface_dissociate(wl_listener* listener, void* data);
void zenith_xwayland_surface_map(wl_listener* listener, void* data);
void zenith_xwayland_surface_unmap(wl_listener* listener, void* data);
void zenith_xwayland_surface_commit(wl_listener* listener, void* data);
