#pragma once

#include <unordered_map>  // for std::unordered_map
#include <memory>         // for std::shared_ptr
#include <vector>         // if you use std::vector anywhere
#include <list>           // if you use std::list anywhere
#include <mutex>
#include <array>
#include <optional>
#include <string>
#include "embedder.h"
#include "platform_channels/binary_messenger.hpp"
#include "platform_channels/incoming_message_dispatcher.hpp"
#include "platform_channels/method_channel.h"
#include "task_runner.hpp"
#include "vsync_scheduler.hpp"
#include "basic_message_channel.h"
#include "document.h"
#include "surface_buffer_chain.hpp"
#include "external_texture_registry.hpp"
#include "callable_queue.hpp"
#include "util/rethreading/stop_signal.hpp"
#include "message_structs.hpp"
#include "text_input_model.h"
#include "text_input_client.hpp"
#include "key_repeater.hpp"

extern "C" {
#include <wlr/render/egl.h>
}

struct ZenithServer;
struct wlr_gles2_buffer;
template<class T> struct SwapChain;

struct EmbedderState {
	EmbedderState(wlr_egl* flutter_gl_context, wlr_egl* flutter_resource_gl_context);

	void run_engine();
	// Called on the Wayland thread, while its objects and release queue live.
	void stop_engine();
	~EmbedderState();

	void schedule_frame();

	FlutterEngine get_engine() const;

	VsyncDispatch poll_vsync(uint64_t now_ns, uint64_t interval_ns, bool frame_ready);

	void set_baton(intptr_t baton);

	void send_window_metrics(FlutterWindowMetricsEvent metrics);

	void on_vsync(intptr_t baton, uint64_t frame_start_time_nanos, uint64_t frame_target_time_nanos);

	void send_pointer_event(FlutterPointerEvent pointer_event);

	void send_key_event(const KeyboardKeyEventMessage& message);

	void register_external_texture(int64_t id);

	void mark_external_texture_frame_available(int64_t id);

	void commit_surface(const SurfaceCommitMessage& message);

	void map_xdg_surface(size_t view_id, int role);

	void unmap_xdg_surface(size_t view_id, int role);

	void map_toplevel_surface(size_t view_id, const std::string& protocol);

	void unmap_toplevel_surface(size_t view_id, const std::string& protocol);

	void commit_toplevel_surface(size_t view_id,
	                             const std::string& protocol,
	                             int x,
	                             int y,
	                             int width,
	                             int height,
	                             std::optional<ToplevelDecoration> decoration = std::nullopt,
	                             std::optional<std::string> title = std::nullopt,
	                             std::optional<std::string> app_id = std::nullopt);

	void map_subsurface(size_t view_id);

	void unmap_subsurface(size_t view_id);

	void send_text_input_event(size_t view_id, TextInputEventType event_type);

	void interactive_move(size_t view_id);

	void interactive_resize(size_t view_id, xdg_toplevel_resize_edge edge);

	void set_window_title(size_t view_id, const std::string& title, const std::string& protocol = "xdg");

	void set_app_id(size_t view_id, const std::string& app_id, const std::string& protocol = "xdg");

	void set_window_state(size_t view_id, bool maximized, bool visible, const std::string& protocol = "xdg");

	void publish_monitor_layout();

	void update_text_editing_state();

	StopSignal stop_signal;

	/*
	 * The following fields have to be public because they are used by Flutter engine callbacks,
	 * which are just plain C functions.
	 */

	IncomingMessageDispatcher message_dispatcher;

	// Flutter needs separate OpenGL contexts to render asynchronously.
	wlr_egl* flutter_gl_context = nullptr;
	wlr_egl* flutter_resource_gl_context = nullptr;

	// Keeps alive surface buffers, even after the surfaces are destroyed.
	// This allows Flutter to play closing animations on surfaces that don't exist anymore.
	ExternalTextureRegistry external_textures;

	// Raster-thread-only transaction; pins a generation across output changes.
	std::shared_ptr<SwapChain<wlr_gles2_buffer>> raster_target;
	uint32_t raster_fbo = 0;

	// A function queue tied to the event loop.
	CallableQueue callable_queue;
	wl_event_loop* event_loop;
	std::optional<TextInputClient> text_input_client = {};

	// Represents the 'flutter/textinput' channel provided by Flutter.
	std::unique_ptr<flutter::MethodChannel<rapidjson::Document>> text_input_method_channel;

private:
	void configure_and_run_engine();

	void register_platform_api();

	FlutterEngine engine = nullptr;
	FlutterEngineAOTData aot_data = nullptr;
	std::atomic_bool thread_stopped{false};
	bool loop_running = true; // Embedder thread only.
	wl_event_source* task_source = nullptr;
	wl_event_source* queue_source = nullptr;
	// The thread on which the Flutter engine and its task runner is run.
	std::thread embedder_thread;

	// Keep Flutter's pending vsync request until the active output's next tick.
	// Output retries and idle callbacks must not generate extra vsync replies.
	VsyncScheduler vsync_scheduler;
	// Flutter needs some tasks run on the main thread. This task runner is tied to the Wayland
	// event loop and schedules tasks at the right time.
	TaskRunner platform_task_runner{};
	// Send messages to Dart code.
	BinaryMessenger messenger{};
	/// Allow Dart code to call C++ methods though this channel.
	std::unique_ptr<flutter::MethodChannel<>> platform_method_channel;
	// Represents the 'flutter/mousecursor' channel provided by Flutter. When the cursor should
	// change, we receive a method call on this channel.
	std::unique_ptr<flutter::MethodChannel<>> mouse_cursor_method_channel;
	// Represents the `flutter/keyevent` channel provided by Flutter to send key events.
	// https://api.flutter.dev/flutter/services/KeyEvent-class.html
	std::unique_ptr<flutter::BasicMessageChannel<rapidjson::Document>> key_event_channel;

	KeyRepeater key_repeater;
};
