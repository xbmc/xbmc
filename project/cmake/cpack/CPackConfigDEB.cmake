# include Macros.cmake to automate generation of time/date stamps, maintainer, etc.
include(${PROJECT_SOURCE_DIR}/scripts/common/Macros.cmake)

# find stuff we need
find_program(LSB_RELEASE_CMD lsb_release)
find_program(DPKG_CMD dpkg)
find_package(Git)
find_program(GZIP_CMD gzip)

# set packaging dir
if(NOT CPACK_PACKAGE_DIRECTORY)
  set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR}/packages)
endif()

# set architecture
if(NOT CPACK_SYSTEM_NAME)
  set(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_PROCESSOR})
  # sanity check
  if(CPACK_SYSTEM_NAME STREQUAL x86_64)
    set(CPACK_SYSTEM_NAME amd64)
  endif()
endif()

# set packaging by components
set(CPACK_DEB_COMPONENT_INSTALL ON)

# enforce Debian policy permission rules
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)

# packaging by components doesn't fully work with CMake/CPack <3.6.0
# CPACK_DEBIAN_<COMPONENT>_FILE_NAME is a 3.6.0 addition
# warn if detected version is lower
if(CMAKE_VERSION VERSION_LESS 3.6)
  message(WARNING "DEB Generator: CMake/CPack 3.6 or higher is needed to produce correctly named packages.")
endif()

# distro codename
if(NOT DISTRO_CODENAME)
  if(NOT LSB_RELEASE_CMD)
    message(WARNING "DEB Generator: Can't find lsb_release in your path. Setting DISTRO_CODENAME to unknown.")
    set(DISTRO_CODENAME unknown)
  else()
    execute_process(COMMAND ${LSB_RELEASE_CMD} -cs
                    OUTPUT_VARIABLE DISTRO_CODENAME
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
endif()

# package version
if(DEBIAN_PACKAGE_VERSION)
  set(DISTRO_CODENAME ${DEBIAN_PACKAGE_VERSION}${DISTRO_CODENAME})
else()
  set(DISTRO_CODENAME 0${DISTRO_CODENAME})
endif()

# package revision
if(DEBIAN_PACKAGE_REVISION)
  set(DISTRO_CODENAME ${DISTRO_CODENAME}${DEBIAN_PACKAGE_REVISION})
endif()

# package type
if(DEBIAN_PACKAGE_TYPE STREQUAL stable)
  set(RELEASE_IDENTIFIER final)
elseif(DEBIAN_PACKAGE_TYPE STREQUAL unstable)
  set(RELEASE_IDENTIFIER ${APP_VERSION_TAG_LC})
else()
  set(RELEASE_IDENTIFIER ${GIT_HASH})
endif()

# package name
string(TIMESTAMP PACKAGE_TIMESTAMP "%Y%m%d.%H%M" UTC)
set(PACKAGE_NAME_VERSION ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}~git${PACKAGE_TIMESTAMP}-${RELEASE_IDENTIFIER}-${DISTRO_CODENAME})
unset(RELEASE_IDENTIFIER)

# package version
if(DEBIAN_PACKAGE_EPOCH)
  set(CPACK_DEBIAN_PACKAGE_VERSION ${DEBIAN_PACKAGE_EPOCH}:${PACKAGE_NAME_VERSION})
else()
  set(CPACK_DEBIAN_PACKAGE_VERSION 2:${PACKAGE_NAME_VERSION})
endif()

# architecture
if(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
  if(NOT DPKG_CMD)
    message(WARNING "DEB Generator: Can't find dpkg in your path. Setting CPACK_DEBIAN_PACKAGE_ARCHITECTURE to i386.")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
  endif()
  execute_process(COMMAND "${DPKG_CMD}" --print-architecture
                  OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

# package maintainer
if(NOT CPACK_DEBIAN_PACKAGE_MAINTAINER)
  userstamp()
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${PACKAGE_MAINTAINER})
  unset(PACKAGE_MAINTAINER)
endif()

# package description common to all packages
if(NOT CPACK_DEBIAN_PACKAGE_DESCRIPTION)
  file(STRINGS ${PROJECT_SOURCE_DIR}/cpack/deb/package-description.txt DESC_LINES)
  foreach(LINE IN LISTS DESC_LINES)
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${CPACK_DEBIAN_PACKAGE_DESCRIPTION} ${LINE}\n")
  endforeach()
endif()

# package homepage
if(NOT CPACK_DEBIAN_PACKAGE_HOMEPAGE)
  set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${APP_WEBSITE})
