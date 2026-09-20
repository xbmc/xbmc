# SWIG generates the CPython binding modules directly from the .i files here

find_package(SWIG REQUIRED ${SEARCH_QUIET})

# 4.5.0 is the release the fix-swig-3535.cmake patterns and the binding test suites are verified against
get_target_property(_swig_version SWIG::SWIG VERSION)
if(_swig_version VERSION_LESS 4.5.0)
  message(FATAL_ERROR "The python bindings require SWIG 4.5.0 or newer "
                      "(found ${_swig_version}). -DENABLE_INTERNAL_SWIG=ON builds a suitable one.")
endif()

function(generate_file file)
  set(CPP_FILE ${file}.cpp)
  set(DEP_FILE ${CMAKE_CURRENT_BINARY_DIR}/${CPP_FILE}.d)

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
  # -MMD/-MF/-MT: header dependencies as a depfile, so swig is not needed at configure
  # time (ENABLE_INTERNAL_SWIG builds it during the build)
  set(SWIG_ARGS -python -c++ -builtin -interface ${module_name} -pyi
                -w401,389,514,362,302 -o ${CPP_FILE}
                -MMD -MF ${DEP_FILE} -MT ${CMAKE_CURRENT_BINARY_DIR}/${CPP_FILE}
                -I${CMAKE_SOURCE_DIR}/xbmc
                ${CMAKE_CURRENT_SOURCE_DIR}/${file})

  add_custom_command(OUTPUT ${CPP_FILE}
                     COMMAND SWIG::SWIG
                     ARGS ${SWIG_ARGS}
                     COMMAND ${CMAKE_COMMAND}
                             -DFILE=${CMAKE_CURRENT_BINARY_DIR}/${CPP_FILE}
                             -DSWIG_VERSION=${_swig_version}
                             -P ${CMAKE_CURRENT_SOURCE_DIR}/fix-swig-3535.cmake
                     DEPENDS SWIG::SWIG ${CMAKE_CURRENT_SOURCE_DIR}/${file}
                             ${CMAKE_CURRENT_SOURCE_DIR}/fix-swig-3535.cmake
                     DEPFILE ${DEP_FILE})

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
  # all fire inside SWIG-generated code only
  target_compile_options(python_binding PRIVATE -Wno-cast-function-type -Wno-double-promotion
                                                -Wno-unused-but-set-variable)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  # the SWIG_DIRECTOR_STATIC effect; GCC-only warning, clang has no such option
  target_compile_options(python_binding PRIVATE -Wno-subobject-linkage)
endif()

# Add target dependencies to lib
core_target_link_libraries(python_binding)
# Link this target lib to core
target_link_libraries(lib${APP_NAME_LC} PUBLIC python_binding)
