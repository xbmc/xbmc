# Fix UWP addons security issue caused by empty __init__.py Python Lib files packaged with Kodi
set(uwp_pythonlibinit_filepattern "${DEPENDENCIES_DIR}/bin/Python/Lib/__init__.py")
file(GLOB_RECURSE uwp_pythonlibinit_foundfiles "${uwp_pythonlibinit_filepattern}")
foreach(uwp_pythonlibinit_file ${uwp_pythonlibinit_foundfiles})
    file(SIZE "${uwp_pythonlibinit_file}" uwp_pythonlibinit_filesize)
    if(${uwp_pythonlibinit_filesize} EQUAL 0)
        message("Adding hash comment character in the following empty file: ${uwp_pythonlibinit_file}")
        file(APPEND ${uwp_pythonlibinit_file} "#")
    endif()
endforeach()
