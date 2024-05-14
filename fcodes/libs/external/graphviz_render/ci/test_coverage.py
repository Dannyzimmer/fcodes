#!/usr/bin/env python3

"""Graphviz test coverage analysis script"""

import argparse
import logging
import subprocess
import sys
from pathlib import Path
from typing import List

# logging output stream, setup in main()
log = None


def main(args: List[str]) -> int:  # pylint: disable=C0116
    # setup logging to print to stderr
    global log
    ch = logging.StreamHandler()
    log = logging.getLogger("test_coverage.py")
    log.addHandler(ch)

    # parse command line arguments
    parser = argparse.ArgumentParser(
        description="Graphviz test coverage analysis script"
    )
    parser.add_argument(
        "--init",
        action="store_true",
        help="Capture initial zero coverage data before running any test",
    )
    parser.add_argument(
        "--analyze",
        action="store_true",
        help="Analyze test coverage after running tests",
    )
    options = parser.parse_args(args[1:])

    if not options.init and not options.analyze:
        log.error("Must specify --init or --analyze; refusing to run")
        return -1

    cwd = Path.cwd()

    generated_files = [
        cwd / "build/cmd/tools/gmlparse.c",
        cwd / "build/cmd/tools/gmlscan.c",
        cwd / "build/lib/cgraph/grammar.c",
        cwd / "build/lib/cgraph/scan.c",
        cwd / "build/lib/common/htmlparse.c",
        cwd / "build/lib/expr/exparse.c",
        cwd / "build/cmd/gvedit/gvedit_autogen/EWIEGA46WW/moc_csettings.cpp",
        cwd / "build/cmd/gvedit/gvedit_autogen/EWIEGA46WW/moc_imageviewer.cpp",
        cwd / "build/cmd/gvedit/gvedit_autogen/EWIEGA46WW/moc_mainwindow.cpp",
        cwd / "build/cmd/gvedit/gvedit_autogen/EWIEGA46WW/moc_mdichild.cpp",
        cwd / "build/cmd/gvedit/gvedit_autogen/EWIEGA46WW/qrc_mdi.cpp",
    ]

    excluded_files = generated_files

    exclude_options = [
        arg
        for exclude_option in [["--exclude", f] for f in excluded_files]
        for arg in exclude_option
    ]

    if options.init:
        subprocess.check_call(
            [
                "lcov",
                "--capture",
                "--initial",
                "--directory",
                ".",
                "--rc",
                "lcov_branch_coverage=1",
                "--no-external",
            ]
            + exclude_options
            + ["--output-file", "app_base.info"]
        )

        return 0

    if options.analyze:
        # capture test coverage data
        subprocess.check_call(
            [
                "lcov",
                "--capture",
                "--directory",
                ".",
                "--rc",
                "lcov_branch_coverage=1",
                "--no-external",
            ]
            + exclude_options
            + ["--output-file", "app_test.info"]
        )
        # combine baseline and test coverage data
        subprocess.check_call(
            [
                "lcov",
                "--rc",
                "lcov_branch_coverage=1",
                "--add-tracefile",
                "app_base.info",
                "-add-tracefile",
                "app_test.info",
                "--output-file",
                "app_total.info",
            ]
        )
        # generate coverage html pages using lcov which are nicer than gcovr's
        Path("coverage/lcov").mkdir(parents=True, exist_ok=True)
        subprocess.check_call(
            [
                "genhtml",
                "--prefix",
                cwd,
                "--rc",
                "lcov_branch_coverage=1",
                "--output-directory",
                "coverage/lcov",
                "--show-details",
                "app_total.info",
            ]
        )
        # generate coverage info for GitLab's Test Coverage Visualization
        Path("coverage/gcovr").mkdir(parents=True, exist_ok=True)
        subprocess.check_call(
            ["gcovr"]
            + exclude_options
            + [
                "--xml-pretty",
                "--html-details=coverage/gcovr/index.html",
                "--exclude-unreachable-branches",
                "--print-summary",
                "--output",
                "coverage.xml",
                "--root",
                cwd,
            ]
        )

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
