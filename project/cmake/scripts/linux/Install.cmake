if(X_FOUND)
  set(USE_X11 1)
else()
  set(USE_X11 0)
endif()
if(OPENGL_FOUND)
  set(USE_OPENGL 1)
else()
  set(USE_OPENGL 0)
endif()
if(OPENGLES_FOUND)
  set(USE_OPENGLES 1)
else()
  set(USE_OPENGLES 0)
endif()

# CMake config
set(APP_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib/${APP_NAME_LC})
set(APP_PREFIX ${CMAKE_INSTALL_PREFIX})
set(APP_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include/${APP_NAME_LC})
set(CXX11_SWITCH "-std=c++11")

# Set XBMC_STANDALONE_SH_PULSE so we can insert PulseAudio block into kodi-standalone
if(EXISTS ${CORE_SOURCE_DIR}/tools/Linux/kodi-standalone.sh.pulse)
  if(ENABLE_PULSEAUDIO AND PULSEAUDIO_FOUND)
    file(READ "${CORE_SOURCE_DIR}/tools/Linux/kodi-standalone.sh.pulse" pulse_content)
    set(XBMC_STANDALONE_SH_PULSE ${pulse_content})
  endif()
endif()

# Configure startup scripts
configure_file(${CORE_SOURCE_DIR}/tools/Linux/kodi.sh.in
               ${CORE_BUILD_DIR}/scripts/${APP_NAME_LC} @ONLY)
configure_file(${CORE_SOURCE_DIR}/tools/Linux/kodi-standalone.sh.in
               ${CORE_BUILD_DIR}/scripts/${APP_NAME_LC}-standalone @ONLY)

# Configure cmake files
configure_file(${PROJECT_SOURCE_DIR}/KodiConfig.cmake.in
               ${CORE_BUILD_DIR}/scripts/${APP_NAME}Config.cmake @ONLY)

# Configure xsession entry
configure_file(${CORE_SOURCE_DIR}/tools/Linux/kodi-xsession.desktop.in
               ${CORE_BUILD_DIR}/${APP_NAME_LC}.desktop @ONLY)

# Install app
install(TARGETS ${APP_NAME_LC}
        DESTINATION lib/${APP_NAME_LC}
        COMPONENT kodi-bin)
if(ENABLE_X11 AND XRANDR_FOUND)
  install(TARGETS ${APP_NAME_LC}-xrandr
          DESTINATION lib/${APP_NAME_LC}
          COMPONENT kodi-bin)
endif()

# Install scripts
install(PROGRAMS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/scripts/${APP_NAME_LC}
                 ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/scripts/${APP_NAME_LC}-standalone
        DESTINATION bin
        COMPONENT kodi)

# Install libraries
foreach(library ${LIBRARY_FILES})
  get_filename_component(dir ${library} PATH)
  string(REPLACE "${CMAKE_BINARY_DIR}/" "" dir ${dir})
  install(PROGRAMS ${library}
          DESTINATION lib/${APP_NAME_LC}/${dir}
          COMPONENT kodi-bin)
endforeach()

# Install add-ons, fonts, icons, keyboard maps, keymaps, etc
# (addons, media, system, userdata folders in share/kodi/)
foreach(file ${install_data})
  get_filename_component(dir ${file} PATH)
  install(FILES ${CMAKE_BINARY_DIR}/${file}
          DESTINATION share/${APP_NAME_LC}/${dir}
          COMPONENT kodi)
endforeach()

# Install xsession entry
install(FILES ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${APP_NAME_LC}.desktop
        DESTINATION share/xsessions
        COMPONENT kodi)

# Install desktop entry
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/kodi.desktop
        DESTINATION share/applications
        COMPONENT kodi)

# Install icons
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon16x16.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/16x16/apps
        COMPONENT kodi)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon22x22.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/22x22/apps
        COMPONENT kodi)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon24x24.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/24x24/apps
        COMPONENT kodi)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon32x32.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/32x32/apps
        COMPONENT kodi)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon48x48.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/48x48/apps
        COMPONENT kodi)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon64x64.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/64x64/apps
        COMPONENT kodi)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon128x128.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/128x128/apps
        COMPONENT kodi)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon256x256.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/256x256/apps
        COMPONENT kodi)
