#!/usr/bin/env python3

"""
wrapper for running Catch2 unit tests

Catch2 supports a `--reporter` parameter which the Graphviz test suite uses to
output unit test results in JUnit format and merge these with the integration
test results. Unfortunately there is no way to write this XML output to a file
while also preserving stderr output for the interactive user (or CI log) to see.
This script works around this limitation by running a JUnit-producing Catch2
test and then, if it fails, re-running it to produce stderr output of the
failure.
"""

import argparse
import subprocess
import sys
from typing import List


def main(args: List[str]) -> int:
    """entry point"""

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("test_name", help="name of test to run")
    options = parser.parse_args(args[1:])

    try:
        subprocess.check_call(
            [
                f"./test_{options.test_name}",
                "--reporter=junit",
                f"--out=test_{options.test_name}.xml",
            ]
        )
    except subprocess.CalledProcessError:
        sys.stderr.write(
            f"{options.test_name} failed. Re-running to get console output of "
            "the failure...\n"
        )
        subprocess.call([f"./test_{options.test_name}"])
        return -1

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
