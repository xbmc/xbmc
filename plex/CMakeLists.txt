
include_directories(
	${root}/xbmc/cores
	${root}/xbmc/guilib
	${root}/xbmc/network
	${root}/xbmc/music
	${root}/xbmc/filesystem
	${root}/xbmc/threads
	${root}/xbmc/video
	${root}/xbmc/music/tags
	${root}/xbmc/dialogs
	${root}/xbmc/pictures
	${root}/xbmc/playlists
	${root}/plex/FileSystem
	${root}/plex/Network
	${root}/plex/Remote
	${root}/plex/Filters
  ${root}/plex/Third-Party/rapidxml
  ${root}/plex/Playlists
  ${CONFIG_INCLUDE_PATH_SPARKLE}
)

if(ENABLE_AUTOUPDATE)
  if (NOT TARGET_RPI)
    include(ExternalProject)
    ExternalProject_Add(Update-Installer
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR} -DCMAKE_OSX_ARCHITECTURES=${OSX_ARCH}
      SOURCE_DIR "${plexdir}/Update-Installer"
      INSTALL_DIR "${CMAKE_BINARY_DIR}"
    )
  endif(NOT TARGET_RPI)
  if(TARGET_OSX)
    set_source_files_properties(${CMAKE_BINARY_DIR}/update_installer/updater PROPERTIES GENERATED true MACOSX_PACKAGE_LOCATION Resources/XBMC/tools)
    set_property(GLOBAL APPEND PROPERTY CONFIG_BUNDLED_FILES ${CMAKE_BINARY_DIR}/update_installer/updater)
  else(TARGET_OSX)
    install(FILES ${CMAKE_BINARY_DIR}/update_installer/updater.exe
            PERMISSIONS WORLD_READ WORLD_EXECUTE OWNER_WRITE OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ
            DESTINATION ${BINPATH} COMPONENT RUNTIME)
  endif(TARGET_OSX)

  add_subdirectory(AutoUpdate)
endif(ENABLE_AUTOUPDATE)

if(TARGET_WIN32)
  # disable safe seh
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO /DELAYLOAD:dnssd.dll /DELAYLOAD:dwmapi.dll /DELAYLOAD:libmicrohttpd-5.dll /DELAYLOAD:sqlite3.dll /DELAYLOAD:libsamplerate-0.dll /MANIFEST:NO")
endif(TARGET_WIN32)

OPTION(USE_RAPIDXML "Use RAPIDXML instead of TinyXML - Faster" ON)
if(USE_RAPIDXML)
  add_definitions(-DUSE_RAPIDXML)
endif(USE_RAPIDXML)
 
if(HAVE_BREAKPAD)
  if(NOT TARGET_WIN32)
   list(APPEND CONFIG_PLEX_LINK_LIBRARIES ${LIBBREAKPAD_CLIENT})
  endif(NOT TARGET_WIN32)
endif(HAVE_BREAKPAD)

add_subdirectory(CrashReporter)
add_subdirectory(Client)
add_subdirectory(FileSystem)
add_subdirectory(GUI)
add_subdirectory(Home)
add_subdirectory(Network)
add_subdirectory(Owned)
add_subdirectory(Utility)
add_subdirectory(Remote)
add_subdirectory(Filters)
add_subdirectory(Playlists)
add_subdirectory(Windows)
add_subdirectory(Third-Party)
add_subdirectory(Tests)

include(BreakpadSymbols)

if(TARGET_WIN32)
  add_subdirectory(Resources/MCE)
endif(TARGET_WIN32)

if(TARGET_OSX)
  add_subdirectory(Helper)
endif(TARGET_OSX)

find_all_sources(. plex_SRCS)

list(REMOVE_ITEM plex_SRCS ./pch_plex.cpp)

set(Headers PlexTypes.h)
if(NOT TARGET_OSX)
  list(REMOVE_ITEM plex_SRCS ./PlexMacUtils.mm)
endif(NOT TARGET_OSX)

if(NOT TARGET_WIN32)
  list(REMOVE_ITEM plex_SRCS ./MediaCenterLaunchHost.cpp)
else(NOT TARGET_WIN32)
  set(WIN32_SRC ${root}/xbmc/win32/Win32DelayedDllLoad.cpp)
endif(NOT TARGET_WIN32)

get_property(PLEX_MODULE_SRCS GLOBAL PROPERTY SRCS_LIST)
set(PLEX_ALL_SRCS ${PLEX_MODULE_SRCS} ${plex_SRCS} ${Headers})

ADD_MSVC_PRECOMPILED_HEADER("pch_plex.h" "pch_plex.cpp" PLEX_ALL_SRCS)

# build all plex related source files
add_library(PlexMain OBJECT ${PLEX_ALL_SRCS})
set_target_properties(PlexMain PROPERTIES
  XCODE_ATTRIBUTE_GCC_PREFIX_HEADER ${plexdir}/pch_plex.h
  XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES
  XCODE_ATTRIBUTE_GCC_INCREASE_PRECOMPILED_HEADER_SHARING YES
  XCODE_ATTRIBUTE_WARNING_CFLAGS "")
if(USE_INTERNAL_FFMPEG)
  add_dependencies(PlexMain ffmpeg Update-Installer)
endif(USE_INTERNAL_FFMPEG)

