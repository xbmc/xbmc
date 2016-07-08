# Kodi CMake based buildsystem

This files describes Kodi's CMake based buildsystem. CMake is a cross-platform
tool for generating makefiles as well as project files used by IDEs.

The current version of the buildsystem is capable of building the main Kodi
executable (but no packaging or dependency management yet) for the following
platforms:

- Linux (GNU Makefiles, Ninja)
- Windows (NMake Makefiles, Visual Studio 14 (2015), Ninja)
- OSX and IOS (GNU Makefiles, Xcode, Ninja)
- Android (GNU Makefiles)
- FreeBSD (GNU Makefiles)

Before building Kodi with CMake, please ensure that you have the platform
specific dependencies installed.

While the legacy build systems typically used in-source builds it's recommended
to use out-of-source builds with CMake. The necessary runtime dependencies such
as dlls, skins and configuration files are copied over to the build directory
automatically.

## Dependency installation

### Linux

The dependencies required to build on Linux can be found in
[docs/README.xxx](https://github.com/xbmc/xbmc/tree/master/docs).

### Raspberry Pi

The cross compilation environment for the Raspberry Pi as well as the
dependencies have to be installed as explained in
[docs/README.raspberrypi](https://github.com/xbmc/xbmc/tree/master/docs/README.raspberrypi).

### Windows

For Windows the dependencies can be found in the
[Wiki](http://kodi.wiki/view/HOW-TO:Compile_Kodi_for_Windows) (Step 1-4). If not already available on your pc, you should
install the [Windows Software Development Kit (SDK)](https://dev.windows.com/en-us/downloads/sdk-archive) for your Windows version. This is required for HLSL shader offline compiling with the [Effect-Compiler Tool](https://msdn.microsoft.com/de-de/library/windows/desktop/bb232919(v=vs.85).aspx) (fxc.exe).

On Windows, the CMake based buildsystem requires that the binary dependencies
are downloaded using `DownloadBuildDeps.bat` and `DownloadMingwBuildEnv.bat`
and that the mingw libs (ffmpeg, libdvd and others) are built using
`make-mingwlibs.bat`.

### OSX

For OSX the required dependencies can be found in
[docs/README.osx](https://github.com/xbmc/xbmc/tree/master/docs/README.osx).

On OSX it is necessary to build the dependencies in `tools/depends` using
`./bootstrap && ./configure --host=<PLATFORM> && make`. The other steps such
as `make -C tools/depends/target/xbmc` and `make xcode_depends` are not needed
as these steps are covered already by the CMake project.

### Android

The dependencies needed to compile for Android can be found in
[docs/README.android](https://github.com/xbmc/xbmc/tree/master/docs/README.android)
. All described steps have to be executed (except 5.2 which is replaced by the
respective CMake command below).

## Building Kodi

This section lists the necessary commands for building Kodi with CMake.
CMake supports different generators that can be classified into two categories:
single- and multiconfiguration generators.

A single configuration generator (GNU/NMake Makefiles) generates project files
for a single build type (e.g. Debug, Release) specified at configure time.
Multi configuration generators (Visual Studio, Xcode) allow to specify the
build type at compile time.

All examples below are for out-of-source builds with Kodi checked out to
`<KODI_SRC>`:

```
mkdir kodi-build && cd kodi-build
```

### Linux with GNU Makefiles

```
cmake <KODI_SRC>/project/cmake/
cmake --build . -- VERBOSE=1 -j$(nproc)  # or: make VERBOSE=1 -j$(nproc)
./kodi.bin
```

`CMAKE_BUILD_TYPE` defaults to `Release`.

#### Debian package generation
The buildsystem is capable of generating Debian packages using CPack. To generate them, `CPACK_GENERATOR` has to be set to *DEB*, i.e. executing CMake's configure step with `-DCPACK_GENERATOR=DEB`.
You should use CMake/CPack 3.6.0 or higher. Lower versions can generate the packages but package names will be mangled.

The following optional variables (which can be passed to buildsystem when executing cmake with the -D`<variable-name>=<value>` format) can be used to manipulate package type, name and version:

- `DEBIAN_PACKAGE_TYPE` controls the name and version of generated packages. Accepted values are `stable`, `unstable` and `nightly` (default is `nightly`).
- `DEBIAN_PACKAGE_EPOCH` controls package epoch (default is `2`)
- `DEBIAN_PACKAGE_VERSION` controls package version (default is `0`)
- `DEBIAN_PACKAGE_REVISION` controls package revision (no default is set)

Packages metadata can be changed simply by editing files present in the `cpack/deb` folder
A lot more variables are available (see cpack/CPackDebian.cmake file) but you shouldn't mess with them unless you know what you're doing.

Generated packages can be found in <BUILD_DIR>/packages.

### Raspberry Pi with GNU Makefiles

```
cmake -DCMAKE_TOOLCHAIN_FILE=<KODI_SRC>/tools/depends/target/Toolchain.cmake <KODI_SRC>/project/cmake/
cmake --build . -- VERBOSE=1 -j$(nproc)  # or: make VERBOSE=1 -j$(nproc)
```

### Windows with NMake Makefiles

```
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release <KODI_SRC>/project/cmake/
cmake --build .  # or: nmake
kodi.exe
```

### Windows with Visual Studio project files

```
cmake -G "Visual Studio 14" <KODI_SRC>/project/cmake/
cmake --build . --config "Debug"  # or: Build solution with Visual Studio
set KODI_HOME="%CD%" && Debug\kodi.exe
```

### OSX with GNU Makefiles

```
cmake -DCMAKE_TOOLCHAIN_FILE=<KODI_SRC>/tools/depends/target/Toolchain.cmake <KODI_SRC>/project/cmake/
cmake --build . -- VERBOSE=1 -j$(sysctl -n hw.ncpu)  # or: make VERBOSE=1 -j$(sysctl -n hw.ncpu)
./kodi.bin
```

### OSX with Xcode project files

```
cmake -DCMAKE_TOOLCHAIN_FILE=<KODI_SRC>/tools/depends/target/Toolchain.cmake -G "Xcode" <KODI_SRC>/project/cmake/
cmake --build . --config "Release" -- -verbose -jobs $(sysctl -n hw.ncpu)  # or: Build solution with Xcode
KODI_HOME=$(pwd) ./Release/kodi.bin
```

### Android with GNU Makefiles

```
cmake -DCMAKE_TOOLCHAIN_FILE=<KODI_SRC>/tools/depends/target/Toolchain.cmake <KODI_SRC>/project/cmake/
cmake --build . -- VERBOSE=1 -j$(nproc)  # or: make VERBOSE=1 -j$(nproc)
```

## Tests

Kodi uses Google Test as its testing framework. Each test file is scanned for tests and these
are added to CTest, which is the native test driver for CMake.

This scanning happens at configuration time. If tests depend on generated support files which
should not be scanned, then those support files should be added to the SUPPORT_SOURCES
variable as opposed to SOURCES before calling core_add_test. You might want to do this where
the generated support files would not exist at configure time, or if they are so large that
scanning them would take up an unreasonable amount of configure time.

## Extra targets

When using the makefile builds a few extra targets are defined:

- `make check` builds and executes the test suite.

Code coverage (with Gcov, LCOV and Gcovr) can be built on Linux:

- CMake has to be executed with `-DCMAKE_BUILD_TYPE=Coverage`
- `make coverage` generates an HTML code coverage report.
- `make coverage_xml` generates an XML code coverage report.

## Sanitizers

Clang and GCC support different kinds of Sanitizers. To enable a Sanitizer call CMake with the
option `-DECM_ENABLE_SANITIZERS=’san1;san2;...'`. For more information about enabling the
Sanitizers read the documentation in 
[modules/extra/ECMEnableSanitizers.cmake](https://github.com/xbmc/xbmc/tree/master/project/cmake/modules/extra/ECMEnableSanitizers.cmake).

It is also recommended to read the sections about the Sanitizers in the [Clang 
documentation](http://clang.llvm.org/docs/).

## Debugging the build

This section covers some tips that can be useful for debugging a CMake
based build.

### Verbosity (show compiler and linker parameters)

In order to see the exact compiler commands `make` and `nmake` can be
executed with a `VERBOSE=1` parameter.

On Windows, this is unfortunately not enough because `nmake` uses
temporary files to workaround `nmake`'s command string length limitations.
In order to see verbose output the file
[Modules/Platform/Windows.cmake](https://github.com/Kitware/CMake/blob/master/Modules/Platform/Windows.cmake#L40)
in the local CMake installation has to be adapted by uncommenting these
lines:

```
# uncomment these out to debug nmake and borland makefiles
#set(CMAKE_START_TEMP_FILE "")
#set(CMAKE_END_TEMP_FILE "")
#set(CMAKE_VERBOSE_MAKEFILE 1)
```
