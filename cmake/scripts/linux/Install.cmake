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
set(APP_BINARY ${APP_NAME_LC}${APP_BINARY_SUFFIX})
set(APP_PREFIX ${prefix})
set(APP_LIB_DIR ${libdir}/${APP_NAME_LC})
set(APP_DATA_DIR ${datarootdir}/${APP_NAME_LC})
set(APP_INCLUDE_DIR ${includedir}/${APP_NAME_LC})

# Set XBMC_STANDALONE_SH_PULSE so we can insert PulseAudio block into kodi-standalone
if(EXISTS ${CMAKE_SOURCE_DIR}/tools/Linux/kodi-standalone.sh.pulse)
  if(ENABLE_PULSEAUDIO AND PULSEAUDIO_FOUND)
    file(READ "${CMAKE_SOURCE_DIR}/tools/Linux/kodi-standalone.sh.pulse" pulse_content)
    set(XBMC_STANDALONE_SH_PULSE ${pulse_content})
  endif()
endif()

# Configure startup scripts
configure_file(${CMAKE_SOURCE_DIR}/tools/Linux/kodi.sh.in
               ${CORE_BUILD_DIR}/scripts/${APP_NAME_LC} @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/Linux/kodi-standalone.sh.in
               ${CORE_BUILD_DIR}/scripts/${APP_NAME_LC}-standalone @ONLY)

# Configure cmake files
configure_file(${CMAKE_SOURCE_DIR}/cmake/KodiConfig.cmake.in
               ${CORE_BUILD_DIR}/scripts/${APP_NAME}Config.cmake @ONLY)

# Configure gbm session entry
configure_file(${CMAKE_SOURCE_DIR}/tools/Linux/kodi-gbm-session.desktop.in
               ${CORE_BUILD_DIR}/${APP_NAME_LC}-gbm-session.desktop @ONLY)

# Configure xsession entry
configure_file(${CMAKE_SOURCE_DIR}/tools/Linux/kodi-xsession.desktop.in
               ${CORE_BUILD_DIR}/${APP_NAME_LC}-xsession.desktop @ONLY)

# Configure desktop entry
configure_file(${CMAKE_SOURCE_DIR}/tools/Linux/kodi.desktop.in
               ${CORE_BUILD_DIR}/${APP_NAME_LC}.desktop @ONLY)

# Configure metainfo
configure_file(${CMAKE_SOURCE_DIR}/tools/Linux/kodi.metainfo.xml.in
               ${CORE_BUILD_DIR}/${APP_PACKAGE}.metainfo.xml @ONLY)

# Install app
install(TARGETS ${APP_NAME_LC}
        DESTINATION ${libdir}/${APP_NAME_LC}
        COMPONENT kodi-bin)
if(TARGET X::X AND TARGET XRandR::XRandR)
  install(TARGETS ${APP_NAME_LC}-xrandr
          DESTINATION ${libdir}/${APP_NAME_LC}
          COMPONENT kodi-bin)
endif()

# Install scripts
install(PROGRAMS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/scripts/${APP_NAME_LC}
                 ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/scripts/${APP_NAME_LC}-standalone
        DESTINATION ${bindir}
        COMPONENT kodi-bin)

# Install libraries
foreach(library ${LIBRARY_FILES})
  get_filename_component(dir ${library} DIRECTORY)
  string(REPLACE "${CMAKE_BINARY_DIR}/" "" dir ${dir})
  install(PROGRAMS ${library}
          DESTINATION ${libdir}/${APP_NAME_LC}/${dir}
          COMPONENT kodi-bin)
endforeach()

# Install add-ons, fonts, icons, keyboard maps, keymaps, etc
# (addons, media, system, userdata folders in share/kodi/)
foreach(file ${install_data})
  get_filename_component(dir ${file} DIRECTORY)
  install(FILES ${CMAKE_BINARY_DIR}/${file}
          DESTINATION ${datarootdir}/${APP_NAME_LC}/${dir}
          COMPONENT kodi)
endforeach()

# Install gbm session entry
install(FILES ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${APP_NAME_LC}-gbm-session.desktop
        RENAME ${APP_NAME_LC}-gbm.desktop
        DESTINATION ${datarootdir}/wayland-sessions
        COMPONENT kodi)

