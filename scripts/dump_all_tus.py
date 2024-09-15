#!/usr/bin/env python

import argparse
import json
import shlex
import subprocess
import sys
from pathlib import Path

def change_std(build_action: str, std: str):
    args = shlex.split(build_action["command"])

    idx = next(
        (i for i, arg in enumerate(args) if arg.startswith("-std=")),
        None)

    if idx is None:
        args.append("-std=" + std)
    else:
        args[idx] = "-std=" + std

    build_action["command"] = shlex.join(args)

def command_line_args():
    parser = argparse.ArgumentParser(description="AST Dump Tool Executor")

    parser.add_argument(
        "file",
        type=argparse.FileType("r"),
        help="Compilation database file")

    parser.add_argument(
        "--std",
        choices=["c++98", "c++03", "c++11", "c++14", "c++17", "c++20"],
        required=True,
        help="C++ standard version")

    parser.add_argument(
        "-o", "--output",
        type=Path,
        required=True,
        help="Output directory for the AST dump files")

    return parser.parse_args()

def main():
    args = command_line_args()

    try:
        args.output.mkdir(parents=True, exist_ok=True)
    except FileExistsError:
        print("Output directory already exists", file=sys.stderr)
        exit(1)

    build_actions = json.load(args.file)
    args.file.close()

    for action in build_actions:
        change_std(action, args.std)

    with open(args.file.name, "w") as f:
        json.dump(build_actions, f, indent=2)

    for action in build_actions:
        with open(args.output / Path(action["file"]).name, "w") as f:
            subprocess.Popen(
                ['ast-dump-tool', action["file"]],
                cwd=action["directory"],
                stdout=f)

if __name__ == "__main__":
    main()
