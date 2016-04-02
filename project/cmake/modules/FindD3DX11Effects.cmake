# - Builds D3DX11Effects as external project
# Once done this will define
#
# D3DX11EFFECTS_FOUND - system has D3DX11Effects
# D3DX11EFFECTS_INCLUDE_DIRS - the D3DX11Effects include directories
# D3DCOMPILER_DLL - Path to the Direct3D Compiler

include(ExternalProject)
ExternalProject_Add(d3dx11effects
            SOURCE_DIR ${CORE_SOURCE_DIR}/lib/win32/Effects11
            PREFIX ${CORE_BUILD_DIR}/Effects11
            CONFIGURE_COMMAND ""
            BUILD_COMMAND msbuild ${CORE_SOURCE_DIR}/lib/win32/Effects11/Effects11_2013.sln
                                  /t:Effects11 /p:Configuration=${CORE_BUILD_CONFIG}
            INSTALL_COMMAND "")

set(D3DX11EFFECTS_FOUND 1)
set(D3DX11EFFECTS_INCLUDE_DIRS ${CORE_SOURCE_DIR}/lib/win32/Effects11/inc)

set(D3DX11EFFECTS_LIBRARY_RELEASE ${CORE_SOURCE_DIR}/lib/win32/Effects11/libs/Effects11/Release/Effects11.lib)
set(D3DX11EFFECTS_LIBRARY_DEBUG ${CORE_SOURCE_DIR}/lib/win32/Effects11/libs/Effects11/Debug/Effects11.lib)
include(SelectLibraryConfigurations)
select_library_configurations(D3DX11EFFECTS)

mark_as_advanced(D3DX11EFFECTS_FOUND)

find_file(D3DCOMPILER_DLL
          NAMES d3dcompiler_47.dll d3dcompiler_46.dll
          PATHS
            "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0;InstallationFolder]/Redist/D3D/x86"
            "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.1;InstallationFolder]/Redist/D3D/x86"
            "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.0;InstallationFolder]/Redist/D3D/x86"
            "$ENV{WindowsSdkDir}Redist/d3d/x86"
          NO_DEFAULT_PATH)
if(NOT D3DCOMPILER_DLL)
  message(WARNING "Could NOT find Direct3D Compiler")
endif()
mark_as_advanced(D3DCOMPILER_DLL)

find_program(FXC fxc
             PATHS
               "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0;InstallationFolder]/bin/x86"
               "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.1;InstallationFolder]/bin/x86"
               "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.0;InstallationFolder]/bin/x86"
               "$ENV{WindowsSdkDir}bin/x86")
if(NOT FXC)
  message(WARNING "Could NOT find DirectX Effects Compiler (FXC)")
endif()
mark_as_advanced(FXC)
