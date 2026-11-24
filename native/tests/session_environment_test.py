#!/usr/bin/python3
"""Isolated user-bus integration test (requires python3-dbus/python3-gi).

Build from the repository root:
clang++ -std=c++17 -Wall -Wextra -Werror -Inative/src \
  $(pkg-config --cflags dbus-1) native/tests/session_environment_test.cpp \
  native/src/util/session_environment.cpp native/src/util/service_notify.cpp \
  $(pkg-config --libs dbus-1) -o /tmp/jdwm-session-environment-test
/usr/bin/python3 native/tests/session_environment_test.py /tmp/jdwm-session-environment-test
"""
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import threading
import time

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib


DBusGMainLoop(set_as_default=True)
EXPECTED = {"DISPLAY": ":73", "WAYLAND_DISPLAY": "wayland-42", "XDG_SESSION_TYPE": "wayland"}


class Manager(dbus.service.Object):
    reject = False
    assignments = None

    @dbus.service.method("org.freedesktop.systemd1.Manager", in_signature="as", out_signature="")
    def SetEnvironment(self, assignments):
        self.assignments = dict(item.split("=", 1) for item in assignments)
        # Withhold the reply: READY must not race ahead of publication.
        time.sleep(0.15)
        expect_no_notification()
        if self.reject:
            raise dbus.exceptions.DBusException("denied", name="org.freedesktop.DBus.Error.AccessDenied")


def expect_no_notification():
    try:
        receiver.recv(64)
    except BlockingIOError:
        return
    raise AssertionError("premature READY=1")


def run(env, success=True):
    result = subprocess.run([sys.argv[1]], env=env, capture_output=True, text=True, timeout=10)
    assert (result.returncode == 0) == success, result.stderr
    if success:
        assert receiver.recv(64) == b"READY=1"
    else:
        expect_no_notification()
    return result


with tempfile.TemporaryDirectory(prefix="jdwm-session-test-") as directory:
    root = Path(directory)
    receiver = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    receiver.bind(str(root / "notify"))
    receiver.setblocking(False)
    env = dict(os.environ, **EXPECTED, XDG_RUNTIME_DIR=directory, NOTIFY_SOCKET=str(root / "notify"))
    env.pop("DBUS_SESSION_BUS_ADDRESS", None)
    run(env)  # No bus and no manager: standalone startup still works.
    run(dict(env, DBUS_SESSION_BUS_ADDRESS="unix:path=" + str(root / "missing")), False)

    # An activated process records what it actually inherited from dbus-daemon.
    probe = root / "probe.py"
    probe.write_text("import os, json\nfrom pathlib import Path\n"
                     f"Path({str(root / 'environment.json')!r}).write_text(json.dumps(dict(os.environ)))\n")
    (root / "org.zenith.EnvironmentProbe.service").write_text(
        "[D-BUS Service]\nName=org.zenith.EnvironmentProbe\n"
        f"Exec=/usr/bin/python3 {probe}\n")
    config = root / "bus.conf"
    config.write_text(f"""<busconfig>
      <type>session</type><listen>unix:path={root}/bus</listen>
      <servicedir>{root}</servicedir>
      <policy context="default"><allow send_destination="*"/>
        <allow receive_sender="*"/><allow own="*"/></policy>
    </busconfig>""")
    daemon = subprocess.Popen(["dbus-daemon", "--nofork", "--print-address=1", f"--config-file={config}"],
                              stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
    loop = GLib.MainLoop()
    thread = threading.Thread(target=loop.run)
    try:
        address = daemon.stdout.readline().strip()
        assert address
        bus = dbus.bus.BusConnection(address)
        run(env)  # Discover $XDG_RUNTIME_DIR/bus; no systemd manager present.
        env["DBUS_SESSION_BUS_ADDRESS"] = address
        name = dbus.service.BusName("org.freedesktop.systemd1", bus=bus)
        manager = Manager(bus, "/org/freedesktop/systemd1")
        thread.start()
        run(env)
        assert manager.assignments == EXPECTED
        # The helper has returned READY; activation must already see the values.
        broker = dbus.Interface(bus.get_object("org.freedesktop.DBus", "/org/freedesktop/DBus"),
                                "org.freedesktop.DBus")
        try:
            broker.StartServiceByName("org.zenith.EnvironmentProbe", dbus.UInt32(0), timeout=3)
        except dbus.DBusException:
            pass  # The probe exits after recording its environment, without owning a name.
        import json
        inherited = json.loads((root / "environment.json").read_text())
        assert {key: inherited[key] for key in EXPECTED} == EXPECTED
        assert "NOTIFY_SOCKET" not in inherited
        manager.reject = True
        assert "SetEnvironment" in run(env, False).stderr
        manager.reject = False
        run(dict(env, WAYLAND_DISPLAY=""), False)
        config.write_text(config.read_text().replace("</policy>",
            '<deny send_destination="org.freedesktop.DBus" '
            'send_interface="org.freedesktop.DBus" '
            'send_member="UpdateActivationEnvironment"/></policy>'))
        broker.ReloadConfig()
        assert "UpdateActivationEnvironment" in run(env, False).stderr
    finally:
        loop.quit()
        if thread.is_alive():
            thread.join(timeout=3)
        daemon.terminate()
        daemon.wait(timeout=3)
        receiver.close()
print("Session environment publication tests passed")
