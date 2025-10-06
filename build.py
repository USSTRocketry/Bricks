#!/usr/bin/env python3

import argparse
import os
import platform
import shlex
import subprocess
import sys
from pathlib import Path


def run_cmd(cmds: list):
    """
    run given shell commands
    """
    for cmd in cmds:
        print(cmd)
        try:
            subprocess.run(cmd, check=True)
        except subprocess.CalledProcessError as e:
            cmd_str = " ".join(shlex.quote(part) for part in e.cmd)
            print(f"Error: `{cmd_str}` failed with exit code {e.returncode}")
            sys.exit(e.returncode)


class Target:
    @staticmethod
    def config():
        """
        Return dict of mode configurations:
        mode_name -> dict (e.g. command parts, description, hooks)
        """
        raise NotImplementedError()

    @staticmethod
    def cmd():
        """
        Return list of valid modes (keys from config)
        """
        return list(Target.config().keys())

    @staticmethod
    def help() -> str:
        """
        Return help string describing available modes
        """
        return ""

    def invoke(self, modes: list, args: list):
        """
        Execute given modes with extra args (dict: mode -> args list)
        """
        raise NotImplementedError()


class CmakeTarget(Target):
    """
    handles all cmake configuration and build target
    """

    def __init__(self, build_dir="Build", args: list = []):
        self.build_dir = build_dir

    @staticmethod
    def config():
        return {
            "configure": {
                "cmd": ["-S", ".", "-G", "Ninja"],
                "description": "Configure the project",
                "working_dir": "-B",
                "pre_hook": "populate_cmake_cmd",
            },
            "debug": {
                "cmd": ["-S", ".", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug"],
                "description": "Configure the project",
                "working_dir": "-B",
                "pre_hook": "populate_cmake_cmd",
            },
            "build": {
                "cmd": [],
                "description": "Build the project",
                "working_dir": "--build",
                "pre_hook": "populate_cmake_cmd",
            },
            "db": {
                "cmd": ["-G", "Ninja", "-S", ".", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"],
                "description": "Generate compile_commands.json",
                "working_dir": "-B",
                "pre_hook": "populate_cmake_cmd",
            },
            "refresh": {
                "cmd": ["--fresh", "-S", ".", "-G", "Ninja"],
                "description": "Fresh configure the project",
                "working_dir": "-B",
                "pre_hook": "populate_cmake_cmd",
            },
            "clean": {
                "cmd": ["--target", "clean"],
                "description": "Clean the build files",
                "working_dir": "--build",
                "pre_hook": "populate_cmake_cmd",
                "post_hook": "clean_artifact",
            },
            "test": {
                "cmd": ["--gtest_brief=1"],
                "description": "run default test",
                "pre_hook": "populate_test_cmd",
                "pre_arg": ["TestRunner", True],
            },
            "ctest": {
                "description": "run cmake test",
                "tool": "ctest",
                "working_dir": "--test-dir",
                "pre_hook": "populate_test_cmd",
                "pre_arg": ["TestRunner"],
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
            self.current_cfg = self.config()[mode].copy()
            cfg = self.current_cfg

            pre_hook, hook_arg, hook_kwarg = self.resolve_hook(
                cfg, cfg.get("pre_hook", ""), "pre_arg", "pre_kwarg"
            )
            if pre_hook:
                pre_hook(*hook_arg, **hook_kwarg)

            cmd = cfg.get("cmd")
            if cmd:
                run_cmd([cmd])

            post_hook, hook_arg, hook_kwarg = self.resolve_hook(
                cfg, cfg.get("post_hook", ""), "post_arg", "post_kwarg"
            )
            if post_hook:
                post_hook(*hook_arg, **hook_kwarg)

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

    ############## Optional #######################

    def populate_cmake_cmd(self):
        cfg = self.current_cfg
        default_tool = "cmake"
        # allow empty cmd array
        if cfg.get("cmd") is None:
            return cfg

        cmd_rest = cfg.get("cmd").copy()

        cmd = [cfg.get("tool", default_tool)]
        build_flag = cfg.get("working_dir")
        if build_flag:
            cmd.extend([build_flag, self.build_dir])

        cmd.extend(cmd_rest)
        cfg["cmd"] = cmd

        return cfg

    @staticmethod
    def clean_artifact():
        """
        On builds where the binary is forced to generate in the root directory,
        msvc will also generate build specific '.ilk' and '.pdb' files
        in the binary directory instead of the build directory.
        """
        if platform.system() == "Windows":
            MSVC_TEMP_EXTS = [".ilk", ".pdb"]

            for file in os.listdir("."):
                if any(file.endswith(ext) for ext in MSVC_TEMP_EXTS):
                    os.remove(file)

    def find_ctest_dir(self) -> Path | None:
        """
        Search recursively under self.build_dir for a directory containing CTestTestfile.cmake.
        Return the first such directory found as a Path object, or None if not found.
        """
        build_path = Path(self.build_dir)
        for root, _, files in os.walk(build_path):
            if "CTestTestfile.cmake" in files:
                return Path(root)
        return None

    def populate_test_cmd(self, test_name: str, as_executable=False):
        """
        Build and set the command list in cfg['cmd'] to run tests.

        Args:
            cfg (dict): Configuration dictionary.
            test_name (str): Name of the test runner.
                            - If `as_executable` is True, this is a binary name (without '.exe').
                            - If `as_executable` is False, this is a directory name

            as_executable (bool): If True, appends executable (adds '.exe' on Windows).

        Modifies:
            cfg['cmd']: List of command parts to execute.
        """
        test_runner_path = self.find_ctest_dir()
        if not test_runner_path:
            print("Warn : cmake test dir not found, trying build dir")
            test_runner_path = Path(self.build_dir)

        if as_executable:
            if platform.system() == "Windows":
                test_runner_path /= test_name + ".exe"

        cmd = []
        cfg = self.current_cfg

        tool = cfg.get("tool")
        if tool:
            cmd.append(tool)

        build_flag = cfg.get("working_dir")
        if build_flag:
            cmd.append(build_flag)

        cmd.append(str(test_runner_path))

        cmd_rest = cfg.get("cmd")
        if cmd_rest:
            cmd.extend(cmd_rest)

        cfg["cmd"] = cmd


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
        nargs="*",
        choices=CmakeTarget.cmd(),
        help=CmakeTarget.help(),
    )

    args = parser.parse_args()

    cmake_target = CmakeTarget(args.working_dir)
    cmake_target.invoke(args.mode)


if __name__ == "__main__":
    main()