endif()

# generate a Debian compliant changelog
set(CHANGELOG_HEADER "${APP_NAME_LC} (${CPACK_DEBIAN_PACKAGE_VERSION}) ${DISTRO_CODENAME}\; urgency=high")
rfc2822stamp()
# two spaces between maintainer and timestamp is NOT a mistake
set(CHANGELOG_FOOTER " -- ${CPACK_DEBIAN_PACKAGE_MAINTAINER}  ${RFC2822_TIMESTAMP}")

if(GIT_FOUND AND GZIP_CMD AND EXISTS ${CORE_SOURCE_DIR}/.git)
  execute_process(COMMAND ${GIT_EXECUTABLE} log --no-merges --pretty=format:"%n  [%an]%n   * %s" --since="last month"
                  OUTPUT_VARIABLE CHANGELOG
                  WORKING_DIRECTORY ${CORE_SOURCE_DIR}
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE "\"" "" CHANGELOG ${CHANGELOG})
  file(WRITE ${CPACK_PACKAGE_DIRECTORY}/deb/changelog.Debian ${CHANGELOG_HEADER}\n${CHANGELOG}\n\n${CHANGELOG_FOOTER})
  execute_process(COMMAND ${GZIP_CMD} -f -9 -n ${CPACK_PACKAGE_DIRECTORY}/deb/changelog.Debian)
  unset(CHANGELOG_HEADER)
  unset(CHANGELOG_FOOTER)
  unset(RFC2822_TIMESTAMP)
else()
  message(WARNING "DEB Generator: Can't find git and/or gzip in your path. DEB packages will be missing changelog.Debian.gz.")
endif()

# Generate NEWS.Debian
configure_file(${PROJECT_SOURCE_DIR}/cpack/deb/NEWS.Debian
               ${CPACK_PACKAGE_DIRECTORY}/deb/NEWS.Debian @ONLY)
if(GZIP_CMD)
  execute_process(COMMAND ${GZIP_CMD} -f -9 -n ${CPACK_PACKAGE_DIRECTORY}/deb/NEWS.Debian)
else()
  message(WARNING "DEB Generator: Can't find gzip in your path. DEB packages will be missing NEWS.Debian.")
endif()

# Generate man pages
configure_file(${CORE_SOURCE_DIR}/docs/manpages/kodi.bin.1
               ${CPACK_PACKAGE_DIRECTORY}/deb/kodi.1 COPYONLY)
configure_file(${CORE_SOURCE_DIR}/docs/manpages/kodi.bin.1
               ${CPACK_PACKAGE_DIRECTORY}/deb/kodi.bin.1 COPYONLY)
configure_file(${CORE_SOURCE_DIR}/docs/manpages/kodi-standalone.1
               ${CPACK_PACKAGE_DIRECTORY}/deb/kodi-standalone.1 COPYONLY)
if(ENABLE_EVENTCLIENTS)
  configure_file(${CORE_SOURCE_DIR}/docs/manpages/kodi-ps3remote.1
                 ${CPACK_PACKAGE_DIRECTORY}/deb/kodi-ps3remote.1 COPYONLY)
  configure_file(${CORE_SOURCE_DIR}/docs/manpages/kodi-send.1
                 ${CPACK_PACKAGE_DIRECTORY}/deb/kodi-send.1 COPYONLY)
  configure_file(${CORE_SOURCE_DIR}/docs/manpages/kodi-wiiremote.1
                 ${CPACK_PACKAGE_DIRECTORY}/deb/kodi-wiiremote.1 COPYONLY)
endif()

