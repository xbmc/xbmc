![Kodi Logo](../docs/resources/banner_slim.png)

# Kodi's CMake Based Build System
Welcome to Kodi's CMake Based Build System. CMake is a cross-platform tool for generating makefiles as well as project files used by IDEs. The current version of the build system is capable of building and packaging Kodi for the following platforms:

* Linux (several distros)
* Windows
* macOS, iOS and tvOS
* Android
* FreeBSD
* webOS

While the legacy build systems typically used in-source builds, it's recommended to use out-of-source builds with CMake. The necessary runtime dependencies such as dlls, skins and configuration files are copied over to the build directory automatically. Instructions are highly dependent on your operating system and target platform but we prepared a set of **[build guides](../docs/README.md)** for your convenience.

## Build options
Kodi supports a number of build options that can enable or disable functionality. These options must be set when running CMake with `-DENABLE_<OPTION>=<ON|OFF|AUTO>`. The default is `AUTO` which enables the option if a certain dependency is found. For example CEC support is enabled if `libCEC` is available. `ON` forcefully enables the dependency and the CMake run will **fail** if the related dependency is not available. `OFF` will disable the feature.

Example for forcefully enabling VAAPI and disabling VDPAU:
```
cmake ... -DENABLE_VAAPI=ON -DENABLE_VDPAU=OFF ...
```

Unfortunately, Kodi's CMake gazillion options are not fully documented yet. For more information and an updated list of options, please check the main **[CMakeLists.txt](../CMakeLists.txt)**.

## Buildsystem variables
The buildsystem uses the following variables (which can be passed into it when executing cmake with the -D`<variable-name>=<value>` format) to manipulate the build process (see READMEs in sub-directories for additional variables):
- `CMAKE_BUILD_TYPE` specifies the type of the build. This can be either *Debug* or *Release* (default is *Release*)
- `CMAKE_TOOLCHAIN_FILE` can be used to pass a toolchain file
- `ARCH_DEFINES` specifies the platform-specific C/C++ preprocessor defines (defaults to empty)

## Building
To trigger the cmake-based buildsystem the following command must be executed with `<path>` set to this directory (absolute or relative) allowing for in-source and out-of-source builds

`cmake <path> -G <generator>`

CMake supports multiple generators. See [here](https://cmake.org/cmake/help/v3.1/manual/cmake-generators.7.html) for a list.

In case of additional options the call might look like this:

cmake `<path>` [-G `<generator>`] \  
      -DCMAKE_BUILD_TYPE=Release \  
      -DARCH_DEFINES="-DTARGET_LINUX" \  
      -DCMAKE_INSTALL_PREFIX="`<path-to-install-directory>`"

## Tests
Kodi uses Google Test as its testing framework. Each test file is scanned for tests and these are added to CTest, which is the native test driver for CMake.

This scanning happens at configuration time. If tests depend on generated support files which should not be scanned, then those support files should be added to the `SUPPORT_SOURCES` variable as opposed to `SOURCES` before calling `core_add_test`. You might want to do this where the generated support files would not exist at configure time, or if they are so large that scanning them would take up an unreasonable amount of configure time.

## Extra targets
When using makefile builds, a few extra targets are defined:

* `make check` builds and executes the test suite.
* `make check-valgrind` builds and executes the test suite with valgrind memcheck.
* `make doc` builds the Doxygen documentation.

Code coverage (with Gcov, LCOV and Gcovr) can be built on Linux:

* CMake has to be executed with `-DCMAKE_BUILD_TYPE=Coverage`.
* `make coverage` generates an HTML code coverage report.
* `make coverage_xml` generates an XML code coverage report.

## Building binary addons
Kodi's CMake build system integrates with the add-on build system if the GNU Makefile generator is used. This offers an easy way to build add-ons for packagers or Kodi developers who don't work on add-ons.

Build all add-ons:
```
make binary-addons
```

Build specific add-ons:
```
make binary-addons ADDONS="visualization.spectrum pvr.demo"
```

Add-on developers can build single add-ons into the Kodi build directory so that the add-on can be tested with self-compiled specific versions of Kodi.
```
mkdir pvr.demo-build
cd pvr.demo-build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=<KODI_BUILD_DIR>/build -DKODI_BUILD_DIR=<KODI_BUILD_DIR> <pvr.demo-SRC>
make
```

It is recommended to specify the directories as absolute paths. If relative paths are used, they are considered relative to the build directory in which `cmake` was executed (the current working directory).

Both methods work only for already existing add-ons. See this **[forum thread](https://forum.kodi.tv/showthread.php?tid=219166&pid=1934922)** and add-ons **[README](cmake/addons/README.md)**
for add-on development and detailed documentation about the add-on build system.

## Sanitizers
Clang and GCC support different kinds of sanitizers. To enable a sanitizer, call CMake with the option `-DECM_ENABLE_SANITIZERS='san1;san2;...'`. For more information about enabling the
sanitizers, read the module **[documentation](modules/extra/ECMEnableSanitizers.cmake)**.

It is also recommended to read the sections about sanitizers in the [Clang documentation](http://clang.llvm.org/docs/).

## Debugging the build
In order to see the exact compiler commands `make` and `nmake` can be executed with a `VERBOSE=1` parameter.

On Windows, this is unfortunately not enough because `nmake` uses temporary files to workaround `nmake`'s command string length limitations.
In order to see verbose output the file **[Modules/Platform/Windows.cmake](https://github.com/Kitware/CMake/blob/master/Modules/Platform/Windows.cmake#L40)** in the local CMake installation has to be adapted by uncommenting these lines:
```
# uncomment these out to debug nmake and borland makefiles
#set(CMAKE_START_TEMP_FILE "")
#set(CMAKE_END_TEMP_FILE "")
#set(CMAKE_VERBOSE_MAKEFILE 1)
```

