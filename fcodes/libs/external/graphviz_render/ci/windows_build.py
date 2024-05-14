#!/usr/bin/env python3

"""Graphviz CI script for compilation on Windows"""

import argparse
import shlex
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, TextIO


def run(args: List[str], cwd: Path, out: TextIO):  # pylint: disable=C0116
    print(f"+ {' '.join(shlex.quote(str(x)) for x in args)}")
    p = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=cwd,
        universal_newlines=True,
    )
    sys.stderr.write(p.stdout)
    out.write(p.stdout)
    p.check_returncode()


def main(args: List[str]) -> int:  # pylint: disable=C0116

    parser = argparse.ArgumentParser(
        description="Graphviz CI script for " "compilation on Windows"
    )
    parser.add_argument(
        "--build-system",
        choices=("cmake", "msbuild"),
        required=True,
        help="build system to run",
    )
    parser.add_argument(
        "--configuration",
        choices=("Debug", "Release"),
        required=True,
        help="build configuration to select",
    )
    parser.add_argument(
        "--logfile",
        type=argparse.FileType("at"),
        required=True,
        help="file to pipe stdout and stderr to",
    )
    parser.add_argument(
        "--platform", choices=("Win32", "x64"), required=True, help="target platform"
    )
    options = parser.parse_args(args[1:])

    # find the repository root directory
    root = Path(__file__).resolve().parent.parent

    if options.build_system == "cmake":
        build = root / "build"
        if build.exists():
            shutil.rmtree(build)
        build.mkdir(parents=True)
        run(["cmake", "--version"], build, options.logfile)
        run(
            [
                "cmake",
                "--log-level=VERBOSE",
                "-G",
                "Visual Studio 16 2019",
                "-A",
                options.platform,
                "-Dwith_cxx_api=ON",
                "-Dwith_gvedit=OFF",
                "--warn-uninitialized",
                "-Werror=dev",
                "..",
            ],
            build,
            options.logfile,
        )
        run(
            ["cmake", "--build", ".", "--config", options.configuration],
            build,
            options.logfile,
        )
        run(["cpack", "-C", options.configuration], build, options.logfile)

    else:
        run(
            [
                "msbuild.exe",
                f"-p:Configuration={options.configuration}",
                f"-p:Platform={options.platform}",
                "graphviz.sln",
            ],
            root,
            options.logfile,
        )
        if options.configuration == "Release":
            for src in (root / "Release" / "Graphviz" / "bin").iterdir():
                if src.suffix in (
                    ".lastcodeanalysissucceeded",
                    ".iobj",
                    ".ipdb",
                    ".ilk",
                ):
                    sys.stderr.write(f"deleting {src}\n")
                    options.logfile.write(f"deleting {src}\n")
                    src.unlink()

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
