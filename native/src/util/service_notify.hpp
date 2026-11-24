#pragma once

namespace zenith {
// Send READY=1 to NOTIFY_SOCKET without requiring libsystemd.
// Returns 1 if sent, 0 if unconfigured, or a negative errno on failure.
int notify_ready();
}
