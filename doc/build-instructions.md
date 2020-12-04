## Build instructions

**TL;DR** *git clone as usual, don't forget submodules, use cmake as usual. See `build-*` scripts for reference.*

### Downloading source

To clone the repository using git, run:

```
git clone --recurse-submodules https://github.com/eliemichel/GrainViewer.git
```

If you forgot to use the `--recurse-submodules`, the build scripts will do it for you anyways.

**NB** This repository contains submodules, so **do not use** the *download zip* button provided by GitHub. If you want a zip, download the source code from [last release](https://github.com/eliemichel/GrainViewer/releases/latest).

### Dependencies

This project uses [cmake](https://cmake.org/) for build configuration. You also need [Python](https://www.python.org/) (for glad generator) and all other dependencies are included as submodules.

On linux, you can install the required dependencies with the following `apt` command line, or something similar on other packet managers:

```
sudo apt install python git cmake build-essential xorg-dev
```

You need a C++17 capable combiler. The build has been tested with Visual Studio 15 (2017), Visual Studio 16 (2019), MinGW and gcc.

### Building

This is a standard CMake project. Building it consits in running:

```
mkdir build
cd build
cmake ..
cmake --build .
```

You can chose which compiler to use in the call to `cmake`, using the `-G` option. See for instance [`build-msvc16.bat`](build-msvc16.bat) to build for Visual Studio 16 (2019), or the other build files for `mingw` or `gcc`. More easily, you can also just run one of those scripts, it will even get the git submodules.

You may set the following additional option using `-DOPTION=value`:

 - `DEV_MOD` Turn this off when building binaries meant to be distributed
 - `GIT_SUBMODULE` Turn this off if you don't want cmake to check out submodules (if you are already including this project as a submodule for instance)
 - `DOWNLOAD_EXAMPLE_DATA` Turn this off to prevent cmake from automatically downloading example data (roughly 800MB).

Once CMake has run, you can build the project. You can do it in command line, from the `build` directory:

```
cmake --build .
```

Or you can use the build process of your compiler, for instance running `make` (for `gcc` projects) or openning the Visual Studio solution.

### Running

The executable file is created within the `build` directory, in `src/GrainViewer` or `src/GrainViewer/Debug` or `src/GrainViewer/Release` or something similar depending on your compiler.

