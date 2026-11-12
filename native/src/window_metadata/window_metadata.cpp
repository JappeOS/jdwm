#include "window_metadata.hpp"
#include "encodable_value.h"
#include "icon_data.hpp"
#include "server.hpp"
#include <algorithm>
#include <drm_fourcc.h>
#include <map>
#include <mutex>
#include <stdexcept>
#include <xcb/xcb.h>
#include <xcb/xcbext.h>

extern "C" {
#define static
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_xdg_toplevel_icon_v1.h>
#undef static
#define class wlroots_xwayland_class
#include <wlr/xwayland/xwayland.h>
#undef class
#include <wlr/util/log.h>
}

using flutter::EncodableList;
using flutter::EncodableMap;
using flutter::EncodableValue;
namespace zenith {

struct WindowMetadata::Impl {
	ZenithServer *server;
	wl_listener icon_changed{};

	struct Mailbox {
		std::mutex mutex;
		bool queued = false;
		std::unique_ptr<EncodableMap> pending;
	};

	struct Entry {
		Impl *owner;
		size_t id;
		bool mapped = false;
		wlr_xwayland_surface *x11 = nullptr;
		wl_listener class_changed{};
		WindowIcon icon;
		std::shared_ptr<Mailbox> mailbox = std::make_shared<Mailbox>();
		uint64_t revision = 0;
		unsigned pending = 0;
		bool dirty = false;

		~Entry() {
			if (class_changed.link.next)
				wl_list_remove(&class_changed.link);
		}
	};

	std::map<size_t, std::unique_ptr<Entry>> entries;
	uint64_t next_revision = 0;
	xcb_connection_t *connection = nullptr;
	xcb_atom_t icon_atom = XCB_ATOM_NONE;
	unsigned atom_pending = 0, supported_pending = 0;
	xcb_atom_t supported_atom = XCB_ATOM_NONE;
	bool advertised = false;
	wl_event_source *source = nullptr;

	explicit Impl(ZenithServer *s) : server(s) {
		auto *manager = wlr_xdg_toplevel_icon_manager_v1_create(s->display, 1);
		if (!manager)
			throw std::runtime_error("Cannot create toplevel icon manager");

		int sizes[] = {32, 48, 64, 128, 256};
		wlr_xdg_toplevel_icon_manager_v1_set_sizes(manager, sizes, 5);
		icon_changed.notify = on_icon;
		wl_signal_add(&manager->events.set_icon, &icon_changed);
	}

	~Impl() {
		wl_list_remove(&icon_changed.link);
		disconnect();
	}

	Entry &entry(size_t id) {
		auto &result = entries[id];
		if (!result) {
			result = std::make_unique<Entry>();
			result->owner = this;
			result->id = id;
		}
		return *result;
	}

	void publish(Entry &e) {
		if (!e.mapped || server->is_shutting_down())
			return;

		e.revision = ++next_revision;
		EncodableList images;
		for (const auto &image : e.icon.images) {
			images.emplace_back(EncodableMap{{EncodableValue("width"), EncodableValue(int(image.width))},
											 {EncodableValue("height"), EncodableValue(int(image.height))},
											 {EncodableValue("scale"), EncodableValue(int(image.scale))},
											 {EncodableValue("rgba"), EncodableValue(image.rgba)}});
		}

		EncodableMap value{
			{EncodableValue("view_id"), EncodableValue(int64_t(e.id))},
			{EncodableValue("revision"), EncodableValue(int64_t(e.revision))},
			{EncodableValue("wm_class"),
			 EncodableValue(e.x11 && e.x11->wlroots_xwayland_class ? e.x11->wlroots_xwayland_class : "")},
			{EncodableValue("wm_instance"), EncodableValue(e.x11 && e.x11->instance ? e.x11->instance : "")},
			{EncodableValue("icon_name"), EncodableValue(e.icon.name)},
			{EncodableValue("images"), EncodableValue(std::move(images))}};

		auto box = e.mailbox;
		std::lock_guard<std::mutex> lock(box->mutex);
		box->pending = std::make_unique<EncodableMap>(std::move(value));
		if (box->queued)
			return;

		box->queued = true;
		auto *embedder = server->embedder_state.get();
		embedder->callable_queue.enqueue([box, embedder] {
			std::unique_ptr<EncodableMap> value;
			{
				std::lock_guard<std::mutex> lock(box->mutex);
				value = std::move(box->pending);
				box->queued = false;
			}

			if (value)
				embedder->deliver_window_metadata(std::make_unique<EncodableValue>(std::move(*value)));
		});
	}

