// Link with the backend and the matching Flutter engine. Place beside a small
// animated Flutter bundle (data/flutter_assets, data/icudtl.dat, lib/libapp.so).
// Run with WLR_BACKENDS=headless and send SIGINT/SIGTERM, or pass "shortcut".
#include "server.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <exception>

extern "C" {
#include <wlr/interfaces/wlr_keyboard.h>
#include <wlr/interfaces/wlr_touch.h>
#include <wlr/interfaces/wlr_pointer.h>
#include <wlr/util/log.h>
}

int main(int argc, char** argv) try {
	wlr_log_init(WLR_INFO, nullptr);
	sigset_t original_mask;
	pthread_sigmask(SIG_SETMASK, nullptr, &original_mask);
	auto* server = ZenithServer::instance();
	wlr_keyboard keyboard{};
	wlr_keyboard_init(&keyboard, nullptr, "shutdown-test-keyboard");
	server_new_input(&server->new_input, &keyboard.base);
	wlr_touch touch{};
	wlr_touch_init(&touch, nullptr, "shutdown-test-touch");
	server_new_input(&server->new_input, &touch.base);
	wlr_pointer pointer{};
	wlr_pointer_init(&pointer, nullptr, "shutdown-test-pointer");
	server_new_input(&server->new_input, &pointer.base);
	// Exercise hot-unplug independently of final teardown.
	wlr_touch unplugged{};
	wlr_touch_init(&unplugged, nullptr, "shutdown-test-unplugged-touch");
	server_new_input(&server->new_input, &unplugged.base);
	wlr_touch_finish(&unplugged);
	auto* key_timer = wl_event_loop_add_timer(wl_display_get_event_loop(server->display), [](void* data) {
		auto* server = static_cast<ZenithServer*>(data);
		server->embedder_state->send_key_event({KeyboardKeyState::press, 30, 38, XKB_KEY_a, 0});
		return 0;
	}, server);
	wl_event_source_timer_update(key_timer, 500); // Hold a key through shutdown.
	const bool early = argc > 1 && std::strcmp(argv[1], "early") == 0;
	if (early || (argc > 1 && std::strcmp(argv[1], "shortcut") == 0)) {
		auto* timer = wl_event_loop_add_timer(wl_display_get_event_loop(server->display), [](void* data) {
			auto* server = static_cast<ZenithServer*>(data);
			assert(handle_shortcuts(server->keyboards.front().get(), WLR_MODIFIER_ALT, XKB_KEY_F4));
			server->request_shutdown(); // Also exercise duplicate requests.
			return 0;
		}, server);
		wl_event_source_timer_update(timer, early ? 1 : 1500);
	}
	int status = server->run("");
	server->shutdown(); // Cleanup must be idempotent.
	wlr_touch_finish(&touch); // Assert no compositor listeners survive cleanup.
	wlr_keyboard_finish(&keyboard);
	wlr_pointer_finish(&pointer);
	delete server;
	sigset_t restored_mask;
	pthread_sigmask(SIG_SETMASK, nullptr, &restored_mask);
	assert(sigismember(&restored_mask, SIGINT) == sigismember(&original_mask, SIGINT));
	assert(sigismember(&restored_mask, SIGTERM) == sigismember(&original_mask, SIGTERM));
	return status;
} catch (const std::exception& error) {
	std::fprintf(stderr, "shutdown smoke: %s\n", error.what());
	return 1;
}
