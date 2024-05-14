#!/usr/bin/env python3

"""
Generate version

Release version entry format : <major>.<minor>.<patch>

Stable release version output format     : <major>.<minor>.<patch>
Development release version output format: <major>.<minor>.<patch>~dev.<YYYYmmdd.HHMM>

The patch version of a development release should be the same as the
next stable release patch number. The string "~dev." and the
committer date will be added.

Example sequence:

Entry version   Entry collection          Output
2.44.1          stable                 => 2.44.1
2.44.2          development            => 2.44.2~dev.20200704.1652
2.44.2          stable                 => 2.44.2
2.44.3          development            => 2.44.3~dev.20200824.1337
"""

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Tuple

CHANGELOG = Path(__file__).parent / "CHANGELOG.md"
assert CHANGELOG.exists(), "CHANGELOG.md file missing"


def get_version() -> Tuple[int, int, int, str]:
    """
    Derive a Graphviz version from the changelog information.

    Returns a tuple of major version, minor version, patch version,
    "stable"/"development".
    """

    # is this a development revision (as opposed to a stable release)?
    is_development = False

    with open(CHANGELOG, encoding="utf-8") as f:
        for line in f:

            # is this a version heading?
            m = re.match(r"## \[(?P<heading>[^\]]*)\]", line)
            if m is None:
                continue
            heading = m.group("heading")

            # is this the leading unreleased heading of a development version?
            UNRELEASED_PREFIX = "Unreleased ("
            if heading.startswith(UNRELEASED_PREFIX):
                is_development = True
                heading = heading[len(UNRELEASED_PREFIX) :]

            # extract the version components
            m = re.match(r"(?P<major>\d+)\.(?P<minor>\d+)\.(?P<patch>\d+)", heading)
            if m is None:
                raise RuntimeError(
                    "non-version ## heading encountered before seeing a "
                    "version heading"
                )

            major = int(m.group("major"))
            minor = int(m.group("minor"))
            patch = int(m.group("patch"))
            break

        else:
            # we read the whole changelog without finding a version
            raise RuntimeError("no version found")

    if is_development:
        coll = "development"
    else:
        coll = "stable"

    return major, minor, patch, coll


graphviz_date_format = "%Y%m%d.%H%M"
changelog_date_format = "%a %b %e %Y"

parser = argparse.ArgumentParser(description="Generate Graphviz version.")
parser.add_argument(
    "--committer-date-changelog",
    dest="date_format",
    action="store_const",
    const=changelog_date_format,
    help="Print changelog formatted committer date in UTC instead of version",
)
parser.add_argument(
    "--committer-date-graphviz",
    dest="date_format",
    action="store_const",
    const=graphviz_date_format,
    help="Print graphviz special formatted committer date in UTC " "instead of version",
)
parser.add_argument(
    "--major",
    dest="component",
    action="store_const",
    const="major",
    help="Print major version",
)
parser.add_argument(
    "--minor",
    dest="component",
    action="store_const",
    const="minor",
    help="Print minor version",
)
parser.add_argument(
    "--patch",
    dest="component",
    action="store_const",
    const="patch",
    help="Print patch version",
)
parser.add_argument(
    "--definition", action="store_true", help="Print a C-style preprocessor #define"
)
parser.add_argument(
    "--output",
    type=argparse.FileType("wt", encoding="ascii"),
    default=sys.stdout,
    help="Path to write result to",
)

args = parser.parse_args()

date_format = args.date_format or graphviz_date_format

major_version, minor_version, patch_version, collection = get_version()

if collection == "development":
    patch_version = f"{patch_version}~dev"
else:
    patch_version = str(patch_version)

if not patch_version.isnumeric() or args.date_format:
    os.environ["TZ"] = "UTC"
    try:
        committer_date = subprocess.check_output(
            [
                "git",
                "log",
                "-n",
                "1",
                "--format=%cd",
                f"--date=format-local:{date_format}",
            ],
            cwd=os.path.abspath(os.path.dirname(__file__)),
            universal_newlines=True,
        ).strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        sys.stderr.write(
            "Warning: build not started in a Git clone, or Git is not "
            "installed: setting version date to 0.\n"
        )
        committer_date = "0"

if not patch_version.isnumeric():
    # Non-numerical patch version; add committer date
    patch_version += f".{committer_date}"

if args.date_format:
    if args.definition:
        args.output.write(f'#define BUILDDATE "{committer_date}"\n')
    else:
        args.output.write(f"{committer_date}\n")
elif args.component == "major":
    if args.definition:
        args.output.write(f'#define VERSION_MAJOR "{major_version}"\n')
    else:
        args.output.write(f"{major_version}\n")
elif args.component == "minor":
    if args.definition:
        args.output.write(f'#define VERSION_MINOR "{minor_version}"\n')
    else:
        args.output.write(f"{minor_version}\n")
elif args.component == "patch":
    if args.definition:
        args.output.write(f'#define VERSION_PATCH "{patch_version}"\n')
    else:
        args.output.write(f"{patch_version}\n")
else:
    if args.definition:
        args.output.write(
            f'#define VERSION "{major_version}.{minor_version}.' f'{patch_version}"\n'
        )
    else:
        args.output.write(f"{major_version}.{minor_version}.{patch_version}\n")