install(CODE "execute_process(COMMAND gtk-update-icon-cache -f -q -t
        $ENV{DESTDIR}${datarootdir}/icons/hicolor ERROR_QUIET)"
        COMPONENT kodi)

# Install docs
install(FILES ${CORE_SOURCE_DIR}/copying.txt
              ${CORE_SOURCE_DIR}/LICENSE.GPL
              ${CORE_SOURCE_DIR}/version.txt
              ${CORE_SOURCE_DIR}/docs/README.linux
        DESTINATION share/doc/${APP_NAME_LC}
        COMPONENT kodi)

# Install kodi-tools-texturepacker
install(PROGRAMS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/texturepacker/TexturePacker
        DESTINATION bin
        COMPONENT kodi-tools-texturepacker)

# Install kodi-addon-dev headers
install(FILES ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/kodi_vfs_types.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/libKODI_adsp.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/libKODI_audioengine.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/libKODI_guilib.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/libKODI_inputstream.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/libKODI_peripheral.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/libXBMC_addon.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/libXBMC_codec.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/libXBMC_pvr.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/xbmc_addon_cpp_dll.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/xbmc_addon_dll.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/xbmc_addon_types.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/xbmc_codec_types.h
              ${CORE_SOURCE_DIR}/xbmc/cores/VideoPlayer/DVDDemuxers/DVDDemuxPacket.h
              ${CORE_SOURCE_DIR}/xbmc/filesystem/IFileTypes.h
        DESTINATION include/${APP_NAME_LC}
        COMPONENT kodi-addon-dev)

# Install kodi-addon-dev add-on bindings
install(FILES ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/scripts/${APP_NAME}Config.cmake
              ${CORE_SOURCE_DIR}/project/cmake/scripts/common/AddonHelpers.cmake
              ${CORE_SOURCE_DIR}/project/cmake/scripts/common/AddOptions.cmake
              ${CORE_SOURCE_DIR}/project/cmake/scripts/common/ArchSetup.cmake
              ${CORE_SOURCE_DIR}/project/cmake/scripts/common/CheckCommits.cmake
              ${CORE_SOURCE_DIR}/project/cmake/scripts/common/CheckTargetPlatform.cmake
              ${CORE_SOURCE_DIR}/project/cmake/scripts/common/GenerateVersionedFiles.cmake
              ${CORE_SOURCE_DIR}/project/cmake/scripts/common/GeneratorSetup.cmake
              ${CORE_SOURCE_DIR}/project/cmake/scripts/common/HandleDepends.cmake
              ${CORE_SOURCE_DIR}/project/cmake/scripts/common/Macros.cmake
              ${CORE_SOURCE_DIR}/project/cmake/scripts/common/PrepareEnv.cmake
              ${CORE_SOURCE_DIR}/project/cmake/scripts/common/ProjectMacros.cmake
              ${CORE_SOURCE_DIR}/project/cmake/scripts/linux/PathSetup.cmake
        DESTINATION lib/${APP_NAME_LC}
        COMPONENT kodi-addon-dev)

# Install kodi-audio-dev
install(FILES ${CORE_SOURCE_DIR}/xbmc/cores/AudioEngine/Utils/AEChannelData.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/kodi_adsp_dll.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/kodi_adsp_types.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/kodi_audiodec_dll.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/kodi_audiodec_types.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/xbmc_audioenc_dll.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/xbmc_audioenc_types.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/kodi_audioengine_types.h
        DESTINATION include/${APP_NAME_LC}
        COMPONENT kodi-audio-dev)