	static void on_icon(wl_listener *listener, void *data) {
		Impl *self = wl_container_of(listener, self, icon_changed);
		auto *event = static_cast<wlr_xdg_toplevel_icon_manager_v1_set_icon_event *>(data);
		auto *surface = static_cast<ZenithSurface *>(event->toplevel->base->surface->data);
		if (!surface)
			return;

		auto &e = self->entry(surface->id);
		e.icon = {};
		if (event->icon) {
			if (event->icon->name && strlen(event->icon->name) <= 4096)
				e.icon.name = event->icon->name;

			size_t bytes = 0;
			wlr_xdg_toplevel_icon_v1_buffer *variant;
			wl_list_for_each(variant, &event->icon->buffers, link) {
				auto *b = variant->buffer;
				if (!valid_icon_size(b->width, b->height) || variant->scale <= 0 ||
					e.icon.images.size() >= icon_variant_limit)
					continue;

				const size_t size = size_t(b->width) * b->height * 4;
				if (size > icon_byte_limit - bytes)
					continue;

				void *pixels;
				uint32_t format;
				size_t stride;
				if (!wlr_buffer_begin_data_ptr_access(b, WLR_BUFFER_DATA_PTR_ACCESS_READ, &pixels, &format,
													  &stride))
					continue;

				if ((format == DRM_FORMAT_ARGB8888 || format == DRM_FORMAT_XRGB8888 ||
					 format == DRM_FORMAT_ABGR8888 || format == DRM_FORMAT_XBGR8888) &&
					stride >= size_t(b->width) * 4) {
					IconImage image{uint32_t(b->width), uint32_t(b->height), uint32_t(variant->scale), {}};
					const bool rgb = format == DRM_FORMAT_ARGB8888 || format == DRM_FORMAT_XRGB8888;
					const bool alpha = format == DRM_FORMAT_ARGB8888 || format == DRM_FORMAT_ABGR8888;
					if (copy_shm_icon(image, pixels, stride, rgb, alpha)) {
						bytes += size;
						e.icon.images.push_back(std::move(image));
					}
				}

				wlr_buffer_end_data_ptr_access(b);
			}
		}

		self->publish(e);
	}

	static void on_class(wl_listener *listener, void *) {
		Entry *e = wl_container_of(listener, e, class_changed);
		e->owner->publish(*e);
	}

	void disconnect() {
		if (source)
			wl_event_source_remove(source);

		source = nullptr;
		if (connection)
			xcb_disconnect(connection);

		connection = nullptr;
		icon_atom = XCB_ATOM_NONE;
		atom_pending = supported_pending = 0;
		supported_atom = XCB_ATOM_NONE;
		advertised = false;

		for (auto &item : entries) {
			item.second->pending = 0;
			item.second->dirty = false;
		}
	}

	void request(Entry &e) {
		if (!connection || !icon_atom || !e.x11 || !e.mapped)
			return;

		if (e.pending) {
			e.dirty = true;
			return;
		}

		e.pending = xcb_get_property(connection, false, e.x11->window_id, icon_atom, XCB_ATOM_CARDINAL, 0,
									 icon_byte_limit / 4)
						.sequence;

		xcb_flush(connection);
	}

	void subscribe(Entry &e) {
		if (!connection || !e.x11)
			return;

		uint32_t mask = XCB_EVENT_MASK_PROPERTY_CHANGE;
		xcb_change_window_attributes(connection, e.x11->window_id, XCB_CW_EVENT_MASK, &mask);
		request(e);
	}

	static int readable(int, uint32_t mask, void *data) {
		auto *self = static_cast<Impl *>(data);
		if (mask & (WL_EVENT_HANGUP | WL_EVENT_ERROR)) {
			self->disconnect();
			return 0;
		}
		self->drain();
		return 0;
	}

	void handle_event(xcb_generic_event_t *event) {
		if ((event->response_type & 127) == XCB_PROPERTY_NOTIFY) {
			auto *property = reinterpret_cast<xcb_property_notify_event_t *>(event);
			if (property->atom == icon_atom)
				for (auto &item : entries) {
					auto &e = *item.second;
					if (e.x11 && e.x11->window_id == property->window)
						request(e);
				}
		}
		free(event);
	}

