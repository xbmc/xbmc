# FindEffectsCompiler
# --------
# Find the DirectX Effects Compiler Tool
#
# This will define the following target:
#
#   windows::FXC - The FXC compiler

if(NOT windows::FXC)
  find_program(FXC_EXECUTABLE fxc
              PATHS
                "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0;InstallationFolder]/bin/x86"
                "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0;InstallationFolder]/bin/[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0;ProductVersion].0/x86"
                "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.1;InstallationFolder]/bin/x86"
                "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.0;InstallationFolder]/bin/x86"
                "$ENV{WindowsSdkDir}bin/x86")

  if(FXC_EXECUTABLE)

    include(FindPackageMessage)
    find_package_message(EffectsCompiler "Found DX Effects Compiler Tool (FXC): ${FXC_EXECUTABLE}" "[${FXC_EXECUTABLE}]")

    add_executable(windows::FXC IMPORTED)
    set_target_properties(windows::FXC PROPERTIES
                                      IMPORTED_LOCATION "${FXC_EXECUTABLE}"
                                      FOLDER "External Projects")
  else()
    if(EffectsCompiler_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find DirectX Effects Compiler (FXC)")
    endif()
  endif()
endif()