if(ENABLE_EVENTCLIENTS)
  # Install kodi-eventclients-common BT python files
  install(PROGRAMS ${CORE_SOURCE_DIR}/tools/EventClients/lib/python/bt/__init__.py
                   ${CORE_SOURCE_DIR}/tools/EventClients/lib/python/bt/bt.py
                   ${CORE_SOURCE_DIR}/tools/EventClients/lib/python/bt/hid.py
          DESTINATION lib/python2.7/dist-packages/${APP_NAME_LC}/bt
          COMPONENT kodi-eventclients-common)

  # Install kodi-eventclients-common PS3 python files
  install(PROGRAMS ${CORE_SOURCE_DIR}/tools/EventClients/lib/python/ps3/__init__.py
                   ${CORE_SOURCE_DIR}/tools/EventClients/lib/python/ps3/keymaps.py
                   ${CORE_SOURCE_DIR}/tools/EventClients/lib/python/ps3/sixaxis.py
                   ${CORE_SOURCE_DIR}/tools/EventClients/lib/python/ps3/sixpair.py
                   ${CORE_SOURCE_DIR}/tools/EventClients/lib/python/ps3/sixwatch.py
          DESTINATION lib/python2.7/dist-packages/${APP_NAME_LC}/ps3
          COMPONENT kodi-eventclients-common)

  # Install kodi-eventclients-common python files
  file(WRITE ${CMAKE_BINARY_DIR}/packages/deb/defs.py ICON_PATH="usr/share/pixmaps/${APP_NAME_LC}/")
  install(PROGRAMS ${CMAKE_BINARY_DIR}/packages/deb/defs.py
                   ${CORE_SOURCE_DIR}/tools/EventClients/lib/python/__init__.py
                   "${CORE_SOURCE_DIR}/tools/EventClients/Clients/PS3 BD Remote/ps3_remote.py"
                   ${CORE_SOURCE_DIR}/tools/EventClients/lib/python/xbmcclient.py
                   ${CORE_SOURCE_DIR}/tools/EventClients/lib/python/zeroconf.py
          DESTINATION lib/python2.7/dist-packages/${APP_NAME_LC}
          COMPONENT kodi-eventclients-common)

  # Install kodi-eventclients-common icons
  install(FILES ${CORE_SOURCE_DIR}/tools/EventClients/icons/bluetooth.png
                ${CORE_SOURCE_DIR}/tools/EventClients/icons/phone.png
                ${CORE_SOURCE_DIR}/tools/EventClients/icons/mail.png
                ${CORE_SOURCE_DIR}/tools/EventClients/icons/mouse.png
          DESTINATION share/pixmaps/${APP_NAME_LC}
          COMPONENT kodi-eventclients-common)

  # Install kodi-eventclients-dev headers
  install(FILES ${CORE_SOURCE_DIR}/tools/EventClients/lib/c++/xbmcclient.h
          DESTINATION include/${APP_NAME_LC}
          COMPONENT kodi-eventclients-dev)

  # Install kodi-eventclients-dev C# examples
  install(FILES "${CORE_SOURCE_DIR}/tools/EventClients/examples/c#/XBMCDemoClient1.cs"
          DESTINATION "share/doc/${APP_NAME_LC}-eventclients-dev/examples/C#"
          COMPONENT kodi-eventclients-dev)

  # Install kodi-eventclients-dev C++ examples
  install(FILES ${CORE_SOURCE_DIR}/tools/EventClients/examples/c++/example_notification.cpp
                ${CORE_SOURCE_DIR}/tools/EventClients/examples/c++/example_log.cpp
                ${CORE_SOURCE_DIR}/tools/EventClients/examples/c++/example_button1.cpp
                ${CORE_SOURCE_DIR}/tools/EventClients/examples/c++/example_mouse.cpp
                ${CORE_SOURCE_DIR}/tools/EventClients/examples/c++/example_button2.cpp
          DESTINATION share/doc/${APP_NAME_LC}-eventclients-dev/examples/C++
          COMPONENT kodi-eventclients-dev)

  # Install kodi-eventclients-dev java examples
  install(FILES ${CORE_SOURCE_DIR}/tools/EventClients/examples/java/XBMCDemoClient1.java
          DESTINATION share/doc/${APP_NAME_LC}-eventclients-dev/examples/java
          COMPONENT kodi-eventclients-dev)

  # Install kodi-eventclients-dev python examples
  install(PROGRAMS ${CORE_SOURCE_DIR}/tools/EventClients/examples/python/example_mouse.py
                   ${CORE_SOURCE_DIR}/tools/EventClients/examples/python/example_button1.py
                   ${CORE_SOURCE_DIR}/tools/EventClients/examples/python/example_notification.py
                   ${CORE_SOURCE_DIR}/tools/EventClients/examples/python/example_action.py
                   ${CORE_SOURCE_DIR}/tools/EventClients/examples/python/example_button2.py
                   ${CORE_SOURCE_DIR}/tools/EventClients/examples/python/example_simple.py
          DESTINATION share/doc/${APP_NAME_LC}-eventclients-dev/examples/python
          COMPONENT kodi-eventclients-dev)

  # Install kodi-eventclients-ps3
  install(PROGRAMS "${CORE_SOURCE_DIR}/tools/EventClients/Clients/PS3 BD Remote/ps3_remote.py"
          RENAME ${APP_NAME_LC}-ps3remote
          DESTINATION bin
          COMPONENT kodi-eventclients-ps3)

  # Install kodi-eventclients-wiiremote
  install(PROGRAMS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/WiiRemote/${APP_NAME_LC}-wiiremote
          DESTINATION bin
          COMPONENT kodi-eventclients-wiiremote)

  # Install kodi-eventclients-xbmc-send
  install(PROGRAMS "${CORE_SOURCE_DIR}/tools/EventClients/Clients/Kodi Send/kodi-send.py"
          RENAME ${APP_NAME_LC}-send
          DESTINATION bin
          COMPONENT kodi-eventclients-xbmc-send)
