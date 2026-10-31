#pragma once
#include <wayland-server-core.h>

struct ZenithServer;
struct wlr_output_manager_v1;
struct wlr_output_configuration_v1;

namespace zenith {

// Protocol adapter. The output manager owns policy and transactions.
class OutputManagement {
public:
	explicit OutputManagement(ZenithServer* server);
	~OutputManagement();

	void publish();
private:
	ZenithServer* server_;
	wlr_output_manager_v1* protocol_ = nullptr; // Owned by wl_display.
	wl_listener apply_{}, test_{}, destroy_{};

	static void apply(wl_listener*, void*);
	static void test(wl_listener*, void*);
	static void destroyed(wl_listener*, void*);
	void request(wlr_output_configuration_v1*, bool test_only);
};
}
