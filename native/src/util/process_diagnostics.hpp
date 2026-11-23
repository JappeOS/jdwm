#pragma once
#include <sys/stat.h>
#include <sys/resource.h>

namespace zenith::diagnostics {
	
// Linux exposes the open descriptor count as the size of /proc/self/fd.
// stat() still works when the process has no spare descriptor for opendir().
inline long long open_fd_count() {
	struct stat info{};
	return stat("/proc/self/fd", &info) == 0 ? static_cast<long long>(info.st_size) : -1;
}

inline long long fd_soft_limit() {
	struct rlimit limit{};
	return getrlimit(RLIMIT_NOFILE, &limit) == 0 ? static_cast<long long>(limit.rlim_cur) : -1;
}

}
