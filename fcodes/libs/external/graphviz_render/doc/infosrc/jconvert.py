#!/usr/bin/env python3

"""
convert a json file to an html file
"""

import argparse
import json

from json2html import json2html


def main():  # pylint: disable=missing-function-docstring
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=argparse.FileType("rt"), help="Input file")
    parser.add_argument("output", type=argparse.FileType("wt"), help="Output file")
    args = parser.parse_args()

    json_input = json.load(args.input)
    html_output = json2html.convert(
        json=json_input, table_attributes='class="jsontable"'
    )
    args.output.write(html_output)


if __name__ == "__main__":
    main()
