The `cbuild` Command
====================

Any folder with a `c.mod` file is a project.

A project has one or more modules which can each be a library, an executable,
or both.

Modules
-------

All modules must have either a `lib.c` file, a `main.c` file, or both within
its folder. If it has a `lib.c` file, it can be built as a library by compiling
this `lib.c` file with the appropriate headers. If it has a `main.c` file, it
can be built as an executable by building this `main.c` file with the
appropriate headers.

The root folder of a project is a module, and any folder contained in the root
may also be a module, called a submodule if its name is not one of the
following:

- `doc`
- `tests`
- `tools`
- Any filename which starts with a dot (`.`)

TODO: think of more

Submodules may not nest inside each other, and may not `#include` themselves.

Imports
-------

Includes using quotation marks have been reworked to remove compelxity involved
in typical build systems. Rather, a set of conventions are enforced to ensure
things are predictable.

Namely,

- All includes which want the normal behaviour must be explicitly relative.
  That is, they must begin with `./` or `../`.

- Any includes which don't are converted to imports, which try to derive a set
  of header files in the following way:

  1. If the include begins with the module name specified in `c.mod`, the
     include is resolved relative to the project root.

  2. Otherwise, it is considered to be a remote import, and the include is
     resolved as a web address hosting the project to import.

  Eventually, this will be configurable so that you can specify exactly where
  this is fetched from in `c.mod`, but for now its simply a remote import. For
  example,

  ```c
  #include "c.eldidi.org/x/fs"
  ```

  will cause my library to be downloaded from the git repository located at the
  address `https://c.eldidi.org/x`, the `fs` submodule to be compiled, and
  its header files to be included at the point of the preprocessor directive.
  
  Eventually, I would like the format these are stored to be set in stone,
  either a git repo accessible over HTTP/HTTPS or a ZIP file or something
  stored on a server.

  I think git repo over HTTP/HTTPS is the way, since this allows importing
  directly from github.

- All headers within a module are automatically included when compiling `lib.c`
  and `main.c`.

- It is an error to `#include` a module from within a module's `.c` files.

This way, `#include` directives are a declaration of dependency rather than a
general purpose textual inclusion mechanism (although this ability still exists
with relative includes).
