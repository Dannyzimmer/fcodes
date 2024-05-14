#!/usr/bin/env python3

"""
steps for deploying a new release (see ../.gitlab-ci.yml)

This is based on Gitlab’s generic package example,
https://gitlab.com/gitlab-org/release-cli/-/tree/master/docs/examples/release-assets-as-generic-package
"""

import hashlib
import json
import logging
import os
import re
import shutil
import stat
import subprocess
import sys
from pathlib import Path
from typing import List, Optional

# logging output stream, setup in main()
log = None


def upload(version: str, path: Path, name: Optional[str] = None) -> str:
    """
    upload a file to the Graphviz generic package with the given version
    """

    # use the path as the name if no other was given
    if name is None:
        name = str(path)

    # Gitlab upload file_name field only allows letters, numbers, dot, dash, and
    # underscore
    safe = re.sub(r"[^a-zA-Z0-9.\-]", "_", name)
    log.info(f"escaped name {name} to {safe}")

    target = (
        f'{os.environ["CI_API_V4_URL"]}/projects/'
        f'{os.environ["CI_PROJECT_ID"]}/packages/generic/graphviz-releases/'
        f"{version}/{safe}"
    )

    log.info(f"uploading {path} to {target}")
    # calling Curl is not the cleanest way to achieve this, but Curl takes care of
    # encodings, headers and part splitting for us
    proc = subprocess.run(
        [
            "curl",
            "--silent",  # no progress bar
            "--include",  # include HTTP response headers in output
            "--verbose",  # more connection details
            "--retry",
            "3",  # retry on transient errors
            "--header",
            f'JOB-TOKEN: {os.environ["CI_JOB_TOKEN"]}',
            "--upload-file",
            path,
            target,
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )
    log.info("Curl response:")
    for i, line in enumerate(proc.stdout.split("\n"), 1):
        log.info(f" {i:3}: {line}")
    proc.check_returncode()

    resp = proc.stdout.split("\n")[-1]
    if json.loads(resp)["message"] != "201 Created":
        raise Exception(f"upload failed: {resp}")

    return target


def checksum(path: Path) -> Path:
    """generate checksum for the given file"""

    assert path.exists()

    log.info(f"SHA256 summing {path}")
    check = Path(f"{path}.sha256")
    data = path.read_bytes()
    check.write_text(f"{hashlib.sha256(data).hexdigest()}  {path}\n", encoding="utf-8")
    return check


def is_macos_artifact(path: Path) -> bool:
    """is this a deployment artifact for macOS?"""
    return re.search(r"\bDarwin\b", str(path)) is not None


def is_windows_artifact(path: Path) -> bool:
    """is this a deployment artifact for Windows?"""
    return re.search(r"\bwindows\b", str(path)) is not None


def get_format(path: Path) -> str:
    """a human readable description of the format of this file"""
    if path.suffix[1:].lower() == "exe":
        return "EXE installer"
    if path.suffix[1:].lower() == "zip":
        return "ZIP archive"
    return path.suffix[1:].lower()


def main() -> int:  # pylint: disable=missing-function-docstring

    # setup logging to print to stderr
    global log
    ch = logging.StreamHandler()
    log = logging.getLogger("deploy.py")
    log.addHandler(ch)
    log.setLevel(logging.INFO)

    if os.environ.get("CI") is None:
        log.error("CI environment variable unset; refusing to run")
        return -1

    # echo some useful things for debugging
    log.info(f"os.uname(): {os.uname()}")
    if Path("/etc/os-release").exists():
        with open("/etc/os-release", "rt", encoding="utf-8") as f:
            log.info("/etc/os-release:")
            for i, line in enumerate(f, 1):
                log.info(f" {i}: {line[:-1]}")

    # bail out early if we do not have release-cli to avoid uploading assets that
    # become orphaned when we fail to create the release
    if not shutil.which("release-cli"):
        log.error("release-cli not found")
        return -1

    # retrieve version name left by prior CI tasks
    log.info("reading GRAPHVIZ_VERSION")
    version = Path("GRAPHVIZ_VERSION").read_text(encoding="utf-8").strip()
    log.info(f"GRAPHVIZ_VERSION == {version}")

    # the generic package version has to be \d+.\d+.\d+ but it does not need to
    # correspond to the release version (which may not conform to this pattern if
    # this is a dev release)
    if re.match(r"\d+\.\d+\.\d+$", version) is None:
        # generate a compliant version
        package_version = f'0.0.{int(os.environ["CI_COMMIT_SHA"], 16)}'
    else:
        # we can use a version corresponding to the release version
        package_version = version
    log.info(f"using generic package version {package_version}")

    # we only create Gitlab releases for stable version numbers
    if re.match(r"\d+\.\d+\.\d+$", version) is None:
        log.warning(
            f"skipping release creation because {version} is not "
            "of the form \\d+.\\d+.\\d+"
        )
        return 0

    # list of assets we have uploaded
    assets: List[str] = []

    # data for the website’s download page
    webdata = {
        "version": f"graphviz-{version}",
        "sources": [],
        "windows": [],
    }

    for tarball in (
        Path(f"graphviz-{version}.tar.gz"),
        Path(f"graphviz-{version}.tar.xz"),
    ):

        if not tarball.exists():
            log.error(f"source {tarball} not found")
            return -1

        # accrue the source tarball and accompanying checksum
        url = upload(package_version, tarball)
        assets.append(url)
        webentry = {"format": get_format(tarball), "url": url}
        check = checksum(tarball)
        url = upload(package_version, check)
        assets.append(url)
        webentry[check.suffix[1:]] = url

        webdata["sources"].append(webentry)

    for stem, _, leaves in os.walk("Packages"):
        for leaf in leaves:
            path = Path(stem) / leaf

            # get existing permissions
            mode = path.stat().st_mode

            # fixup permissions, o-rwx g-wx
            path.chmod(mode & ~stat.S_IRWXO & ~stat.S_IWGRP & ~stat.S_IXGRP)

            url = upload(package_version, path, str(path)[len("Packages/") :])
            assets.append(url)

            webentry = {
                "format": get_format(path),
                "url": url,
            }
            if "win32" in str(path):
                webentry["bits"] = 32
            elif "win64" in str(path):
                webentry["bits"] = 64

            # if this is a standalone Windows or macOS package, also provide
            # checksum(s)
            if is_macos_artifact(path) or is_windows_artifact(path):
                c = checksum(path)
                url = upload(package_version, c, str(c)[len("Packages/") :])
                assets.append(url)
                webentry[c.suffix[1:]] = url

            # only expose a subset of the Windows artifacts
            if "/windows/10/cmake/Release/" in str(
                path
            ) or "/windows/10/msbuild/Release/" in str(path):
                webdata["windows"].append(webentry)

    # various release pages truncate the viewable artifacts to 100 or even 50
    if len(assets) > 50:
        log.error(
            f"upload has {len(assets)} assets, which will result in some of "
            "them being unviewable in web page lists"
        )
        return -1

    assert len(webdata["windows"]) > 0, "no Windows artifacts found"

    # construct a command to create the release itself
    cmd = [
        "release-cli",
        "create",
        "--name",
        version,
        "--tag-name",
        version,
        "--description",
        "See the [CHANGELOG](https://gitlab.com/"
        "graphviz/graphviz/-/blob/main/CHANGELOG.md).",
    ]
    for a in assets:
        name = os.path.basename(a)
        url = a
        cmd += ["--assets-link", json.dumps({"name": name, "url": url})]

    # create the release
    subprocess.check_call(cmd)

    # output JSON data for the website
    log.info(f"dumping {webdata} to graphviz-{version}.json")
    with open(f"graphviz-{version}.json", "wt", encoding="utf-8") as f:
        json.dump(webdata, f, indent=2)

    return 0


if __name__ == "__main__":
    sys.exit(main())
