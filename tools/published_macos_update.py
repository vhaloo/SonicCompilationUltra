"""Exercise the published macOS bundle and updater in a disposable library."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time
from urllib.request import Request, urlopen


REPOSITORY = "vhaloo/SonicCompilationUltra"
APP = "Sonic Compilation Ultra.app"


def hash_file(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def download(version, architecture, directory):
    tag = "runtime-preview-" + version
    endpoint = f"https://api.github.com/repos/{REPOSITORY}/releases/tags/{tag}"
    with urlopen(Request(endpoint, headers={"User-Agent": "SCU-release-test"}), timeout=30) as response:
        release = json.load(response)
    assert not release["draft"] and release["tag_name"] == tag
    name = f"SonicCompilationUltra-{version}-macos-{architecture}.tar.gz"
    asset = next(asset for asset in release["assets"] if asset["name"] == name)
    file = directory / name
    assert asset["browser_download_url"] == f"https://github.com/{REPOSITORY}/releases/download/{tag}/{name}"
    with urlopen(asset["browser_download_url"], timeout=60) as response, file.open("wb") as target:
        shutil.copyfileobj(response, target)
    assert file.stat().st_size == asset["size"] and "sha256:" + hash_file(file) == asset["digest"]
    with tarfile.open(file) as archive:
        archive.extractall(directory, filter="data")
    return directory / name.removesuffix(".tar.gz")


def validate_bundle(bundle, reference, version):
    manifest = json.loads((reference / "MANIFEST.json").read_text())
    assert manifest["version"] == version
    checked = 0
    for item in manifest["files"]:
        path = Path(item["path"])
        if path.parts[0] == APP:
            assert hash_file(bundle / path.relative_to(APP)) == item["sha256"], path
            checked += 1
    assert checked > 10
    subprocess.run(["/usr/bin/codesign", "--verify", "--deep", "--strict", bundle], check=True)
    return checked


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("previous")
    parser.add_argument("target")
    options = parser.parse_args()
    for version in (options.previous, options.target):
        if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version):
            parser.error("Expected a numeric release version")
    if platform.system() != "Darwin":
        parser.error("Run on macOS")
    arch = {"arm64": "arm64", "x86_64": "x64"}[platform.machine()]
    with tempfile.TemporaryDirectory(prefix="scu-live-") as temp:
        work = Path(temp).resolve()
        original = download(options.previous, arch, work)
        reference = download(options.target, arch, work)
        bundle = original / APP
        validate_bundle(bundle, original, options.previous)
        data = work / "UserData"
        session = work / "session"
        session.mkdir()
        personal = [data / "saves" / "a.state", data / "ROMs" / "owner.test", data / "option-profiles" / "1" / "owner.txt"]
        for file in personal:
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(b"SCU update fixture")
        (data / "settings.ini").write_text("language=1\nvolume=0\n")
        env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
        macos = bundle / "Contents" / "MacOS"
        helper = macos / "scu-update"
        subprocess.run([helper, "check", options.previous, session, bundle], check=True, env=env, timeout=40)
        status = session / "status"
        assert status.read_text() == "available " + options.target, status.read_text()
        status.unlink()
        copied = session / "scu-update"
        shutil.copy2(helper, copied)
        # The old library acknowledges a verified download, then exits before any bundle file changes.
        with subprocess.Popen([sys.executable, "-c", "import time; time.sleep(140)"]) as previous:
            args = [copied, "install", options.previous, session, bundle, options.target, data, work, str(previous.pid)]
            with subprocess.Popen(args, env=env) as updater:
                try:
                    deadline = time.monotonic() + 125
                    while time.monotonic() < deadline and updater.poll() is None:
                        if status.exists() and status.read_text().strip() == "ready":
                            (session / "closed").write_text("1")
                            previous.terminate()
                            previous.wait(timeout=8)
                            break
                        time.sleep(0.1)
                    assert previous.poll() is not None, "Download did not reach installation"
                    assert updater.wait(timeout=35) == 0
                finally:
                    if previous.poll() is None:
                        previous.terminate()
                    if updater.poll() is None:
                        updater.terminate()
        count = validate_bundle(bundle, reference, options.target)
        assert all(file.read_bytes() == b"SCU update fixture" for file in personal)
        assert "language=1" in (data / "settings.ini").read_text()
        process = None
        deadline = time.monotonic() + 35
        try:
            while time.monotonic() < deadline:
                rows = subprocess.check_output(["/bin/ps", "-axo", "pid=,command="], text=True)
                for row in rows.splitlines():
                    columns = row.strip().split(None, 1)
                    if len(columns) == 2 and columns[1].startswith(str(macos / "SonicCompilationUltra") + " --menu "):
                        process = int(columns[0])
                if process and any(p.read_text().strip() == "current" for p in data.glob("update-sessions/*/status")):
                    break
                time.sleep(0.25)
            else:
                raise RuntimeError("Updated library did not reopen and report the current release")
        finally:
            if process:
                os.kill(process, 15)
        print(f"PASS macOS {arch}: {options.previous} -> {options.target}, {count} signed bundle files, "
              "private files preserved, updated library alive and current.")


if __name__ == "__main__":
    main()
