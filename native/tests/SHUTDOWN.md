# Shutdown tests

From the repository root, run the cancellation and queue-ordering regression:

```sh
clang++ -std=c++17 -Wall -Wextra -Werror -pthread -DWLR_USE_UNSTABLE \
  -Inative/src -I../wlroots-install/include/wlroots-0.19 \
  $(pkg-config --cflags pixman-1 wayland-server) \
  native/tests/shutdown_primitives_test.cpp \
  native/src/util/rethreading/callable_queue.cpp \
  -o /tmp/jdwm-shutdown-primitives-test
timeout 10 /tmp/jdwm-shutdown-primitives-test
```

This leaves GPU-fence substitutes permanently unsignalled and verifies that
shutdown wakes the blocked writer without overwriting a displayed buffer. It
also races queue closure against a running batch, checks finalizer ordering,
rejects late work, and verifies descriptor closure. Add
`-fsanitize=address,undefined` for memory/UB checks. Use
`ASAN_OPTIONS=detect_leaks=0` if the environment cannot run LeakSanitizer.

Run the existing swapchain backpressure, readiness, read-lease, damage,
output-frame retry, external-texture registry and vsync regressions as well.

## Real engine and wlroots

The fixture consists of:

- `shutdown_smoke.dart`: a continuously animated Flutter app with no plugins or
  desktop services. Build it as `lib/main.dart` in a temporary Flutter project,
  using the SDK version matching the Sony engine. A mismatched snapshot can
  abort inside the engine before any shutdown test runs.
- `shutdown_smoke.cpp`: a runner linked against the backend and matching engine,
  using the normal compositor runner's include/link flags. Include `-lGL`:
  the engine's default GL resolver needs its exported extension entry points.
  Place the runner beside the app's `data/flutter_assets`, `data/icudtl.dat`
  and `lib/libapp.so` in the generated release bundle. It adds synthetic pointer,
  keyboard and touch devices, tests touch unplug, holds a key through shutdown,
  verifies listener detachment, and checks signal-mask restoration.
- `shutdown_client.c`: a small mapped XDG/SHM client kept connected through exit.
- `shutdown_smoke.py`: launches two headless outputs on a private Wayland socket,
  checks active Flutter rendering and three mapped clients, and tests keyboard,
  SIGINT, SIGTERM and combined-signal exits with one and two outputs. It also
  checks early exit and rejected-renderer startup cleanup: 11 cases total.

Build the client:

```sh
wayland-scanner client-header \
  /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  /tmp/xdg-shell-client-protocol.h
wayland-scanner private-code \
  /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  /tmp/xdg-shell-protocol.c
clang -Wall -Wextra -Werror -I/tmp native/tests/shutdown_client.c \
  /tmp/xdg-shell-protocol.c -lwayland-client -o /tmp/jdwm-shutdown-client
```

Run with the required engine/wlroots shared libraries available through rpaths
or `LD_LIBRARY_PATH`, and with access to a DRM render node:

```sh
python3 native/tests/shutdown_smoke.py \
  /path/to/test/bundle/shutdown_smoke /tmp/jdwm-shutdown-client \
  /tmp/jdwm-shutdown-logs
```

This uses the real engine, renderer and Xwayland without taking a VT or connecting
to the host compositor. Test logs are retained in the specified directory.
For ASan/UBSan, rebuild both the backend and runner with sanitizer flags; the
prebuilt engine and wlroots remain uninstrumented. The fixture checks teardown
under active rendering, not the desktop's visual correctness or physical scanout.

## Validation on the development machine

The release backend passed all 11 cases, including three mapped clients, held
key repeat, touch unplug, two outputs, early exit and rejected-renderer startup.
Cleanup measured from `stopping Flutter` to `shutdown complete` took 7–24 ms for
normal/early exits locally. This is not a bound on physical DRM/driver teardown.

All 10 normal/early exit cases also passed with the native backend and runner
instrumented by ASan/UBSan. The rejected-renderer case completed its cleanup,
then ASan reported `operator new` versus `free` in the system libc++ exception
string destructor. The same report reproduces with a standalone program that
only throws/catches `std::runtime_error`, without JDWM or the engine. That
sanitizer check was not disabled; startup-failure acceptance was verified in the
release build. The cancellation/queue regression passed ASan/UBSan separately.
