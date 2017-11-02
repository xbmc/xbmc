# Kodi CMake based buildsystem

This files describes Kodi's CMake based buildsystem. CMake is a cross-platform
tool for generating makefiles as well as project files used by IDEs.

The current version of the buildsystem is capable of building and packaging
Kodi for the following platforms:

- Linux (GNU Makefiles, Ninja)
- Windows (NMake Makefiles, Visual Studio 14 (2015), Ninja)
- macOS and iOS (GNU Makefiles, Xcode, Ninja)
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
are downloaded using `download-dependencies.bat` and `download-msys2.bat`
and that the mingw libs (ffmpeg, libdvd and others) are built using
`make-mingwlibs.bat`.

### macOS

For macOS the required dependencies can be found in
[docs/README.osx.md](https://github.com/xbmc/xbmc/tree/master/docs/README.osx.md).

On macOS it is necessary to build the dependencies in `tools/depends` using
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
cmake <KODI_SRC>
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
cmake -DCMAKE_TOOLCHAIN_FILE=<KODI_SRC>/tools/depends/target/Toolchain.cmake <KODI_SRC>
cmake --build . -- VERBOSE=1 -j$(nproc)  # or: make VERBOSE=1 -j$(nproc)
```

### Windows with Visual Studio project files
#### Build for win32
```
cmake -G "Visual Studio 14" <KODI_SRC>
cmake --build . --config "Debug"  # or: Build solution with Visual Studio
Debug\kodi.exe
```

Building on a x64 cpu can be improved, if you're on a cmake version > 3.8:
```
cmake -G "Visual Studio 14" -T host=x64 <KODI_SRC>
cmake --build . --config "Debug"  # or: Build solution with Visual Studio
Debug\kodi.exe
```
This will choose the x64 toolset, as windows uses the x32 toolset by default.

#### Build for x64
```
cmake -G "Visual Studio 14 Win64" <KODI_SRC>
cmake --build . --config "Debug"  # or: Build solution with Visual Studio
Debug\kodi.exe
```

Building on a x64 cpu can be improved, if you're on a cmake version > 3.8:
```
cmake -G "Visual Studio 14 Win64" -T host=x64 <KODI_SRC>
cmake --build . --config "Debug"  # or: Build solution with Visual Studio
Debug\kodi.exe
```
This will choose the x64 toolset, as windows uses the x32 toolset by default.

You can always check ``cmake --help` to see which generators are available and how to call those.

#### Windows installer generation

The script [project/Win32BuildSetup](https://github.com/xbmc/xbmc/blob/master/tools/buildsteps/windows/win32/BuildSetup.bat) or [project/Win64BuildSetup](https://github.com/xbmc/xbmc/blob/master/tools/buildsteps/windows/x64/BuildSetup.bat)
builds an installable package for Windows. Choose either 32bit or 64bit, depending on what your trying to build.

### Windows with NMake Makefiles

```
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release <KODI_SRC>
cmake --build .  # or: nmake
kodi.exe
```

### macOS with GNU Makefiles

```
cmake -DCMAKE_TOOLCHAIN_FILE=<KODI_SRC>/tools/depends/target/Toolchain.cmake <KODI_SRC>
cmake --build . -- VERBOSE=1 -j$(sysctl -n hw.ncpu)  # or: make VERBOSE=1 -j$(sysctl -n hw.ncpu)
./kodi.bin
```

### macOS with Xcode project files

```
cmake -DCMAKE_TOOLCHAIN_FILE=<KODI_SRC>/tools/depends/target/Toolchain.cmake -G "Xcode" <KODI_SRC>
cmake --build . --config "Release" -- -verbose -jobs $(sysctl -n hw.ncpu)  # or: Build solution with Xcode
./Release/kodi.bin
```

#### macOS installer generation

Afterwards an installable DMG for macOS can be built with the following command:

```
cmake --build . --config "Release" --target "dmg"  # or: make dmg
```

#### iOS package generation

Consequently an installable DEB for iOS can be built with the following command:

```
make deb
```

### Android with GNU Makefiles

```
cmake -DCMAKE_TOOLCHAIN_FILE=<KODI_SRC>/tools/depends/target/Toolchain.cmake <KODI_SRC>
cmake --build . -- VERBOSE=1 -j$(nproc)  # or: make VERBOSE=1 -j$(nproc)
```

#### Android package generation

An installable APK for Android can be built with the following command:

```
make apk
```

## Options

Kodi supports a number of build options that can enable or disable certain
functionality.i These options must be set when running CMake with
`-DENABLE_<OPTION>=<ON|OFF|AUTO`. The default is `AUTO` which enables
the option if a certain dependency is found. For example CEC support is
enabled if libCEC is available. `ON` forcefully enables the dependency
and the CMake run will fail if the related dependency is not available.
This is mostly useful for packagers. `OFF` will disable the feature.

Example for forcefully enabling VAAPI and disabling VDPAU:

```
cmake ... -DENABLE_VAAPI=ON -DENABLE_VDPAU=OFF ...
```

Example for building with external FFMPEG:

```
cmake ... -DFFMPEG_PATH=/opt/ffmpeg -DENABLE_INTERNAL_FFMPEG=OFF ...
```

For more information and an updated list of option, please check the
main [CMakeLists.txt](https://github.com/xbmc/xbmc/tree/master/CMakeLists.txt).

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
- `make check-valgrind` builds and executes the test suite with valgrind memcheck.
- `make doc` builds the Doxygen documentation.

Code coverage (with Gcov, LCOV and Gcovr) can be built on Linux:

- CMake has to be executed with `-DCMAKE_BUILD_TYPE=Coverage`
- `make coverage` generates an HTML code coverage report.
- `make coverage_xml` generates an XML code coverage report.

## Building binary addons

The CMake build system integrates with the addon build system if the GNU
Makefile generator is used. This offers an easy way to build addons for
packagers or Kodi developers who don't work on addons.

```
make binary-addons
```

Specific addons can be built with:

```
make binary-addons ADDONS="visualization.spectrum pvr.demo"
```

Addon developers can build single addons into the Kodi build directory
so that the addon can be tested with self-compiled specific versions of Kodi.

```
mkdir pvr.demo-build && cd pvr.demo-build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=<KODI_BUILD_DIR>/build -DKODI_BUILD_DIR=<KODI_BUILD_DIR> <pvr.demo-SRC>
make
```

It is recommended to specify the directories as absolute paths. If relative
paths are used, they are considered relative to the build directory in which
`cmake` was executed (aka the current working working directory).

Both methods work only for already existing addons. See this
[forum thread](http://forum.kodi.tv/showthread.php?tid=219166&pid=1934922#pid1934922)
and [addons/README.md](https://github.com/xbmc/xbmc/blob/master/cmake/addons/README.md)
for addon development and detailed documentation about the addon build system.

## Sanitizers

Clang and GCC support different kinds of Sanitizers. To enable a Sanitizer call CMake with the
option `-DECM_ENABLE_SANITIZERS=’san1;san2;...'`. For more information about enabling the
Sanitizers read the documentation in 
[modules/extra/ECMEnableSanitizers.cmake](https://github.com/xbmc/xbmc/tree/master/cmake/modules/extra/ECMEnableSanitizers.cmake).

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
