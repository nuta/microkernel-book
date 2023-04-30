"""

tests.py で使うユーティリティを定義 (pytestが自動で読み込む)

"""
import pytest
import os
import subprocess
import struct
import tempfile
import shlex
import sys
import re

AUTORUN_PLACEHOLDER = "__REPLACE_ME_AUTORUN__" + "a" * 512

cached_runner = None

@pytest.fixture
def run_hinaos(request):
    global cached_runner
    if cached_runner is None:
        build_argv = [request.config.getoption("--make"), "build", f"AUTORUN={AUTORUN_PLACEHOLDER}", f"-j{os.cpu_count()}"]
        if request.config.getoption("--release"):
            build_argv.append("RELEASE=1")
        subprocess.run(build_argv, check=True)

        cached_runner = Runner(
            os.environ.get("QEMU", request.config.getoption("--qemu")),
            shlex.split(os.environ["QEMUFLAGS"]),
            "build/hinaos.elf"
        )
    return do_run_hinaos

def do_run_hinaos(script, timeout=None, ignore_timeout=False, qemu_net0_options=None):
    if timeout is None:
        timeout = int(os.environ.get("TEST_DEFAULT_TIMEOUT", 3))
    return cached_runner.run(script + "; shutdown", timeout=timeout, ignore_timeout=ignore_timeout, qemu_net0_options=qemu_net0_options)

class Result:
    def __init__(self, log, raw_log):
        self.log = log
        self.raw_log = raw_log

ANSI_ESCAPE_SEQ_REGEX = re.compile(r'\x1B\[[^m]+m')

class Runner:
    def __init__(self, qemu, qemu_flags, image_path):
        self.qemu = qemu
        self.qemu_args = qemu_flags
        self.image = open(image_path, "rb").read()
        self.placeholder_bytes = AUTORUN_PLACEHOLDER.encode('ascii')

    def run(self, script, timeout=None, ignore_timeout=False, qemu_net0_options=None):
        if len(script.encode('ascii')) > len(self.placeholder_bytes):
            raise ValueError("script is too long")

        qemu_args = []
        for arg in self.qemu_args:
            if qemu_net0_options is not None and "-netdev user,id=net0" in arg:
                qemu_args.append(arg + "," + ",".join(qemu_net0_options))
            else:
                qemu_args.append(arg)

        script_bytes = struct.pack(f"{len(AUTORUN_PLACEHOLDER)}s", script.encode('ascii'))

        # make run AUTORUN="..." でも実行できるが、この方法だとAUTORUNが変わるたびにビルド
        # し直してしまう。そこで、AUTORUN_PLACEHOLDERを使ってビルドした後に、それを単純に置換
        # する。
        with tempfile.NamedTemporaryFile(mode="wb+", delete=False) as f:
            f.write(self.image.replace(self.placeholder_bytes, script_bytes))
            # Windowsではファイルをいったん閉じないと使えない。
            f.close()

            qemu_argv = [self.qemu, "-kernel", f.name, "-snapshot"] + qemu_args
            try:
                raw_log = subprocess.check_output(qemu_argv, timeout=timeout)
            except subprocess.TimeoutExpired as e:
                raw_log = e.stdout
                pcap_dump = ""
                try:
                    pcap_dump = subprocess.check_output(["tshark", "-r", "virtio-net.pcap"], text=True)
                except Exception as e:
                    print(f"Failed to run tshark ({e}). Skipping pcap dump...")
                if pcap_dump:
                    print("\n\npcap dump:\n\n" + pcap_dump)
                if not ignore_timeout:
                    sys.stdout.buffer.write(raw_log)
                    raise Exception("QEMU timed out")
            finally:
                os.remove(f.name)

            sys.stdout.buffer.write(raw_log)

            log = raw_log.decode('utf-8', errors='backslashreplace')
            log = ANSI_ESCAPE_SEQ_REGEX.sub('', log) # 色を抜く

        return Result(log, raw_log)

def pytest_addoption(parser):
    parser.addoption("--qemu", required=True)
    parser.addoption("--make", required=True)
    parser.addoption("--release", action="store_true")
