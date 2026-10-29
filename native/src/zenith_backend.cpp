#include "zenith_backend/zenith_backend.hpp"

#include "server.hpp"
#include <exception>
#include <cstdio>

int zenith_backend_run(const char* startup_cmd) {
	try {
		auto server = std::unique_ptr<ZenithServer>(ZenithServer::instance());
		return server->run(startup_cmd ? startup_cmd : "");
	} catch (const std::exception& error) {
		std::fprintf(stderr, "zenith: %s\n", error.what());
		return 1;
	}
}
