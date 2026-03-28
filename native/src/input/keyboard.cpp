#include "keyboard.hpp"
#include "server.hpp"
#include "encodable_value.h"
#include "keyboard_helpers.hpp"

extern "C" {
#include <wayland-util.h>
#include <linux/vt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#define static
#include <wlr/types/wlr_xdg_shell.h>
#undef static
}

ZenithKeyboard::ZenithKeyboard(ZenithServer* server, wlr_input_device* device)
	  : server(server), device(device) {
	/* We need to prepare an XKB keymap and assign it to the keyboard. This
	 * assumes the defaults (e.g. layout = "us"). */
	xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	xkb_keymap* keymap = xkb_keymap_new_from_names(context, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);

	wlr_keyboard_set_keymap(wlr_keyboard_from_input_device(device), keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);
	wlr_keyboard_set_repeat_info(wlr_keyboard_from_input_device(device), 25, 300);

	/* Here we set up listeners for keyboard events. */
	modifiers.notify = keyboard_handle_modifiers;
	wl_signal_add(&wlr_keyboard_from_input_device(device)->events.modifiers, &modifiers);
	key.notify = keyboard_handle_key;
	wl_signal_add(&wlr_keyboard_from_input_device(device)->events.key, &key);
	destroy.notify = keyboard_handle_destroy;
	wl_signal_add(&device->events.destroy, &destroy);
	listeners_attached = true;

	wlr_seat_set_keyboard(server->seat, wlr_keyboard_from_input_device(device));
}

ZenithKeyboard::~ZenithKeyboard() {
	detach_listeners();
}

void ZenithKeyboard::detach_listeners() {
	if (!listeners_attached) {
		return;
	}
	wl_list_remove(&modifiers.link);
	wl_list_remove(&key.link);
	wl_list_remove(&destroy.link);
	listeners_attached = false;
}

void keyboard_handle_modifiers(wl_listener* listener, void* data) {
	ZenithKeyboard* keyboard = wl_container_of(listener, keyboard, modifiers);
	wlr_seat* seat = keyboard->server->seat;

	/*
	 * A seat can only have one keyboard, but this is a limitation of the
	 * Wayland protocol - not wlroots. We assign all connected keyboards to the
	 * same seat. You can swap out the underlying wlr_keyboard like this and
	 * wlr_seat handles this transparently.
	 */
	wlr_seat_set_keyboard(seat, wlr_keyboard_from_input_device(keyboard->device));
	/* Send modifiers to the client. */
	wlr_seat_keyboard_notify_modifiers(seat, &wlr_keyboard_from_input_device(keyboard->device)->modifiers);
}

void keyboard_handle_key(wl_listener* listener, void* data) {
	ZenithKeyboard* keyboard = wl_container_of(listener, keyboard, key);
	wlr_seat* seat = keyboard->server->seat;
	auto* event = static_cast<wlr_keyboard_key_event*>(data);

	wlr_seat_set_keyboard(seat, wlr_keyboard_from_input_device(keyboard->device));

	// Translate libinput keycode to xkbcommon.
	// This is actually a scan code because it's independent of the keyboard layout.
	// https://code.woboq.org/gtk/include/xkbcommon/xkbcommon.h.html#160
	xkb_keycode_t scan_code = event->keycode + 8;

	wlr_keyboard* wlr_keyboard = wlr_keyboard_from_input_device(keyboard->device);
	xkb_state* state = wlr_keyboard->xkb_state;
	xkb_keysym_t keysym = xkb_state_key_get_one_sym(state, scan_code);

	uint32_t modifiers = wlr_keyboard_get_modifiers(wlr_keyboard);
	bool super_like_key = keysym == XKB_KEY_Super_L || keysym == XKB_KEY_Super_R ||
	                      keysym == XKB_KEY_Meta_L || keysym == XKB_KEY_Meta_R ||
	                      keysym == XKB_KEY_Hyper_L || keysym == XKB_KEY_Hyper_R;
	auto is_mod_active = [state](const char* mod_name) {
		return xkb_state_mod_name_is_active(state, mod_name, XKB_STATE_MODS_EFFECTIVE) > 0;
	};
	bool super_modifier_active = is_mod_active("Mod4") || is_mod_active("Super") ||
	                             is_mod_active("Meta") || is_mod_active("Hyper");
	if (super_modifier_active) {
		modifiers |= WLR_MODIFIER_LOGO;
	} else {
		modifiers &= ~WLR_MODIFIER_LOGO;
	}
	if (super_like_key) {
		if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
			modifiers |= WLR_MODIFIER_LOGO;
		} else {
			modifiers &= ~WLR_MODIFIER_LOGO;
		}
	}

	if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		bool shortcut_handled = handle_shortcuts(keyboard, modifiers, keysym);
		if (shortcut_handled) {
			return;
		}
	}

	auto message = KeyboardKeyEventMessage{
		  .state = event->state == WL_KEYBOARD_KEY_STATE_PRESSED ? KeyboardKeyState::press : KeyboardKeyState::release,
		  .keycode = event->keycode,
		  .scan_code = scan_code,
		  .keysym = keysym,
		  .modifiers = wlr_modifiers_to_gtk(modifiers),
	};
	// FIXME: Flutter doesn't like it when you press Shift + Alt, and sends an empty reply.
	ZenithServer::instance()->embedder_state->send_key_event(message);
}

void keyboard_handle_destroy(wl_listener* listener, void* data) {
	ZenithKeyboard* keyboard = wl_container_of(listener, keyboard, destroy);
	auto* server = keyboard->server;
	keyboard->detach_listeners();

	server->keyboards.remove_if([keyboard](const std::unique_ptr<ZenithKeyboard>& kb) {
		return kb.get() == keyboard;
	});

	uint32_t caps = 0;
	if (server->pointer != nullptr) {
		caps |= WL_SEAT_CAPABILITY_POINTER;
	}
	if (!server->keyboards.empty()) {
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	}
	if (!server->touch_devices.empty()) {
		caps |= WL_SEAT_CAPABILITY_TOUCH;
	}
	wlr_seat_set_capabilities(server->seat, caps);
}

bool handle_shortcuts(struct ZenithKeyboard* keyboard, uint32_t modifiers, xkb_keysym_t keysym) {
	auto is_key_pressed = [&keysym](xkb_keysym_t key) {
		return key == keysym;
	};

	auto is_modifier_pressed = [&modifiers](wlr_keyboard_modifier modifier) {
		return (modifiers & modifier) != 0;
	};

	// Alt + Esc OR Alt + F4
	if (is_modifier_pressed(WLR_MODIFIER_ALT) &&
	    (is_key_pressed(XKB_KEY_Escape) || is_key_pressed(XKB_KEY_F4))) {
		wl_display_terminate(keyboard->server->display);
		return true;
	}

	// Ctrl + Alt + Backspace
	if (is_modifier_pressed(WLR_MODIFIER_CTRL) &&
	    is_modifier_pressed(WLR_MODIFIER_ALT) &&
	    is_key_pressed(XKB_KEY_BackSpace)) {
		wl_display_terminate(keyboard->server->display);
		return true;
	}

	// Ctrl + Alt + F<num>
	for (int vt = 1; vt <= 12; vt++) {
		if (is_key_pressed(XKB_KEY_XF86Switch_VT_1 + vt - 1)) {
			int fd = open("/dev/tty", O_RDWR);
			if (fd > 0) {
				ioctl(fd, VT_ACTIVATE, vt);
				ioctl(fd, VT_WAITACTIVE, vt);
				close(fd);
			}
			return true;
		}
	}

	return false;
}
