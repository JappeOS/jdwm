#pragma once

namespace zenith {
// Publish DISPLAY, WAYLAND_DISPLAY and XDG_SESSION_TYPE on the user bus.
// Waits for replies. No bus/manager is allowed; other errors throw and must
// prevent readiness notification. Call after setting the display environment.
void publish_session_environment();
}
