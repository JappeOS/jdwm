#include "server.hpp"
#include "debug.hpp"
#include "assert.hpp"
#include "util/egl/egl_extensions.hpp"
#include "egl/create_shared_egl_context.hpp"
#include "zenith_toplevel_decoration.hpp"
#include "multimonitor/multi_monitor_mode.hpp"
#include "output/zenith_output_manager.hpp"
#include "build_info.hpp"
#include "xwayland_debug.hpp"
#include "cursor_debug.hpp"
#include <unistd.h>
#include <sys/eventfd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>

extern "C" {
#define static
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/render/allocator.h>
#include <wlr/backend/libinput.h>
#include <wlr/backend/drm.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>
#include <wlr/render/gles2.h>
#include <wlr/interfaces/wlr_touch.h>
#define class wlroots_xwayland_class
#include <wlr/xwayland/xwayland.h>
#undef class
#undef static
// Private wlroots headers
#include <render/egl.h>
}

ZenithServer* ZenithServer::_instance = nullptr;

ZenithServer* ZenithServer::instance() {
	if (_instance == nullptr) {
		_instance = new ZenithServer();
	}
	return _instance;
}

static float read_display_scale();
static void detach_active_pointer_constraint_listener(ZenithServer* server);
static void server_xwayland_ready(wl_listener* listener, void* data);
static void zenith_maybe_default_libseat_backend();
static void zenith_preflight_libseat();
static void zenith_debug_log_loaded_libs();
static void zenith_configure_drm_stability(multimonitor::MultiMonitorMode mode);

static bool path_exists(const char* path) {
	return path != nullptr && access(path, F_OK) == 0;
}

static bool env_equals(const char* env_value, const char* expected) {
	return env_value != nullptr && expected != nullptr && std::string(env_value) == expected;
}

static bool equals_ignore_case(const char* value, const char* expected) {
	if (value == nullptr || expected == nullptr) {
		return false;
	}
	while (*value != '\0' && *expected != '\0') {
		char a = *value;
		char b = *expected;
		if (a >= 'A' && a <= 'Z') {
			a = (char)(a - 'A' + 'a');
		}
		if (b >= 'A' && b <= 'Z') {
			b = (char)(b - 'A' + 'a');
		}
		if (a != b) {
			return false;
		}
		++value;
		++expected;
	}
	return *value == '\0' && *expected == '\0';
}

static bool env_is_truthy(const char* value) {
	if (value == nullptr || value[0] == '\0') {
		return false;
	}
	if (equals_ignore_case(value, "0") ||
	    equals_ignore_case(value, "false") ||
	    equals_ignore_case(value, "off") ||
	    equals_ignore_case(value, "no")) {
		return false;
	}
	return true;
}

static void zenith_configure_drm_stability(multimonitor::MultiMonitorMode mode) {
	if (env_is_truthy(std::getenv("ZENITH_DRM_NO_MODIFIERS"))) {
		setenv("WLR_DRM_NO_MODIFIERS", "1", 1);
		wlr_log(
			WLR_INFO,
			"zenith: forcing WLR_DRM_NO_MODIFIERS=1 (ZENITH_DRM_NO_MODIFIERS). "
			"Warning: this may break multi-GPU output on some systems."
		);
	}

	if (env_is_truthy(std::getenv("ZENITH_DRM_NO_ATOMIC"))) {
		setenv("WLR_DRM_NO_ATOMIC", "1", 1);
		wlr_log(WLR_INFO, "zenith: forcing WLR_DRM_NO_ATOMIC=1 (ZENITH_DRM_NO_ATOMIC)");
	}

	(void)mode;
}

