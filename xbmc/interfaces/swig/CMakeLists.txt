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

find_package(Java COMPONENTS Runtime REQUIRED ${SEARCH_QUIET})
find_package(SWIG REQUIRED ${SEARCH_QUIET})

# Default URL/Versions/Hashes to use unless *_SOURCE_DIR are set for the relevant binary
set(GROOVY_VER 4.0.30)
set(GROOVY_ARCHIVE apache-groovy-binary-${GROOVY_VER}.zip)
set(GROOVY_URL ${KODI_MIRROR}/build-deps/sources/${GROOVY_ARCHIVE})
set(GROOVY_URL_HASH SHA512=789e956ecad3c302d545a16caccd45454357556b566ee83abfafe102371cf218f5ed5dc83d748595fbb2d465bde6c7306a498806e58b512cfc742abe5f5b101d)

set(APACHE_COMMONS_LANG_VER 3.20.0)
set(APACHE_COMMONS_LANG_ARCHIVE commons-lang3-${APACHE_COMMONS_LANG_VER}-bin.tar.gz)
set(APACHE_COMMONS_LANG_URL ${KODI_MIRROR}/build-deps/sources/${APACHE_COMMONS_LANG_ARCHIVE})
set(APACHE_COMMONS_LANG_URL_HASH SHA512=eead2c04a85c85cfc83ef8a858f954e7abf668367d5a42e1152eb5d656eb8f4c1d5e7ad03f46859e99393e92b0d5fc69c927301c1a2ca466a3c49fb1f186ce89)

set(APACHE_COMMONS_TEXT_VER 1.15.0)
set(APACHE_COMMONS_TEXT_ARCHIVE commons-text-${APACHE_COMMONS_TEXT_VER}-bin.tar.gz)
set(APACHE_COMMONS_TEXT_URL ${KODI_MIRROR}/build-deps/sources/${APACHE_COMMONS_TEXT_ARCHIVE})
set(APACHE_COMMONS_TEXT_URL_HASH SHA512=6d1fcbd85a85f38e95ea33c9ab7f58798723b0e6357077246a135c417e75c7b39e278ebdb77fcc8f9b56b52b1eeff113eea9463eaaaee8d38df9afdc149b6421)

if(NOT groovy_SOURCE_DIR)
  # file wont be downloaded if EXPECTED_HASH matches existing archive
  file(DOWNLOAD ${GROOVY_URL} ${TARBALL_DIR}/${GROOVY_ARCHIVE} SHOW_PROGRESS EXPECTED_HASH ${GROOVY_URL_HASH})

  set(groovy_exe_name groovy)
  if(WIN32 OR WINDOWS_STORE)
    string(APPEND groovy_exe_name ".bat")
  endif()

  find_program(groovy_executable NAMES ${groovy_exe_name}
                                 HINTS ${DEPENDS_PATH}/share/groovy/bin)

  if(groovy_executable)
    execute_process(COMMAND "${groovy_executable}" --version
                    OUTPUT_VARIABLE groovy_VERSION_output
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

      string(REGEX REPLACE ".*Groovy Version: [^0-9.]*\([0-9.]+\).*" "\\1"
             groovy_VERSION "${groovy_VERSION_output}")
  endif()

  if("${groovy_VERSION}" VERSION_LESS "${GROOVY_VER}")
    if(VERBOSE OR VERBOSE_FIND)
      message(STATUS "Extracting Groovy (${GROOVY_VER})")
    endif()

    # Cleanup any potential conflicting dirs
    file(REMOVE_RECURSE "${DEPENDS_PATH}/share/groovy")
    file(ARCHIVE_EXTRACT INPUT "${TARBALL_DIR}/${GROOVY_ARCHIVE}" DESTINATION "${DEPENDS_PATH}/share")
    file(RENAME "${DEPENDS_PATH}/share/groovy-${GROOVY_VER}" "${DEPENDS_PATH}/share/groovy")
  endif()

  set(groovy_SOURCE_DIR ${DEPENDS_PATH}/share/groovy)
endif()

if(NOT apache-commons-lang_SOURCE_DIR)
  # file wont be downloaded if EXPECTED_HASH matches existing archive
  file(DOWNLOAD ${APACHE_COMMONS_LANG_URL} ${TARBALL_DIR}/${APACHE_COMMONS_LANG_ARCHIVE} SHOW_PROGRESS EXPECTED_HASH ${APACHE_COMMONS_LANG_URL_HASH})

  if(NOT EXISTS ${DEPENDS_PATH}/share/java/lang/commons-lang3-${APACHE_COMMONS_LANG_VER}.jar)

    if(VERBOSE OR VERBOSE_FIND)
      message(STATUS "Extracting apache-commons-lang3 (${APACHE_COMMONS_LANG_VER})")
    endif()

    # Cleanup any potential conflicting dirs
    file(REMOVE_RECURSE "${DEPENDS_PATH}/share/java/lang")
    file(ARCHIVE_EXTRACT INPUT "${TARBALL_DIR}/${APACHE_COMMONS_LANG_ARCHIVE}" DESTINATION "${DEPENDS_PATH}/share/java")
    file(RENAME "${DEPENDS_PATH}/share/java/commons-lang3-${APACHE_COMMONS_LANG_VER}" "${DEPENDS_PATH}/share/java/lang")
  endif()

  set(apache-commons-lang_SOURCE_DIR ${DEPENDS_PATH}/share/java/lang)
endif()

if(NOT apache-commons-text_SOURCE_DIR)
  # file wont be downloaded if EXPECTED_HASH matches existing archive
  file(DOWNLOAD ${APACHE_COMMONS_TEXT_URL} ${TARBALL_DIR}/${APACHE_COMMONS_TEXT_ARCHIVE} SHOW_PROGRESS EXPECTED_HASH ${APACHE_COMMONS_TEXT_URL_HASH})

  if(NOT EXISTS ${DEPENDS_PATH}/share/java/text/commons-text-${APACHE_COMMONS_TEXT_VER}.jar)

    if(VERBOSE OR VERBOSE_FIND)
      message(STATUS "Extracting apache-commons-text (${APACHE_COMMONS_TEXT_VER})")
    endif()

    # Cleanup any potential conflicting dirs
    file(REMOVE_RECURSE "${DEPENDS_PATH}/share/java/text")
    file(ARCHIVE_EXTRACT INPUT "${TARBALL_DIR}/${APACHE_COMMONS_TEXT_ARCHIVE}" DESTINATION "${DEPENDS_PATH}/share/java")
    file(RENAME "${DEPENDS_PATH}/share/java/commons-text-${APACHE_COMMONS_TEXT_VER}" "${DEPENDS_PATH}/share/java/text")
  endif()

  set(apache-commons-text_SOURCE_DIR ${DEPENDS_PATH}/share/java/text)
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
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set_target_properties(python_binding PROPERTIES
                        COMPILE_FLAGS -Wno-cast-function-type) # from -Wextra
endif()

# Add target dependencies to lib
core_target_link_libraries(python_binding)
# Link this target lib to core
target_link_libraries(lib${APP_NAME_LC} PUBLIC python_binding)
