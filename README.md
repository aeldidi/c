Like the `go` tool for Go, this is intended to be a simple tool for C which
allows for managing C codebases.

Planned commands:

- `build`: build C projects which abide by the following structure:
  
  1. All code is in the `src/` subdirectory, and all non-system includes are
     relative to `src/`.

     System includes are ones enclosed in angle brackets, and are relative to
     some system-defined search directory.

     All `#include`s using quotation marks are relative to `src/`.

     There will be a way to define which flags should be passed to the compiler
     for different system headers if necessary (`-lm` for math, etc).

  2. All subdirectories of `src/` are libraries, except for those in `src/cmd`,
     which are binaries.

  3.`#include`s are scanned in all `.c` and `.h` files to determine
    dependencies between libraries and binaries.

- `fmt`: formats C code automatically. Whatever style I end up defining this to
  format will essentially be the "canonical" format for code using this tool.

- `doc`: generates HTML documentation from C code and (optionally) opens the
  web browser to the output folder.

- `clean`: removes all artifacts left behind by these commands.

- `run`: compiles and runs the specified program.

- `version`: prints the version of the `c` tool and compiler being used.

- `test`: Compiles and runs tests, but I don't know exactly how we'll do that
  for now.

- `tool`: Compiles and runs a tool from the `tools/` subdirectory of the
  project.

  In the `tools/` subdirectory, each `.c` file or directory will be built as a
  binary (following the same rules as `src/cmd`), and their name will be the
  name of the tool which can be run with `c tool X` assuming `X` is the tool's
  name.

- `compile`: Runs the C compiler which would be used if you ran `c build`.
  
  For example, if I'm using `clang` as my compiler, `c compile --version`
  should print something like
  
  ```
  Ubuntu clang version 14.0.0-1ubuntu1.1
  Target: x86_64-pc-linux-gnu
  Thread model: posix
  InstalledDir: /usr/bin
  ```

  On my home computer.

All this will eventually mean we write a C parser, but for now I'll use basic
string searching to find includes and implement `build` (which means it won't
work on all C codebases, including things with conditional includes).