static void zenith_maybe_default_libseat_backend() {
	// libseat prefers logind on systemd systems. That works great when we're launched from a real
	// VT-backed logind session (PAM), but it often fails when launched as a systemd service/transient
	// unit without a TTY/session context, because logind then requires polkit (interactive auth) for
	// session activation/device control. In those cases, seatd is the safer default when available.
	if (std::getenv("LIBSEAT_BACKEND") != nullptr) {
		return;
	}

	const bool has_seatd_socket = path_exists("/run/seatd.sock");
	if (!has_seatd_socket) {
		return;
	}

	const char* xdg_session_id = std::getenv("XDG_SESSION_ID");
	const char* xdg_session_type = std::getenv("XDG_SESSION_TYPE");
	const char* xdg_vtnr = std::getenv("XDG_VTNR");
	const bool has_display_env =
		std::getenv("WAYLAND_DISPLAY") != nullptr || std::getenv("DISPLAY") != nullptr;

	const bool looks_like_logind_tty_session =
		xdg_session_id != nullptr && xdg_session_id[0] != '\0' && xdg_vtnr != nullptr &&
		xdg_vtnr[0] != '\0' && !has_display_env &&
		(xdg_session_type == nullptr || std::string(xdg_session_type) == "tty");

	if (!looks_like_logind_tty_session) {
		setenv("LIBSEAT_BACKEND", "seatd", 0);
		wlr_log(WLR_INFO,
		        "zenith: LIBSEAT_BACKEND not set and no VT-backed logind session detected; "
		        "defaulting to LIBSEAT_BACKEND=seatd (override by setting LIBSEAT_BACKEND).");
	}
}

static void zenith_preflight_libseat() {
	const char* libseat_backend = std::getenv("LIBSEAT_BACKEND");
	const bool has_seatd_socket = path_exists("/run/seatd.sock");
	const bool has_system_bus = path_exists("/run/dbus/system_bus_socket");
	const char* xdg_session_id = std::getenv("XDG_SESSION_ID");
	const char* xdg_session_type = std::getenv("XDG_SESSION_TYPE");
	const char* xdg_vtnr = std::getenv("XDG_VTNR");
	const bool has_display_env =
		std::getenv("WAYLAND_DISPLAY") != nullptr || std::getenv("DISPLAY") != nullptr;

	if (libseat_backend != nullptr) {
		const std::string backend(libseat_backend);
		if (backend == "seatd" && !has_seatd_socket) {
			wlr_log(WLR_ERROR,
			        "zenith: LIBSEAT_BACKEND=seatd but /run/seatd.sock is missing. "
			        "Install+start seatd, or unset LIBSEAT_BACKEND (or set it to logind) to use systemd-logind.");
		} else if (backend == "logind" && !has_system_bus) {
			wlr_log(WLR_ERROR,
			        "zenith: LIBSEAT_BACKEND=logind but /run/dbus/system_bus_socket is missing. "
			        "Start a system D-Bus (e.g. dbus/dbus-broker) or use seatd instead.");
		} else if (backend == "logind") {
			if (xdg_session_id == nullptr || xdg_session_id[0] == '\0') {
				wlr_log(WLR_ERROR,
				        "zenith: LIBSEAT_BACKEND=logind but XDG_SESSION_ID is not set. "
				        "Logind sessions are created by PAM (e.g. logging in on a TTY); "
				        "starting from ssh/sudo/systemd services often won't work for DRM.");
			}
			if (has_display_env) {
				wlr_log(WLR_ERROR,
				        "zenith: LIBSEAT_BACKEND=logind while DISPLAY/WAYLAND_DISPLAY is set. "
				        "For the DRM backend, run from a real TTY (not from inside another desktop session).");
			}
			if (xdg_session_type != nullptr && std::string(xdg_session_type) != "tty") {
				wlr_log(WLR_ERROR,
				        "zenith: LIBSEAT_BACKEND=logind but XDG_SESSION_TYPE=%s. "
				        "For the DRM backend, start from a TTY login session (XDG_SESSION_TYPE=tty).",
				        xdg_session_type);
			}
			if (xdg_vtnr == nullptr || xdg_vtnr[0] == '\0') {
				wlr_log(WLR_ERROR,
				        "zenith: LIBSEAT_BACKEND=logind but XDG_VTNR is not set. "
				        "This usually means you're not running on a real virtual terminal.");
			}
		}
		return;
	}

	if (!has_system_bus && !has_seatd_socket) {
		wlr_log(WLR_ERROR,
		        "zenith: no seat management backend detected (missing /run/dbus/system_bus_socket and /run/seatd.sock). "
		        "Install+start system D-Bus/systemd-logind, or install+start seatd.");
	}
}

