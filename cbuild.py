#!/usr/bin/env python3
import urllib.request
import os
import sys
import pathlib
import argparse
import subprocess
import shutil
import platform
from typing import Literal

# A python implementation of the `cbuild` command. Used for bootstrapping this
# repository, but also kept up to date with the C implementation.

# TODO: make the C implementation.
# TODO: actually make sure things need to be rebuilt

DEFAULT_CACHE_DIR = ".cache"
C_CACHE_DIR: pathlib.Path = (
    pathlib.Path(os.environ.get("XDG_CACHE_HOME", DEFAULT_CACHE_DIR)) / "c"
)
C_CLANG_COMMAND: pathlib.Path = os.environ.get("CLANG_COMMAND", "clang")
if C_CACHE_DIR.exists():
    shutil.rmtree(C_CACHE_DIR)
C_CACHE_DIR.mkdir(parents=True, exist_ok=True)


def todo(thing: str):
    print(f"not implemented: {thing}")
    exit(1)


def error(msg: str):
    print(f"error: {msg}", file=sys.stderr)
    exit(1)


def gen_module_h(modpath: pathlib.Path, modname: str, submodule: str) -> pathlib.Path:
    implicit = C_CACHE_DIR / (modname + "/" + submodule) / "__module.h"
    implicit_include = ""
    for header in (modpath / submodule).glob("*.h"):
        if header == implicit:
            continue
        implicit_include += f'#include "{header.resolve()}"\n'
    implicit_include += "\n"

    # put file in cache
    implicit.write_text(implicit_include)

    return implicit


def download_module(mod: str, args: argparse.Namespace):
    if (C_CACHE_DIR / mod).exists():
        currentmod = mod
        path = C_CACHE_DIR / mod
        while True:
            if (path / "c.mod").exists():
                break

            currentmod = "/".join(currentmod.rsplit("/", maxsplit=1)[:-1])
            path = path.parent
        cmod = C_CACHE_DIR / currentmod / "c.mod"
        if not cmod.exists():
            error("downloaded repository is not a cbuild project (no c.mod file found)")
        _, std, deps = parse_cmod(cmod)

        return mod, mod[len(currentmod) + 1 :], std, deps

    print(f"downloading module {mod}...")
    currentmod = mod

    git = None
    if args.verbose:
        print(str(C_CACHE_DIR / currentmod))
    (C_CACHE_DIR / currentmod).mkdir(parents=True)
    while True:
        url = "https://" + currentmod
        try:
            with urllib.request.urlopen("https://" + currentmod, timeout=1) as resp:
                url = resp.url
        except Exception as _:
            modsplit = currentmod.rsplit("/", maxsplit=1)
            if len(modsplit) == 1:
                print(git.stderr.decode("utf-8"), file=sys.stderr)
                error(f"no module with name {mod}")

            currentmod = "/".join(modsplit[:-1])
            continue
        argv = [
            "git",
            "-C",
            str((C_CACHE_DIR / currentmod).resolve()),
            "init",
        ]
        if args.verbose:
            print(" ".join(map(str, argv)))
        git = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert git.returncode == 0

        argv = [
            "git",
            "-C",
            str((C_CACHE_DIR / currentmod).resolve()),
            "remote",
            "add",
            "origin",
            url,
        ]
        if args.verbose:
            print(" ".join(map(str, argv)))
        git = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert git.returncode == 0

        argv = [
            "git",
            "-C",
            str((C_CACHE_DIR / currentmod).resolve()),
            "fetch",
            "--depth=1",
        ]
        if args.verbose:
            print(" ".join(map(str, argv)))
        git = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if git.returncode != 0:
            continue

        argv = [
            "git",
            "-C",
            str((C_CACHE_DIR / currentmod).resolve()),
            "checkout",
            "main",
        ]
        if args.verbose:
            print(" ".join(map(str, argv)))
        git = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if git.returncode == 0:
            break

        modsplit = currentmod.rsplit("/", maxsplit=1)
        if len(modsplit) == 1:
            print(git.stderr.decode("utf-8"), file=sys.stderr)
            error(f"no module with name {mod}")

        currentmod = "/".join(modsplit[:-1])

    cmod = C_CACHE_DIR / currentmod / "c.mod"
    if not cmod.exists():
        error("downloaded repository is not a cbuild project (no c.mod file found)")
    _, std, deps = parse_cmod(cmod)

    return mod, mod[len(currentmod) + 1 :], std, deps


