/// Backported functionality from newer versions of wlroots.

#include "wlr_extensions.hpp"
#include <wayland-client-protocol.h>
#include <wayland-server-protocol.h>
#include <wayland-server-core.h>

extern "C" {
#define static
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_compositor.h>
#undef static
}

namespace zenith {
	// Wrapper that takes a wlr_surface* and resolves to the wlr_seat_client* needed by wlroots 0.19 API.
	void wlr_seat_touch_notify_cancel(struct ::wlr_seat* seat, struct ::wlr_surface* surface) {
		struct wl_client* client = wl_resource_get_client(surface->resource);
		struct wlr_seat_client* seat_client = wlr_seat_client_for_wl_client(seat, client);
		if (seat_client == NULL) {
			return;
		}
		::wlr_seat_touch_notify_cancel(seat, seat_client);
	}
}