static void zenith_debug_log_loaded_libs() {
	const char* enabled = std::getenv("ZENITH_DEBUG_LOADED_LIBS");
	if (enabled == nullptr || std::string(enabled) != "1") {
		return;
	}

	const char* ld_library_path = std::getenv("LD_LIBRARY_PATH");
	wlr_log(WLR_INFO, "zenith: LD_LIBRARY_PATH=%s", ld_library_path != nullptr ? ld_library_path : "(unset)");

	FILE* f = std::fopen("/proc/self/maps", "r");
	if (f == nullptr) {
		wlr_log(WLR_ERROR, "zenith: failed to open /proc/self/maps for debugging");
		return;
	}

	std::set<std::string> libs;
	char buf[4096];
	while (std::fgets(buf, sizeof(buf), f) != nullptr) {
		char* path = std::strchr(buf, '/');
		if (path == nullptr) {
			continue;
		}

		if (std::strstr(path, "libdrm.so") == nullptr && std::strstr(path, "libgbm.so") == nullptr &&
		    std::strstr(path, "libEGL.so") == nullptr && std::strstr(path, "libGLES") == nullptr &&
		    std::strstr(path, "libwayland-") == nullptr) {
			continue;
		}

		char* nl = std::strchr(path, '\n');
		if (nl != nullptr) {
			*nl = '\0';
		}
		libs.insert(std::string(path));
	}

	std::fclose(f);

	for (const auto& lib : libs) {
		wlr_log(WLR_INFO, "zenith: loaded-lib: %s", lib.c_str());
	}
}

