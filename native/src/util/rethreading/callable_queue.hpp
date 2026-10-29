#pragma once

#include <functional>
#include <queue>
#include <mutex>
#include <sys/eventfd.h>

/*
 * Queue that stores and executes functions.
 * An event loop using epoll or other Linux APIs can listen the readability of `fd`.
 * When `fd` becomes readable, the event loop can call `execute` which will execute all
 * stored functions.
 *
 * This is useful for executing some tasks on another thread that has an event loop listening
 * to file descriptors.
 * It is mainly used to execute tasks on the main thread which has a `wl_event_loop`.
 */
class CallableQueue {
	using Callable = std::function<void(void)>;

	int fd;
	std::queue<Callable> callables = {};
	std::mutex mutex = {};
	bool closed = false;

public:
	CallableQueue();
	~CallableQueue();

	[[nodiscard]] int get_fd() const;

	bool enqueue(const Callable& callable);

	// Reject new work atomically, then run final after all accepted work.
	// Captures are released by the consumer, on its owning thread.
	void close(const Callable& final = {});

	bool execute();
};
