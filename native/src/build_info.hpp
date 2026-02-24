#pragma once

#ifndef ZENITH_BUILD_VERSION
#define ZENITH_BUILD_VERSION "unknown"
#endif

#ifndef ZENITH_BUILD_GIT_COMMIT
#define ZENITH_BUILD_GIT_COMMIT "unknown"
#endif

#ifndef ZENITH_BUILD_TIMESTAMP
#define ZENITH_BUILD_TIMESTAMP "unknown"
#endif

inline const char* zenith_build_version() {
	return ZENITH_BUILD_VERSION;
}

inline const char* zenith_build_git_commit() {
	return ZENITH_BUILD_GIT_COMMIT;
}

inline const char* zenith_build_timestamp() {
	return ZENITH_BUILD_TIMESTAMP;
}