def fix_includes(
    modpath: pathlib.Path,
    modname: str,
    submodule: str,
    moddir: pathlib.Path,
    std: str,
    args: argparse.Namespace,
    mod_deps: list[str],
) -> list[str]:
    if args.verbose:
        print(
            f"fix_includes({modpath}, {modname}, {submodule}, {moddir}, {std}, {args}, {mod_deps})"
        )
    deps = []
    for file in (modpath / submodule).glob("*.c"):
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

            # its not
            include = line[1].strip('"')
            if include.startswith("./") or include.startswith("../"):
                continue

            if include.startswith(str(C_CACHE_DIR.resolve())):
                continue

            incname = ""
            if not include.startswith(modname):
                submodname, subname, std, mod_deps = download_module(include, args)
                path = C_CACHE_DIR / submodname
                lib, h = build(path, submodname, "", std, "lib", args, mod_deps)
                deps.extend([lib, h])
                incname = subname
                lines[i] = f'#include "{(path  /"__module.h").resolve()}"'
                continue

            # is it a submodule?
            incname = include[len(modname) + 1 :]
            # TODO: check if this is an infinite loop. We don't allow
            # including ourself.
            if modname + "/" + incname == modname + "/" + submodule:
                error(
                    f"file {file.resolve()} includes "
                    + f'"{modname + "/" + incname}", which causes a circular '
                    + "dependency"
                )

            if (
                not (C_CACHE_DIR / modname / incname / "lib.o").exists()
                and pathlib.Path(incname).exists()
            ):
                # print(f"adding dependency {modname}/{incname}")
                build(modpath, modname, incname, std, "lib", args, mod_deps)
            lootdir = C_CACHE_DIR / modname / incname
            deps.extend(
                [
                    str((lootdir / "lib.o").resolve()),
                    f'--include={(lootdir / "__module.h").resolve()}',
                ]
            )
            lines[
                i
            ] = f'#include "{(C_CACHE_DIR / modname / incname /"__module.h").resolve()}"'

            # lines[i] = f'#include "{(moddir / incname /"__module.h").resolve()}"'
            # deps += [str((moddir / incname / "lib.o").resolve())]

        # if the file is not local to the project, it will be in the cache
        # directory, and we should refer to it by its module path, not file
        # path.
        name = str(pathlib.Path(modname) / submodule / file.relative_to(modpath))
        lines.insert(0, f'#line 1 "{name}"')
        lines = "\n".join(lines)
        # put preprocessed file in cache
        if args.verbose:
            print(f"writing include {C_CACHE_DIR / modname / submodule / file.name}")
        (C_CACHE_DIR / modname / submodule / file.name).write_text(lines)

    # print(deps)
    return deps