# build test cases
if(ENABLE_TESTING)
  get_property(TEST_CASES GLOBAL PROPERTY PLEX_TEST_CASES)
  foreach(cases ${TEST_CASES})
    message(STATUS "Adding test case ${cases}")
    list(APPEND TEST_CASES_OBJS $<TARGET_OBJECTS:${cases}>)
  endforeach(cases ${TEST_CASES})
  add_executable(PlexHomeTheaterTests ${TEST_CASES_OBJS} $<TARGET_OBJECTS:xbmctest> $<TARGET_OBJECTS:PlexMain> Tests/PlexTestUtils.cpp Tests/PlexTestUtils.h)
  target_link_libraries(PlexHomeTheaterTests xbmc pcre gtest gmock ${CONFIG_INTERNAL_LIBS} ${CONFIG_PLEX_LINK_LIBRARIES})
  add_test(PlexHomeTheaterTests ${EXECUTABLE_OUTPUT_PATH}/PlexHomeTheaterTests --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/PlexHomeTheaterTests.xml --gtest_color=yes)

  add_custom_target(all_tests DEPENDS PlexHomeTheaterTests)
  add_custom_target(run_tests COMMAND ${CMAKE_CTEST_COMMAND} -V DEPENDS all_tests)
endif(ENABLE_TESTING)

find_program(LIZARD lizard HINTS /usr/local/bin)
if(NOT LIZARD MATCHES "-NOTFOUND")
  add_custom_command(
    OUTPUT lizard-output.xml
    COMMAND ${LIZARD} --xml -t 4 ${plexdir} -x"${plexdir}/Dependencies/*" -x"${plexdir}/Update-Installer/*" -x"${plexdir}/Third-Party/*" > ${CMAKE_CURRENT_BINARY_DIR}/lizard-output.xml
  )
  add_custom_command(
    OUTPUT lizard-stdout
    COMMAND ${LIZARD} -w -t 4 ${plexdir} -x"${plexdir}/Dependencies/*" -x"${plexdir}/Update-Installer/*" -x"${plexdir}/Third-Party/*"
  )

  add_custom_target(lizard DEPENDS lizard-output.xml)
  add_custom_target(lizard-warnings DEPENDS lizard-stdout)
endif(NOT LIZARD MATCHES "-NOTFOUND")

include(CMakeResources.cmake)

# main target
if(TARGET_COMMON_DARWIN)
  get_property(BUNDLED_FILES GLOBAL PROPERTY CONFIG_BUNDLED_FILES)
  add_executable(${PLEX_TARGET_NAME} MACOSX_BUNDLE ${root}/xbmc/main/main.cpp
                                                   ${root}/xbmc/main/osx/SDLMain.mm
                                                   ${root}/xbmc/main/osx/SDLMain.h
                                                   $<TARGET_OBJECTS:PlexMain>
                                                   ${BUNDLED_FILES}
                                                   ${CONFIG_PLEX_INSTALL_LIBRARIES})
  set_target_properties(${PLEX_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${plexdir}/Resources/Info.plist.in")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PLEX_VERSION_STRING}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PLEX_VERSION_STRING_SHORT}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${PLEX_VERSION_STRING}")
  install(TARGETS ${PLEX_TARGET_NAME} BUNDLE DESTINATION . COMPONENT RUNTIME)
else()
  add_executable(${PLEX_TARGET_NAME} WIN32 $<TARGET_OBJECTS:PlexMain> $<TARGET_OBJECTS:xbmcmain> ${WIN32_SRC} Resources/Plex.rc Resources/Plex.rc2)
  install(TARGETS ${PLEX_TARGET_NAME} RUNTIME DESTINATION ${BINPATH} COMPONENT RUNTIME)
endif()

# Set the correct binary name
set_target_properties(${PLEX_TARGET_NAME} PROPERTIES OUTPUT_NAME ${EXECUTABLE_NAME})
set_target_properties(${PLEX_TARGET_NAME} PROPERTIES PDB_NAME "Plex Home Theater")

if(ENABLE_DUMP_SYMBOLS)
  message(STATUS "Generating breakpad symbols")
  GENERATE_BREAKPAD_SYMBOLS(${PLEX_TARGET_NAME})
  add_custom_target(symbols DEPENDS ${PLEX_TARGET_NAME}_symbols)
endif(ENABLE_DUMP_SYMBOLS)

target_link_libraries(${PLEX_TARGET_NAME} xbmc pcre ${CONFIG_INTERNAL_LIBS} ${CONFIG_PLEX_LINK_LIBRARIES})

if(TARGET_WIN32)
  add_custom_command(
    TARGET ${PLEX_TARGET_NAME}
	POST_BUILD
	COMMAND "mt.exe" -manifest "${plexdir}/Resources/pht.manifest" -outputresource:"$<TARGET_FILE:${PLEX_TARGET_NAME}>"
	COMMENT "Embedding manifest")
endif(TARGET_WIN32)

if(ENABLE_PYTHON)
  target_link_libraries(${PLEX_TARGET_NAME} addons_python)
endif(ENABLE_PYTHON)


# Set some variables we need to configure the CMakeCompleteBundle file
set(PLEXDIR ${plexdir})
set(ROOTDIR ${root})
set(DEPENDDIR ${dependdir})
configure_file(CMakeCompleteBundle.cmake.in CMakeCompleteBundle.cmake @ONLY)


if(CREATE_BUNDLE)
  if(TARGET_COMMON_DARWIN OR TARGET_WIN32)
    install(SCRIPT ${CMAKE_BINARY_DIR}/plex/CMakeCompleteBundle.cmake COMPONENT RUNTIME)
  endif()
endif()
