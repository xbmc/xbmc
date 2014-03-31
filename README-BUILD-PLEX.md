# Buliding instructions for Plex Home Theater

## Generic instructions

### Get the source

```
git clone https://github.com/plexinc/plex-home-theater.
git checkout pht-frodo
```

Or download a tarfile from https://github.com/plexinc/plex-home-theater-public/releases

### CMake options

Our CMake project handles the following variables:

* OSX_SDK_VERSION=[10.8] - Only available on OSX, decides which SDK version you should use.
* OSX_ARCH=[i386/x86_64] - Only available on OSX, decide what arch you to build for. Defaults to x86_64
* COMPRESS_TEXTURES=[on/off] - If we should compress skin textures. It's expensive and takes a lot of time. If you are just building for yourself or for development, I suggest you turn this off.
* CREATE_BUNDLE=[on/off] - Generates a finalized release bundle and signing the code. You usually want to set this to off.
* ENABLE_AUTOUPDATE=[on/off] - If you want to include the autoupdate code. Should be disabled on Linux since it won't work there.
* ENABLE_DUMP_SYMBOLS=[on/off] - Enable crashreporter system. Should be set to off on developer builds and on Linux.
* CMAKE_INSTALL_PREFIX=[path] - When you run the install command this is where it will be installed.

You also need to pass the -G option to CMake to tell what generator to use, I suggest Ninja since it's fast.

Example, if you want to build a developer build locally:

```
cmake -GNinja -DCMAKE_INSTALL_PREFIX=/usr/local/PlexHomeTheater -DCOMPRESS_TEXTURES=off -DCREATE_BUNDLE=off -DENABLE_DUMP_SYMBOLS=off [path to source]
```

## MacOSX

You need to install the following things:

* Xcode 5
* Homebrew

When you have those installed you need to install the following things from Homebrew

* git
* cmake
* gnu-tar
* xz
* ninja (optional)
* ccache (optional)

### Xcode build

Make sure to download the source then generate the Xcode xcodeproj with cmake:

```
mkdir pht-xcode-build
cd pht-xcode-build
cmake -GXcode [CMAKE OPTIONS (see above)] ../plex-home-theater-public
```

This will try to download the dependencies from our server so you don't need to build them. But if they can't be found you might need to build them yourself:

```
cd ../plex-home-theater-public
plex/scripts/build_osx_depends.sh osx64 (or osx for 32bit build)
```

When that is done (takes a long time dudes) you rerun cmake from above and it should use the local deps instead.

When CMake is finished you should now have a PlexHomeTheater.xcodeproj that you can open in Xcode and build, make sure you build the "Install" target. To run PHT inside Xcode you need to edit schemes, select the Run option and then select the binary that is installed into the path you specified in CMAKE_INSTALL_PATH. When you press run now it will run the install target and then run the binary we output.

## Ninja build (faster)

```
mkdir pht-xcode-build
cd pht-xcode-build
cmake -GNinja [CMAKE OPTIONS (see above)] ../plex-home-theater-public
ninja install
```

## Windows

You need to install the following things:

* Visual Studio 2012 (Express or Pro doesn't matter)
* CMake
* Git (I use the github client from windows.github.com)
* DirectX 9 SDK

Get the source via the client or download a release.

Now get the dependencies via the script in plex\scripts that is called fetch-depends-windows.bat, just run that and wait for it to finish.

I suggest you open CMakeGUI now (should have a start menu entry from the cmake install).

Select the source directory, create a new build directory. Select the Visual Studio 11 toolchain.

Press configure, adjust options (see above), and then press generate.

Now you have a Visual Studio project that you can open and right click to build.

## Linux

TBA