function(generate_file file)
  set(classpath ${GROOVY_DIR}/groovy-all-${GROOVY_VER}.jar
                ${GROOVY_DIR}/commons-lang-${COMMONS_VER}.jar
                ${CMAKE_SOURCE_DIR}/tools/codegenerator
                ${CMAKE_CURRENT_SOURCE_DIR}/../python)
  if(NOT CORE_SYSTEM_NAME STREQUAL windows)
    set(devnull "/dev/null")
    string(REPLACE ";" ":" classpath "${classpath}")
  else()
    set(devnull "nul")
  endif()

	set(CPP_FILE ${file}.cpp)
execute_process(
				COMMAND ${SWIG_EXECUTABLE} -w401 -c++ -o ${CMAKE_CURRENT_BINARY_DIR}/${file}.xml -xml -I${CMAKE_SOURCE_DIR}/xbmc -xmllang python ${CMAKE_CURRENT_SOURCE_DIR}/../swig/${file}
				COMMAND ${Java_JAVA_EXECUTABLE} -cp "${classpath}" groovy.ui.GroovyMain ${CMAKE_SOURCE_DIR}/tools/codegenerator/Generator.groovy ${CMAKE_CURRENT_BINARY_DIR}/${file}.xml ${CMAKE_CURRENT_SOURCE_DIR}/../python/PythonSwig.cpp.template ${CMAKE_CURRENT_BINARY_DIR}/${file}.cpp
				)
	set(SOURCES ${SOURCES} "${CMAKE_CURRENT_BINARY_DIR}/${CPP_FILE}" PARENT_SCOPE)
endfunction()

find_package(Java COMPONENTS Runtime REQUIRED)
find_package(SWIG REQUIRED)

# The generated bindings
set(INPUTS AddonModuleXbmcaddon.i
           AddonModuleXbmcgui.i
           AddonModuleXbmc.i
           AddonModuleXbmcplugin.i
           AddonModuleXbmcvfs.i
           AddonModuleXbmcwsgi.i)

set(GROOVY_DIR ${CMAKE_SOURCE_DIR}/tools/codegenerator/groovy)
set(GROOVY_VER 2.4.4)
set(COMMONS_VER 2.6)

foreach(INPUT IN LISTS INPUTS)
  generate_file(${INPUT})
  list(APPEND GEN_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${INPUT}.cpp)
endforeach()

core_add_library(python_binding)
