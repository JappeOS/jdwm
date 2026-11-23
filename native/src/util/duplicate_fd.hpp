#pragma once
#include <cerrno>
#include <fcntl.h>

namespace zenith {
inline int duplicate_cloexec(int fd) {
	int copy;
	do {
		copy = fcntl(fd, F_DUPFD_CLOEXEC, 0);
	} while (copy < 0 && errno == EINTR);
	return copy;
}
}
