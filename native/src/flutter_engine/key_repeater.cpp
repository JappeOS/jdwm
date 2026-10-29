#include "server.hpp"
#include "key_repeater.hpp"
#include <algorithm>

void KeyRepeater::schedule_repeat(KeyboardKeyEventMessage event) {
	cancel_repeat();
	auto* server = ZenithServer::instance();
	const auto request = generation;
	// Never block the platform thread waiting for Wayland (especially on exit).
	server->callable_queue.enqueue([this, server, request, event] {
		auto* keyboard = server->seat->keyboard_state.keyboard;
		if (!keyboard || server->is_shutting_down())
			return;

		const auto repeat = keyboard->repeat_info;
		server->embedder_state->callable_queue.enqueue([this, server, request, event, repeat] {
			if (request != generation || repeat.rate <= 0)
				return;

			delay = repeat.delay;
			rate = std::max(1, 1000 / repeat.rate); // Wayland rate is keys/second.
			repeated_event = event;
			repeated_event.state = KeyboardKeyState::repeat;

			timer = wl_event_loop_add_timer(server->embedder_state->event_loop, [](void* data) -> int {
				auto* self = static_cast<KeyRepeater*>(data);
				ZenithServer::instance()->embedder_state->send_key_event(self->repeated_event);
				wl_event_source_timer_update(self->timer, self->rate);
				return 0;
			}, this);

			if (timer)
				wl_event_source_timer_update(timer, std::max(1, delay));
		});
	});
}

// Both repeat requests and cancellation run on Flutter's platform thread.
void KeyRepeater::cancel_repeat() {
	++generation;
	if (timer) {
		wl_event_source_remove(timer);
		timer = nullptr;
	}
}
