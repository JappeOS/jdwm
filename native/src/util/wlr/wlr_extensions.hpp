#pragma once
/// Backported functionality from newer versions of wlroots.

namespace zenith {
	/**
	 * Wrapper around wlr_seat_touch_notify_cancel that takes a wlr_surface*
	 * and resolves it to the appropriate wlr_seat_client*.
	 */
	void wlr_seat_touch_notify_cancel(struct wlr_seat* seat, struct wlr_surface* surface);
}
