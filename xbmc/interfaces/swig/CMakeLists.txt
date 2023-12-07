function(generate_file file)
  set(classpath ${groovy_SOURCE_DIR}/lib/*
                ${apache-commons-lang_SOURCE_DIR}/*
                ${apache-commons-text_SOURCE_DIR}/*
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

# Default URL/Versions/Hashes to use unless *_SOURCE_DIR are set for the relevant binary
set(GROOVY_VER 4.0.16)
set(GROOVY_URL ${KODI_MIRROR}/build-deps/sources/apache-groovy-binary-${GROOVY_VER}.zip)
set(GROOVY_URL_HASH URL_HASH SHA512=743698979c801f3cfb36545888b5303e1e25dae5692bb45cab7ebbe102a6eda31f09abc6978349ad90ecaf630416fc87eacba7781244855e7e771a3c44041fa0)

set(APACHE_COMMONS_LANG_VER 3.14.0)
set(APACHE_COMMONS_LANG_URL ${KODI_MIRROR}/build-deps/sources/commons-lang3-${APACHE_COMMONS_LANG_VER}-bin.tar.gz)
set(APACHE_COMMONS_LANG_URL_HASH URL_HASH SHA512=8e7e62418a49ba810512c13a640a8bf35f878fcd54af32fdaab1111e37817f58b21b475980ba663fba4887e45ef8d88af8ff17796f20d202e929e8e2574546dc)

set(APACHE_COMMONS_TEXT_VER 1.11.0)
set(APACHE_COMMONS_TEXT_URL ${KODI_MIRROR}/build-deps/sources/commons-text-${APACHE_COMMONS_TEXT_VER}-bin.tar.gz)
set(APACHE_COMMONS_TEXT_URL_HASH URL_HASH SHA512=2e94877000dd270b69e2e8cbf49f258a90b4c628b6b6b0814e300a2f0e9c391f0816dceb0707e596ae3b7c9532f93e7a4917df47c77f44b3a810e14042ce5f3f)

include(FetchContent)
FetchContent_Declare(
  groovy
  URL ${GROOVY_URL}
  ${GROOVY_URL_HASH}
  DOWNLOAD_DIR ${TARBALL_DIR}
  SOURCE_DIR ${DEPENDS_PATH}/share/groovy
)

FetchContent_Declare(
  apache-commons-lang
  URL ${APACHE_COMMONS_LANG_URL}
  ${APACHE_COMMONS_LANG_URL_HASH}
  DOWNLOAD_DIR ${TARBALL_DIR}
  SOURCE_DIR ${DEPENDS_PATH}/share/java/lang
)

FetchContent_Declare(
  apache-commons-text
  URL ${APACHE_COMMONS_TEXT_URL}
  ${APACHE_COMMONS_TEXT_URL_HASH}
  DOWNLOAD_DIR ${TARBALL_DIR}
  SOURCE_DIR ${DEPENDS_PATH}/share/java/text
)

if(NOT groovy_SOURCE_DIR)
  if(NOT EXISTS ${DEPENDS_PATH}/share/groovy/lib/groovy-${GROOVY_VER}.jar OR
     NOT EXISTS ${DEPENDS_PATH}/share/groovy/lib/groovy-xml-${GROOVY_VER}.jar OR
     NOT EXISTS ${DEPENDS_PATH}/share/groovy/lib/groovy-templates-${GROOVY_VER}.jar)
    FetchContent_MakeAvailable(groovy)
  else()
    set(groovy_SOURCE_DIR ${DEPENDS_PATH}/share/groovy)
  endif()
endif()

if(NOT apache-commons-lang_SOURCE_DIR)
  if(NOT EXISTS ${DEPENDS_PATH}/share/java/lang/commons-lang3-${APACHE_COMMONS_LANG_VER}.jar)
    FetchContent_MakeAvailable(apache-commons-lang)
  else()
    set(apache-commons-lang_SOURCE_DIR ${DEPENDS_PATH}/share/java/lang)
  endif()
endif()

if(NOT apache-commons-text_SOURCE_DIR)
  if(NOT EXISTS ${DEPENDS_PATH}/share/java/text/commons-text-${APACHE_COMMONS_TEXT_VER}.jar)
    FetchContent_MakeAvailable(apache-commons-text)
  else()
    set(apache-commons-text_SOURCE_DIR ${DEPENDS_PATH}/share/java/text)
  endif()
endif()

# The generated bindings
set(INPUTS AddonModuleXbmcaddon.i
           AddonModuleXbmcdrm.i
           AddonModuleXbmcgui.i
           AddonModuleXbmc.i
           AddonModuleXbmcplugin.i
           AddonModuleXbmcvfs.i
           AddonModuleXbmcwsgi.i)

set(GROOVY_DIR ${CMAKE_SOURCE_DIR}/tools/codegenerator/groovy)

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
