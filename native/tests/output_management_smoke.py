"""Exercise a real JDWM runner via wlr-randr on two private headless outputs.

Usage: python3 native/tests/output_management_smoke.py RUNNER WLR_RANDR LOG
Use the animated shutdown_smoke runner and its matching Flutter bundle. A GLES
render node is required; this never takes a VT or connects to the host display.
"""
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time


def run(runner, randr, log_path):
    with tempfile.TemporaryDirectory(prefix="jdwm-output-test-") as runtime:
        env = dict(os.environ, XDG_RUNTIME_DIR=runtime, WAYLAND_DISPLAY="wayland-0",
                   WLR_BACKENDS="headless", WLR_HEADLESS_OUTPUTS="2",
                   WLR_RENDERER="gles2", WLR_RENDERER_ALLOW_SOFTWARE="1",
                   ZENITH_TRACE_FRAME_TIMING="1")
        with log_path.open("w") as log:
            process = subprocess.Popen([runner, "output-management"], env=env, stdout=log, stderr=log)
            def command(*args, success=True):
                result = subprocess.run([randr, *args], env=env, text=True,
                                        capture_output=True, timeout=10)
                assert (result.returncode == 0) == success, result.stdout + result.stderr
                return result.stdout
            try:
                deadline = time.monotonic() + 15
                while "Running Wayland compositor on" not in log_path.read_text():
                    assert process.poll() is None, log_path.read_text()
                    assert time.monotonic() < deadline, "startup timed out"
                    time.sleep(0.02)
                initial = command()
                names = [line.split()[0] for line in initial.splitlines()
                         if line and not line[0].isspace()]
                assert len(names) == 2, initial
                first, second = names
                proposal = ["--output", first, "--custom-mode", "800x600@60",
                            "--pos", "-800,0", "--scale", "1",
                            "--output", second, "--custom-mode", "1024x768@75",
                            "--pos", "0,0", "--scale", "1.25", "--transform", "90"]
                command("--dryrun", *proposal)
                assert command() == initial, "test request changed live configuration"
                command(*proposal)
                configured = command()
                assert configured != initial
                assert "-800,0" in configured and "1.250000" in configured, configured
                command("--output", first, "--custom-mode", "1000000x1000000@60", success=False)
                assert command() == configured, "rejected configuration changed live state"
                command("--output", first, "--off")
                assert "Enabled: no" in command()
                before_blank = command()
                for powered in (0, 1):
                    process.send_signal(signal.SIGUSR1)
                    deadline = time.monotonic() + 10
                    while f"output-test: power={powered}" not in log_path.read_text():
                        assert process.poll() is None, log_path.read_text()
                        assert time.monotonic() < deadline, "blank/wake timed out"
                        time.sleep(0.02)
                    assert command() == before_blank, "blank/wake changed configured outputs"
                command("--output", second, "--off")
                assert command().count("Enabled: no") == 2
                time.sleep(0.2)
                assert process.poll() is None, "all-off terminated the compositor"
                # Re-enable using only the protocol, without generating input.
                command("--output", first, "--on", "--custom-mode", "800x600@60",
                        "--pos", "-800,0", "--scale", "1", "--transform", "normal",
                        "--output", second, "--on", "--custom-mode", "1024x768@75",
                        "--pos", "0,0", "--scale", "1.25", "--transform", "90")
                assert command() == configured, "enable did not restore configuration"
                # Multiple fresh clients must observe the same published configuration.
                for _ in range(3):
                    assert command() == configured
                time.sleep(1)
                process.send_signal(signal.SIGTERM)
                assert process.wait(timeout=10) == 0
                content = log_path.read_text()
                assert "shutdown complete" in content
                assert "stage=frame" in content, "Flutter never rendered"
                assert "ERROR: AddressSanitizer" not in content
                assert "runtime error:" not in content
                print("PASS output management: test/apply, modes, layout, rejection, all-off/re-enable, shutdown")
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()

if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    run(str(Path(sys.argv[1]).resolve()), str(Path(sys.argv[2]).resolve()), Path(sys.argv[3]).resolve())
