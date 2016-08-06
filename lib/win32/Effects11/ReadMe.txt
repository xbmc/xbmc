EFFECTS FOR DIRECT3D 11 (FX11)
------------------------------

Copyright (c) Microsoft Corporation. All rights reserved.

August 1, 2016

Effects for Direct3D 11 (FX11) is a management runtime for authoring HLSL shaders, render
state, and runtime variables together.

The source is written for Visual Studio 2013 or 2015. It is recommended that you
make use of VS 2013 Update 5, VS 2015 Update 2, and Windows 7 Service Pack 1 or later.

These components are designed to work without requiring any content from the DirectX SDK. For details,
see "Where is the DirectX SDK?" <http://msdn.microsoft.com/en-us/library/ee663275.aspx>.

All content and source code for this package are subject to the terms of the MIT License.
<http://opensource.org/licenses/MIT>.

For the latest version of FX11, more detailed documentation, etc., please visit the project site.

http://go.microsoft.com/fwlink/p/?LinkId=271568

This project has adopted the Microsoft Open Source Code of Conduct. For more information see the
Code of Conduct FAQ or contact opencode@microsoft.com with any additional questions or comments.

https://opensource.microsoft.com/codeofconduct/


-------
SAMPLES
-------

Direct3D Tutorial 11-14
BasicHLSLFX11, DynamicShaderLinkageFX11, FixedFuncEMUFX11, InstancingFX11

These samples are hosted on GitHub <https://github.com/walbourn/directx-sdk-samples>


----------
DISCLAIMER
----------

Effects 11 is being provided as a porting aid for older code that makes use of the Effects 10 (FX10) API or Effects 9 (FX9)
API in the deprecated D3DX9 library. See MSDN for a list of differences compared to the Effects 10 (FX10) library.

https://msdn.microsoft.com/en-us/library/windows/desktop/ff476141.aspx

The Effects 11 library is for use in Win32 desktop applications. FX11 requires the D3DCompiler API be available at runtime
to provide shader reflection functionality, and this API is not deployable for Windows Store apps on Windows 8.0, Windows RT,
or Windows phone 8.0.

The fx_5_0 profile support in the HLSL compiler is deprecated, and does not fully support DirectX 11.1 HLSL features
such as minimum precision types. It is supported in the Windows 8.1 SDK version of the HLSL compiler (FXC.EXE) and
D3DCompile API (#46), is supported but generates a deprecation warning with D3DCompile API (#47), and could be removed
in a future update.


---------------
RELEASE HISTORY
---------------

August 2, 2016 (11.17)
    Updated for VS 2015 Update 3 and Windows 10 SDK (14393)
    Added 'D' suffix to debug libraries per request

April 26, 2016 (11.16)
    Retired VS 2012 projects
    Minor code and project file cleanup

November 30, 2015 (11.15)
    Updated for VS 2015 Update 1 and Windows 10 SDK (10586)

July 29, 2015 (11.14)
    Updated for VS 2015 and Windows 10 SDK RTM
    Retired VS 2010 projects

June 17, 2015 (11.13)
    Fix for Get/SetFloatVectorArray with an offset

April 14, 2015 (11.12)
    More updates for VS 2015

November 24, 2014 (11.11)
    Updates for Visual Studio 2015 Technical Preview

July 15, 2014 (11.10)
    Minor code review fixes

January 24, 2014 (11.09)
    VS 2010 projects now require Windows 8.1 SDK
    Added pragma for needed libs to public header
    Minor code cleanup

October 21, 2013 (11.08)
    Updated for Visual Studio 2013 and Windows 8.1 SDK RTM

July 16, 2013 (11.07)
    Added VS 2013 Preview project files
    Cleaned up project files
    Fixed a number of /analyze issues

June 13, 2013 (11.06)
    Added GetMatrixPointerArray, GetMatrixTransposePointerArray, SetMatrixPointerArray, SetMatrixTransposePointerArray methods
    Reverted back to BOOL in some cases because sizeof(bool)==1, sizeof(BOOL)==4
    Some code-cleanup: minor SAL fix, removed bad assert, and added use of override keyword

February 22, 2013 (11.05)
    Cleaned up some warning level 4 warnings

November 6, 2012 (11.04)
    Added IUnknown as a base class for all Effects 11 interfaces to simplify use in managed interop sceanrios, although the
    lifetime for these objects is still based on the lifetime of the parent ID3DX11Effect object. Therefore reference counting
    is ignored for these interfaces.
        ID3DX11EffectType, ID3DX11EffectVariable and derived classes, ID3DX11EffectPass,
        ID3DX11EffectTechnique, and ID3DX11EffectGroup

October 24, 2012 (11.03)
    Removed the dependency on the D3DX11 headers, so FX11 no longer requires the legacy DirectX SDK to build.
    It does require the d3dcompiler.h header from either the Windows 8.0 SDK or from the legacy DirectX SDK
    Removed references to D3D10 constants and interfaces
    Deleted the d3dx11dbg.cpp and d3dx11dbg.h files
    Deleted the D3DX11_EFFECT_PASS flags which were never implemented
    General C++ code cleanups (nullptr, C++ style casting, stdint.h types, Safer CRT, etc.) which are compatible with Visual C++ 2010 and 2012
    SAL2 annotation and /analyze cleanup
    Added population of Direct3D debug names for object naming support in PIX and the SDK debug layer
    Added additional optional parameter to D3DX11CreateEffectFromMemory to provide a debug name
    Added D3DX11CreateEffectFromFile, D3DX11CompileEffectFromMemory, and D3DX11CompileEffectFromFile

June 2010 (11.02)
    The DirectX SDK (June 2010) included an update with some minor additional bug fixes. This also included the Effects 11-based
    sample DynamicShaderLinkageFX11. This is the last version to support Visual Studio 2008.  The source code is located in
    Samples\C++\Effects11.

February 2010 (11.01)
    An update was shipped with the DirectX SDK (February 2010). This fixed a problem with the library which prevented it from
    working correctly on 9.x and 10.x feature levels. This is the last version to support Visual Studio 2005. The source code
    is located in Samples\C++\Effects11.

August 2009 (11.00)
    The initial release of Effects 11 (FX11) was in the DirectX SDK (August 2009). The source code is located in
    Utilities\Source\Effects11. This is essentially the Effects 10 (FX10) system ported to Direct3D 11.0 with
    support for effects pools removed and support for groups added.
