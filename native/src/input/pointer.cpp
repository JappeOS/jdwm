/*
 * Pointer support is not perfectly implemented
 */

#include "pointer.hpp"
#include "server.hpp"
#include "time.hpp"
#include "output/zenith_output_manager.hpp"
#include <iostream>
#include <cmath>

extern "C" {
#define static
#include "wlr/types/wlr_pointer.h"
#include "wlr/util/log.h"
#undef static
}

static void schedule_cursor_frame(ZenithServer* server) {
	if (server->pointer != nullptr) {
		server->output_manager->schedule_cursor_frame(server->pointer->cursor->x, server->pointer->cursor->y);
	}
}

static float pointer_scale(ZenithServer* server, ZenithPointer* pointer) {
	if (server == nullptr) {
		return 1.0f;
	}
	const float scale = server->display_scale > 0.0f ? server->display_scale : 1.0f;
	if (pointer != nullptr && server->output_manager != nullptr) {
		static bool logged_mismatch = false;
		if (!logged_mismatch) {
			const float output_scale =
				server->output_manager->pointer_scale_at(pointer->cursor->x, pointer->cursor->y);
			if (std::fabs(output_scale - scale) > 0.001f) {
				logged_mismatch = true;
				wlr_log(
					WLR_INFO,
					"zenith:pointer-scale using display_scale=%.3f while output_scale_at_cursor=%.3f",
					scale,
					output_scale
				);
			}
		}
	}
	return scale;
}

static bool has_any_output(ZenithServer* server) {
	return server->output != nullptr || !server->outputs.empty();
}

void ZenithPointer::set_visible(bool value) {
	if (visible == value) {
		return;
	}
	visible = value;
	if (visible) {
		wlr_cursor_set_xcursor(cursor, cursor_mgr, cursor_name.c_str());
	} else {
		wlr_cursor_set_surface(cursor, nullptr, 0, 0);
	}

	// Software cursors are composited during output frames.
	schedule_cursor_frame(server);
}

void ZenithPointer::set_forced_hidden(bool value) {
	if (forced_hidden == value) {
		return;
	}
	forced_hidden = value;
	set_visible(!forced_hidden);
}

void ZenithPointer::reveal_from_input_activity() {
	if (forced_hidden) {
		return;
	}
	set_visible(true);
}

void ZenithPointer::set_cursor_name(const char* value) {
	const char* next = (value != nullptr && value[0] != '\0') ? value : "left_ptr";
	if (cursor_name == next) {
		return;
	}
	cursor_name = next;
	if (!visible) {
		return;
	}
	wlr_cursor_set_xcursor(cursor, cursor_mgr, cursor_name.c_str());
	schedule_cursor_frame(server);
}

void ZenithPointer::set_manual_locked(bool value) {
	if (manual_lock == value) {
		return;
	}
	manual_lock = value;
}

void ZenithPointer::set_client_locked(bool value) {
	if (client_lock == value) {
		return;
	}
	client_lock = value;
}

void ZenithPointer::restore_default_cursor() {
	set_cursor_name("left_ptr");
}