def build(
    modpath: pathlib.Path,
    modname: str,
    submodule: str,
    std: str,
    type: Literal["exe", "lib"],
    args: argparse.Namespace,
    mod_deps: list[str],
) -> tuple[list[str], list[str]]:
    if args.verbose:
        print(
            f"build({modpath}, {modname}, {submodule}, {std}, {type}, {args}, {mod_deps})"
        )
    moddir = C_CACHE_DIR / modname / submodule
    moddir.mkdir(parents=True, exist_ok=True)

    outname = "lib.o"
    if type == "exe":
        outname = "main"
        if submodule != "":
            outname = submodule
        else:
            if "/" in modname:
                outname = modname.rsplit("/", maxsplit=1)[-1]

    # generate __module.h
    module_h = gen_module_h(modpath, modname, submodule)

    # fix includes
    deps = fix_includes(modpath, modname, submodule, moddir, std, args, mod_deps)

    if submodule != "" and (
        (moddir / "lib.c").exists() and (moddir / (moddir.name + ".c")).exists()
    ):
        error(
            f"{modname}/{submodule}: "
            + "a module can only have 1 library main file: either lib.c or a "
            + "file of the same name as the module"
        )

    libfile = ""
    if (moddir / "lib.c").exists():
        libfile = "lib.c"
    else:
        libfile = moddir.name + ".c"

    # run clang
    argv = [C_CLANG_COMMAND]
    if type == "exe":
        argv.extend(
            [
                "-o",
                str((moddir / outname).resolve()),
                f"-std={std}",
                f"--include={module_h.resolve()}",
                # TODO: gatekeep behind debug flag or release flag
                "-fsanitize=address,undefined",
                "-g3",
                # f"-I{C_CACHE_DIR}",
                # "-I.",
                str((moddir / "main.c").resolve()),
            ]
        )
        argv.extend(deps)
        argv.extend(mod_deps)
    else:
        argv.extend(
            [
                "-o",
                str((moddir / "lib.o").resolve()),
                f"-std={std}",
                f"--include={module_h.resolve()}",
                # TODO: gatekeep behind debug flag or release flag
                "-fsanitize=address,undefined",
                "-g3",
                # f"-I{C_CACHE_DIR}",
                # "-I.",
                "-xc",
                "-c",
                str((moddir / libfile).resolve()),
            ]
        )
        argv.extend(mod_deps)

    if args.verbose:
        print(f"building module {modname}/{submodule} with -std={std} as {type}")

    if args.verbose:
        print(" ".join(argv))
    clang = subprocess.run(argv)
    if clang.returncode != 0:
        exit(clang.returncode)

    if type == "exe":
        shutil.copy(moddir / outname, pathlib.Path(submodule))

    return (moddir / "lib.o").resolve(), f"--include={module_h.resolve()}"


def parse_cmod(path: pathlib.Path) -> tuple[str, str, list[str]]:
    # parse c.mod file
    deps = []
    modname = None
    std = None
    with path.open(encoding="utf-8") as cmod:
        for line in cmod.readlines():
            line = line.split()
            if len(line) == 0:
                continue
            if len(line) < 2:
                error("c.mod: syntax error")
            if line[0] == "module":
                if len(line) != 2:
                    error("c.mod: syntax error")
                if modname is not None:
                    error("c.mod: only one module directive may be specified")
                modname = line[1].strip()
                continue
            elif line[0] == "version":
                if len(line) != 2:
                    error("c.mod: syntax error")
                if std is not None:
                    error("c.mod: only one version directive may be specified")
                std = line[1].strip()
                continue
            elif line[0] == "os":
                os = platform.system().lower()
                if os == "darwin":
                    os = "macos"
                if os == line[1]:
                    deps += line[2:]
                continue

            error(f"c.mod: unrecognized directive: {line[0]}")

    return modname, std, deps


def main(args: argparse.Namespace):
    cmod = pathlib.Path("c.mod")
    if not cmod.exists():
        error("current folder is not a module")

    modname, std, flags = parse_cmod(cmod)

    if len(args.modules) != 0:
        for module in args.modules:
            path = pathlib.Path(module)
            if (path / "lib.c").exists() or (path / (module + ".c")).exists():
                build(pathlib.Path("."), modname, module, std, "lib", args, flags)
            elif (path / "main.c").exists():
                build(pathlib.Path("."), modname, module, std, "exe", args, flags)
            else:
                error(f"submodule '{module} has neither a main.c file or lib.c file'")
        return

    if pathlib.Path("lib.c").exists():
        build(pathlib.Path("."), modname, "", std, "lib", args, flags)
    if pathlib.Path("main.c").exists():
        build(pathlib.Path("."), modname, "", std, "exe", args, flags)


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
    main(args)
