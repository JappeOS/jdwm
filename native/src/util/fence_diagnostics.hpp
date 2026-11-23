#pragma once

#include <cerrno>
#include <cstdint>
#include <cstring>
#include "process_diagnostics.hpp"

extern "C" {
#include <wlr/util/log.h>
}

namespace zenith::diagnostics {

struct FenceFailures {
	uint64_t count = 0;
	int last_errno = 0;
	int last_egl_error = 0;

	void record(
		const char* operation,
		int system_error = 0,
		int egl_error = 0,
	    int fd = -1,
		const char* output = "-"
	) {
		const int saved_errno = errno;
		if (system_error != last_errno || egl_error != last_egl_error) {
			count = 0;
			last_errno = system_error;
			last_egl_error = egl_error;
		}

		++count;

		// Keep the first failure and powers of two during a retry storm.
		if ((count & (count - 1)) == 0) {
			wlr_log(WLR_ERROR,
			        "zenith:fence op=%s count=%llu errno=%d (%s) egl_error=%#x fd=%d "
			        "open_fds=%lld soft_limit=%lld output=%s",
			        operation, static_cast<unsigned long long>(count), system_error,
			        system_error ? std::strerror(system_error) : "unspecified",
			        egl_error, fd, open_fd_count(), fd_soft_limit(), output);
		}
		
		errno = saved_errno;
	}
};

}