endif()

# Install kodi-inputstream-dev
install(FILES ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/kodi_inputstream_dll.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/kodi_inputstream_types.h
        DESTINATION include/${APP_NAME_LC}
        COMPONENT kodi-inputstream-dev)

# Install kodi-pvr-dev
install(FILES ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/xbmc_epg_types.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_dll.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h
        DESTINATION include/${APP_NAME_LC}
        COMPONENT kodi-pvr-dev)

# Install kodi-screensaver-dev
install(FILES ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/xbmc_scr_dll.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/xbmc_scr_types.h
        DESTINATION include/${APP_NAME_LC}
        COMPONENT kodi-screensaver-dev)

# Install kodi-visualization-dev
install(FILES ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/xbmc_vis_dll.h
              ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/xbmc_vis_types.h
        DESTINATION include/${APP_NAME_LC}
        COMPONENT kodi-visualization-dev)

# Install XBT skin files
foreach(texture ${XBT_FILES})
  string(REPLACE "${CMAKE_BINARY_DIR}/" "" dir ${texture})
  get_filename_component(dir ${dir} PATH)
  install(FILES ${texture}
          DESTINATION share/${APP_NAME_LC}/${dir}
          COMPONENT kodi)
endforeach()

# Install extra stuff if it exists
if(EXISTS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/extra-installs)
  install(CODE "file(STRINGS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/extra-installs dirs)
              foreach(dir \${dirs})
                file(GLOB_RECURSE FILES RELATIVE ${CMAKE_BINARY_DIR} \${dir}/*)
                foreach(file \${FILES})
                  get_filename_component(dir \${file} PATH)
                  file(INSTALL \${file} DESTINATION share/${APP_NAME_LC}/\${dir})
                endforeach()
              endforeach()")
endif()

if(NOT "$ENV{DESTDIR}" STREQUAL "")
  set(DESTDIR ${CMAKE_BINARY_DIR}/$ENV{DESTDIR})
endif()
foreach(subdir ${build_dirs})
  if(NOT subdir MATCHES kodi-platform)
    string(REPLACE " " ";" subdir ${subdir})
    list(GET subdir 0 id)
    install(CODE "execute_process(COMMAND ${CMAKE_MAKE_PROGRAM} -C ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${id}/src/${id}-build install DESTDIR=${DESTDIR})")
  endif()
endforeach()

# generate packages? yes please, if everything checks out
if(CPACK_GENERATOR)
  if(CPACK_GENERATOR STREQUAL DEB AND CORE_SYSTEM_NAME STREQUAL linux)
    if(CMAKE_BUILD_TYPE STREQUAL Debug)
      message(STATUS "DEB Generator: Build type is set to 'Debug'. Packaged binaries will be unstripped.")
    endif()
    include(${PROJECT_SOURCE_DIR}/cpack/CPackConfigDEB.cmake)
  else()
    message(FATAL_ERROR "DEB Generator: Can't configure CPack to generate Debian packages on non-linux systems.")
  endif()
endif()
