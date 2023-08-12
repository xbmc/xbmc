function(generate_file file)
  set(classpath ${GROOVY_DIR}/groovy-${GROOVY_VER}.jar
                ${GROOVY_DIR}/groovy-xml-${GROOVY_VER}.jar
                ${GROOVY_DIR}/groovy-templates-${GROOVY_VER}.jar
                ${GROOVY_DIR}/commons-lang3-${COMMONS_LANG_VER}.jar
                ${GROOVY_DIR}/commons-text-${COMMONS_TEXT_VER}.jar
                ${CMAKE_SOURCE_DIR}/tools/codegenerator
                ${CMAKE_CURRENT_SOURCE_DIR}/../python)
  if(NOT CORE_SYSTEM_NAME STREQUAL windows AND NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
    set(devnull "/dev/null")
    string(REPLACE ";" ":" classpath "${classpath}")
  else()
    set(devnull "nul")
  endif()

  set(CPP_FILE ${file}.cpp)
  if(CLANGFORMAT_FOUND)
    set(CLANG_FORMAT_COMMAND COMMAND ${CLANG_FORMAT_EXECUTABLE} ARGS -i ${CPP_FILE})
  endif()

  if(Java_VERSION_MAJOR GREATER 8)
    set(JAVA_OPEN_OPTS --add-opens java.base/java.util=ALL-UNNAMED --add-opens java.base/java.util.regex=ALL-UNNAMED --add-opens java.base/java.io=ALL-UNNAMED --add-opens java.base/java.lang=ALL-UNNAMED  --add-opens java.base/java.net=ALL-UNNAMED)
  endif()

  add_custom_command(OUTPUT ${CPP_FILE}
                     COMMAND ${SWIG_EXECUTABLE}
                     ARGS -w401 -c++ -o ${file}.xml -xml -I${CMAKE_SOURCE_DIR}/xbmc -xmllang python ${CMAKE_CURRENT_SOURCE_DIR}/../swig/${file}
                     COMMAND ${Java_JAVA_EXECUTABLE}
                     ARGS ${JAVA_OPEN_OPTS} -cp "${classpath}" groovy.ui.GroovyMain ${CMAKE_SOURCE_DIR}/tools/codegenerator/Generator.groovy ${file}.xml ${CMAKE_CURRENT_SOURCE_DIR}/../python/PythonSwig.cpp.template ${file}.cpp > ${devnull}
                     ${CLANG_FORMAT_COMMAND}
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../swig/${file} ${CMAKE_CURRENT_SOURCE_DIR}/../python/PythonSwig.cpp.template)
  set(SOURCES ${SOURCES} "${CPP_FILE}" PARENT_SCOPE)
endfunction()

find_package(Java COMPONENTS Runtime REQUIRED)
find_package(SWIG REQUIRED)

# The generated bindings
set(INPUTS AddonModuleXbmcaddon.i
           AddonModuleXbmcdrm.i
           AddonModuleXbmcgui.i
           AddonModuleXbmc.i
           AddonModuleXbmcplugin.i
           AddonModuleXbmcvfs.i
           AddonModuleXbmcwsgi.i)

set(GROOVY_DIR ${CMAKE_SOURCE_DIR}/tools/codegenerator/groovy)
set(GROOVY_VER 4.0.6)
set(COMMONS_LANG_VER 3.12.0)
set(COMMONS_TEXT_VER 1.10.0)

foreach(INPUT IN LISTS INPUTS)
  generate_file(${INPUT})
  list(APPEND GEN_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${INPUT}.cpp)
endforeach()

add_library(python_binding STATIC ${SOURCES})
set_target_properties(python_binding PROPERTIES POSITION_INDEPENDENT_CODE TRUE
                                                FOLDER "Build Utilities")
set(core_DEPENDS python_binding ${core_DEPENDS} CACHE STRING "" FORCE)
add_dependencies(python_binding ${GLOBAL_TARGET_DEPS})
# This propagates target options from dependencies (eg spdlog definitions)
target_link_libraries(python_binding PRIVATE ${GLOBAL_TARGET_DEPS})

if(CORE_SYSTEM_NAME STREQUAL windowsstore)
  set_target_properties(python_binding PROPERTIES STATIC_LIBRARY_FLAGS "/ignore:4264")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  set_target_properties(python_binding PROPERTIES
                        COMPILE_FLAGS -Wno-cast-function-type) # from -Wextra
endif()
