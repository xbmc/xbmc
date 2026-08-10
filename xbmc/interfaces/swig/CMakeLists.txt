# The python bindings glue (xbmc/interfaces/python) is generated from the
# `swig -xml` output by the code generator in tools/codegenerator/. The host
# Python it runs on is already a Kodi build requirement (ENABLE_PYTHON).

# Host Python 3 interpreter used to run the code generator at build time. This
# is the BUILD-host interpreter, distinct from Kodi's target Python in
# cmake/modules/FindPython.cmake. FindPythonInterpreter searches the host
# (hinting ${NATIVEPREFIX}/bin) and, unlike find_package(Python3), does not
# pick up a cross-compiled target interpreter from the CMake cache.
find_package(PythonInterpreter REQUIRED ${SEARCH_QUIET})
if(PYTHON3_INTERPRETER_VERSION VERSION_LESS 3.8)
  message(FATAL_ERROR "The python bindings code generator requires a host "
                      "Python 3.8 or newer (found ${PYTHON3_INTERPRETER_VERSION})")
endif()

find_package(SWIG REQUIRED ${SEARCH_QUIET})

# The code generator modules (driver + helpers).
set(CODEGEN_DIR ${CMAKE_SOURCE_DIR}/tools/codegenerator)
set(CODEGEN_SOURCES
    ${CODEGEN_DIR}/generate.py
    ${CODEGEN_DIR}/emit.py
    ${CODEGEN_DIR}/helper.py
    ${CODEGEN_DIR}/typemaps.py
    ${CODEGEN_DIR}/python_tools.py
    ${CODEGEN_DIR}/method_type.py
    ${CODEGEN_DIR}/swig_type_parser.py
    ${CODEGEN_DIR}/transform.py
    ${CODEGEN_DIR}/node.py)

function(generate_file file)
  set(CPP_FILE ${file}.cpp)
  if(CLANGFORMAT_FOUND)
    set(CLANG_FORMAT_COMMAND COMMAND ${CLANG_FORMAT_EXECUTABLE} ARGS -i ${CPP_FILE})
  endif()

  # Args used for swig execution
  set(SWIG_ARGS -w401 -c++ -o ${file}.xml -xml -I${CMAKE_SOURCE_DIR}/xbmc ${CMAKE_CURRENT_SOURCE_DIR}/../swig/${file})

  # This generates a file that has the dependencies the preprocessed swig template finds
  # execute_process needs explicit use of explicit swig executable location. it is not TARGET aware
  # retrieve location property from TARGET
  get_target_property(_swig_exe SWIG::SWIG IMPORTED_LOCATION)
  execute_process(COMMAND ${_swig_exe} -MM -MF ${CMAKE_BINARY_DIR}/build/swig/${file}.dep ${SWIG_ARGS})
  file(READ ${CMAKE_BINARY_DIR}/build/swig/${file}.dep swig_deps)

  # Match all lines except the first one until " \"
  string(REGEX MATCHALL "\n  [^ ]+" temp ${swig_deps})
  set(swig_deps)
  foreach(t ${temp})
    string(STRIP "${t}" t)
    set(swig_deps ${swig_deps} "${t}")
  endforeach()

  # We set this to allow cmake to detect if changes in the headers/template files are made
  # and to force a cmake reconfigure to detect new dependencies that may have been added.
  set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${swig_deps})

  # generate.py is quiet on stdout (diagnostics go to stderr).
  add_custom_command(OUTPUT ${CPP_FILE}
                     COMMAND SWIG::SWIG
                     ARGS ${SWIG_ARGS}
                     COMMAND ${PYTHON_EXECUTABLE}
                     ARGS ${CODEGEN_DIR}/generate.py ${file}.xml ${CPP_FILE}
                     ${CLANG_FORMAT_COMMAND}
                     DEPENDS SWIG::SWIG ${swig_deps} ${CODEGEN_SOURCES}
                     BYPRODUCTS ${CMAKE_BINARY_DIR}/build/swig/${file}.xml)

  set(SOURCES ${SOURCES} "${CPP_FILE}" PARENT_SCOPE)
endfunction()

# The generated bindings
set(INPUTS AddonModuleXbmcaddon.i
           AddonModuleXbmcdrm.i
           AddonModuleXbmcgui.i
           AddonModuleXbmc.i
           AddonModuleXbmcplugin.i
           AddonModuleXbmcvfs.i
           AddonModuleXbmcwsgi.i)

foreach(INPUT IN LISTS INPUTS)
  generate_file(${INPUT})
endforeach()

add_library(python_binding STATIC ${SOURCES})
set_target_properties(python_binding PROPERTIES POSITION_INDEPENDENT_CODE TRUE
                                                FOLDER "Build Utilities")

if(CORE_SYSTEM_NAME STREQUAL windowsstore)
  set_target_properties(python_binding PROPERTIES STATIC_LIBRARY_FLAGS "/ignore:4264")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set_target_properties(python_binding PROPERTIES
                        COMPILE_FLAGS -Wno-cast-function-type) # from -Wextra
endif()

# Add target dependencies to lib
core_target_link_libraries(python_binding)
# Link this target lib to core
target_link_libraries(lib${APP_NAME_LC} PUBLIC python_binding)
