# Run a JDWM app in a TTY

Make sure that the user is in the `video` group, then set environment:

```bash
unset WAYLAND_DISPLAY DISPLAY
export XDG_SESSION_TYPE=tty
export ZENITH_MULTI_MONITOR_MODE=extend
export LIBSEAT_BACKEND=seatd
```

And run:
```bash
./run_build.sh --run
```

Or with logging:
```bash
./run_build.sh --run 2>&1 | tee out.txt
```
