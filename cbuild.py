#!/usr/bin/env python3
import itertools
import urllib.request
import copy
import os
import sys
import pathlib
import argparse
import subprocess
import shutil
import platform
from dataclasses import dataclass, field
from typing import Literal, Optional

# A python implementation of the `cbuild` command. Used for bootstrapping this
# repository, but also kept up to date with the C implementation.

# TODO: make the C implementation.
# TODO: actually make sure things need to be rebuilt

DEFAULT_CACHE_DIR = ".cache"
C_CACHE_DIR: pathlib.Path = (
    pathlib.Path(os.environ.get("XDG_CACHE_HOME", DEFAULT_CACHE_DIR)) / "c"
)
C_CLANG_COMMAND: pathlib.Path = os.environ.get("CLANG_COMMAND", "clang")
C_BUILD_CACHE_DIR = C_CACHE_DIR / "build"
C_DOWNLOAD_DIR = C_CACHE_DIR / "pkg"
if C_BUILD_CACHE_DIR.exists():
    shutil.rmtree(C_BUILD_CACHE_DIR)
C_CACHE_DIR.mkdir(parents=True, exist_ok=True)
C_DOWNLOAD_DIR.mkdir(parents=True, exist_ok=True)
C_BUILD_CACHE_DIR.mkdir(parents=True, exist_ok=True)
# A in-memory cache of the module objects we've created, indexed by
# import_path.
MOD_CACHE = {}
args: argparse.Namespace


def get_os() -> str:
    result = platform.system().lower()
    if result == "darwin":
        return "macos"

    return result


def todo(thing: str):
    print(f"not implemented: {thing}")
    exit(1)


def error(msg: str):
    print(f"error: {msg}", file=sys.stderr)
    exit(1)


