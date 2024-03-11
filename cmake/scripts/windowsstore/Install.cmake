# Fix UWP addons security issue caused by empty __init__.py Python Lib files packaged with Kodi
# Encapsulate fix script to allow post generation execution in the event the python lib is
# built after project generation.

file(REMOVE ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/GeneratedUWPPythonInitFix.cmake)
file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/GeneratedUWPPythonInitFix.cmake
"set(uwp_pythonlibinit_filepattern \"\$\{DEPENDS_PATH\}/bin/Python/Lib/__init__.py\")
file(GLOB_RECURSE uwp_pythonlibinit_foundfiles \"\$\{uwp_pythonlibinit_filepattern\}\")
foreach(uwp_pythonlibinit_file \$\{uwp_pythonlibinit_foundfiles\})
    file(SIZE \"\$\{uwp_pythonlibinit_file\}\" uwp_pythonlibinit_filesize)
    if(\$\{uwp_pythonlibinit_filesize\} EQUAL 0)
        message(\"Adding hash comment character in the following empty file: \$\{uwp_pythonlibinit_file\}\")
        file(APPEND \$\{uwp_pythonlibinit_file\} \"#\")
    endif()
endforeach()\n")

# Change to Python3::Python target when built internal
add_custom_target(generate-UWP-pythonfix
                   COMMAND ${CMAKE_COMMAND} -DDEPENDS_PATH=${DEPENDS_PATH}
                                            -P ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/GeneratedUWPPythonInitFix.cmake
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Make sure we apply fix to dependspath before we export-files copy to buildtree
add_dependencies(export-files generate-UWP-pythonfix)
