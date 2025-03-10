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

  add_custom_command(OUTPUT ${CPP_FILE}
                     COMMAND SWIG::SWIG
                     ARGS ${SWIG_ARGS}
                     COMMAND ${Java_JAVA_EXECUTABLE}
                     ARGS ${JAVA_OPEN_OPTS} -cp "${classpath}" groovy.ui.GroovyMain ${CMAKE_SOURCE_DIR}/tools/codegenerator/Generator.groovy ${file}.xml ${CMAKE_CURRENT_SOURCE_DIR}/../python/PythonSwig.cpp.template ${file}.cpp > ${devnull}
                     ${CLANG_FORMAT_COMMAND}
                     DEPENDS SWIG::SWIG ${swig_deps} ${CMAKE_CURRENT_SOURCE_DIR}/../python/PythonSwig.cpp.template
                     BYPRODUCTS ${CMAKE_BINARY_DIR}/build/swig/${file}.xml)

  set(SOURCES ${SOURCES} "${CPP_FILE}" PARENT_SCOPE)
endfunction()

find_package(Java COMPONENTS Runtime REQUIRED)
find_package(SWIG REQUIRED)

# Default URL/Versions/Hashes to use unless *_SOURCE_DIR are set for the relevant binary
set(GROOVY_VER 4.0.26)
set(GROOVY_URL ${KODI_MIRROR}/build-deps/sources/apache-groovy-binary-${GROOVY_VER}.zip)
set(GROOVY_URL_HASH URL_HASH SHA512=60d1aea85008a438a9498178dc2137385653f1ea294e7e257959e9e0f94382e160e217e1090047c2a944e328cf46e8ebcfc4b015f7d0cb9f2fcf9d880c6c1344)

set(APACHE_COMMONS_LANG_VER 3.17.0)
set(APACHE_COMMONS_LANG_URL ${KODI_MIRROR}/build-deps/sources/commons-lang3-${APACHE_COMMONS_LANG_VER}-bin.tar.gz)
set(APACHE_COMMONS_LANG_URL_HASH URL_HASH SHA512=8927a406a1bd084b548f89cf15fe4bc7567dcaa50ae8abae54e9c883c1574c648fcc8ad6c3abaa381dfbdf1801727ac5e6c4572063203a1dfac293d122282f05)

set(APACHE_COMMONS_TEXT_VER 1.13.0)
set(APACHE_COMMONS_TEXT_URL ${KODI_MIRROR}/build-deps/sources/commons-text-${APACHE_COMMONS_TEXT_VER}-bin.tar.gz)
set(APACHE_COMMONS_TEXT_URL_HASH URL_HASH SHA512=a51667463e88b2d017c3baebb9bbe42c4e50cba96ffe19c89680e82002a4db8b9007daff75f8dbf8bf3725be13a3f03f3c2bc4e1f11e8e02dcab4408abdb44a1)

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
endforeach()

add_library(python_binding STATIC ${SOURCES})
set_target_properties(python_binding PROPERTIES POSITION_INDEPENDENT_CODE TRUE
                                                FOLDER "Build Utilities")

if(CORE_SYSTEM_NAME STREQUAL windowsstore)
  set_target_properties(python_binding PROPERTIES STATIC_LIBRARY_FLAGS "/ignore:4264")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  set_target_properties(python_binding PROPERTIES
                        COMPILE_FLAGS -Wno-cast-function-type) # from -Wextra
endif()

# Add target dependencies to lib
core_target_link_libraries(python_binding)
# Link this target lib to core
target_link_libraries(lib${APP_NAME_LC} PUBLIC python_binding)
