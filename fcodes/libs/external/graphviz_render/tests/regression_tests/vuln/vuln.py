"""
Some legacy tests against previous bugs.

FIXME: this should probably be integrated into ../../../tests/test_regression.py
"""

import subprocess
import sys
from pathlib import Path

# Import helper function to compare graphs from tests/regressions_tests
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from regression_test_helpers import (  # pylint: disable=import-error,wrong-import-position
    compare_graphs,
)

vulnfiles = ["nullderefrebuildlist"]

output_types = [("xdot", "xdot:xdot:core")]


def generate_vuln_graph(vulnfile, output_type):
    """
    Generate a graph from the given file in the given output format.
    """
    if not Path("output").exists():
        Path("output").mkdir(parents=True)

    output_file = Path("output") / f"{vulnfile}.{output_type[0]}"
    input_file = Path("input") / f"{vulnfile}.dot"
    try:
        subprocess.check_call(
            ["dot", f"-T{output_type[1]}", "-o", output_file, input_file]
        )
    except subprocess.CalledProcessError:
        print(f"An error occurred while generating: {output_file}")
        sys.exit(1)


failures = 0
for v in vulnfiles:
    for o in output_types:
        generate_vuln_graph(v, o)
        if not compare_graphs(v, o[0]):
            failures += 1

print("")
print('Results for "vuln" regression test:')
print(f"    Number of tests: {len(vulnfiles) * len(output_types)}")
print(f"    Number of failures: {failures}")

if not failures == 0:
    sys.exit(1)
