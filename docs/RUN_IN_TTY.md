# Run a JDWM app in a TTY

Make sure you're launching from a real TTY (not from inside another Wayland/X11 session), then set environment:

```bash
unset WAYLAND_DISPLAY DISPLAY
export XDG_SESSION_TYPE=tty
export ZENITH_MULTI_MONITOR_MODE=extend
```

Run as the *same user you logged into the TTY with* (avoid `sudo`/`su`), so PAM/systemd-logind set up the session
environment (notably `XDG_SESSION_ID`, `XDG_VTNR`, `XDG_RUNTIME_DIR`).

By default, wlroots/libseat will auto-pick a session backend (usually `logind` on systemd distros). Only set
`LIBSEAT_BACKEND` if you know you need it:

```bash
# Systemd + logind:
# export LIBSEAT_BACKEND=logind
#
# No logind / no system D-Bus (e.g. minimal install/container):
# export LIBSEAT_BACKEND=seatd
```

If you select `seatd`, you also need the seatd daemon running and permissions set up (e.g. add your user to the
`seat` and `video` groups or use `seatd-launch`).

And run:
```bash
./run_build.sh --run
```

Or with logging:
```bash
./run_build.sh --run 2>&1 | tee out.txt
```

## Running from a systemd unit (greeter/DE)

If you launch the compositor as a systemd service/transient unit (e.g. via `StartTransientUnit`), you often won't
have a VT-backed logind session created by PAM. In that case the default `logind` backend can fail with an
interactive authentication/polkit error when libseat tries to activate the session.

Two options:

- Prefer `seatd` (recommended for service-style launch): set `LIBSEAT_BACKEND=seatd` and ensure `seatd` is running.
- Or ensure you're starting inside a real logind TTY session (PAM + `TTYPath`/VT), so `XDG_SESSION_ID` and
  `XDG_VTNR` are set and the session is authorized to take DRM control.

## EGL troubleshooting

If wlroots logs `EGL_EXT_platform_base not supported`, it couldn't create an EGL context and the compositor won't
start. Verify you have a working EGL/GBM-capable graphics stack for your GPU/VM and that the expected `libEGL` is
being used.

If you're launching from a bundle that sets `LD_LIBRARY_PATH` (common in systemd units), be careful about shipping
and preferring core graphics stack libraries like `libdrm.so.2` or `libwayland-*.so`. If those come from a different
distro/build image than your Mesa packages, EGL driver loading can fail in subtle ways (including missing
`EGL_EXT_platform_base`). Prefer system `/usr/lib` versions of Mesa/DRM/Wayland, and only ship `libwlroots` + the
non-graphics leaf deps you truly need.