ZenithPointer::ZenithPointer(ZenithServer* server)
	  : server(server) {
	/*
	 * Creates a cursor, which is a wlroots utility for tracking the cursor
	 * image shown on screen.
	 */
	cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(cursor, server->output_layout);

	/* Creates an xcursor manager, another wlroots utility which loads up
     * Xcursor themes to source cursor images from and makes sure that cursor
     * images are available at all scale factors on the screen (necessary for
     * HiDPI support). We add a cursor theme at scale factor 1 to begin with. */
	cursor_mgr = wlr_xcursor_manager_create(nullptr, 20);
	if (!wlr_xcursor_manager_load(cursor_mgr, 1)) {
		std::cerr << "Failed to load default XCursor theme; trying Adwaita fallback" << std::endl;
		auto* fallback = wlr_xcursor_manager_create("Adwaita", 20);
		if (fallback != nullptr && wlr_xcursor_manager_load(fallback, 1)) {
			wlr_xcursor_manager_destroy(cursor_mgr);
			cursor_mgr = fallback;
		}
	}
	set_cursor_name("left_ptr");

	/*
	 * wlr_cursor *only* displays an image on screen. It does not move around
	 * when the pointer moves. However, we can attach input devices to it, and
	 * it will generate aggregate events for all of them. In these events, we
	 * can choose how we want to process them, forwarding them to clients and
	 * moving the cursor around. More detail on this process is described in my
	 * input handling blog post:
	 *
	 * https://drewdevault.com/2018/07/17/Input-handling-in-wlroots.html
	 *
	 * And more comments are sprinkled throughout the notify functions above.
	 */
	cursor_motion.notify = server_cursor_motion;
	wl_signal_add(&cursor->events.motion, &cursor_motion);

	cursor_motion_absolute.notify = server_cursor_motion_absolute;
	wl_signal_add(&cursor->events.motion_absolute, &cursor_motion_absolute);

	cursor_button.notify = server_cursor_button;
	wl_signal_add(&cursor->events.button, &cursor_button);

	cursor_axis.notify = server_cursor_axis;
	wl_signal_add(&cursor->events.axis, &cursor_axis);

	cursor_frame.notify = server_cursor_frame;
	wl_signal_add(&cursor->events.frame, &cursor_frame);
}

void server_cursor_motion(wl_listener* listener, void* data) {
	ZenithPointer* pointer = wl_container_of(listener, pointer, cursor_motion);
	ZenithServer* server = pointer->server;
	auto* event = static_cast<wlr_pointer_motion_event*>(data);

	if (!has_any_output(server)) {
		return;
	}

	server_update_pointer_constraint(server);
	pointer->reveal_from_input_activity();

	if (server->relative_pointer_manager != nullptr) {
		wlr_relative_pointer_manager_v1_send_relative_motion(
			server->relative_pointer_manager,
			server->seat,
			current_time_microseconds(),
			event->delta_x,
			event->delta_y,
			event->unaccel_dx,
			event->unaccel_dy
		);
	}

	/* The cursor doesn't move unless we tell it to. The cursor automatically
	 * handles constraining the motion to the output layout, as well as any
	 * special configuration applied for the specific input device which
	 * generated the event. You can pass NULL for the device if you want to move
	 * the cursor around without any input. */
	if (!pointer->is_locked()) {
		wlr_cursor_move(pointer->cursor, &event->pointer->base, event->delta_x, event->delta_y);
	}
	server->output_manager->update_active_output_from_cursor(pointer->cursor->x, pointer->cursor->y);
	schedule_cursor_frame(server);

	FlutterPointerEvent e = {};
	e.struct_size = sizeof(FlutterPointerEvent);
	e.phase = pointer->mouse_button_tracker.are_any_buttons_pressed() ? kMove : kHover;
	e.timestamp = current_time_microseconds();
	const float scale = pointer_scale(server, pointer);
	e.x = pointer->cursor->x * scale;
	e.y = pointer->cursor->y * scale;
	e.device_kind = kFlutterPointerDeviceKindMouse;
	e.buttons = pointer->mouse_button_tracker.get_flutter_mouse_state();

	server->embedder_state->send_pointer_event(e);
}

