#pragma once

#include <platform_channels/binary_messenger.hpp>
#include <platform_channels/incoming_message_dispatcher.hpp>
#include <platform_channels/method_channel.h>
#include <memory>
#include <mutex>
#include "embedder_state.hpp"
#include "swap_chain.hpp"
#include "util/wlr/wlr_helpers.hpp"
#include <GLES2/gl2.h>

extern "C" {
#define static
#include <wlr/util/addon.h>
#include <wlr/types/wlr_scene.h>
#undef static
}

struct ZenithServer;

struct ZenithOutput {
	explicit ZenithOutput(struct wlr_output* wlr_output);

	struct wlr_output* wlr_output = nullptr;
	wl_listener frame_listener{};
	wl_listener request_state_listener{};
	wl_listener destroy{};
	wl_event_source* schedule_frame_timer;
	bool software_cursor_locked = false;
	bool cursor_mode_logged = false;
	bool last_software_cursor_active = false;

	wlr_scene_output* scene_output = nullptr;
	wlr_scene_buffer* scene_buffer = nullptr;
	wlr_buffer* last_scene_buffer = nullptr;
	int swapchain_width = 0;
	int swapchain_height = 0;

	std::unique_ptr<SwapChain<wlr_gles2_buffer>> swap_chain;

	bool enable();

	bool disable();

	void recreate_swapchain();
	void recreate_swapchain(int width, int height);
};

/*
 * This event is raised when a new output is detected, like a monitor or a projector.
 */
void output_create_handle(wl_listener* listener, void* data);

/*
 * This function is called every time an output is ready to display a frame, generally at the output's refresh rate.
 */
void output_frame(wl_listener* listener, void* data);

void output_request_state(wl_listener* listener, void* data);

int vsync_callback(void* data);

void output_destroy(wl_listener* listener, void* data);