ZenithServer::ZenithServer() {
	main_thread_id = std::this_thread::get_id();
	wlr_log(WLR_INFO, "zenith: build version=%s commit=%s built=%s",
	        zenith_build_version(), zenith_build_git_commit(), zenith_build_timestamp());

	display_scale = read_display_scale();
	const auto multi_monitor_mode = multimonitor::parse_multi_monitor_mode_from_env();
	output_manager = std::make_unique<zenith::ZenithOutputManager>(this, multi_monitor_mode);
	wlr_log(WLR_INFO, "zenith: multi-monitor mode=%s", multimonitor::to_string(multi_monitor_mode));
	zenith_configure_drm_stability(multi_monitor_mode);

	display = wl_display_create();
	if (display == nullptr) {
		wlr_log(WLR_ERROR, "Could not create Wayland display");
		exit(1);
	}

	const bool user_specified_libseat_backend = std::getenv("LIBSEAT_BACKEND") != nullptr;
	zenith_maybe_default_libseat_backend();
	zenith_preflight_libseat();

	const bool has_seatd_socket = path_exists("/run/seatd.sock");
	const bool has_system_bus = path_exists("/run/dbus/system_bus_socket");
	backend = wlr_backend_autocreate(wl_display_get_event_loop(display), NULL);
	if (backend == nullptr && !user_specified_libseat_backend) {
		// Retry once with the alternative backend when available, to be more robust in
		// systemd service/transient-unit scenarios.
		const char* current = std::getenv("LIBSEAT_BACKEND");
		if (has_seatd_socket && !env_equals(current, "seatd")) {
			wlr_log(WLR_INFO, "zenith: retrying wlroots backend creation with LIBSEAT_BACKEND=seatd");
			setenv("LIBSEAT_BACKEND", "seatd", 1);
			backend = wlr_backend_autocreate(wl_display_get_event_loop(display), NULL);
		}
		current = std::getenv("LIBSEAT_BACKEND");
		if (backend == nullptr && has_system_bus && !env_equals(current, "logind")) {
			wlr_log(WLR_INFO, "zenith: retrying wlroots backend creation with LIBSEAT_BACKEND=logind");
			setenv("LIBSEAT_BACKEND", "logind", 1);
			backend = wlr_backend_autocreate(wl_display_get_event_loop(display), NULL);
		}
	}
	if (backend == nullptr) {
		wlr_log(WLR_ERROR, "Could not create wlroots backend");
		exit(2);
	}

	zenith_debug_log_loaded_libs();
	renderer = wlr_renderer_autocreate(backend);
	if (renderer == nullptr) {
		wlr_log(WLR_ERROR,
		        "Could not create wlroots renderer (EGL/GLES2 init failed). "
		        "Check your EGL/GBM/Mesa setup, or build wlroots with a software renderer fallback.");
		exit(3);
	}
	if (!wlr_renderer_init_wl_display(renderer, display)) {
		wlr_log(WLR_ERROR, "Could not initialize wlroots renderer");
		exit(3);
	}

	/*
	 * Auto-creates an allocator for us.
	 * The allocator is the bridge between the renderer and the backend. It handles the buffer creation,
	 * allowing wlroots to render onto the screen.
	 */
	allocator = wlr_allocator_autocreate(backend, renderer);
	if (allocator == nullptr) {
		wlr_log(WLR_ERROR, "Could not create wlroots allocator");
		exit(12);
	}

	compositor = wlr_compositor_create(display, 6, renderer);
	if (compositor == nullptr) {
		wlr_log(WLR_ERROR, "Could not create wlroots compositor");
		exit(4);
	}
	if (wlr_subcompositor_create(display) == nullptr) {
		wlr_log(WLR_ERROR, "Could not create wlroots subcompositor");
		exit(4);
	}
//	surface_destroyed.notify = server_surface_destroyed;
//	new_surface.notify = server_new_surface;
//	wl_signal_add(&compositor->events.new_surface, &new_surface);

	if (wlr_data_device_manager_create(display) == nullptr) {
		wlr_log(WLR_ERROR, "Could not create wlroots data device manager");
		exit(5);
	}

	output_layout = wlr_output_layout_create(display);
	if (output_layout == nullptr) {
		wlr_log(WLR_ERROR, "Could not create wlroots output layout");
		exit(6);
	}

	scene = wlr_scene_create();
	if (scene == nullptr) {
		wlr_log(WLR_ERROR, "Could not create wlroots scene");
		exit(6);
	}

	scene_output_layout = wlr_scene_attach_output_layout(scene, output_layout);
	if (scene_output_layout == nullptr) {
		wlr_log(WLR_ERROR, "Could not attach scene to output layout");
		exit(6);
	}

	xdg_shell = wlr_xdg_shell_create(display, 6);
	if (xdg_shell == nullptr) {
		wlr_log(WLR_ERROR, "Could not create wlroots XDG shell");
		exit(7);
	}

	/*
	 * Configures a seat, which is a single "seat" at which a user sits and
	 * operates the computer. This conceptually includes up to one keyboard,
	 * pointer, touch, and drawing tablet device.
	 */
	seat = wlr_seat_create(display, "seat0");
	if (seat == nullptr) {
		wlr_log(WLR_ERROR, "Could not create wlroots seat");
		exit(8);
	}

	xwayland = wlr_xwayland_create(display, compositor, false);
	if (xwayland == nullptr) {
		wlr_log(WLR_ERROR, "Could not create wlroots Xwayland");
		exit(8);
	}
	wlr_xwayland_set_seat(xwayland, seat);

	text_input_manager = wlr_text_input_manager_v3_create(display);
	if (text_input_manager == nullptr) {
		wlr_log(WLR_ERROR, "Could not create text input manager");
		exit(-1);
	}

	decoration_manager = wlr_xdg_decoration_manager_v1_create(display);
	if (decoration_manager == nullptr) {
		wlr_log(WLR_ERROR, "Could not create text input manager");
		exit(-1);
	}

	data_device_manager = wlr_data_device_manager_create(display);
	if (data_device_manager == nullptr) {
		wlr_log(WLR_ERROR, "Could not create text input manager");
		exit(-1);
	}

	relative_pointer_manager = wlr_relative_pointer_manager_v1_create(display);
	if (relative_pointer_manager == nullptr) {
		wlr_log(WLR_ERROR, "Could not create relative pointer manager");
		exit(-1);
	}

	pointer_constraints = wlr_pointer_constraints_v1_create(display);
	if (pointer_constraints == nullptr) {
		wlr_log(WLR_ERROR, "Could not create pointer constraints manager");
		exit(-1);
	}
	active_pointer_constraint_destroy.notify = server_active_pointer_constraint_destroy;
	wl_list_init(&active_pointer_constraint_destroy.link);

	// Called at the start for each available output, but also when the user plugs in a monitor.
	new_output.notify = output_create_handle;
	wl_signal_add(&backend->events.new_output, &new_output);

	new_surface.notify = zenith_surface_create;
	wl_signal_add(&compositor->events.new_surface, &new_surface);

	new_xdg_toplevel.notify = zenith_xdg_toplevel_create;
	wl_signal_add(&xdg_shell->events.new_toplevel, &new_xdg_toplevel);

	new_xdg_popup.notify = zenith_xdg_popup_create;
	wl_signal_add(&xdg_shell->events.new_popup, &new_xdg_popup);

	xwayland_ready.notify = server_xwayland_ready;
	wl_signal_add(&xwayland->events.ready, &xwayland_ready);

	new_xwayland_surface.notify = zenith_xwayland_surface_create;
	wl_signal_add(&xwayland->events.new_surface, &new_xwayland_surface);

	// Called at the start for each available input device, but also when the user plugs in a new input
	// device, like a mouse, keyboard, drawing tablet, etc.
	new_input.notify = server_new_input;
	wl_signal_add(&backend->events.new_input, &new_input);

	// Programs can request to change the cursor image.
	request_cursor.notify = server_seat_request_cursor;
	wl_signal_add(&seat->events.request_set_cursor, &request_cursor);

	new_text_input.notify = text_input_create_handle;
	wl_signal_add(&text_input_manager->events.text_input, &new_text_input);

	new_toplevel_decoration.notify = toplevel_decoration_create_handle;
	wl_signal_add(&decoration_manager->events.new_toplevel_decoration, &new_toplevel_decoration);

	request_set_selection.notify = server_seat_request_set_selection;
	wl_signal_add(&seat->events.request_set_selection, &request_set_selection);

	auto callable_queue_function = [](int fd, uint32_t mask, void* data) {
		auto* server = ZenithServer::instance();
		return (int) server->callable_queue.execute();
	};

	auto* event_loop = wl_display_get_event_loop(display);
	wl_event_loop_add_fd(event_loop, callable_queue.get_fd(), WL_EVENT_READABLE, callable_queue_function, nullptr);

	load_egl_extensions();
	// TODO: Implement drag and drop.
}