# Install xsession entry
install(FILES ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${APP_NAME_LC}-xsession.desktop
        RENAME ${APP_NAME_LC}.desktop
        DESTINATION ${datarootdir}/xsessions
        COMPONENT kodi)

# Install desktop entry
install(FILES ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${APP_NAME_LC}.desktop
        DESTINATION ${datarootdir}/applications
        COMPONENT kodi)

# Install metainfo
install(FILES ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${APP_PACKAGE}.metainfo.xml
        DESTINATION ${datarootdir}/metainfo
        COMPONENT kodi)

# Install icons
install(FILES ${CMAKE_SOURCE_DIR}/tools/Linux/packaging/media/icon16x16.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION ${datarootdir}/icons/hicolor/16x16/apps
        COMPONENT kodi)
install(FILES ${CMAKE_SOURCE_DIR}/tools/Linux/packaging/media/icon22x22.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION ${datarootdir}/icons/hicolor/22x22/apps
        COMPONENT kodi)
install(FILES ${CMAKE_SOURCE_DIR}/tools/Linux/packaging/media/icon24x24.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION ${datarootdir}/icons/hicolor/24x24/apps
        COMPONENT kodi)
install(FILES ${CMAKE_SOURCE_DIR}/tools/Linux/packaging/media/icon32x32.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION ${datarootdir}/icons/hicolor/32x32/apps
        COMPONENT kodi)
install(FILES ${CMAKE_SOURCE_DIR}/tools/Linux/packaging/media/icon48x48.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION ${datarootdir}/icons/hicolor/48x48/apps
        COMPONENT kodi)
install(FILES ${CMAKE_SOURCE_DIR}/tools/Linux/packaging/media/icon64x64.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION ${datarootdir}/icons/hicolor/64x64/apps
        COMPONENT kodi)
install(FILES ${CMAKE_SOURCE_DIR}/tools/Linux/packaging/media/icon128x128.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION ${datarootdir}/icons/hicolor/128x128/apps
        COMPONENT kodi)
install(FILES ${CMAKE_SOURCE_DIR}/tools/Linux/packaging/media/icon256x256.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION ${datarootdir}/icons/hicolor/256x256/apps
        COMPONENT kodi)

# Install firewalld service definitions
install(FILES ${CMAKE_SOURCE_DIR}/tools/Linux/firewalld-services/kodi-eventserver.xml
              ${CMAKE_SOURCE_DIR}/tools/Linux/firewalld-services/kodi-http.xml
              ${CMAKE_SOURCE_DIR}/tools/Linux/firewalld-services/kodi-jsonrpc.xml
        DESTINATION ${prefix}/lib/firewalld/services
        COMPONENT kodi)

# Install docs
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE.md
              ${CMAKE_SOURCE_DIR}/version.txt
              ${CMAKE_SOURCE_DIR}/docs/README.Linux.md
        DESTINATION ${docdir}
        COMPONENT kodi)

install(FILES ${CMAKE_SOURCE_DIR}/privacy-policy.txt
        DESTINATION ${datarootdir}/${APP_NAME_LC}
        COMPONENT kodi)

# Install kodi-tools-texturepacker
if(INTERNAL_TEXTUREPACKER_INSTALLABLE)
  install(PROGRAMS $<TARGET_FILE:TexturePacker::TexturePacker::Installable>
          DESTINATION ${bindir}
          RENAME "${APP_NAME_LC}-TexturePacker"
          COMPONENT kodi-tools-texturepacker)
endif()

# Install kodi-addon-dev headers
include(${CMAKE_SOURCE_DIR}/xbmc/addons/AddonBindings.cmake)
install(DIRECTORY ${CORE_ADDON_BINDINGS_DIRS}/
        DESTINATION ${includedir}/${APP_NAME_LC}
        COMPONENT kodi-addon-dev
        REGEX ".txt" EXCLUDE)

install(FILES ${CORE_ADDON_BINDINGS_FILES}
        DESTINATION ${includedir}/${APP_NAME_LC}
        COMPONENT kodi-addon-dev)