if(GZIP_CMD)
  execute_process(COMMAND ${GZIP_CMD} -f -9 -n ${CPACK_PACKAGE_DIRECTORY}/deb/kodi.1)
  execute_process(COMMAND ${GZIP_CMD} -f -9 -n ${CPACK_PACKAGE_DIRECTORY}/deb/kodi.bin.1)
  execute_process(COMMAND ${GZIP_CMD} -f -9 -n ${CPACK_PACKAGE_DIRECTORY}/deb/kodi-standalone.1)
  if(ENABLE_EVENTCLIENTS)
    execute_process(COMMAND ${GZIP_CMD} -f -9 -n ${CPACK_PACKAGE_DIRECTORY}/deb/kodi-ps3remote.1)
    execute_process(COMMAND ${GZIP_CMD} -f -9 -n ${CPACK_PACKAGE_DIRECTORY}/deb/kodi-send.1)
    execute_process(COMMAND ${GZIP_CMD} -f -9 -n ${CPACK_PACKAGE_DIRECTORY}/deb/kodi-wiiremote.1)
  endif()
else()
  message(WARNING "DEB Generator: Can't find gzip in your path. Several DEB packages will be missing man pages.")
endif()

install(FILES ${CPACK_PACKAGE_DIRECTORY}/deb/kodi.1.gz
              ${CPACK_PACKAGE_DIRECTORY}/deb/kodi.bin.1.gz
              ${CPACK_PACKAGE_DIRECTORY}/deb/kodi-standalone.1.gz
        DESTINATION share/man/man1
        COMPONENT kodi)
if(ENABLE_EVENTCLIENTS)
install(FILES ${CPACK_PACKAGE_DIRECTORY}/deb/kodi-ps3remote.1.gz
        DESTINATION share/man/man1
        COMPONENT kodi-eventclients-ps3)
install(FILES ${CPACK_PACKAGE_DIRECTORY}/deb/kodi-send.1.gz
        DESTINATION share/man/man1
        COMPONENT kodi-eventclients-xbmc-send)
install(FILES ${CPACK_PACKAGE_DIRECTORY}/deb/kodi-wiiremote.1.gz
        DESTINATION share/man/man1
        COMPONENT kodi-eventclients-wiiremote)
endif()