@dataclass
class Module:
    """Module contains all the information associated with a C source module."""

    std: str
    has_executable: bool
    has_lib: bool
    # The path containing the sources for the module.
    # The preprocessed sources always go in C_BUILD_CACHE_DIR / self.import_path
    path: pathlib.Path
    import_path: str
    # A list of the names of each submodule.
    submodules: list[str]
    platform_flags: dict[str, list[str]]
    parent: Optional["Module"] = None
    dependencies: list["Module"] = field(default_factory=list)

    def get_submodule(self, name: str) -> Optional["Module"]:
        assert name in self.submodules
        result = copy.copy(self)
        result.parent = self
        result.submodules = []
        result.path /= name
        result.import_path += "/" + name
        result.has_executable = (result.path / "main.c").exists()
        result.has_lib = (result.path / "lib.c").exists() or (
            result.path / (name + ".c")
        ).exists()

        MOD_CACHE[result.import_path] = result
        return result

    def lib_c(self) -> pathlib.Path:
        """Returns the module's library main file if it exists."""
        assert self.has_lib
        libc = C_BUILD_CACHE_DIR / self.import_path / "lib.c"
        modc = C_BUILD_CACHE_DIR / self.import_path / (self.module_name() + ".c")
        assert any([libc.exists(), modc.exists()]) and not all(
            [libc.exists(), modc.exists()]
        )

        if libc.exists():
            return libc.resolve()

        return modc.resolve()

    def main_c(self) -> pathlib.Path:
        """Returns the module's main.c file if it exists."""
        assert self.has_executable
        return C_BUILD_CACHE_DIR / self.import_path / "main.c"

    def h(self) -> pathlib.Path:
        """Generates a __module.h file for self."""
        implicit = C_BUILD_CACHE_DIR / self.import_path / "__module.h"
        if implicit.exists():
            return implicit.resolve()

        implicit_include = ""
        for header in (C_BUILD_CACHE_DIR / self.import_path).glob("*.h"):
            if header == implicit:
                continue
            implicit_include += f'#include "{header.resolve()}"\n'
        implicit_include += "\n"

        # put file in cache
        implicit.write_text(implicit_include)
        return implicit.resolve()

    def module_name(self) -> str:
        return self.import_path.split("/")[-1]

    def lib(self) -> Optional[pathlib.Path]:
        if not self.has_lib:
            return None

        return C_BUILD_CACHE_DIR / self.import_path / "lib.o"

    def exe(self) -> Optional[pathlib.Path]:
        if not self.has_executable:
            return None

        return C_BUILD_CACHE_DIR / self.import_path / self.module_name()

    def is_submodule(self) -> bool:
        return self.parent is not None

    def resolve_import(self, import_path: str) -> "Module":
        """Resolves an import in the context of building self."""

        self_modname = self.import_path
        if self.parent is not None:
            self_modname = self.parent.import_path
        if import_path.startswith(self_modname):
            # It is a submodule
            root = self
            if self.is_submodule():
                root = self.parent

            modname = import_path.split("/")[-1]
            if modname not in root.submodules:
                error(
                    f"module '{root.import_path}' does not have a "
                    + f"submodule '{modname}'"
                )

            return root.get_submodule(modname)

        # If it already was fetched, we're done.
        # NOTE: This is only the case because we're not actually caching
        #       anything. This only checks if we've already downloaded it
        #       during the current build, not if it was here from a previous
        #       build.
        if (C_DOWNLOAD_DIR / import_path).exists():
            # if (C_BUILD_CACHE_DIR / import_path).exists():
            #     todo("make a module from")
            moddir = C_DOWNLOAD_DIR / import_path
            if (moddir / "c.mod").exists():
                if import_path in MOD_CACHE:
                    return MOD_CACHE[import_path]
                return Module.from_directory(moddir)

            submod = import_path.split("/")[-1]
            if import_path in MOD_CACHE:
                return MOD_CACHE[import_path]
            return Module.from_directory(moddir.parent).get_submodule(submod)

        # It is a remote module
        print(f"downloading module {import_path}...")

        # First create the path where it will go
        (C_DOWNLOAD_DIR / import_path).mkdir(parents=True, exist_ok=True)

        # Convenience function to call git for us
        def git(*git_argv) -> subprocess.CompletedProcess[bytes]:
            argv = [
                "git",
                "-C",
                str((C_DOWNLOAD_DIR / currentmod).resolve()),
            ]
            argv.extend(git_argv)
            if args.verbose:
                print(" ".join(map(str, argv)))
            return subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        assert import_path[-1] != "/", "sanitize trailing slashes from import paths"
        currentmod = import_path
        while True:
            # Resolve any redirects to get the actual repository name (required
            # when trying to access github hosted repositories begind a
            # redirect, which is the case for many of my c.eldidi.org modules).
            url = "https://" + currentmod
            try:
                with urllib.request.urlopen("https://" + currentmod, timeout=1) as resp:
                    url = resp.url
            except Exception as _:
                modsplit = currentmod.rsplit("/", maxsplit=1)
                # If we've reached first part of the module name and still
                # haven't found anything, it must not exist.
                if len(modsplit) == 1:
                    error(f"no module with name {import_path}")

                currentmod = "/".join(modsplit[:-1])
                continue

            break

        if args.verbose:
            print(str(C_DOWNLOAD_DIR / currentmod))

        # TODO: see if this is necessary
        # We do `git init; git remote add url git fetch; git checkout;`
        # instead of `git clone folder url` in case the folder is not
        # empty, which git doesn't allow.
        ret = git("init")
        assert ret.returncode == 0

        ret = git("remote", "add", "origin", url)
        assert ret.returncode == 0

        ret = git("fetch", "--depth=1")
        if ret.returncode != 0:
            print(ret.stderr.decode("utf-8"), file=sys.stderr)
            error(
                f"failed to fetch module '{import_path}' (imported by "
                + f"'{self.import_path}')"
            )

        ret = git("checkout", "main")
        if ret.returncode != 0:
            print(ret.stderr.decode("utf-8"), file=sys.stderr)
            error(
                f"failed to fetch module '{import_path}' (imported by "
                + f"'{self.import_path}')"
            )

        moddir = C_DOWNLOAD_DIR / currentmod
        cmod = moddir / "c.mod"
        if not cmod.exists():
            error(
                "downloaded repository is not a cbuild project (no c.mod "
                + "file found)"
            )

        result = Module.from_directory(moddir)
        if currentmod == import_path:
            # we imported the root module
            return result

        # we imported a submodule
        submodname = import_path[len(currentmod) + 1 :]
        if submodname not in result.submodules:
            error(
                f"module '{currentmod}' has no submodule '{submodname}'"
                + f"(imported by {self.import_path})"
            )

        return result.get_submodule(submodname)

    @staticmethod
    def _preprocess_dir(mod: "Module"):
        if args.verbose:
            print(f"preprocessing module '{mod.import_path}'")
        for file in itertools.chain(mod.path.glob("*.c"), mod.path.glob("*.h")):
            if file.is_dir():
                continue
            if file.name == "c.mod":
                continue

            lines = file.read_text(encoding="utf-8").splitlines()
            for i in range(len(lines)):
                line = lines[i].strip().split()
                if len(line) == 0:
                    continue

                if line[0] != "#include":
                    continue

                # is it a system include?
                if line[1].startswith("<"):
                    continue

                include = line[1].strip('"')
                if include.startswith("./") or include.startswith("../"):
                    continue

                # TODO: I don't know if this is necessary
                # if include.startswith(str(C_CACHE_DIR.resolve())):
                #     continue

                imported_mod = mod.resolve_import(include)
                if imported_mod.import_path == mod.import_path:
                    error(
                        f"module '{mod.import_path}' includes itself, "
                        + "which is not allowed."
                    )

                mod.dependencies.append(imported_mod)
                mod.dependencies.extend(imported_mod.dependencies)
                new_include = C_BUILD_CACHE_DIR / imported_mod.import_path
                lines[i] = f'#include "{(new_include /"__module.h").resolve()}"'

            # if the file is not local to the project, it will be in the cache
            # directory, and we should refer to it by its module path, not file
            # path.
            name = str(pathlib.Path(mod.import_path) / file.relative_to(mod.path))
            lines.insert(0, f'#line 1 "{name}"')
            lines = "\n".join(lines)
            # put preprocessed file in cache
            out = C_BUILD_CACHE_DIR / mod.import_path
            if args.verbose:
                print(f"writing include {out / file.name}")
            out.mkdir(exist_ok=True, parents=True)
            (out / file.name).write_text(lines)

    def _preprocess(self):
        """_preprocess preprocesses self so it can be built."""
        Module._preprocess_dir(self)

        for submod in self.submodules:
            Module._preprocess_dir(self.get_submodule(submod))

    @staticmethod
    def from_directory(path: pathlib.Path) -> "Module":
        """Returns a module given a directory with a c.mod file."""
        # parse c.mod file
        # result = Module()
        cmod_file = path / "c.mod"
        if not cmod_file.exists():
            path = path.parent
            cmod_file = path / "c.mod"

        assert cmod_file.exists(), (
            "Module.from_directory must only be called with a directory "
            + "containing a c.mod file or its subdirectory"
        )

        import_path = None
        name = ""
        std = None
        platform_flags = {}
        with cmod_file.open(encoding="utf-8") as cmod:
            for line in cmod.readlines():
                line = line.split()
                if len(line) == 0:
                    continue
                if len(line) < 2:
                    error("c.mod: syntax error")
                if line[0] == "module":
                    if len(line) != 2:
                        error("c.mod: syntax error")
                    if import_path is not None:
                        error("c.mod: only one module directive may be specified")
                    import_path = line[1].strip()
                    name = import_path.split("/")[-1]
                    continue
                elif line[0] == "version":
                    if len(line) != 2:
                        error("c.mod: syntax error")
                    if std is not None:
                        error("c.mod: only one version directive may be specified")
                    std = line[1].strip()
                    continue
                elif line[0] == "os":
                    deps = []
                    os = platform.system().lower()
                    if os == "darwin":
                        os = "macos"
                    if os == line[1]:
                        deps += line[2:]

                    platform_flags[os] = deps
                    continue

                error(f"c.mod: unrecognized directive: {line[0]}")

        if import_path in MOD_CACHE:
            return MOD_CACHE[import_path]

        has_exe = (path / "main.c").exists()
        has_lib = (path / "lib.c").exists() or (path / (name + ".c")).exists()
        if (path / "lib.c").exists():
            has_lib = True
        if (path / "lib.c").exists() and (path / (name + ".c")).exists():
            error(
                f"{import_path}: a module can only have 1 library main "
                + "file: either lib.c or a file of the same name as the module"
            )

        submodules = []
        for child in path.glob("*"):
            if not child.is_dir():
                continue

            if not (
                (child / "main.c").exists()
                or (child / "lib.c").exists()
                or (child / (child.name + ".c")).exists()
            ):
                continue

            if (child / "lib.c").exists() and (child / (child.name + ".c")).exists():
                error(
                    f"{import_path}: a module can only have 1 "
                    + "library main file: either lib.c or a file of the same "
                    + "name as the module"
                )

            submodules.append(child.name)
        result = Module(
            std=std,
            has_executable=has_exe,
            has_lib=has_lib,
            path=path,
            import_path=import_path,
            submodules=submodules,
            platform_flags=platform_flags,
        )

        result._preprocess()
        MOD_CACHE[import_path] = result
        return result


