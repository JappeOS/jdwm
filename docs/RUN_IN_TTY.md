# Run a JDWM app in a TTY

Make sure you're launching from a real TTY (not from inside another Wayland/X11 session), then set environment:

```bash
unset WAYLAND_DISPLAY DISPLAY
export XDG_SESSION_TYPE=tty
export ZENITH_MULTI_MONITOR_MODE=extend
```

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
