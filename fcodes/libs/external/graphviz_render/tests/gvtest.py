"""common Graphviz test functionality"""

import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import sysconfig
import tempfile
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Union

ROOT = Path(__file__).resolve().parent.parent
"""absolute path to the root of the repository"""


def compile_c(
    src: Path,
    cflags: List[str] = None,
    link: List[str] = None,
    dst: Optional[Union[Path, str]] = None,
) -> Path:
    """compile a C program"""

    if cflags is None:
        cflags = []
    if link is None:
        link = []

    # include compiler flags from both the caller and the environment
    cflags = os.environ.get("CFLAGS", "").split() + cflags
    ldflags = os.environ.get("LDFLAGS", "").split()

    # if the user did not give us destination, use a temporary path
    if dst is None:
        _, dst = tempfile.mkstemp(".exe")

    if platform.system() == "Windows" and not is_mingw():
        # determine which runtime library option we need
        rtflag = "-MDd" if os.environ.get("configuration") == "Debug" else "-MD"

        # construct an invocation of MSVC
        args = ["cl", src, "-Fe:", dst, "-nologo", rtflag] + cflags
        if len(link) > 0:
            args += ["-link"] + [f"{l}.lib" for l in link] + ldflags

    else:
        # construct an invocation of the default C compiler
        cc = os.environ.get("CC", "cc")
        args = [cc, "-std=c99", src, "-o", dst] + cflags
        if len(link) > 0:
            args += [f"-l{l}" for l in link] + ldflags

    # dump the command being run for the user to observe if the test fails
    print(f'+ {" ".join(shlex.quote(str(x)) for x in args)}')

    # compile the program
    try:
        subprocess.check_call(args)
    except subprocess.CalledProcessError:
        try:
            os.remove(dst)
        except FileNotFoundError:
            pass
        raise

    return dst


def dot(
    T: str, source_file: Optional[Path] = None, source: Optional[str] = None
) -> Union[bytes, str]:
    """
    run Graphviz on the given source file or text

    Args:
      T: Output format, as would be supplied to `-T` on the command line.
      source_file: Input file to parse. Can be `None` if `source` is provided
        instead.
      source: Input text to parse. Can be `None` if `source_file` is provided
        instead.

    Returns:
      Dot output as text if a textual format was selected or as binary if not.
    """

    assert (
        source_file is not None or source is not None
    ), "one of `source_file` or `source` needs to be provided"

    # is the output format a textual format?
    output_is_text = T in ("canon", "cmapx", "dot", "json", "pic", "svg", "xdot")

    kwargs = {}
    if output_is_text:
        kwargs["encoding"] = "utf-8"
        kwargs["universal_newlines"] = True

    args = ["dot", f"-T{T}"]

    if source_file is not None:
        args += [source_file]
    elif not output_is_text:
        kwargs["input"] = source.encode("utf-8")
    else:
        kwargs["input"] = source

    return subprocess.check_output(args, **kwargs)


def freedesktop_os_release() -> Dict[str, str]:
    """
    polyfill for `platform.freedesktop_os_release`
    """
    release = {}
    os_release = Path("/etc/os-release")
    if os_release.exists():
        with open(os_release, "rt", encoding="utf-8") as f:
            for line in f.readlines():
                if line.startswith("#") or "=" not in line:
                    continue
                key, _, value = (x.strip() for x in line.partition("="))
                # remove quotes
                if len(value) >= 2 and value[0] == '"' and value[-1] == '"':
                    value = value[1:-1]
                release[key] = value
    return release


def gvpr(program: Path) -> str:
    """run a GVPR program on empty input"""

    assert which("gvpr") is not None, "attempt to run GVPR without it available"

    return subprocess.check_output(
        ["gvpr", "-f", program], stdin=subprocess.DEVNULL, universal_newlines=True
    )


def is_cmake() -> bool:
    """was the Graphviz under test built with CMake?"""
    return os.getenv("build_system") == "cmake"


def is_mingw() -> bool:
    """is the current platform MinGW?"""
    return "mingw" in sysconfig.get_platform()


def remove_xtype_warnings(s: str) -> str:
    """
    Remove macOS XType warnings from a string. These appear to be harmless, but
    occur in CI.
    """

    # avoid doing this anywhere except on macOS
    if platform.system() != "Darwin":
        return s

    return re.sub(r"^.* XType: .*\.$\n", "", s, flags=re.MULTILINE)


def run_c(
    src: Path,
    args: List[str] = None,
    input: str = "",
    cflags: List[str] = None,
    link: List[str] = None,
) -> Tuple[str, str]:
    """compile and run a C program"""

    if args is None:
        args = []
    if cflags is None:
        cflags = []
    if link is None:
        link = []

    # create some temporary space to work in
    with tempfile.TemporaryDirectory() as tmp:

        # output filename to write our compiled code to
        exe = Path(tmp) / "a.exe"

        # compile the program
        compile_c(src, cflags, link, exe)

        # dump the command being run for the user to observe if the test fails
        argv = [exe] + args
        print(f'+ {" ".join(shlex.quote(str(x)) for x in argv)}')

        input_bytes = None
        if input is not None:
            input_bytes = input.encode("utf-8")

        # run it
        p = subprocess.run(
            argv,
            input=input_bytes,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        # decode output manually rather than using `universal_newlines=True`
        # above to avoid exceptions from non-UTF-8 bytes in the output
        stdout = p.stdout.decode("utf-8", "replace")
        stderr = p.stderr.decode("utf-8", "replace")

        # check it succeeded
        if p.returncode != 0:
            sys.stdout.write(stdout)
            sys.stderr.write(stderr)
        p.check_returncode()

        return stdout, stderr


def which(cmd: str) -> Optional[Path]:
    """
    `shutil.which` but only return results that are adjacent to the main Graphviz
    executable.

    Graphviz has numerous optional components. So if you choose to e.g. disable
    `mingle` during compilation and installation, you may find that a naive
    `shutil.which("mingle")` after this returns the `mingle` from a prior
    system-wide installation of the full Graphviz. This is undesirable during
    testing, as this older `mingle` will then load shared libraries from the
    installation you just created, most likely crashing.
    """

    # try a straightforward lookup of the command
    abs_cmd = shutil.which(cmd)
    if abs_cmd is None:
        return None
    abs_cmd = Path(abs_cmd)

    # where does the main Graphviz program live?
    abs_dot = shutil.which("dot")
    assert abs_dot is not None, "dot not in $PATH"
    abs_dot = Path(abs_dot)

    # discard the result we found if it does not live in the same directory
    if abs_cmd.parent != abs_dot.parent:
        return None

    return abs_cmd
