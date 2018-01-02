#PLATINUM UPNP SDK [![Carthage compatible](https://img.shields.io/badge/Carthage-compatible-4BC51D.svg?style=flat)](https://github.com/Carthage/Carthage) [![Build Status](https://travis-ci.org/plutinosoft/Platinum.svg?branch=master)](https://travis-ci.org/plutinosoft/Platinum)

This toolkit consists of 2 modules:
* Neptune : a C++ Runtime Library
* Platinum: a modular UPnP Framework [Platinum depends on Neptune]

Unless you intend to use Neptune independently from Platinum, it is recommended that you build binaries directly from the Platinum root directory. All the dependent binaries will be rebuilt automatically (including Neptune).

#Building SDK and Sample Applications

## Windows:
Open the Visual Studio 2010 solution located @ Build\Targets\x86-microsoft-win32-vs2010\Platinum.sln

## Mac, iOS:
First install carthage (https://github.com/Carthage/Carthage)
```
> brew update || brew install carthage
```
Build Neptune & Platinum frameworks
```
> carthage bootstrap
> carthage build --no-skip-current
```

Both Neptune and Platinum frameworks binaries can be found under Carthage/Build folders which you can link with your applications.
Follow the instructions on the [carthage page](https://github.com/Carthage/Carthage).
If you are building for iOS, special [instructions here](https://github.com/Carthage/Carthage#if-youre-building-for-ios).

If you are interested in building sample apps or tests, you can also open the XCode project file located @ Build/Targets/universal-apple-macosx/Platinum.xcodeproj.

## Linux, Cygwin, etc ...
Open a shell, go to the Platinum root directory and type 'scons' (http://scons.org).
```
> brew update || brew install scons
> git submodule update --init
> scons target={TARGET} build_config={Debug|Release}
```
The output of the scons build will be found under Build/Targets/{TARGET}/{Debug|Release}.
Additionally, the output is copied under Targets/{TARGET}/{Debug|Release} for convenience when applicable.

#Running Sample Applications

## FileMediaServerTest
This is an example of a UPnP MediaServer. Given a path, it allows a UPnP ControlPoint to browse the content of the directory and its sub-directories. Additionally, files can be streamed (Note that only files with known mimetypes are advertised).

```
FileMediaServerTest [-f <friendly_name>] <path>
    -f : optional upnp server friendly name
    <path> : local path to serve
```

Once started, type 'q' to quit.

## MediaRendererTest
This is an example shell of a UPnP MediaRenderer. It is to be contolled by a UPnP ControlPoint. This is just a SHELL, this won't play anything yet. You need to hook up the playback functionality yourself.

```
MediaRendererTest [-f <friendly_name>]
    -f : optional upnp server friendly name
```

Once started, type 'q' to quit.

## MediaCrawler
This is a combo UPnP MediaServer + ControlPoint. It browses content from other MediaServers it finds on the network and present them under one single aggregated view. This is useful for some devices that need to select one single MediaServer at boot time (i.e. Roku).

Once started, type 'q' to quit.

## MicroMediaController
This is a ControlPoint (synchronous) that lets you browse any MediaServer using a shell-like interface. Once started, a command prompt lets you enter commands such as:
```
     quit    -   shutdown
     exit    -   same as quit
     setms   -   select a media server to become the active media server
     getms   -   print the friendly name of the active media server
     ls      -   list the contents of the current directory on the active
                 media server
     cd      -   traverse down one level in the content tree on the active
                 media server
     cd ..   -   traverse up one level in the content tree on the active
                 media server
     pwd     -   print the path from the root to your current position in the
                 content tree on the active media server
```

Experimental MediaRenderer commands (not yet full implemented):
```
     setmr   -   select a media renderer to become the active media renderer
     getmr   -   print the friendly name of the active media renderer
     open    -   set the uri on the active media renderer
     play    -   play the active uri on the active media renderer
     stop    -   stop the active uri on the active media renderer
```

## MediaConnect
This is a derived implementation of the FileMediaServerTest with the only difference that it makes it visible to a XBox 360.

## MediaServerCocoaTest
A basic cocoa test server app showing how to use the Platinum framework on Mac OSX.

#Language Bindings

## Objective-C
Under Source/Extras/ObjectiveC

## C++/CLR
Under Source/Extras/Managed

## Android Java/JNI
To build the JNI shared library, you will need to install the Android NDK and set up the proper environment variables such as ANDROID_NDK_ROOT.
```
> scons target=arm-android-linux build_config=Release
> cd Source/Platform/Android/module/platinum
> ndk-build NDK_DEBUG=0
```

This will create the libplatinum-jni.so files under the Source/Platform/Android/module/platinum/libs folder.
You can then import eclipse Android .project located @ Source/Platform/Android/modules/platinum to create the jar file @ Source/Platform/Android/modules/platinum/bin/platinum.jar

To Test the Platinum jni layer, import into eclipse both Android projects located @ Source/Platform/Android/samples/sample-upnp & Source/Platform/Android/modules/platinum.

#Contributing

We're glad you're interested in Platinum, and we'd love to see where you take it.

Any contributors to the master Platinum repository must sign the [Individual Contributor License Agreement (CLA)](https://docs.google.com/forms/d/1-SuyEu0LfYuhY3kKDDdfdYn5cmTU2lrQRSQSDHau4PI/viewform).
It's a short form that covers our bases and makes sure you're eligible to contribute.

When you have a change you'd like to see in the master repository, [send a pull request](https://github.com/plutinosoft/Platinum/pulls). Before we merge your request, we'll make sure you're in the list of people who have signed a CLA.

Thanks!
