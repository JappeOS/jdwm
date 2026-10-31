"""Exercise the shutdown_smoke runner with a real engine and private sockets.

Usage: python3 native/tests/shutdown_smoke.py RUNNER CLIENT LOG_DIRECTORY
Set LD_LIBRARY_PATH for the runner's matching engine/wlroots dependencies.
Requires a render node accessible to the headless GLES backend; no VT is taken.
"""

import os
from pathlib import Path
import resource
import signal
import subprocess
import sys
import tempfile
import time


def run_case(runner, client_exe, logs, output_count, action):
    path = logs / f"{output_count}-outputs-{action}.log"
    with tempfile.TemporaryDirectory(prefix="jdwm-shutdown-") as runtime:
        env = dict(
            os.environ,
            XDG_RUNTIME_DIR=runtime,
            WLR_BACKENDS="headless",
            WLR_RENDERER="pixman" if action == "startup-failure" else "gles2",
            WLR_RENDERER_ALLOW_SOFTWARE="1",
            WLR_HEADLESS_OUTPUTS=str(output_count),
            ZENITH_TRACE_FRAME_TIMING="1",
        )
        args = [str(runner)]
        if action in ("shortcut", "early"):
            args.append(action)
        clients = []
        with path.open("w") as log:
            process = subprocess.Popen(args, env=env, stdout=log, stderr=log)
            try:
                if action not in ("early", "startup-failure"):
                    deadline = time.monotonic() + 10
                    while "Running Wayland compositor on" not in path.read_text():
                        assert process.poll() is None, f"startup failed: {path}"
                        assert time.monotonic() < deadline, f"startup timed out: {path}"
                        time.sleep(0.02)
                    client_env = dict(env, WAYLAND_DISPLAY="wayland-0")
                    for _ in range(3):
                        clients.append(subprocess.Popen(
                            [str(client_exe)], env=client_env, stdout=log, stderr=log,
                        ))
                    if action != "shortcut":
                        time.sleep(1.5)
                        process.send_signal(signal.SIGINT if action in ("sigint", "both") else signal.SIGTERM)
                        if action == "both":
                            process.send_signal(signal.SIGTERM)
                status = process.wait(timeout=10)
                content = path.read_text()
                assert status == (1 if action == "startup-failure" else 0), f"exit {status}: {path}"
                assert "shutdown complete" in content, f"cleanup incomplete: {path}"
                assert "ERROR: AddressSanitizer" not in content, path
                assert "runtime error:" not in content, path
                if clients:
                    assert "stage=frame" in content, f"Flutter did not render: {path}"
                    assert content.count("xdg_surface map,") == 3, f"clients not mapped: {path}"
                    for client in clients:
                        assert client.wait(timeout=2) == 0, f"client failed: {path}"
                print(f"PASS {output_count} outputs {action}", flush=True)
            finally:
                for child in [*clients, process]:
                    if child.poll() is None:
                        child.kill()
                        child.wait()


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    runner, client, logs = (Path(arg).resolve() for arg in sys.argv[1:])
    logs.mkdir(parents=True, exist_ok=True)
    for output_count in (1, 2):
        for action in ("shortcut", "sigint", "sigterm", "both", "early"):
            run_case(runner, client, logs, output_count, action)
    run_case(runner, client, logs, 1, "startup-failure")
