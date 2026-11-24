#include "service_notify.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace zenith {
int notify_ready() {
	const char* path = std::getenv("NOTIFY_SOCKET");
	if (!path || !*path) return 0;

	sockaddr_un address{};
	const size_t length = std::strlen(path);
	const bool abstract = path[0] == '@';
	if ((!abstract && path[0] != '/') || length < 2)
		return -EINVAL;
	if (length > sizeof(address.sun_path) ||
		(!abstract && length == sizeof(address.sun_path)))
		return -ENAMETOOLONG;

	address.sun_family = AF_UNIX;
	std::memcpy(address.sun_path, path, length);
	if (abstract)
		address.sun_path[0] = '\0';
	const auto address_length = static_cast<socklen_t>(
		offsetof(sockaddr_un, sun_path) + length + (abstract ? 0 : 1));

	// Never stall compositor startup on a full or unresponsive notify socket.
	const int fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (fd < 0)
		return -errno;

	constexpr char message[] = "READY=1";
	ssize_t sent;
	do {
		sent = sendto(fd, message, sizeof(message) - 1, MSG_NOSIGNAL,
			reinterpret_cast<const sockaddr*>(&address), address_length);
	} while (sent < 0 && errno == EINTR);

	const int result = sent < 0 ? -errno : (sent == sizeof(message) - 1 ? 1 : -EIO);
	close(fd);

	return result;
}
}
