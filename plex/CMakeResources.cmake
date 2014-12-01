# find all image files in the skin
file(GLOB MEDIA_IMAGES_PLEX_SKIN ${root}/addons/skin.plex/Media/*.png ${root}/addons/skin.plex/Media/*.gif)

if(COMPRESS_TEXTURES AND NOT TARGET_RPI)
  # Build the packed textures
  if(WIN32)
    set(WORKDIR ${root}/tools/TexturePacker)
  else(WIN32)
    set(WORKDIR ${CMAKE_CURRENT_BINARY_DIR})
  endif(WIN32)

  if(TEXTUREPACKERPATH)
    set(TEXTUREPACKER_EXE ${TEXTUREPACKERPATH})
  else(TEXTUREPACKERPATH)
    set(TEXTUREPACKER_EXE $<TARGET_FILE:TexturePacker>)
  endif(TEXTUREPACKERPATH)

  add_custom_command(
    OUTPUT Textures.xbt
    COMMAND ${TEXTUREPACKER_EXE} -input ${root}/addons/skin.plex/Media -output ${CMAKE_CURRENT_BINARY_DIR}/Textures.xbt
    MAIN_DEPENDENCY ${MEDIA_IMAGES_PLEX_SKIN}
    DEPENDS TexturePacker
    WORKING_DIRECTORY ${WORKDIR}
  )
  add_custom_target(CompressTextures ALL DEPENDS Textures.xbt)
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/Textures.xbt PROPERTIES GENERATED true MACOSX_PACKAGE_LOCATION Resources/XBMC/addons/skin.plex/Media)
  set_property(GLOBAL APPEND PROPERTY CONFIG_BUNDLED_FILES ${CMAKE_CURRENT_BINARY_DIR}/Textures.xbt)

  if(NOT TARGET_OSX)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Textures.xbt DESTINATION ${RESOURCEPATH}/addons/skin.plex/Media COMPONENT RUNTIME)
  endif()

  set(EXCLUDE_TEXTURES "skin.plex/Media/*")
else()
  set(EXCLUDE_TEXTURES ^foo)
endif()

if(NOT DEFINED PLEX_SPLASH)
  if(TARGET_RPI)
    set(PLEX_SPLASH Splash-RPI.png)
  else(TARGET_RPI)
    set(PLEX_SPLASH Splash.png)
  endif(TARGET_RPI)
endif(NOT DEFINED PLEX_SPLASH)


function(set_bundle_dir)
  set(args SOURCES DEST EXCLUDE)
  include(CMakeParseArguments)
  cmake_parse_arguments(BD "" "" "${args}" ${ARGN})

  foreach(_BDIR ${BD_SOURCES}) 
    file(GLOB _DIRCONTENTS ${_BDIR}/*)
    foreach(_BDFILE ${_DIRCONTENTS})
      get_filename_component(_BDFILE_NAME ${_BDFILE} NAME)

      set(PROCESS_FILE 1)
      foreach(EX_FILE ${BD_EXCLUDE})
        string(REGEX MATCH ${EX_FILE} DID_MATCH ${_BDFILE})
        if(NOT "${DID_MATCH}" STREQUAL "")
          set(PROCESS_FILE 0)
        endif(NOT "${DID_MATCH}" STREQUAL "")
      endforeach(EX_FILE ${BD_EXCLUDE})
      
      if(PROCESS_FILE STREQUAL "1")
        if(IS_DIRECTORY ${_BDFILE})
          set_bundle_dir(SOURCES ${_BDFILE} DEST ${BD_DEST}/${_BDFILE_NAME} EXCLUDE ${BD_EXCLUDE})
        else(IS_DIRECTORY ${_BDFILE})
          #message("set_bundle_dir : setting package_location ${_BDFILE} = ${BD_DEST}")
          set_source_files_properties(${_BDFILE} PROPERTIES MACOSX_PACKAGE_LOCATION ${BD_DEST})
          get_property(BUNDLED_FILES GLOBAL PROPERTY CONFIG_BUNDLED_FILES)
          set_property(GLOBAL PROPERTY CONFIG_BUNDLED_FILES ${BUNDLED_FILES} ${_BDFILE})
        endif(IS_DIRECTORY ${_BDFILE})
      endif()
    endforeach(_BDFILE ${_DIRCONTENTS})
  endforeach(_BDIR ${BD_SOURCES})
endfunction(set_bundle_dir)

if(TARGET_COMMON_DARWIN)
  set_source_files_properties(${CONFIG_PLEX_INSTALL_LIBRARIES} PROPERTIES MACOSX_PACKAGE_LOCATION Frameworks)
  set_bundle_dir(SOURCES ${root}/media DEST Resources/XBMC/media EXCLUDE .*/Splash.png Credits.html)
  set_bundle_dir(SOURCES ${root}/sounds DEST Resources/XBMC/sounds)
  set_bundle_dir(SOURCES ${root}/language DEST Resources/XBMC/language)
  set_bundle_dir(SOURCES ${root}/system DEST Resources/XBMC/system EXCLUDE .*/keymaps.* .*/python/.* .*/playercorefactory.xml .*/peripherals.xml)
  set_bundle_dir(SOURCES ${plexdir}/addons DEST Resources/XBMC/addons)
  set_bundle_dir(SOURCES ${plexdir}/Resources/system DEST Resources/XBMC/system)

  set_bundle_dir(SOURCES ${root}/addons
                 DEST Resources/XBMC/addons
                 EXCLUDE .*/skin.confluence.*
                         .*/skin.touched.*
                         .*/screensaver.rsxs.*
                         .*/library.*
                         .*/metadata.*
                         .*/weather.*
                         .*/repository.*
                         .*/${EXCLUDE_TEXTURES}
                         .*/.git.*
                         .*/xbmc.python.*
                 )
  
  set(RESOURCE_FILES ${plexdir}/Resources/Plex.icns ${plexdir}/Resources/Credits.html ${MEDIA_RESOURCES})
  set_source_files_properties(${RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  set_source_files_properties(${plexdir}/Resources/com.plexapp.ht.helper.plist PROPERTIES MACOSX_PACKAGE_LOCATION Resources/XBMC/tools/darwin/runtime)

  set(MEDIA_FILES
    ${plexdir}/Resources/${PLEX_SPLASH}
    ${plexdir}/Resources/plex-icon-120.png
    ${plexdir}/Resources/plex-icon-256.png
    ${plexdir}/Resources/SlideshowOverlay.png
  )
  set_source_files_properties(${MEDIA_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/XBMC/media)

  get_property(BUNDLED_FILES GLOBAL PROPERTY CONFIG_BUNDLED_FILES)
  set_property(GLOBAL PROPERTY CONFIG_BUNDLED_FILES
    ${BUNDLED_FILES}
    ${RESOURCE_FILES}
    ${MEDIA_FILES}
    ${plexdir}/Resources/com.plexapp.ht.helper.plist
  )

else(TARGET_COMMON_DARWIN)
  install(FILES ${CONFIG_PLEX_INSTALL_LIBRARIES} DESTINATION ${LIBPATH} COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/Credits.html DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME)

  install(DIRECTORY ${root}/media ${root}/sounds ${root}/language DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME
          PATTERN ${PLEX_SPLASH} EXCLUDE
          PATTERN Credits.html EXCLUDE)

  install(DIRECTORY ${root}/system DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME
        PATTERN python EXCLUDE
        PATTERN keymaps EXCLUDE
        PATTERN playercorefactory.xml EXCLUDE
        PATTERN peripherals.xml EXCLUDE)

  install(DIRECTORY ${root}/addons DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME
          PATTERN skin.confluence EXCLUDE
          PATTERN skin.touched EXCLUDE
          REGEX screensaver.rsxs* EXCLUDE
          REGEX library.* EXCLUDE
          REGEX metadata.* EXCLUDE
          REGEX weather.* EXCLUDE
          PATTERN repository.xbmc.org EXCLUDE
          REGEX ${EXCLUDE_TEXTURES} EXCLUDE
          PATTERN .git EXCLUDE
          PATTERN xbmc.python EXCLUDE
  )

  install(FILES ${plexdir}/Resources/${PLEX_SPLASH} DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME RENAME ${PLEX_SPLASH})
  install(DIRECTORY ${plexdir}/addons DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/plex-icon-120.png DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/plex-icon-256.png DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/SlideshowOverlay.png DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME)
  install(DIRECTORY ${plexdir}/Resources/system/keymaps DESTINATION ${RESOURCEPATH}/system COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/system/peripherals.xml ${plexdir}/Resources/system/playercorefactory.xml ${plexdir}/Resources/system/plexca.pem ${plexdir}/Resources/system/cacert.pem DESTINATION ${RESOURCEPATH}/system COMPONENT RUNTIME)

  if(TARGET_WIN32)
    install(FILES ${root}/system/zlib1.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
    install(FILES ${root}/project/Win32BuildSetup/dependencies/libcdio-13.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
    install(FILES ${plexdir}/build/dependencies/vcredist/2012/vcredist_x86.exe DESTINATION ${BINPATH}/Dependencies COMPONENT VCREDIST RENAME vcredist_2012_x86.exe)
    install(FILES ${plexdir}/build/dependencies/vcredist/2010/vcredist_x86.exe DESTINATION ${BINPATH}/Dependencies COMPONENT VCREDIST RENAME vcredist_2010_x86.exe)
    install(DIRECTORY ${plexdir}/build/dependencies/dxsetup DESTINATION ${BINPATH}/Dependencies COMPONENT QDXSETUP)
    install(FILES ${root}/project/Win32BuildSetup/dependencies/glew32.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
    install(FILES ${root}/project/Win32BuildSetup/dependencies/libiconv-2.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
    install(FILES ${plexdir}/Resources/Plex.ico ${plexdir}/Resources/PlexBanner.bmp DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME)
  endif(TARGET_WIN32)
endif(TARGET_COMMON_DARWIN)
