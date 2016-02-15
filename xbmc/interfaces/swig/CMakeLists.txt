macro(generate_file file)
  set(classpath ${GROOVY_DIR}/groovy-all-${GROOVY_VER}.jar
                ${GROOVY_DIR}/commons-lang-${COMMONS_VER}.jar
                ${CORE_SOURCE_DIR}/tools/codegenerator
                ${CMAKE_CURRENT_SOURCE_DIR}/../python)
  if(NOT CORE_SYSTEM_NAME STREQUAL windows)
    set(devnull "/dev/null")
    string(REPLACE ";" ":" classpath "${classpath}")
  else()
    set(devnull "nul")
  endif()

  add_custom_command(OUTPUT ${file}.cpp
                     COMMAND ${SWIG_EXECUTABLE}
                     ARGS -w401 -c++ -o ${file}.xml -xml -I${CORE_SOURCE_DIR}/xbmc -xmllang python ${CMAKE_CURRENT_SOURCE_DIR}/../swig/${file}
                     COMMAND ${Java_JAVA_EXECUTABLE}
                     ARGS -cp "${classpath}" groovy.ui.GroovyMain ${CORE_SOURCE_DIR}/tools/codegenerator/Generator.groovy ${file}.xml ${CMAKE_CURRENT_SOURCE_DIR}/../python/PythonSwig.cpp.template ${file}.cpp ${CMAKE_CURRENT_BINARY_DIR}/doxygenxml > ${devnull}
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file} ${INTERFACE_FILES} ${CMAKE_CURRENT_BINARY_DIR}/.doxygen-gen
                     IMPLICIT_DEPENDS CXX ${CMAKE_CURRENT_SOURCE_DIR}/../swig/${file})
  list(APPEND SOURCES ${file}.cpp)
endmacro()

find_package(Java COMPONENTS Runtime REQUIRED)
find_package(SWIG REQUIRED)
find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(doxy_dir ${CMAKE_CURRENT_SOURCE_DIR}/../legacy)
  set(legacy_doxy_output_dir ${CMAKE_CURRENT_BINARY_DIR})
  configure_file(${doxy_dir}/Doxyfile.in Doxyfile @ONLY)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/.doxygen-gen
                     COMMAND ${DOXYGEN_EXECUTABLE}
                     ARGS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                     WORKING_DIRECTORY ${doxy_dir}
                     COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/.doxygen-gen
                     MAIN_DEPENDENCY ${doxy_dir}/Doxyfile.in)
else()
  message(STATUS "Warning: No doxygen installed. The Api will not have any docstrings.")
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/.doxygen-gen
                     COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/generated/doxygenxml
                     COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/.doxygen-gen)
endif()


# The generated bindings
set(INPUTS AddonModuleXbmcaddon.i
           AddonModuleXbmcgui.i
           AddonModuleXbmc.i
           AddonModuleXbmcplugin.i
           AddonModuleXbmcvfs.i
           AddonModuleXbmcwsgi.i)

set(GROOVY_DIR ${CORE_SOURCE_DIR}/tools/codegenerator/groovy)
set(GROOVY_VER 2.4.4)
set(COMMONS_VER 2.6)

file(GLOB INTERFACE_FILES ${CORE_SOURCE_DIR}/xbmc/lib/legacy)

set(SOURCES "")

foreach(INPUT ${INPUTS})
  generate_file(${INPUT})
endforeach()

core_add_library(python_binding)
add_dependencies(python_binding python_interface libcpluff)