# Install kodi-addon-dev add-on bindings
install(FILES ${CMAKE_SOURCE_DIR}/cmake/scripts/common/AddonHelpers.cmake
              ${CMAKE_SOURCE_DIR}/cmake/scripts/common/AddOptions.cmake
              ${CMAKE_SOURCE_DIR}/cmake/scripts/common/ArchSetup.cmake
              ${CMAKE_SOURCE_DIR}/cmake/scripts/common/CheckCommits.cmake
              ${CMAKE_SOURCE_DIR}/cmake/scripts/common/CheckTargetPlatform.cmake
              ${CMAKE_SOURCE_DIR}/cmake/scripts/common/GenerateCompileInfo.cmake
              ${CMAKE_SOURCE_DIR}/cmake/scripts/common/GeneratorSetup.cmake
              ${CMAKE_SOURCE_DIR}/cmake/scripts/common/HandleDepends.cmake
              ${CMAKE_SOURCE_DIR}/cmake/scripts/common/Macros.cmake
              ${CMAKE_SOURCE_DIR}/cmake/scripts/common/PrepareEnv.cmake
              ${CMAKE_SOURCE_DIR}/cmake/scripts/common/ProjectMacros.cmake
              ${CMAKE_SOURCE_DIR}/cmake/scripts/linux/PathSetup.cmake
        DESTINATION ${datarootdir}/${APP_NAME_LC}/cmake
        COMPONENT kodi-addon-dev)
# ${APP_NAME}Config.cmake contains architecture-specific paths so it
# should be installed in ${libdir}/${APP_NAME_LC}/${dir}
install(FILES ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/scripts/${APP_NAME}Config.cmake
        DESTINATION ${libdir}/${APP_NAME_LC}/cmake
        COMPONENT kodi-addon-dev)

