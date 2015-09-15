# KODI ADDON DEFINITIONS BOOTSTRAPPING
This directory contains the cmake-based buildsystem for addon definitions
bootstrapping which downloads the addon definitions from one or more addon
definitions repositories. These addon definitions are then used by the addon
buildsystem to figure out which addons and which versions to build. It looks
into the "repositories" sub-directory and parses all *.txt files recursively.
Each addon definitions repository must have its own <repository>.txt file which
must follow the following defined format:
```
<repository> <git-url> <git-revision>
```
where
* `<repository>` is the identification of the repository.
* `<git-url>` must be the URL of the git repository containing the addon
    definitions
* `<git-revision>` must be a valid git tag/branch/commit in the addon
    definitions repository's git repository which will be used for the build

The buildsystem uses the following variables (which can be passed into it when
executing cmake with the `-D<variable-name>=<value>` option):
* `CMAKE_INSTALL_PREFIX` points to the directory where the downloaded addon
definitions will be installed to (defaults to `../addons`).
* `BUILD_DIR` points to the directory where the addon definitions repositories
will be downloaded to.
* `REPOSITORY_TO_BUILD` specifies a single addon definitions repository to be
downloaded and processed (defaults to `"all"`).
* `REPOSITORY_REVISION` specifies the git revision in the addon definitions
repository which will be used for the build. This option is only valid in
combination with the `REPOSITORY_TO_BUILD` option (defaults to the git
revision specified in the repository's definition file).
* `ADDONS_TO_BUILD` is a quoted, space delimited list of `<addon-id>`s that
should be bootstrapped (default is `"all"`).

To trigger the cmake-based buildsystem the following command must be executed
with <path> being the path to this directory (absolute or relative, allowing for
in-source and out-of-source builds).
```
cmake <path> -G <generator>
```

cmake supports multiple generators, see
http://www.cmake.org/cmake/help/v2.8.8/cmake.html#section_Generators for a list.

In case of additional options the call might look like this
```
cmake <path> [-G <generator>] \
      -DCMAKE_INSTALL_PREFIX="<path-to-install-directory>"
```