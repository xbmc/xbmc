# Effects for Direct3D 11 (FX11)

http://go.microsoft.com/fwlink/?LinkId=271568

## Release History

## August 17, 2022 (11.28)
* CMake and MSBuild project updates

## May 23, 2022 (11.27)
* Add VS 2022 projects, retired VS 2017 projects
* Update build switches for SDL recommendations
* CMake project cleanup, added CMakePresets.json

## December 2, 2021
* Minor project cleanup

## February 7, 2021
* Added CMake project
* Removed Windows Vista support
* No code changes

## June 1, 2020
* Minor update to VS 2019 project
* Retired VS 2015 projects
* No code changes

### April 26, 2019 (11.26)
* Added VS 2019 desktop projects
* VS 2017 updated for Windows 10 October 2018 Update SDK (17763)
* Minor code cleanup

### July 12, 2018 (11.25)
* Added ``D3DX11DebugMute`` function
* Minor project and code cleanup

### May 31, 2018 (11.24)
* VS 2017 updated for Windows 10 April 2018 Update SDK (17134)

### May 11, 2018 (11.23)
* Retired VS 2013 projects
* Code cleanup

### February 27, 2018 (11.22)
* Minor code update

### November 2, 2017 (11.21)
* VS 2017 updated for Windows 10 Fall Creators Update SDK (16299)

### October 13, 2017 (11.20)
* Updated for VS 2017 update 15.1 - 15.3 and Windows 10 SDK (15063)    

### March 10, 2017 (11.19)
* Add VS 2017 projects
* Minor code cleanup    

### September 15, 2016 (11.18)
* Minor code cleanup

### August 2, 2016 (11.17)
* Updated for VS 2015 Update 3 and Windows 10 SDK (14393)
* Added 'D' suffix to debug libraries per request

### April 26, 2016 (11.16)
* Retired VS 2012 projects
* Minor code and project file cleanup

### November 30, 2015 (11.15)
* Updated for VS 2015 Update 1 and Windows 10 SDK (10586)

### July 29, 2015 (11.14)
* Updated for VS 2015 and Windows 10 SDK RTM
* Retired VS 2010 projects

### June 17, 2015 (11.13)
* Fix for Get/SetFloatVectorArray with an offset

### April 14, 2015 (11.12)
* More updates for VS 2015

### November 24, 2014 (11.11)
* Updates for Visual Studio 2015 Technical Preview

### July 15, 2014 (11.10)
* Minor code review fixes

### January 24, 2014 (11.09)
* VS 2010 projects now require Windows 8.1 SDK
* Added pragma for needed libs to public header
* Minor code cleanup

### October 21, 2013 (11.08)
* Updated for Visual Studio 2013 and Windows 8.1 SDK RTM

### July 16, 2013 (11.07)
* Added VS 2013 Preview project files
* Cleaned up project files
* Fixed a number of /analyze issues

### June 13, 2013 (11.06)
* Added ``GetMatrixPointerArray``, ``GetMatrixTransposePointerArray``, ``SetMatrixPointerArray``, ``SetMatrixTransposePointerArray`` methods
* Reverted back to ``BOOL`` in some cases because sizeof(bool)==1, sizeof(BOOL)==4
* Some code-cleanup: minor SAL fix, removed bad assert, and added use of override keyword

### February 22, 2013 (11.05)
* Cleaned up some warning level 4 warnings

### November 6, 2012 (11.04)
* Added ``IUnknown`` as a base class for all Effects 11 interfaces to simplify use in managed interop sceanrios, although the lifetime for these objects is still based on the lifetime of the parent ID3DX11Effect object. Therefore reference counting is ignored for these interfaces.
  + ID3DX11EffectType, ID3DX11EffectVariable and derived classes, ID3DX11EffectPass, ID3DX11EffectTechnique, and ID3DX11EffectGroup

### October 24, 2012 (11.03)
* Removed the dependency on the D3DX11 headers, so FX11 no longer requires the legacy DirectX SDK to build.
* It does require the d3dcompiler.h header from either the Windows 8.0 SDK or from the legacy DirectX SDK
* Removed references to D3D10 constants and interfaces
* Deleted the d3dx11dbg.cpp and d3dx11dbg.h files
* Deleted the ``D3DX11_EFFECT_PASS`` flags which were never implemented
* General C++ code cleanups (nullptr, C++ style casting, stdint.h types, Safer CRT, etc.) which are compatible with Visual C++ 2010 and 2012
* SAL2 annotation and /analyze cleanup
* Added population of Direct3D debug names for object naming support in PIX and the SDK debug layer
* Added additional optional parameter to D3DX11CreateEffectFromMemory to provide a debug name
* Added ``D3DX11CreateEffectFromFile``,``D3DX11CompileEffectFromMemory``, and ``D3DX11CompileEffectFromFile``

### June 2010 (11.02)
The DirectX SDK (June 2010) included an update with some minor additional bug fixes. This also included the Effects 11-based sample DynamicShaderLinkageFX11. This is the last version to support Visual Studio 2008.  The source code is located in ``Samples\C++\Effects11``.

### February 2010 (11.01)
An update was shipped with the DirectX SDK (February 2010). This fixed a problem with the library which prevented it from working correctly on 9.x and 10.x feature levels. This is the last version to support Visual Studio 2005. The source code is located in ``Samples\C++\Effects11``.

### August 2009 (11.00)
The initial release of Effects 11 (FX11) was in the DirectX SDK (August 2009). The source code is located in ``Utilities\Source\Effects11``. This is essentially the Effects 10 (FX10) system ported to Direct3D 11.0 with support for effects pools removed and support for groups added.
