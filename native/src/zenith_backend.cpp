#include "zenith_backend/zenith_backend.hpp"

#include "server.hpp"

int zenith_backend_run(const char* startup_cmd) {
	ZenithServer::instance()->run(startup_cmd ? startup_cmd : "");
	return 0;
}
