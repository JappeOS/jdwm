#pragma once

#include <cstddef>
#include <optional>
#include "flutter_engine/message_structs.hpp"

struct ZenithToplevel {
	virtual ~ZenithToplevel() = default;

	virtual void focus(bool focus) const = 0;
	virtual void maximize(bool value) const = 0;
	virtual void resize(size_t width, size_t height) const = 0;
	virtual void request_close() const = 0;

	virtual void set_visible(bool value) = 0;
	virtual bool visible() const = 0;
	virtual bool maximized() const = 0;
	virtual std::optional<ToplevelDecoration> decoration() const = 0;
	virtual const char* protocol() const = 0;
};