	void drain() {
		bool progressed;
		do {
			progressed = false;
			// Polling events also reads replies into XCB's queue; never wait for replies.
			while (auto *event = xcb_poll_for_event(connection))
				handle_event(event);

			if (xcb_connection_has_error(connection)) {
				disconnect();
				return;
			}

			void *reply = nullptr;
			xcb_generic_error_t *error = nullptr;
			if (atom_pending && xcb_poll_for_reply(connection, atom_pending, &reply, &error)) {
				progressed = true;
				atom_pending = 0;
				if (reply)
					icon_atom = static_cast<xcb_intern_atom_reply_t *>(reply)->atom;
				free(reply);
				free(error);
				for (auto &item : entries)
					subscribe(*item.second);
			}

			if (supported_pending) {
				reply = nullptr;
				error = nullptr;
				if (xcb_poll_for_reply(connection, supported_pending, &reply, &error)) {
					progressed = true;
					supported_pending = 0;
					if (reply)
						supported_atom = static_cast<xcb_intern_atom_reply_t *>(reply)->atom;
					free(reply);
					free(error);
				}
			}

			if (icon_atom && supported_atom && !advertised) {
				auto root = xcb_setup_roots_iterator(xcb_get_setup(connection)).data->root;
				xcb_change_property(connection, XCB_PROP_MODE_APPEND, root, supported_atom, XCB_ATOM_ATOM, 32,
									1, &icon_atom);
				xcb_flush(connection);
				advertised = true;
			}

			for (auto &item : entries) {
				auto &e = *item.second;
				reply = nullptr;
				error = nullptr;
				if (!e.pending || !xcb_poll_for_reply(connection, e.pending, &reply, &error))
					continue;

				progressed = true;
				e.pending = 0;
				e.icon = {};
				auto *prop = static_cast<xcb_get_property_reply_t *>(reply);
				if (prop && prop->type == XCB_ATOM_CARDINAL && prop->format == 32 && prop->bytes_after == 0)
					e.icon.images = parse_x11_icons(static_cast<uint32_t *>(xcb_get_property_value(prop)),
													xcb_get_property_value_length(prop) / 4);

				free(reply);
				free(error);

				if (e.dirty) {
					e.dirty = false;
					request(e);
				} else
					publish(e);
			}

			// Reply polling can queue events internally without leaving the fd readable.
			while (auto *event = xcb_poll_for_queued_event(connection)) {
				handle_event(event);
				progressed = true;
			}
		} while (progressed);
	}
};

WindowMetadata::WindowMetadata(ZenithServer *server) : impl_(std::make_unique<Impl>(server)) {}
WindowMetadata::~WindowMetadata() = default;

void WindowMetadata::connect_xwayland(const char *display) {
	auto &self = *impl_;
	self.disconnect();
	self.connection = xcb_connect(display, nullptr);

	if (xcb_connection_has_error(self.connection)) {
		self.disconnect();
		return;
	}

	self.source = wl_event_loop_add_fd(wl_display_get_event_loop(self.server->display),
									   xcb_get_file_descriptor(self.connection), WL_EVENT_READABLE,
									   Impl::readable, &self);

	if (!self.source) {
		self.disconnect();
		return;
	}

	self.atom_pending = xcb_intern_atom(self.connection, false, 12, "_NET_WM_ICON").sequence;
	self.supported_pending = xcb_intern_atom(self.connection, false, 14, "_NET_SUPPORTED").sequence;
	xcb_flush(self.connection);
}

void WindowMetadata::map(size_t id, wlr_xwayland_surface *x11) {
	auto &self = *impl_;
	auto &e = self.entry(id);
	e.mapped = true;

	if (x11 && !e.x11) {
		e.x11 = x11;
		e.class_changed.notify = Impl::on_class;
		wl_signal_add(&x11->events.set_class, &e.class_changed);
	}

	self.publish(e);
	self.subscribe(e);
}

void WindowMetadata::unmap(size_t id) {
	auto &self = *impl_;
	auto it = self.entries.find(id);
	if (it == self.entries.end())
		return;

	auto &e = *it->second;
	e.mapped = false;
	if (e.pending && self.connection)
		xcb_discard_reply(self.connection, e.pending);

	e.pending = 0;
	e.dirty = false;
	{
		std::lock_guard<std::mutex> lock(e.mailbox->mutex);
		e.mailbox->pending.reset();
	}

	// A remap must enqueue after the new map event, not reuse an old queued job.
	e.mailbox = std::make_shared<Impl::Mailbox>();
}

void WindowMetadata::forget(size_t id) {
	unmap(id);
	impl_->entries.erase(id);
}

} // namespace zenith
