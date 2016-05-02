![Kodi logo](https://raw.githubusercontent.com/xbmc/xbmc-forum/master/xbmc/images/logo-sbs-black.png)
# Kodi add-ons CMake based buildsystem
This directory contains the cmake-based buildsystem for Kodi add-ons. It looks into the directory pointed to by the *ADDONS_DEFINITION_DIR* option (which defaults to the *addons* sub-directory) and parses all *.txt files recursively. Each add-on must have its own `<addon-id>.txt` file in a separate sub-directory that must follow one of the defined formats:

  - `<addon-id> <git-url> <git-revision>`
  - `<addon-id> <tarball-url>`
  - `<addon-id> <file://path>`
 
where
- `<addon-id>` must be identical to the add-on's ID as defined in the add-on's addon.xml
- `<git-url>` must be the URL of the git repository containing the add-on
- `<git-revision>` must be a valid git tag/branch/commit in the add-on's git repository which will be used for the build
- `<tarball-url>` must be the URL to a .tar.gz tarball containing the add-on
- `<file://path>` must be a *file://* based path to the directory containing the add-on

## Reserved filenames
- **platforms.txt**

List of platforms to build an add-on for (or *all*). Negating platforms is supported using a leading exclamation mark, e.g. *!windows*.

Available platforms are: linux, windows, osx, ios, android, rbpi and freebsd.

#### Attention
If no add-on definitions could be found, the buildsystem assumes that the bootstrapping of the add-on definition repositories hasn't been performed yet and automatically executes the add-on bootstrapping buildsystem located in the *bootstrap* sub-directory with the default settings (i.e. *all* add-ons from all pre-defined add-on definition repositories are bootstrapped into the directory pointed to by the *ADDONS_DEFINITION_DIR* option).

## Buildsystem variables
The buildsystem uses the following variables (which can be passed into it when executing cmake with the -D`<variable-name>=<value>` format) to manipulate the build process:
- `ADDONS_TO_BUILD` has two variations, which are tested in order:
     - a quoted, space delimited list of `<addon-id>s` that you want to build (default is *all*)
     - a regular expression that every `<addon-id>` is matched against (e.g. `ADDONS_TO_BUILD="pvr.*"`) to build all pvr add-ons
- `ADDONS_DEFINITION_DIR` points to the directory containing the definitions for the addons to be built
- `ADDON_SRC_PREFIX` can be used to override the add-on repository location. It must point to the locally available parent directory of the add-on(s) to build. `<addon-id>` will be appended to this path automatically
- `CMAKE_BUILD_TYPE` specifies the type of the build. This can be either *Debug* or *Release* (default is *Release*)
- `CMAKE_INSTALL_PREFIX` points to the directory where the built add-ons and their additional files (addon.xml, resources, ...) will be installed to (defaults to `<DEPENDS_PATH>`)
- `CMAKE_TOOLCHAIN_FILE` can be used to pass a toolchain file into the add-on builds
- `DEPENDS_PATH` points to the directory containing the *include* and *lib* directories of the add-ons' dependencies.
- `CORE_SOURCE_DIR` points to the root directory of the project (default is the absolute representation of ../../.. starting from this directory)
- `BUILD_DIR` points to the directory where the add-ons and their dependencies will be downloaded and built
- `PACKAGE_ZIP=ON` means that the add-ons will be 'packaged' into a common folder, rather than being placed in `<CMAKE_INSTALL_PREFIX>/lib/kodi/addons` and `<CMAKE_INSTALL_PREFIX>/share/kodi/addons`
- `PACKAGE_DIR` points to the directory where the ZIP archived add-ons will be stored after they have been packaged (defaults to `<BUILD_DIR>/zips`)
- `ARCH_DEFINES` specifies the platform-specific C/C++ preprocessor defines (defaults to empty)
- `ADDON_TARBALL_CACHING` specifies whether downloaded add-on source tarballs should be cached or not (defaults to *ON*)

## Deprecated buildsystem variables
Buildsystem will print a warning if you use any of the below-listed variables. For now they still work but you should adapt your workflow to the new variables.
- `APP_ROOT` - Use `CORE_SOURCE_DIR` instead

## Building
The buildsystem makes some assumptions about the environment which must be met by whoever uses it:
- Any dependencies of the add-ons must already be built and their include and library files must be present in the path pointed to by `<CMAKE_PREFIX_PATH>` (in *include* and *lib* sub-directories)

To trigger the cmake-based buildsystem the following command must be executed with `<path>` set to this directory (absolute or relative) allowing for in-source and out-of-source builds

`cmake <path> -G <generator>`

CMake supports multiple generators. See [here] (https://cmake.org/cmake/help/v3.1/manual/cmake-generators.7.html) for a list.

In case of additional options the call might look like this:

cmake `<path>` [-G `<generator>`] \  
      -DCMAKE_BUILD_TYPE=Release \  
      -DCORE_SOURCE_DIR="`<path-to-app-root>`" \  
      -DARCH_DEFINES="-DTARGET_LINUX" \  
      -DDEPENDS_PATH=`<path-to-built-depends>` \  
      -DCMAKE_INSTALL_PREFIX="`<path-to-install-directory`"
