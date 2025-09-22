#!/bin/python

import argparse
import os
import platform
import shlex
import subprocess
import sys


def run_cmd(cmds: list):
    """
    run given shell commands
    """
    for cmd in cmds:
        try:
            subprocess.run(cmd, check=True)
        except subprocess.CalledProcessError as e:
            cmd_str = " ".join(shlex.quote(part) for part in e.cmd)
            print(f"Error: `{cmd_str}` failed with exit code {e.returncode}")
            sys.exit(e.returncode)


class CmakeTarget:
    """
    handles all cmake configuration and build target
    """

    def __init__(self, build_dir="build"):
        self.default_tool = "cmake"
        self.build_dir = build_dir

    @staticmethod
    def config():
        return {
            "configure": {
                "cmd": ["-S", ".", "-G", "Ninja"],
                "description": "Configure the project",
                "working_dir": "-B",
            },
            "build": {
                "cmd": [],
                "description": "Build the project",
                "working_dir": "--build",
            },
            "db": {
                "cmd": ["-S", ".", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"],
                "description": "Generate compile_commands.json",
                "working_dir": "-B",
            },
            "refresh": {
                "cmd": ["--fresh", "-S", ".", "-G", "Ninja"],
                "description": "Fresh configure the project",
                "working_dir": "-B",
            },
            "clean": {
                "cmd": ["--target", "clean"],
                "description": "Clean the build files",
                "working_dir": "--build",
                "post_hook": "clean_artifact",
            },
            "test": {
                "cmd": ["--output-on-failure"],
                "description": "Run tests",
                "working_dir": "--test-dir",
                "tool": "ctest",
            },
            "config_dump": {
                "description": "Dumps complete configurations, for debugging",
                "post_hook": "print_cfg_dump",
            },
        }

    ############## Main #######################

    @staticmethod
    def cmd():
        """
        array of possible configurations
        """
        return CmakeTarget.config().keys()

    @staticmethod
    def help() -> str:
        """
        help msg for this class
        """
        lines = []
        for mode, info in CmakeTarget.config().items():
            lines.append(f"  {mode:<15} - {info['description']}")
        return "\n".join(lines)

    def invoke(self, modes: list):
        """
        entry point : runs associated Cmake commands given modes
        """
        for mode in modes:
            cfg = self.generate_cfg_mode(mode)
            cmd = cfg.get("cmd")
            if cmd:
                run_cmd([cmd])

            post_hook, args, kwarg = self.resolve_hook(
                cfg, cfg.get("post_hook"), "post_arg", "post_kwarg"
            )
            if post_hook:
                post_hook(*args, **kwarg)

    def resolve_hook(self, cfg, hook_name: str, arg_name: str, kwarg_name: str):
        """
        resolve the function name at run time from given string
        """
        if not hook_name:
            return None, [], {}

        method = getattr(self, hook_name, None) or getattr(CmakeTarget, hook_name, None)
        if not method or not callable(method):
            raise AttributeError(f"Post method '{hook_name}' not found or not callable")

        args = cfg.get(arg_name, [])
        kwargs = cfg.get(kwarg_name, {})
        return method, args, kwargs

    def generate_cfg_mode(self, mode):
        """
        generate a single cfg based on mode

        if 'post_hook' depends on args, new entries :
            ['post_arg'] = []
            ['post_kwarg'] = {}
        must be added to the cfg
        """
        cfg = self.config()[mode].copy()

        cmd = cfg.get("cmd")
        if cmd:
            cmd = cmd.copy()
            cmd.insert(0, cfg.get("tool", self.default_tool))

            build_flag = cfg.get("working_dir")
            if build_flag:
                cmd[1:1] = [build_flag, self.build_dir]

            cfg["cmd"] = cmd

        return cfg

    ############## Optional #######################

    def generate_all(self):
        """
        generate all configurations
        """
        return {mode: self.generate_cfg_mode(mode) for mode in self.config()}

    def print_cfg_dump(self):
        print(self.generate_all())

    @staticmethod
    def clean_artifact():
        """
        msvc sometimes generates build files outside of the specified dir
        need to clean it up
        """
        if platform.system() == "Windows":
            MSVC_TEMP_EXTS = [".ilk", ".pdb"]

            for file in os.listdir("."):
                if any(file.endswith(ext) for ext in MSVC_TEMP_EXTS):
                    os.remove(file)


def main():
    parser = argparse.ArgumentParser(
        prog="build.py",
        formatter_class=argparse.RawTextHelpFormatter,
    )

    parser.add_argument(
        "-C",
        "--working-dir",
        default="build",
        help="Specify custom build directory (default: build)",
    )

    parser.add_argument(
        "mode",
        metavar="mode",
        nargs="+",
        choices=CmakeTarget.cmd(),
        help=CmakeTarget.help(),
    )
    args = parser.parse_args()

    cmake_target = CmakeTarget(args.working_dir)
    cmake_target.invoke(args.mode)


if __name__ == "__main__":
    main()
