#pragma once
#include <cstddef>
#include <memory>

struct ZenithServer;
struct wlr_xwayland_surface;

namespace zenith {

// Protocol acquisition only: application and theme policy belongs to the desktop.
class WindowMetadata {
  public:
	explicit WindowMetadata(ZenithServer *server);
	~WindowMetadata();

	void connect_xwayland(const char *display);
	void map(size_t id, wlr_xwayland_surface *x11 = nullptr);
	void unmap(size_t id);
	void forget(size_t id);

  private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace zenith