void server_cursor_motion_absolute(wl_listener* listener, void* data) {
	ZenithPointer* pointer = wl_container_of(listener, pointer, cursor_motion_absolute);
	ZenithServer* server = pointer->server;
	auto* event = static_cast<wlr_pointer_motion_absolute_event*>(data);

	if (!has_any_output(server)) {
		return;
	}

	server_update_pointer_constraint(server);
	pointer->reveal_from_input_activity();

	if (!pointer->is_locked()) {
		wlr_cursor_warp_absolute(pointer->cursor, &event->pointer->base, event->x, event->y);
	}
	server->output_manager->update_active_output_from_cursor(pointer->cursor->x, pointer->cursor->y);
	schedule_cursor_frame(server);

	FlutterPointerEvent e = {};
	e.struct_size = sizeof(FlutterPointerEvent);
	e.phase = pointer->mouse_button_tracker.are_any_buttons_pressed() ? kMove : kHover;
	e.timestamp = current_time_microseconds();

	// Map from [0, 1] to [output_width, output_height].
	const float scale = pointer_scale(server, pointer);
	e.x = pointer->cursor->x * scale;
	e.y = pointer->cursor->y * scale;
	e.device_kind = kFlutterPointerDeviceKindMouse;
	e.buttons = pointer->mouse_button_tracker.get_flutter_mouse_state();

	server->embedder_state->send_pointer_event(e);
}

void server_cursor_button(wl_listener* listener, void* data) {
	ZenithPointer* pointer = wl_container_of(listener, pointer, cursor_button);
	ZenithServer* server = pointer->server;
	auto* event = static_cast<wlr_pointer_button_event*>(data);

	if (!has_any_output(server)) {
		return;
	}

	server_update_pointer_constraint(server);
	pointer->reveal_from_input_activity();

	FlutterPointerEvent e = {};
	e.struct_size = sizeof(FlutterPointerEvent);

	if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
		pointer->mouse_button_tracker.release_button(event->button);
		e.phase = pointer->mouse_button_tracker.are_any_buttons_pressed() ? kMove : kUp;
	} else {
		bool are_any_buttons_pressed = pointer->mouse_button_tracker.are_any_buttons_pressed();
		pointer->mouse_button_tracker.press_button(event->button);
		e.phase = are_any_buttons_pressed ? kMove : kDown;
	}
	schedule_cursor_frame(server);

	e.timestamp = current_time_microseconds();
	const float scale = pointer_scale(server, pointer);
	e.x = pointer->cursor->x * scale;
	e.y = pointer->cursor->y * scale;
	e.device_kind = kFlutterPointerDeviceKindMouse;
	e.buttons = pointer->mouse_button_tracker.get_flutter_mouse_state();

	server->embedder_state->send_pointer_event(e);
}

void server_cursor_axis(wl_listener* listener, void* data) {
	ZenithPointer* pointer = wl_container_of(listener, pointer, cursor_axis);
	ZenithServer* server = pointer->server;
	auto* event = static_cast<wlr_pointer_axis_event*>(data);

	if (!has_any_output(server)) {
		return;
	}

	server_update_pointer_constraint(server);
	pointer->reveal_from_input_activity();
	schedule_cursor_frame(server);

	/* Notify the client with pointer focus of the axis event. */
	wlr_seat_pointer_notify_axis(server->seat,
	                             event->time_msec, event->orientation, event->delta,
	                             event->delta_discrete, event->source, event->relative_direction);

	bool are_any_buttons_pressed = pointer->mouse_button_tracker.are_any_buttons_pressed();

	FlutterPointerEvent e = {};
	e.struct_size = sizeof(FlutterPointerEvent);
	e.phase = are_any_buttons_pressed ? kMove : kDown;
	e.timestamp = current_time_microseconds();
	const float scale = pointer_scale(server, pointer);
	e.x = pointer->cursor->x * scale;
	e.y = pointer->cursor->y * scale;
	e.device_kind = kFlutterPointerDeviceKindMouse;
	e.buttons = pointer->mouse_button_tracker.get_flutter_mouse_state();
	e.signal_kind = kFlutterPointerSignalKindScroll;
	switch (event->orientation) {
		case WL_POINTER_AXIS_VERTICAL_SCROLL:
			e.scroll_delta_y = event->delta;
			break;
		case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
			e.scroll_delta_x = event->delta;
			break;
	}
	server->embedder_state->send_pointer_event(e);
}

void server_cursor_frame(wl_listener* listener, void* data) {
	/* Notify the client with pointer focus of the frame event. */
	wlr_seat_pointer_notify_frame(ZenithServer::instance()->seat);
}
