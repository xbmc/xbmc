# SWIG generates the CPython binding modules directly from the .i files here

find_package(SWIG REQUIRED ${SEARCH_QUIET})

# 4.5.0 is the release the fix-swig-3535.cmake patterns and the binding test suites are verified against
get_target_property(_swig_version SWIG::SWIG VERSION)
if(_swig_version VERSION_LESS 4.5.0)
  message(FATAL_ERROR "The python bindings require SWIG 4.5.0 or newer "
                      "(found ${_swig_version})")
endif()

function(generate_file file)
  set(CPP_FILE ${file}.cpp)

  # AddonModuleXbmcgui.i -> low-level module/init name "xbmcgui"
  get_filename_component(module_name ${file} NAME_WE)
  string(REPLACE "AddonModule" "" module_name ${module_name})
  string(TOLOWER ${module_name} module_name)

  # No -threads: wrappers hold the GIL; the legacy implementations release it
  # themselves through DelayedCallGuard where they block.
  # -interface names the init symbol PyInit_<module> for the invoker inittabs
  # -w401/514: AddonClass base is deliberately unreflected; 389: operator[]; 362: operator= in %imported Tuple.h
  # 302: kodi_generics.i declares Tuple/Alternative/Dictionary to carry their typemaps, so
  # SWIG ignores the header definitions it later sees. That is the intent, not a collision.
  # -pyi: PEP 484 stub files for addon authors, a build artifact beside the wrappers
  set(SWIG_ARGS -python -c++ -builtin -interface ${module_name} -pyi
                -w401,389,514,362,302 -o ${CPP_FILE}
                -I${CMAKE_SOURCE_DIR}/xbmc
                ${CMAKE_CURRENT_SOURCE_DIR}/../swig/${file})

  # This generates a file that has the dependencies the swig input finds
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

  # We set this to allow cmake to detect if changes in the headers are made
  # and to force a cmake reconfigure to detect new dependencies that may have been added.
  set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${swig_deps})

  add_custom_command(OUTPUT ${CPP_FILE}
                     COMMAND SWIG::SWIG
                     ARGS ${SWIG_ARGS}
                     COMMAND ${CMAKE_COMMAND}
                             -DFILE=${CMAKE_CURRENT_BINARY_DIR}/${CPP_FILE}
                             -DSWIG_VERSION=${_swig_version}
                             -P ${CMAKE_CURRENT_SOURCE_DIR}/fix-swig-3535.cmake
                     DEPENDS SWIG::SWIG ${swig_deps}
                             ${CMAKE_CURRENT_SOURCE_DIR}/fix-swig-3535.cmake)

  set(SOURCES ${SOURCES} "${CPP_FILE}" PARENT_SCOPE)
endfunction()

# The generated bindings
set(INPUTS AddonModuleXbmcaddon.i
           AddonModuleXbmcdrm.i
           AddonModuleXbmcgui.i
           AddonModuleXbmc.i
           AddonModuleXbmcplugin.i
           AddonModuleXbmcvfs.i)

# The wsgi module is only used by the web server's WSGI invoker.
if(TARGET ${APP_NAME_LC}::MicroHttpd)
  list(APPEND INPUTS AddonModuleXbmcwsgi.i)
endif()

foreach(INPUT IN LISTS INPUTS)
  generate_file(${INPUT})
endforeach()

# swigpyrun.h gives SwigRuntime.cpp the swig_module_info definition matching the generated modules
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/swigpyrun.h
                   COMMAND SWIG::SWIG
                   ARGS -python -c++ -external-runtime ${CMAKE_CURRENT_BINARY_DIR}/swigpyrun.h
                   DEPENDS SWIG::SWIG)
list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/SwigRuntime.cpp
                    ${CMAKE_CURRENT_BINARY_DIR}/swigpyrun.h)

add_library(python_binding STATIC ${SOURCES})
target_include_directories(python_binding PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(python_binding PROPERTIES POSITION_INDEPENDENT_CODE TRUE
                                                FOLDER "Build Utilities")

# director-enabled modules in one binary otherwise collide on Swig::Director symbols
# SWIG_LINK_RUNTIME: modules find the shared type table via SwigRuntime.cpp instead of a Python capsule owned by one interpreter
target_compile_definitions(python_binding PRIVATE SWIG_DIRECTOR_STATIC SWIG_LINK_RUNTIME)

if(CORE_SYSTEM_NAME STREQUAL windowsstore)
  set_target_properties(python_binding PROPERTIES STATIC_LIBRARY_FLAGS "/ignore:4264")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # all three fire inside SWIG-generated code only (the last is the SWIG_DIRECTOR_STATIC effect)
  set_target_properties(python_binding PROPERTIES
                        COMPILE_FLAGS "-Wno-cast-function-type -Wno-double-promotion -Wno-subobject-linkage")
endif()

# Add target dependencies to lib
core_target_link_libraries(python_binding)
# Link this target lib to core
target_link_libraries(lib${APP_NAME_LC} PUBLIC python_binding)