def build(mod: Module, type: Literal["exe", "lib"]):
    if args.verbose:
        print(f'building module "{mod.import_path}" as {type}')

    out = C_BUILD_CACHE_DIR / mod.import_path
    if mod.is_submodule():
        out = C_BUILD_CACHE_DIR / mod.parent.import_path / mod.module_name()

    out.mkdir(parents=True, exist_ok=True)

    if type == "exe":
        outname = mod.module_name()
    else:
        outname = "lib.o"

    if type == "exe":
        libfile = "main.c"
    elif (mod.path / "lib.c").exists():
        libfile = "lib.c"
    else:
        libfile = mod.module_name() + ".c"

    # TODO: This doesn't work since we can specify different platform_flags for
    #       each library!
    headers = []
    sources = []
    flags = []
    previously_checked = set()
    for dep in mod.dependencies:
        if dep.import_path in previously_checked:
            continue
        previously_checked.add(dep.import_path)
        headers.append(f"--include={dep.h()}")
        sources.append(dep.lib_c())
        flags.extend(dep.platform_flags.get(get_os(), []))

    # assemble compile command
    argv = [
        C_CLANG_COMMAND,
        "-o",
        str((out / outname).resolve()),
        f"-std={mod.std}",
        f"--include={mod.h()}",
        # TODO: remove if release flag is present.
        "-fsanitize=address,undefined",
        "-g3",
    ]
    if type == "lib":
        argv.append("-c")
    moddir = C_BUILD_CACHE_DIR / mod.import_path
    argv.append(str((moddir / libfile).resolve()))
    argv.extend(headers)
    argv.extend(sources)

    # remove duplicates
    argv = list(dict.fromkeys(argv))

    if args.verbose:
        print(f"building module {mod.import_path} with -std={mod.std} as {type}")
        print(" ".join(map(str, argv)))
    clang = subprocess.run(argv)
    if clang.returncode != 0:
        exit(clang.returncode)

    if type == "exe":
        shutil.copy(out / outname, mod.path)


def main():
    cmod = pathlib.Path("c.mod")
    if not cmod.exists():
        error("current folder is not a module")

    root = Module.from_directory(pathlib.Path("."))
    if len(args.modules) != 0:
        for module in args.modules:
            if module == ".":
                if not root.has_executable:
                    error(f"no executable to build for module '{module}'")
                build(root, "exe")
                return

            if module not in root.submodules:
                error(f"no submodule named '{module}' exists")

            mod = root.get_submodule(module)
            if not mod.has_executable:
                error(f"no executable to build for module '{module}'")
            build(root.get_submodule(module), "exe")
        return

    if not root.has_executable:
        error(f"no executable to build for module '{module}'")
    build(root, "exe")


if __name__ == "__main__":
    argparser = argparse.ArgumentParser(prog="cbuild", description="builds C code.")
    argparser.add_argument(
        "modules",
        metavar="MODULE",
        type=str,
        nargs="*",
        help="the module or modules to build. Tries to build the module in the "
        + "current directory if no arguments are given.",
    )
    argparser.add_argument(
        "-v", "--verbose", action="store_true", help="enables verbose output."
    )
    args = argparser.parse_args()
    main()