if(ENABLE_EVENTCLIENTS)
  execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(prefix=''))"
                  OUTPUT_VARIABLE PYTHON_LIB_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
  # Install kodi-eventclients-common BT python files
  install(PROGRAMS ${CMAKE_SOURCE_DIR}/tools/EventClients/lib/python/bt/__init__.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/lib/python/bt/bt.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/lib/python/bt/hid.py
          DESTINATION ${PYTHON_LIB_PATH}/${APP_NAME_LC}/bt
          COMPONENT kodi-eventclients-common)

  # Install kodi-eventclients-common PS3 python files
  install(PROGRAMS ${CMAKE_SOURCE_DIR}/tools/EventClients/lib/python/ps3/__init__.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/lib/python/ps3/keymaps.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/lib/python/ps3/sixaxis.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/lib/python/ps3/sixpair.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/lib/python/ps3/sixwatch.py
          DESTINATION ${PYTHON_LIB_PATH}/${APP_NAME_LC}/ps3
          COMPONENT kodi-eventclients-common)

  # Install kodi-eventclients-common python files
  file(WRITE ${CMAKE_BINARY_DIR}/packages/deb/defs.py ICON_PATH="/usr/share/pixmaps/${APP_NAME_LC}/")
  install(PROGRAMS ${CMAKE_BINARY_DIR}/packages/deb/defs.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/lib/python/__init__.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/Clients/PS3BDRemote/ps3_remote.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/lib/python/xbmcclient.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/lib/python/zeroconf.py
          DESTINATION ${PYTHON_LIB_PATH}/${APP_NAME_LC}
          COMPONENT kodi-eventclients-common)

  # Install kodi-eventclients-common icons
  install(FILES ${CMAKE_SOURCE_DIR}/tools/EventClients/icons/bluetooth.png
                ${CMAKE_SOURCE_DIR}/tools/EventClients/icons/phone.png
                ${CMAKE_SOURCE_DIR}/tools/EventClients/icons/mail.png
                ${CMAKE_SOURCE_DIR}/tools/EventClients/icons/mouse.png
          DESTINATION ${datarootdir}/pixmaps/${APP_NAME_LC}
          COMPONENT kodi-eventclients-common)

  # Install kodi-eventclients-dev headers
  install(FILES ${CMAKE_SOURCE_DIR}/tools/EventClients/lib/c++/xbmcclient.h
          DESTINATION ${includedir}/${APP_NAME_LC}
          COMPONENT kodi-eventclients-dev)

  # Install kodi-eventclients-dev C# examples
  install(FILES "${CMAKE_SOURCE_DIR}/tools/EventClients/examples/c#/XBMCDemoClient1.cs"
          DESTINATION "${docdir}/${APP_NAME_LC}-eventclients-dev/examples/C#"
          COMPONENT kodi-eventclients-dev)

  # Install kodi-eventclients-dev C++ examples
  install(FILES ${CMAKE_SOURCE_DIR}/tools/EventClients/examples/c++/example_notification.cpp
                ${CMAKE_SOURCE_DIR}/tools/EventClients/examples/c++/example_log.cpp
                ${CMAKE_SOURCE_DIR}/tools/EventClients/examples/c++/example_button1.cpp
                ${CMAKE_SOURCE_DIR}/tools/EventClients/examples/c++/example_mouse.cpp
                ${CMAKE_SOURCE_DIR}/tools/EventClients/examples/c++/example_button2.cpp
          DESTINATION ${docdir}/${APP_NAME_LC}-eventclients-dev/examples/C++
          COMPONENT kodi-eventclients-dev)

  # Install kodi-eventclients-dev java examples
  install(FILES ${CMAKE_SOURCE_DIR}/tools/EventClients/examples/java/XBMCDemoClient1.java
          DESTINATION ${docdir}/${APP_NAME_LC}-eventclients-dev/examples/java
          COMPONENT kodi-eventclients-dev)

  # Install kodi-eventclients-dev python examples
  install(PROGRAMS ${CMAKE_SOURCE_DIR}/tools/EventClients/examples/python/example_mouse.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/examples/python/example_button1.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/examples/python/example_notification.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/examples/python/example_action.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/examples/python/example_button2.py
                   ${CMAKE_SOURCE_DIR}/tools/EventClients/examples/python/example_simple.py
          DESTINATION ${docdir}/${APP_NAME_LC}-eventclients-dev/examples/python
          COMPONENT kodi-eventclients-dev)

  # Install kodi-eventclients-ps3
  install(PROGRAMS ${CMAKE_SOURCE_DIR}/tools/EventClients/Clients/PS3BDRemote/ps3_remote.py
          RENAME ${APP_NAME_LC}-ps3remote
          DESTINATION ${bindir}
          COMPONENT kodi-eventclients-ps3)

  if(TARGET Bluetooth::Bluetooth AND CWIID_FOUND AND GLU_FOUND)
    # Install kodi-eventclients-wiiremote
    install(PROGRAMS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/WiiRemote/${APP_NAME_LC}-wiiremote
            DESTINATION ${bindir}
            COMPONENT kodi-eventclients-wiiremote)
  endif()

  # Install kodi-eventclients-kodi-send
  install(PROGRAMS ${CMAKE_SOURCE_DIR}/tools/EventClients/Clients/KodiSend/kodi-send.py
          RENAME ${APP_NAME_LC}-send
          DESTINATION ${bindir}
          COMPONENT kodi-eventclients-kodi-send)
endif()

# Install XBT skin files
foreach(texture ${XBT_FILES})
  string(REPLACE "${CMAKE_BINARY_DIR}/" "" dir ${texture})
  get_filename_component(dir ${dir} DIRECTORY)
  install(FILES ${texture}
          DESTINATION ${datarootdir}/${APP_NAME_LC}/${dir}
          COMPONENT kodi)
endforeach()

# Install extra stuff if it exists
if(EXISTS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/extra-installs)
  install(CODE "file(STRINGS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/extra-installs dirs)
              foreach(dir \${dirs})
                file(GLOB_RECURSE FILES RELATIVE ${CMAKE_BINARY_DIR} \${dir}/*)
                foreach(file \${FILES})
                  get_filename_component(dir \${file} DIRECTORY)
                  file(INSTALL \${file} DESTINATION ${datarootdir}/${APP_NAME_LC}/\${dir})
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
    include(${CMAKE_SOURCE_DIR}/cmake/cpack/CPackConfigDEB.cmake)
  else()
    message(FATAL_ERROR "DEB Generator: Can't configure CPack to generate Debian packages on non-linux systems.")
  endif()
endif()

if("webos" IN_LIST CORE_PLATFORM_NAME_LC)
  include(${CMAKE_SOURCE_DIR}/cmake/scripts/webos/Install.cmake)
endif()
