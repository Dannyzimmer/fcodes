"""
Support functions for some other test files.

FIXME: This should probably be integrated into ../../tests/gvtest.py
"""

import difflib
from pathlib import Path


def compare_graphs(name, output_type):
    """
    Compare a given graph in the given output format against its reference.
    """
    filename = Path(f"{name}.{output_type}")
    filename_reference = Path("reference") / filename
    filename_output = Path("output") / filename
    if not filename_reference.is_file():
        print(f"Failure: {filename} - No reference file present.")
        return False

    with open(filename_reference, "rt", encoding="utf-8") as reference_file:
        with open(filename_output, "rt", encoding="utf-8") as output_file:
            reference = reference_file.readlines()
            output = output_file.readlines()
            diff_generator = difflib.context_diff(
                output, reference, str(filename_output), str(filename_reference)
            )

            # if diff contains at least one line, the files are different
            diff = []
            for line in diff_generator:
                diff.append(line)

            if len(diff) == 0:
                print(f"Success: {filename}")
                return True
            difference = Path("difference")
            if not difference.exists():
                difference.mkdir(parents=True)

            # Write diff to console
            for line in diff:
                print(line)

            # Store diff in file
            with open(
                f"difference/{str(filename)}", "wt", encoding="utf-8"
            ) as diff_file:
                diff_file.writelines(diff)

            print(
                f"Failure: {filename} - Generated file does not match reference file."
            )
            return False