# configure package metadata files
file(GLOB DEBIAN_PACKAGE_FILES ${PROJECT_SOURCE_DIR}/cpack/deb/packages/*.txt.in)
foreach(file ${DEBIAN_PACKAGE_FILES})
  get_filename_component(package ${file} NAME_WE)
  # filter eventclients so we don't have to support two more deps
  # (libbluetooth-dev and libcwiid-dev) just because of wii-remote
  string(SUBSTRING ${package} 0 17 PACKAGE_FILTER)
  if(NOT ENABLE_EVENTCLIENTS AND PACKAGE_FILTER STREQUAL kodi-eventclients)
    message(STATUS "DEB Generator: ${package} matches ${PACKAGE_FILTER}, skipping.")
    # do nothing
  else()
    configure_file(${file}
                   ${CPACK_PACKAGE_DIRECTORY}/deb/${package}.txt @ONLY)
    list(APPEND DEBIAN_PACKAGES ${package})
  endif()
endforeach()
unset(DEBIAN_PACKAGE_FILES)

# generate packages
include(CMakeParseArguments)
foreach(file ${DEBIAN_PACKAGES})
  core_file_read_filtered(DEBIAN_METADATA ${CPACK_PACKAGE_DIRECTORY}/deb/${file}.txt)
  string(REPLACE " " ";" DEBIAN_METADATA "${DEBIAN_METADATA}")
  cmake_parse_arguments(DEB
                        ""
                        "PACKAGE_NAME;PACKAGE_ARCHITECTURE;PACKAGE_SECTION;PACKAGE_PRIORITY;PACKAGE_SHLIBDEPS"
                        "PACKAGE_DEPENDS;PACKAGE_RECOMMENDS;PACKAGE_SUGGESTS;PACKAGE_BREAKS;PACKAGE_REPLACES;PACKAGE_PROVIDES;PACKAGE_DESCRIPTION_HEADER;PACKAGE_DESCRIPTION_FOOTER"
                        ${DEBIAN_METADATA})
  string(REPLACE ";" " " DEB_PACKAGE_DEPENDS "${DEB_PACKAGE_DEPENDS}")
  string(REPLACE ";" " " DEB_PACKAGE_RECOMMENDS "${DEB_PACKAGE_RECOMMENDS}")
  string(REPLACE ";" " " DEB_PACKAGE_SUGGESTS "${DEB_PACKAGE_SUGGESTS}")
  string(REPLACE ";" " " DEB_PACKAGE_BREAKS "${DEB_PACKAGE_BREAKS}")
  string(REPLACE ";" " " DEB_PACKAGE_REPLACES "${DEB_PACKAGE_REPLACES}")
  string(REPLACE ";" " " DEB_PACKAGE_PROVIDES "${DEB_PACKAGE_PROVIDES}")
  string(REPLACE ";" " " DEB_PACKAGE_DESCRIPTION_HEADER "${DEB_PACKAGE_DESCRIPTION_HEADER}")
  string(REPLACE ";" " " DEB_PACKAGE_DESCRIPTION_FOOTER "${DEB_PACKAGE_DESCRIPTION_FOOTER}")

  string(TOUPPER ${file} COMPONENT)

  if(NOT DEB_PACKAGE_ARCHITECTURE)
    message(STATUS "DEB Generator: Mandatory variable CPACK_DEBIAN_${COMPONENT}_PACKAGE_ARCHITECTURE is empty. Setting to ${CPACK_SYSTEM_NAME}.")
    set(CPACK_DEBIAN_${COMPONENT}_PACKAGE_ARCHITECTURE ${CPACK_SYSTEM_NAME})
  else()
    set(CPACK_DEBIAN_${COMPONENT}_PACKAGE_ARCHITECTURE ${DEB_PACKAGE_ARCHITECTURE})
  endif()

  if(DEB_PACKAGE_NAME)
    if(DEB_PACKAGE_ARCHITECTURE)
      set(CPACK_DEBIAN_${COMPONENT}_FILE_NAME ${DEB_PACKAGE_NAME}_${PACKAGE_NAME_VERSION}_${DEB_PACKAGE_ARCHITECTURE}.deb)
    else()
      set(CPACK_DEBIAN_${COMPONENT}_FILE_NAME ${DEB_PACKAGE_NAME}_${PACKAGE_NAME_VERSION}_${CPACK_SYSTEM_NAME}.deb)
    endif()
  else()
    message(FATAL_ERROR "DEB Generator: Mandatory variable CPACK_DEBIAN_${COMPONENT}_FILE_NAME is not set.")
  endif()

  set(CPACK_DEBIAN_${COMPONENT}_PACKAGE_SOURCE ${APP_NAME_LC})

  if(DEB_PACKAGE_NAME)
    set(CPACK_DEBIAN_${COMPONENT}_PACKAGE_NAME ${DEB_PACKAGE_NAME})
  else()
    message(FATAL_ERROR "DEB Generator: Mandatory variable CPACK_DEBIAN_${COMPONENT}_PACKAGE_NAME is not set.")
  endif()

  if(DEB_PACKAGE_SECTION)
    set(CPACK_DEBIAN_${COMPONENT}_PACKAGE_SECTION ${DEB_PACKAGE_SECTION})
  else()
    message(FATAL_ERROR "DEB Generator: Mandatory variable CPACK_DEBIAN_${COMPONENT}_PACKAGE_SECTION is not set.")
  endif()

  if(DEB_PACKAGE_PRIORITY)
    set(CPACK_DEBIAN_${COMPONENT}_PACKAGE_PRIORITY ${DEB_PACKAGE_PRIORITY})
  else()
    message(FATAL_ERROR "DEB Generator: Mandatory variable CPACK_DEBIAN_${COMPONENT}_PACKAGE_PRIORITY is not set.")
  endif()

  if(DEB_PACKAGE_SHLIBDEPS)
    set(CPACK_DEBIAN_${COMPONENT}_PACKAGE_SHLIBDEPS ON)
  else()
    if(DEB_PACKAGE_DEPENDS)
      set(CPACK_DEBIAN_${COMPONENT}_PACKAGE_DEPENDS "${DEB_PACKAGE_DEPENDS}")
    endif()
  endif()

  if(DEB_PACKAGE_RECOMMENDS)
    set(CPACK_DEBIAN_${COMPONENT}_PACKAGE_RECOMMENDS "${DEB_PACKAGE_RECOMMENDS}")
  endif()

  if(DEB_PACKAGE_SUGGESTS)
    set(CPACK_DEBIAN_${COMPONENT}_PACKAGE_SUGGESTS "${DEB_PACKAGE_SUGGESTS}")
  endif()

  if(DEB_PACKAGE_BREAKS)
    set(CPACK_DEBIAN_${COMPONENT}_PACKAGE_BREAKS "${DEB_PACKAGE_BREAKS}")
  endif()

  if(DEB_PACKAGE_REPLACES)
    set(CPACK_DEBIAN_${COMPONENT}_PACKAGE_REPLACES "${DEB_PACKAGE_REPLACES}")
  endif()

  if(DEB_PACKAGE_PROVIDES)
    set(CPACK_DEBIAN_${COMPONENT}_PACKAGE_PROVIDES "${DEB_PACKAGE_PROVIDES}")
  endif()

  if(NOT DEB_PACKAGE_DESCRIPTION_HEADER OR NOT DEB_PACKAGE_DESCRIPTION_FOOTER)
    message(FATAL_ERROR "DEB Generator: Mandatory variable CPACK_COMPONENT_${COMPONENT}_DESCRIPTION is not set.")
  else()
    set(CPACK_COMPONENT_${COMPONENT}_DESCRIPTION "\
${DEB_PACKAGE_DESCRIPTION_HEADER}\n\
${CPACK_DEBIAN_PACKAGE_DESCRIPTION} \
${DEB_PACKAGE_DESCRIPTION_FOOTER}")
  endif()

  install(FILES ${CPACK_PACKAGE_DIRECTORY}/deb/changelog.Debian.gz
                ${CPACK_PACKAGE_DIRECTORY}/deb/NEWS.Debian.gz
                ${PROJECT_SOURCE_DIR}/cpack/deb/copyright
          DESTINATION share/doc/${file}
          COMPONENT ${file})

  # kodi package exclusive files
  if(CPACK_DEBIAN_KODI_PACKAGE_NAME)
    set(CPACK_DEBIAN_KODI_PACKAGE_CONTROL_EXTRA
        "${PROJECT_SOURCE_DIR}/cpack/deb/postinst;${PROJECT_SOURCE_DIR}/cpack/deb/postrm")
    install(FILES ${PROJECT_SOURCE_DIR}/cpack/deb/lintian/overrides/kodi
        DESTINATION share/lintian/overrides
        COMPONENT kodi)
    install(FILES ${PROJECT_SOURCE_DIR}/cpack/deb/menu/kodi
        DESTINATION share/menu
        COMPONENT kodi)
  endif()
endforeach()
unset(DEBIAN_PACKAGES)

### source package generation specific variables
# source generator
set(CPACK_SOURCE_GENERATOR TGZ)

# source package name
set(CPACK_SOURCE_PACKAGE_FILE_NAME ${APP_NAME_LC}_${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}~git${PACKAGE_TIMESTAMP}-${GIT_HASH}.orig)

# source dir
set(CMAKE_SOURCE_DIR ${CORE_SOURCE_DIR})

# ignore files for source package
set(CPACK_SOURCE_IGNORE_FILES
  "/build/"
  "/debian/"
  "/.git/"
  ".gitignore"
  "yml$"
  "~$")

# unset variables
unset(PACKAGE_TIMESTAMP)
unset(DISTRO_CODENAME)

# reference docs
# https://cmake.org/cmake/help/latest/module/CPack.html
# https://cmake.org/cmake/help/latest/module/CPackDeb.html
# https://cmake.org/cmake/help/latest/module/CPackComponent.html
include(CPack)