void ZenithServer::run(const char* startup_command) {
	this->startup_command = startup_command;

	wlr_egl_make_current(wlr_gles2_renderer_get_egl(renderer), NULL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	const char* socket = wl_display_add_socket_auto(display);
	if (!socket) {
		wlr_log(WLR_ERROR, "Could not create a Wayland socket");
		wlr_backend_destroy(backend);
		exit(9);
	}

	// Make sure the X11 session from the host is not visible because some programs prefer talking
	// to the X server instead of defaulting to Wayland.
	unsetenv("DISPLAY");

	setenv("WAYLAND_DISPLAY", socket, true);
	setenv("XDG_SESSION_TYPE", "wayland", true);

	wlr_egl* main_egl = wlr_gles2_renderer_get_egl(renderer);

	// Create 2 OpenGL shared contexts for rendering operations.
	wlr_egl* flutter_gl_context = create_shared_egl_context(main_egl);
	wlr_egl* flutter_resource_gl_context = create_shared_egl_context(main_egl);

	embedder_state = std::make_unique<EmbedderState>(flutter_gl_context, flutter_resource_gl_context);

	// Run the engine.
	embedder_state->run_engine();

	if (!wlr_backend_start(backend)) {
		wlr_log(WLR_ERROR, "Could not start backend");
		wlr_backend_destroy(backend);
		wl_display_destroy(display);
		exit(10);
	}

	// Fallback startup kick: make sure at least one frame is scheduled right
	// after backend start so first client content can appear without input.
	output_manager->schedule_compositor_frame();

	wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);

	wl_display_run(display);

	// wlroots asserts protocol event listener lists are empty on teardown.
	wl_list_remove(&new_output.link);
	wl_list_remove(&new_surface.link);
	wl_list_remove(&new_xdg_toplevel.link);
	wl_list_remove(&new_xdg_popup.link);
	wl_list_remove(&xwayland_ready.link);
	wl_list_remove(&new_xwayland_surface.link);
	wl_list_remove(&new_input.link);
	wl_list_remove(&request_cursor.link);
	wl_list_remove(&new_text_input.link);
	wl_list_remove(&new_toplevel_decoration.link);
	wl_list_remove(&request_set_selection.link);

	keyboards.clear();

	wlr_backend_destroy(backend);
	wl_display_destroy_clients(display);
	wl_display_destroy(display);
}

