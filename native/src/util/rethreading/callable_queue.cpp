#include "callable_queue.hpp"
#include <unistd.h>
#include <cerrno>
#include <system_error>

CallableQueue::CallableQueue() : fd(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) {
	if (fd < 0)
		throw std::system_error(errno, std::generic_category(), "callable queue eventfd");
}

CallableQueue::~CallableQueue() {
	close();
	// Destroy captures while the queue's mutex still exists. A capture may
	// release another object whose destructor attempts to enqueue cleanup.
	std::queue<Callable> discarded;
	{
		std::scoped_lock lock(mutex);
		discarded.swap(callables);
	}
	::close(fd);
}

int CallableQueue::get_fd() const {
	return fd;
}

bool CallableQueue::enqueue(const CallableQueue::Callable& callable) {
	std::scoped_lock lock(mutex);
	if (closed)
		return false;
	callables.push(callable);
	while (eventfd_write(fd, 1) < 0 && errno == EINTR) {}
	return true;
}

void CallableQueue::close(const CallableQueue::Callable& final) {
	std::scoped_lock lock(mutex);
	if (closed) return;
	closed = true;
	if (final) callables.push(final);
	while (eventfd_write(fd, 1) < 0 && errno == EINTR) {}
}

bool CallableQueue::execute() {
	std::queue<Callable> processing;
	{
		std::scoped_lock lock(mutex);
		eventfd_t value;
		int result;
		do { result = eventfd_read(fd, &value); } while (result < 0 && errno == EINTR);
		if (result == -1) {
			return false;
		}

		processing = std::move(callables);
		callables = {};
	}

	while (!processing.empty()) {
		Callable callable = std::move(processing.front());
		processing.pop();
		callable();
	}
	return true;
}
