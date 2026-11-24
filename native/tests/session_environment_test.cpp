// Driver for session_environment_test.py; uses the production publication and
// notification functions in their startup order, without requiring a GPU.
#include "util/session_environment.hpp"
#include "util/service_notify.hpp"
#include <cstdio>
#include <exception>

int main() {
	try {
		zenith::publish_session_environment();
		return zenith::notify_ready() < 0 ? 2 : 0;
	} catch (const std::exception& error) {
		std::fprintf(stderr, "%s\n", error.what());
		return 1;
	}
}
