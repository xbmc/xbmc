
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
  ${CONFIG_INCLUDE_PATH_SPARKLE}
)

add_subdirectory(FileSystem)
add_subdirectory(GUI)
add_subdirectory(Network)
add_subdirectory(Owned)
add_subdirectory(Players)
add_subdirectory(Utility)
if(APPLE)
  add_subdirectory(Helper)
endif(APPLE)
add_subdirectory(AutoUpdate)

find_all_sources(. plex_SRCS)

list(REMOVE_ITEM plex_SRCS ./pch_plex.cpp)
list(APPEND plex_SRCS Resources/Plex.rc)
list(APPEND plex_SRCS Resources/Plex.rc2)

set(Headers PlexTypes.h)
if(APPLE)
  list(REMOVE_ITEM plex_SRCS ./CocoaUtils.cpp)
endif(APPLE)

if(NOT APPLE)
  list(REMOVE_ITEM plex_SRCS ./CocoaUtils.m)
  list(REMOVE_ITEM plex_SRCS ./CocoaUtilsPlus.mm)
endif(NOT APPLE)

if(NOT WIN32)
  list(REMOVE_ITEM plex_SRCS ./MediaCenterLaunchHost.cpp)
endif(NOT WIN32)

get_property(PLEX_MODULE_SRCS GLOBAL PROPERTY SRCS_LIST)
set(PLEX_ALL_SRCS ${PLEX_MODULE_SRCS} ${plex_SRCS} ${Headers})

ADD_MSVC_PRECOMPILED_HEADER("pch_plex.h" "pch_plex.cpp" PLEX_ALL_SRCS)

