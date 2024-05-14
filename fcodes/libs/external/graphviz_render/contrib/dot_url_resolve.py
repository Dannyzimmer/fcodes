#!/usr/bin/env python3

"""
Resolve external image references in a Graphviz source.

Nodes in a graph can have an associated image, `my_node[image="foo.png"]`, but
the image string must be a path to a local file. Using a URL to point to a
remotely hosted image is not supported natively. This script resolves such
external references allowing the use of such references:

  echo 'graph { a[image="https://graphviz.org/Resources/app.png"]; }' \
    | dot_url_resolve.py \
    | dot -Tpng -o my_output.png

This script does not have a sophisticated understanding of the Graphviz
language. It simply treats anything that looks like a string containing a URL as
something that should be downloaded.
"""

import argparse
import hashlib
import io
import logging
import re
import sys
import tempfile
import urllib.request
from pathlib import Path
from typing import Dict, List, Optional, TextIO


def _translate(
    source: str,
    translations: Dict[str, Path],
    local_store: Path,
    log: Optional[logging.Logger],
) -> str:
    """
    convert a remote URL to a local path, downloading if necessary

    If `source` is not a remote URL, it is returned as-is.

    Args:
        source: URL to resolve
        translations: accumulated mapping from URLs to local paths
        local_store: directory to write downloaded files to
        log: optional progress sink

    Returns:
        local path corresponding to where the URL was downloaded to
    """

    # does this look like a remote URL?
    if re.match(r"https?:", source, flags=re.IGNORECASE):
        # have we not yet downloaded this ?
        local = translations.get(source)
        if local is None:
            # generate a unique local filename to write to
            digest = hashlib.sha256(source.encode("utf-8")).hexdigest()
            extension = Path(source).suffix
            dest = local_store / f"{digest}{extension}"

            # download the file
            if log is not None:
                log.info(f"downloading {source} → {dest}")
            urllib.request.urlretrieve(source, dest)
            translations[source] = dest

        return str(translations[source])

    return source


def resolve(
    inp: TextIO, outp: TextIO, local_store: Path, log: Optional[logging.Logger] = None
) -> Dict[str, Path]:
    """
    process Graphviz source, converting remote URLs to local paths

    Args:
        inp: source to read from
        outp: destination to write to
        local_store: directory to write downloaded files to
        log: optional progress sink

    Returns:
        a mapping from URLs discovered to paths to which they were downloaded
    """

    # translations from original URLs to local paths
    downloaded: Dict[str, Path] = {}

    in_string = False
    pending = io.StringIO()
    while True:
        c = inp.read(1)

        if in_string:
            # does this terminate a string we were accruing?
            if c in ("", '"'):
                accrued = pending.getvalue()
                pending = io.StringIO()

                outp.write(_translate(accrued, downloaded, local_store, log))

                in_string = False
            else:
                pending.write(c)
                continue

        elif not in_string and c == '"':
            in_string = True

        if c == "":
            break

        outp.write(c)

    return downloaded


def main(args: List[str]) -> int:
    """
    entry point
    """

    # parse command line options
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "input",
        nargs="?",
        type=argparse.FileType("rt"),
        default=sys.stdin,
        help="Graphviz source to read",
    )
    parser.add_argument(
        "output",
        nargs="?",
        type=argparse.FileType("wt"),
        default=sys.stdout,
        help="Graphviz source to write",
    )
    parser.add_argument(
        "--local-dir",
        help="path to write resolved files to (default: temporary location)",
    )
    parser.add_argument(
        "--quiet", "-q", action="store_true", help="suppress progress messages"
    )
    options = parser.parse_args(args[1:])

    # use a temporary directory if we were not given one
    if options.local_dir is None:
        options.local_dir = Path(tempfile.mkdtemp())
    else:
        options.local_dir = Path(options.local_dir)

    # setup logging
    log = logging.getLogger()
    log.setLevel(logging.WARNING if options.quiet else logging.INFO)
    handler = logging.StreamHandler(sys.stderr)
    log.addHandler(handler)

    resolve(options.input, options.output, options.local_dir, log)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
