#include "session_environment.hpp"

#include <dbus/dbus.h>
#include <cerrno>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

namespace zenith {
namespace {
struct Error {
	DBusError value = DBUS_ERROR_INIT;
	~Error() { dbus_error_free(&value); }
};

struct ConnectionDeleter {
	void operator()(DBusConnection* connection) const {
		dbus_connection_close(connection);
		dbus_connection_unref(connection);
	}
};

using Message = std::unique_ptr<DBusMessage, decltype(&dbus_message_unref)>;

void check(bool success) {
	if (!success)
		throw std::bad_alloc();
}

void call(DBusConnection* connection, DBusMessage* message, bool optional_manager) {
	dbus_message_set_auto_start(message, false);
	Error error;
	Message reply(dbus_connection_send_with_reply_and_block(connection, message, 5000, &error.value),
		&dbus_message_unref);

	if (reply)
		return;

	// A non-systemd session still needs its D-Bus activation environment updated.
	if (optional_manager &&
		(dbus_error_has_name(&error.value, DBUS_ERROR_SERVICE_UNKNOWN) ||
		 dbus_error_has_name(&error.value, DBUS_ERROR_NAME_HAS_NO_OWNER)))
		return;

	throw std::runtime_error(std::string("could not publish session environment via ") +
		dbus_message_get_member(message) + ": " +
		(error.value.message ? error.value.message : "no D-Bus reply"));
}
}

void publish_session_environment() {
	// Do not autolaunch a private bus via X11. Use the session address, or the
	// conventional user bus under XDG_RUNTIME_DIR when no address was supplied.
	const char* configured = std::getenv("DBUS_SESSION_BUS_ADDRESS");
	std::string address = configured ? configured : "";
	if (address.empty()) {
		const char* runtime = std::getenv("XDG_RUNTIME_DIR");
		if (!runtime || !*runtime)
			return;

		const std::string path = std::string(runtime) + "/bus";
		struct stat status{};
		if (stat(path.c_str(), &status) != 0) {
			if (errno == ENOENT)
				return;
			throw std::runtime_error("could not inspect user bus socket");
		}

		std::unique_ptr<char, decltype(&dbus_free)> escaped(dbus_address_escape_value(path.c_str()), &dbus_free);
		check(escaped != nullptr);
		address = std::string("unix:path=") + escaped.get();
	}

	check(dbus_threads_init_default());

	Error error;
	std::unique_ptr<DBusConnection, ConnectionDeleter> connection(
		dbus_connection_open_private(address.c_str(), &error.value));
	if (!connection)
		throw std::runtime_error(std::string("could not connect to user bus: ") + error.value.message);
	dbus_connection_set_exit_on_disconnect(connection.get(), false);
	if (!dbus_bus_register(connection.get(), &error.value))
		throw std::runtime_error(std::string("could not register on user bus: ") + error.value.message);

	Message manager(dbus_message_new_method_call("org.freedesktop.systemd1",
		"/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager", "SetEnvironment"), &dbus_message_unref);
	Message activation(dbus_message_new_method_call(DBUS_SERVICE_DBUS,
		DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "UpdateActivationEnvironment"), &dbus_message_unref);
	check(manager && activation);

	DBusMessageIter manager_iter, assignments, activation_iter, dictionary;
	dbus_message_iter_init_append(manager.get(), &manager_iter);
	dbus_message_iter_init_append(activation.get(), &activation_iter);
	check(dbus_message_iter_open_container(&manager_iter, DBUS_TYPE_ARRAY, "s", &assignments));
	check(dbus_message_iter_open_container(&activation_iter, DBUS_TYPE_ARRAY, "{ss}", &dictionary));
	for (const char* name : {"DISPLAY", "WAYLAND_DISPLAY", "XDG_SESSION_TYPE"}) {
		const char* value = std::getenv(name);
		if (!value || !*value || !dbus_validate_utf8(value, nullptr))
			throw std::runtime_error(std::string("invalid session environment variable: ") + name);
		const std::string assignment = std::string(name) + "=" + value;
		const char* text = assignment.c_str();
		check(dbus_message_iter_append_basic(&assignments, DBUS_TYPE_STRING, &text));
		DBusMessageIter entry;
		check(dbus_message_iter_open_container(&dictionary, DBUS_TYPE_DICT_ENTRY, nullptr, &entry));
		check(dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &name));
		check(dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &value));
		check(dbus_message_iter_close_container(&dictionary, &entry));
	}
	check(dbus_message_iter_close_container(&manager_iter, &assignments));
	check(dbus_message_iter_close_container(&activation_iter, &dictionary));
	call(connection.get(), manager.get(), true);
	call(connection.get(), activation.get(), false);
}
}