if(APPLE)
  add_executable(${PLEX_TARGET_NAME} MACOSX_BUNDLE ${PLEX_ALL_SRCS})
  set_target_properties(${PLEX_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${plexdir}/Resources/Info.plist.in")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PLEX_VERSION_STRING}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PLEX_VERSION_STRING_SHORT}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${PLEX_VERSION_STRING}")
else()
  add_executable(${PLEX_TARGET_NAME} WIN32 ${PLEX_ALL_SRCS})
endif()

# Set the correct binary name
set_target_properties(${PLEX_TARGET_NAME} PROPERTIES OUTPUT_NAME ${EXECUTABLE_NAME})

add_dependencies(${PLEX_TARGET_NAME} git_revision.h)
target_link_libraries(${PLEX_TARGET_NAME} xbmc pcre ${CONFIG_INTERNAL_LIBS} ${CONFIG_PLEX_LINK_LIBRARIES})


# find all image files in the skin
file(GLOB MEDIA_IMAGES_MEDIASTREAM ${root}/addons/skin.mediastream/Media/*.png ${root}/addons/skin.mediastream/Media/*.gif)

if(NOT WIN32)
  # Build the packed textures
  if(DEFINED USING_XCODE)
    set(TEXTURE_PACKER_BIN ${CMAKE_BINARY_DIR}/tools/TexturePacker/Debug/TexturePacker)
  else()
    set(TEXTURE_PACKER_BIN ${CMAKE_BINARY_DIR}/tools/TexturePacker/TexturePacker)
  endif()
  add_custom_command(
    OUTPUT Textures_MediaStream.xbt
    COMMAND ${TEXTURE_PACKER_BIN} -input ${root}/addons/skin.mediastream/Media -output Textures_MediaStream.xbt
    MAIN_DEPENDENCY ${MEDIA_IMAGES_MEDIASTREAM}
    DEPENDS TexturePacker
  )

  if(PLEX_SKIP_BUNDLING STREQUAL "1")
    set(EXCLUDE_TEXTURES ^foo)
  else()
    add_custom_target(CompressTextures ALL DEPENDS Textures_MediaStream.xbt)
    set(EXCLUDE_TEXTURES skin.mediastream/Media/*)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Textures_MediaStream.xbt DESTINATION ${RESOURCEPATH}/addons/skin.mediastream/Media RENAME Textures.xbt COMPONENT RUNTIME)
  endif()
else()
  set(EXCLUDE_TEXTURES ^foo)
endif()

# Set some variables we need to configure the CMakeCompleteBundle file
set(PLEXDIR ${plexdir})
set(ROOTDIR ${root})
set(DEPENDDIR ${dependdir})
configure_file(CMakeCompleteBundle.cmake.in CMakeCompleteBundle.cmake @ONLY)

if(APPLE OR WIN32)
  install(FILES ${CONFIG_PLEX_INSTALL_LIBRARIES} DESTINATION ${LIBPATH} COMPONENT RUNTIME)
endif()

if(APPLE)
  install(TARGETS ${PLEX_TARGET_NAME} BUNDLE DESTINATION . COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/Plex.icns DESTINATION ${EXECUTABLE_NAME}.app/Contents/Resources COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/Credits.html DESTINATION ${EXECUTABLE_NAME}.app/Contents/Resources COMPONENT RUNTIME)
else()
  install(TARGETS ${PLEX_TARGET_NAME} RUNTIME DESTINATION ${BINPATH} COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/Credits.html DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME)
endif()

install(DIRECTORY ${root}/media ${root}/sounds ${root}/language DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME
        PATTERN Splash.png EXCLUDE
        PATTERN Credits.html EXCLUDE)

install(DIRECTORY ${root}/system DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME
        PATTERN python EXCLUDE
        PATTERN peripherals.xml EXCLUDE)

install(DIRECTORY ${root}/addons DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME
        PATTERN skin.confluence EXCLUDE
        PATTERN skin.touched EXCLUDE
        REGEX screensaver.rsxs* EXCLUDE
        REGEX library.* EXCLUDE
        REGEX metadata.* EXCLUDE
        REGEX weather.* EXCLUDE
        PATTERN repository.xbmc.org EXCLUDE
        REGEX visualization.* EXCLUDE
        REGEX ${EXCLUDE_TEXTURES} EXCLUDE
        PATTERN .git EXCLUDE
        PATTERN xbmc.python EXCLUDE)

install(DIRECTORY ${plexdir}/addons DESTINATION DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME)
install(FILES ${plexdir}/Resources/Splash.png DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME)
install(FILES ${plexdir}/Resources/plex-icon-120.png DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME)
install(FILES ${plexdir}/Resources/plex-icon-256.png DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME)
install(FILES ${plexdir}/Resources/peripherals.xml DESTINATION ${RESOURCEPATH}/system COMPONENT RUNTIME)

if(APPLE)
  install(FILES ${plexdir}/Resources/com.plexapp.ht.helper.plist DESTINATION ${RESOURCEPATH}/tools/darwin/runtime COMPONENT RUNTIME)
elseif(WIN32)
  install(FILES ${root}/system/zlib1.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
  install(FILES ${plexdir}/build/dependencies/vcredist/2010/vcredist_x86.exe DESTINATION ${BINPATH}/Dependencies COMPONENT VCREDIST)
  install(DIRECTORY ${plexdir}/build/dependencies/dxsetup DESTINATION ${BINPATH}/Dependencies COMPONENT QDXSETUP)
  install(FILES ${root}/project/Win32BuildSetup/dependencies/glew32.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
  install(FILES ${root}/project/Win32BuildSetup/dependencies/libiconv-2.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/Plex.ico ${plexdir}/Resources/PlexBanner.bmp DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME)
endif()

if(APPLE)
  add_custom_command(
    OUTPUT "${EXECUTABLE_NAME}.dSYM"
    COMMAND dsymutil -o "${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_NAME}.dSYM" "${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_NAME}.app/Contents/MacOS/${EXECUTABLE_NAME}"
    DEPENDS ${PLEX_TARGET_NAME}
  )
  add_custom_command(
    OUTPUT "${EXECUTABLE_NAME}.dSYM.zip"
    COMMAND ditto -c -k "${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_NAME}.dSYM" "${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_NAME}.dSYM.zip"
    DEPENDS "${EXECUTABLE_NAME}.dSYM"
  )
  add_custom_target(Symbols DEPENDS "${EXECUTABLE_NAME}.dSYM.zip")
endif()

if(NOT PLEX_SKIP_BUNDLING STREQUAL "1")
  if(APPLE OR WIN32)
    install(SCRIPT ${CMAKE_BINARY_DIR}/plex/CMakeCompleteBundle.cmake COMPONENT RUNTIME)
  endif()
  if(APPLE)
    add_custom_target(BundlingSymbol ALL DEPENDS Symbols)
  endif()
else()
  message("-- Skipping bundling ${PLEX_SKIP_BUNDLING}")
endif()
