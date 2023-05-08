xsys4dbg
========

Graphical debugger font-end for [xsystem4](https://github.com/nunuhara/xsystem4).

This is a work in progress.

Building
--------

First install the dependencies (corresponding Debian package in parentheses):

* bison (bison)
* flex (flex)
* meson (meson)
* libpng (libpng-dev)
* libturbojpeg (libturbojpeg0-dev)
* libwebp (libwebp-dev)
* zlib (zlib1g-dev)
* qt5 (qtbase5-dev)

Then fetch the git submodules,

    git submodule init
    git submodule update

(Alternatively, pass `--recurse-submodules` when cloning this repository)

Then build the program with meson,

    mkdir build
    meson build
    ninja -C build

Installation
------------

### From Source

If you've followed the above instructions to build xsys4dbg from source, run

    ninja -C build install

to install it.

### Windows

Windows builds are not yet available.

Usage
-----

### Open a game for debugging

* Open the *File* menu
* Select the *Open...* menu item
* Choose the game directory

### Navigate to a specific function

* Select the combo box in the toolbar
* Begin typing the function name (it should auto-complete)
* Hit enter once the full function name is shown in the combo box

### Set a breakpoint

* Hover your mouse over the instruction address at the left side of the main bytecode viewer
* Right click the address
* Select *Toggle Breakpoint*

### Begin debugging

* Debugger execution commands are available in the toolbar and *Debug* menu
  * *Run* begins or continues execution of the game
  * *Pause* temporarily halts execution of the game
  * *Stop* exits the game
  * *Next* steps to the next instruction within the current function
  * *Step In* steps to the next instruction to be executed
  * *Step Out* steps out of the current function

### Inspecting variables

Currently only local and member variables in active stack frames can be viewed. They are
available through the panel on the right-hand side of the main bytecode viewer.

Planned Features
----------------

- [x] Basic debugging (breakpoints, stepping, viewing locals)
- [ ] Heap viewer (view all allocated objects)
- [ ] Setting value of variables
- [ ] Advanced breakpoints (conditional breakpoints, logging)
- [ ] Scene viewer (SACT2, etc.)
- [ ] Parts viewer (PartsEngine, etc.)