static void server_xwayland_ready(wl_listener* listener, void* data) {
	(void)data;
	ZenithServer* server = wl_container_of(listener, server, xwayland_ready);
	if (server->xwayland == nullptr || server->xwayland->display_name == nullptr) {
		return;
	}
	server->xwayland_is_ready = true;
	setenv("DISPLAY", server->xwayland->display_name, true);
	wlr_log(WLR_INFO, "Running Xwayland on DISPLAY=%s", server->xwayland->display_name);
	if (server->output_manager != nullptr) {
		server->output_manager->refresh_xwayland_workareas();
	}
}

static float read_display_scale() {
	static const char* display_scale_str = getenv("ZENITH_SCALE");
	if (display_scale_str == nullptr) {
		return 1.0f;
	}
	try {
		return std::stof(display_scale_str);
	} catch (std::invalid_argument&) {
		return 1.0f;
	} catch (std::out_of_range&) {
		return 1.0f;
	}
}

void server_new_input(wl_listener* listener, void* data) {
	ZenithServer* server = wl_container_of(listener, server, new_input);
	auto* wlr_device = static_cast<wlr_input_device*>(data);

	switch (wlr_device->type) {
		case WLR_INPUT_DEVICE_KEYBOARD: {
			auto keyboard = std::make_unique<ZenithKeyboard>(server, wlr_device);
			server->keyboards.push_back(std::move(keyboard));
			break;
		}
		case WLR_INPUT_DEVICE_POINTER: {
			if (server->pointer == nullptr) {
				// Regardless of the number of input devices, there will be only one pointer on the
				// screen if at least one input device exists.
				server->pointer = std::make_unique<ZenithPointer>(server);
			}

			bool is_touchpad = wlr_input_device_is_libinput(wlr_device);
			if (is_touchpad) {
				// Enable tapping by default on all touchpads.
				libinput_device* device = wlr_libinput_get_device_handle(wlr_device);
				libinput_device_config_tap_set_enabled(device, LIBINPUT_CONFIG_TAP_ENABLED);
				libinput_device_config_tap_set_drag_enabled(device, LIBINPUT_CONFIG_DRAG_ENABLED);
				libinput_device_config_scroll_set_natural_scroll_enabled(device, true);
				libinput_device_config_dwt_set_enabled(device, LIBINPUT_CONFIG_DWT_ENABLED);
			}

			wlr_cursor_attach_input_device(server->pointer->cursor, wlr_device);
			break;
		}
		case WLR_INPUT_DEVICE_TOUCH: {
			auto touch_device = std::make_unique<ZenithTouchDevice>(server, wlr_device);
			server->touch_devices.push_back(std::move(touch_device));
			break;
			// TODO: handle destruct callback
		}
		default:
			break;
	}

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

void server_seat_request_cursor(wl_listener* listener, void* data) {
	ZenithServer* server = wl_container_of(listener, server, request_cursor);

	auto* event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
	wlr_seat_client* focused_client = server->seat->pointer_state.focused_client;
	const bool allowed = focused_client == event->seat_client;
	if (zenith_xwayland_input_debug_enabled()) {
		wlr_surface* focused_surface = server->seat->pointer_state.focused_surface;
		auto* focused_xwayland = focused_surface != nullptr
			? wlr_xwayland_surface_try_from_wlr_surface(focused_surface)
			: nullptr;
		const bool focused_is_xwayland = focused_xwayland != nullptr;
		if (focused_is_xwayland || !allowed) {
			wlr_log(
				WLR_INFO,
				"zenith:xw-cursor-request allowed=%d focused_client=%p req_client=%p focused_surface=%p focused_xw=%d focused_title=\"%s\"",
				allowed ? 1 : 0,
				(void*) focused_client,
				(void*) event->seat_client,
				(void*) focused_surface,
				focused_is_xwayland ? 1 : 0,
				focused_xwayland != nullptr && focused_xwayland->title != nullptr ? focused_xwayland->title : ""
			);
		}
	}
	/* This can be sent by any client, so we check to make sure this one is
	 * actually has pointer focus first. */
	if (allowed) {
		if (zenith_cursor_debug_enabled()) {
			wlr_log(
				WLR_INFO,
				"zenith:cursor request_set_cursor allowed=1 event_surface=%p mapped=%d pointer_visible=%d forced_hidden=%d current=%s",
				(void*) event->surface,
				(event->surface != nullptr && event->surface->mapped) ? 1 : 0,
				(server->pointer != nullptr && server->pointer->is_visible()) ? 1 : 0,
				(server->pointer != nullptr && server->pointer->is_forced_hidden()) ? 1 : 0,
				server->pointer != nullptr ? server->pointer->current_cursor_name() : "<none>"
			);
		}
		/* Once we've vetted the client, we can tell the cursor to use the
		 * provided surface as the cursor image. It will set the hardware cursor
		 * on the output that it's currently on and continue to do so as the
		 * cursor moves between outputs. */
		if (server->pointer != nullptr && server->pointer->is_visible()) {
			if (event->surface != nullptr) {
				if (event->surface->mapped) {
					wlr_cursor_set_surface(
						server->pointer->cursor,
						event->surface,
						event->hotspot_x,
						event->hotspot_y
					);
				} else {
					// Ignore stale cursor surfaces from unmapped windows.
					if (zenith_cursor_debug_enabled()) {
						wlr_log(WLR_INFO, "zenith:cursor request_set_cursor ignored unmapped surface");
					}
					server->pointer->restore_default_cursor();
				}
			} else {
				// Some clients may request a null cursor surface during teardown.
				// Keep compositor cursor visible unless explicitly hidden by JDWM.
				if (zenith_cursor_debug_enabled()) {
					wlr_log(WLR_INFO, "zenith:cursor request_set_cursor null surface -> restore default");
				}
				server->pointer->restore_default_cursor();
			}
			// If this is a software cursor, it becomes visible on the next rendered frame.
			server->output_manager->schedule_cursor_frame(
				server->pointer->cursor->x, server->pointer->cursor->y);
		}
	}
}

void server_update_pointer_constraint(ZenithServer* server) {
	if (server == nullptr || server->pointer == nullptr || server->pointer_constraints == nullptr) {
		return;
	}
	wlr_surface* focused = server->seat->pointer_state.focused_surface;
	wlr_pointer_constraint_v1* constraint = nullptr;
	if (focused != nullptr) {
		constraint = wlr_pointer_constraints_v1_constraint_for_surface(
			server->pointer_constraints,
			focused,
			server->seat
		);
	}

	if (constraint == server->active_pointer_constraint) {
		server->pointer->set_client_locked(
			constraint != nullptr && constraint->type == WLR_POINTER_CONSTRAINT_V1_LOCKED
		);
		return;
	}

	if (server->active_pointer_constraint != nullptr) {
		wlr_pointer_constraint_v1_send_deactivated(server->active_pointer_constraint);
		detach_active_pointer_constraint_listener(server);
	}
	server->active_pointer_constraint = constraint;
	if (constraint != nullptr) {
		wl_signal_add(&constraint->events.destroy, &server->active_pointer_constraint_destroy);
		wlr_pointer_constraint_v1_send_activated(constraint);
	}
	server->pointer->set_client_locked(
		constraint != nullptr && constraint->type == WLR_POINTER_CONSTRAINT_V1_LOCKED
	);
}

void server_active_pointer_constraint_destroy(wl_listener* listener, void* data) {
	(void) data;
	ZenithServer* server = wl_container_of(listener, server, active_pointer_constraint_destroy);
	server->active_pointer_constraint = nullptr;
	if (server->pointer != nullptr) {
		server->pointer->set_client_locked(false);
	}
	detach_active_pointer_constraint_listener(server);
}

static void detach_active_pointer_constraint_listener(ZenithServer* server) {
	auto* link = &server->active_pointer_constraint_destroy.link;
	if (link->next != nullptr && link->prev != nullptr) {
		wl_list_remove(link);
		wl_list_init(link);
	}
}

void server_seat_request_set_selection(wl_listener* listener, void* data) {
	ZenithServer* server = wl_container_of(listener, server, request_set_selection);
	auto* event = static_cast<wlr_seat_request_set_selection_event*>(data);
	wlr_seat_set_selection(server->seat, event->source, event->serial);
	// TODO: Add security. Don't let any client overwrite the clipboard randomly.
}

bool is_main_thread() {
	return std::this_thread::get_id() == ZenithServer::instance()->main_thread_id;
}
