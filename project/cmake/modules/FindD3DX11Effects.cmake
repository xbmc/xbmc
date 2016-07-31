# - Finds D3DX11 dependencies
# Once done this will define
#
# D3DCOMPILER_DLL - Path to the Direct3D Compiler
# FXC - Path to the DirectX Effects Compiler (FXC)

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
copy_file_to_buildtree(${D3DCOMPILER_DLL} DIRECTORY .)

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
