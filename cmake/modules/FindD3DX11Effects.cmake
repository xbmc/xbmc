# - Finds D3DX11 dependencies
# Once done this will define
#
# FXC - Path to the DirectX Effects Compiler (FXC)

find_program(FXC fxc
             PATHS
               "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0;InstallationFolder]/bin/x86"
               "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0;InstallationFolder]/bin/[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0;ProductVersion].0/x86"
               "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.1;InstallationFolder]/bin/x86"
               "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.0;InstallationFolder]/bin/x86"
               "$ENV{WindowsSdkDir}bin/x86")
if(NOT FXC)
  message(WARNING "Could NOT find DirectX Effects Compiler (FXC)")
endif()
mark_as_advanced(FXC)
