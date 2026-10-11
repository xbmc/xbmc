if(NOT CMAKE_GENERATOR MATCHES "Visual Studio" OR
   NOT (CMAKE_C_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
  return()
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/ccache")
file(COPY_FILE "${CCACHE_PROGRAM}" "${CMAKE_BINARY_DIR}/ccache/cl.exe" ONLY_IF_DIFFERENT)
list(APPEND CMAKE_VS_GLOBALS "CLToolExe=cl.exe"
                           "CLToolPath=${CMAKE_BINARY_DIR}/ccache"
                           "UseMultiToolTask=true"
                           "EnforceProcessCountAcrossBuilds=true")
if(DEFINED ENV{MAXTHREADS})
  list(APPEND CMAKE_VS_GLOBALS "CL_MPCount=$ENV{MAXTHREADS}")
endif()

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)
set(_ccache_variables CMAKE_VS_GLOBALS CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
                      CMAKE_DISABLE_PRECOMPILE_HEADERS)

# Explicit debug flags take precedence over MSVC_DEBUG_INFORMATION_FORMAT.
foreach(_language C CXX)
  foreach(_configuration "" _DEBUG _RELEASE _RELWITHDEBINFO _MINSIZEREL)
    set(_variable CMAKE_${_language}_FLAGS${_configuration})
    string(REGEX REPLACE "(^|[ \t])[-/]Z[iI]([ \t]|$)" "\\1/Z7\\2"
           ${_variable} "${${_variable}}")
    list(APPEND _ccache_variables ${_variable})
  endforeach()
endforeach()
