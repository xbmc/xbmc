![Kodi Logo](resources/banner_slim.png)

# Arch Linux build guide

## Table of Contents
1. **[Introduction](#1-Introduction)**
2. **[Document conventions](#2-document-conventions)**
3. **[Setup a clean chroot](#3-Setup-a-clean-chroot)**
4. **[Get the PKGBUILD and optionally edit it](#4-Get-the-PKGBUILD-and-optionally-edit-it)**
5. **[Build the package](#5-Build-the-package)**

## 1. Introduction
The aim of this document is to enable developers to compile/package Kodi ***from a local git repo*** using Arch-native build tools.  It is not meant to a be an in depth tutorial of Arch-specific tools.  Readers are encouraged to follow the provided links to the [Arch Wiki](https://wiki.archlinux.org) to learn more.

The Distro standard for building and packaging is to do so in a [clean chroot](https://wiki.archlinux.org/index.php/DeveloperWiki:Building_in_a_clean_chroot#Setting_up_a_chroot) using a [PKGBUILD](https://wiki.archlinux.org/index.php/PKGBUILD) to direct the build system.  Building in this fashion is advantageous for several reasons including:
* Auto-managed dependencies.
* The live file system remains untouched (all the dependencies are installed to the chroot for the build).
* High degree of reproducibility.

Beyond the benefits of the building in this fashion, the resulting files from the build are packaged in an Arch-native format so installation and removal is trivial as it is handled by [pacman](https://wiki.archlinux.org/index.php/Pacman), the Distro package manager.

Manually compiling can be done but it is discouraged and out of the scope of this document.

**[back to top](#table-of-contents)**

## 2. Document conventions
Commands prefixed by a `$` are meant to be run as a non-priviliged user whereas commands prefixed by a `#` are meant to be run by the root user.

This is a comment that provides context:
```
$ this command is run by a standard user
# this command is run by the root user
```

**[back to top](#table-of-contents)**

## 3. Setup a clean chroot

Install the devtools package:
```
# pacman -S devtools
```

Create a directory in which the chroot will reside:
```
$ CHROOT=/home/foo/buildroot
$ mkdir $CHROOT/root
```

Create the chroot:
```
$ mkarchroot $CHROOT/root base-devel
```
Once finished, the minimum set of build dependencies will be installed to `$CHROOT/root`

**[back to top](#table-of-contents)**

## 4. Get the PKGBUILD and optionally edit it
Download and untar the [kodi-git](https://aur.archlinux.org/packages/kodi-git/) tarball from the Arch User Repository:
```
$ wget https://aur.archlinux.org/cgit/aur.git/snapshot/kodi-git.tar.gz
$ tar zxf kodi-git.tar.gz
```

By default, the `PKGBUILD` points to the master branch of the live repo for Kodi hosted on github.  Edit the `PKGBUILD` as follows to define the ***physical fully qualified path to your local git repo***.

For the example, our local xbmc repo is `/scratch/xbmc` and we are working on the `coolnew` branch. 

Edit the PKGBUILD's `source` array to redefine the remote source to this local one.

Original line:
```
"git://github.com/xbmc/xbmc.git#branch=master"
```

Edited line:
```
"git+file:///scratch/xbmc#branch=coolnew"
```

One can accomplish this with a simple call to `sed` as follows:
```
$ cd kodi-git
$ MYREPO=/scratch/xbmc
$ MYBRANCH=coolnew
$ sed -i s"|git://.*|git+file://$MYREPO#branch=$MYBRANCH\"|" PKGBUILD
```

One can optionally edit the PKGBUILD to adjust any aspect of the build, for example, to pass different cmake options.  The PKGBUILD is nothing more then a bash script used by the build system.  For more, see the [PKGBUILD](https://wiki.archlinux.org/index.php/PKGBUILD) article on the Arch Wiki.

**[back to top](#table-of-contents)**

## 5. Build the package and install Kodi

To initiate the build, simply execute the following command in the directory containing the PKGBUILD modified in step 4.  Note that the number of threads (usually the number of physical cores + 1) can optionally be defined in the `MAKEFLAGS` variable which will be passed to the build script if defined.

Adjusts this number to match your build hardware:

```
$ MAKEFLAGS=-j9 makechrootpkg -c -r $CHROOT
```

If the build completes without error, several packages will be created corresponding to the recipe in the PKGBUILD.  Arch users can install a Kodi package that uses either the X11, Wayland, or GBM composer.   For more on differences, see the [kodi](https://wiki.archlinux.org/index.php/Kodi) wiki article.  

To install the built x11 package for example:

```
# pacman -U kodi-git-xxx-y-x86_64.pkg.tar.xz kodi-bin-git-xxx-y-x86_64.pkg.tar.xz
```

To install the built gbm package for example:

```
# pacman -U kodi-git-gbm-xxx-y-x86_64.pkg.tar.xz kodi-bin-git-xxx-y-x86_64.pkg.tar.xz
```

Note that variables have been substituted for the actual version and release number as these will vary with builds.

**[back to top](#table-of-contents)**
