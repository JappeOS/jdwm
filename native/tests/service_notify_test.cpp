#include "util/service_notify.hpp"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static void check_delivery(const std::string& path) {
	const int receiver = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	assert(receiver >= 0);
	// A real supervisor requests credentials to identify the sending process.
	const int enabled = 1;
	assert(setsockopt(receiver, SOL_SOCKET, SO_PASSCRED, &enabled, sizeof(enabled)) == 0);
	sockaddr_un address{};
	address.sun_family = AF_UNIX;
	const bool abstract = path[0] == '@';
	std::memcpy(address.sun_path, path.data(), path.size());
	if (abstract) address.sun_path[0] = '\0';
	assert(bind(receiver, reinterpret_cast<const sockaddr*>(&address),
		offsetof(sockaddr_un, sun_path) + path.size() + (abstract ? 0 : 1)) == 0);
	assert(setenv("NOTIFY_SOCKET", path.c_str(), 1) == 0);
	assert(zenith::notify_ready() == 1);

	char payload[32]{};
	iovec vector{payload, sizeof(payload)};
	alignas(cmsghdr) char control[CMSG_SPACE(sizeof(ucred))]{};
	msghdr message{};
	message.msg_iov = &vector;
	message.msg_iovlen = 1;
	message.msg_control = control;
	message.msg_controllen = sizeof(control);
	assert(recvmsg(receiver, &message, 0) == 7);
	assert(std::memcmp(payload, "READY=1", 7) == 0);
	const auto* header = CMSG_FIRSTHDR(&message);
	assert(header && header->cmsg_level == SOL_SOCKET && header->cmsg_type == SCM_CREDENTIALS);
	ucred credentials{};
	std::memcpy(&credentials, CMSG_DATA(header), sizeof(credentials));
	assert(credentials.pid == getpid());
	assert(recv(receiver, payload, sizeof(payload), 0) == -1 && errno == EAGAIN);
	close(receiver);
	if (!abstract) assert(unlink(path.c_str()) == 0);
}

int main() {
	assert(unsetenv("NOTIFY_SOCKET") == 0);
	assert(zenith::notify_ready() == 0);
	assert(setenv("NOTIFY_SOCKET", "", 1) == 0);
	assert(zenith::notify_ready() == 0);
	for (const char* invalid : {"relative", "@", "/"}) {
		assert(setenv("NOTIFY_SOCKET", invalid, 1) == 0);
		assert(zenith::notify_ready() == -EINVAL);
	}
	const std::string too_long = "/" + std::string(108, 'x');
	assert(setenv("NOTIFY_SOCKET", too_long.c_str(), 1) == 0);
	assert(zenith::notify_ready() == -ENAMETOOLONG);
	char directory[] = "/tmp/zenith-notify-XXXXXX";
	assert(mkdtemp(directory));
	const std::string path = std::string(directory) + "/notify";
	assert(setenv("NOTIFY_SOCKET", path.c_str(), 1) == 0);
	assert(zenith::notify_ready() == -ENOENT);
	check_delivery(path);
	check_delivery("@zenith-notify-" + std::to_string(getpid()));
	// Exercise the maximum abstract address length (no terminating NUL).
	check_delivery("@" + std::string(90, 'x') + std::string(17 - std::to_string(getpid()).size(), 'y') + std::to_string(getpid()));
	assert(rmdir(directory) == 0);
}
