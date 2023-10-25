Like the `go` tool for Go, this is intended to be a simple tool for C which
allows for managing C codebases.

Planned commands:

- `build`: build C projects which abide by a specific structure.

  See `doc/cbuild.md` for details.
  
- `fmt`: formats C code automatically. Whatever style I end up defining this to
  format will essentially be the "canonical" format for code using this tool.

- `doc`: generates HTML documentation from C code and (optionally) opens the
  web browser to the output folder.

- `run`: compiles and runs the specified program.

- `version`: prints the version of the `c` tool and compiler being used.

- `test`: Compiles and runs tests, but I don't know exactly how we'll do that
  for now.

  See [my encoding project](https://github.com/aeldidi/encoding) for an idea of
  what I'm thinking of.

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

License
-------

Public Domain or 0-Clause BSD. The 0BSD license is included in the `LICENSE`
file in the repository's root.
