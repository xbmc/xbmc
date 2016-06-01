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

# Install cmake files
# TODO: revisit, refactor and nuke txt file globbing
install(FILES ${cmake_files} DESTINATION lib/${APP_NAME_LC})
install(FILES ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/scripts/${APP_NAME}Config.cmake
              ${PROJECT_SOURCE_DIR}/scripts/common/AddOptions.cmake
              ${PROJECT_SOURCE_DIR}/scripts/common/AddonHelpers.cmake
              ${PROJECT_SOURCE_DIR}/scripts/linux/PathSetup.cmake
        DESTINATION lib/${APP_NAME_LC})

# Install app
install(TARGETS ${APP_NAME_LC} DESTINATION lib/${APP_NAME_LC})
if(ENABLE_X11 AND XRANDR_FOUND)
  install(TARGETS ${APP_NAME_LC}-xrandr DESTINATION lib/${APP_NAME_LC})
endif()

# Install scripts
install(PROGRAMS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/scripts/${APP_NAME_LC}
                 ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/scripts/${APP_NAME_LC}-standalone
        DESTINATION bin)

# Install libraries
foreach(wraplib ${WRAP_FILES})
  get_filename_component(dir ${wraplib} PATH)
  install(PROGRAMS ${CMAKE_BINARY_DIR}/${wraplib}
          DESTINATION lib/${APP_NAME_LC}/${dir})
endforeach()

# Install add-ons, fonts, icons, keyboard maps, keymaps, etc
# (addons, media, system, userdata folders in share/kodi/)
foreach(file ${install_data})
  get_filename_component(dir ${file} PATH)
  install(FILES ${CMAKE_BINARY_DIR}/${file}
          DESTINATION share/${APP_NAME_LC}/${dir})
endforeach()

# Install add-on bindings
install(FILES ${addon_bindings} DESTINATION include/${APP_NAME_LC})

# Install xsession entry
install(FILES ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${APP_NAME_LC}.desktop
        DESTINATION share/xsessions)

# Install desktop entry
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/kodi.desktop
        DESTINATION share/applications/${APP_NAME_LC}.desktop)

# Install icons
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon16x16.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/16x16/apps)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon22x22.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/22x22/apps)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon24x24.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/24x24/apps)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon32x32.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/32x32/apps)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon48x48.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/48x48/apps)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon64x64.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/64x64/apps)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon128x128.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/128x128/apps)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/packaging/media/icon256x256.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION share/icons/hicolor/256x256/apps)
install(CODE "execute_process(COMMAND gtk-update-icon-cache -f -q -t
        $ENV{DESTDIR}${datarootdir}/icons/hicolor ERROR_QUIET)")

# Install docs
install(FILES ${CORE_SOURCE_DIR}/copying.txt
              ${CORE_SOURCE_DIR}/LICENSE.GPL
              ${CORE_SOURCE_DIR}/version.txt
              ${CORE_SOURCE_DIR}/docs/README.linux
        DESTINATION share/doc/${APP_NAME_LC})

# Install XBT skin files
foreach(texture ${XBT_FILES})
  string(REPLACE "${CMAKE_BINARY_DIR}/" "" dir ${texture})
  get_filename_component(dir ${dir} PATH)
  install(FILES ${texture}
          DESTINATION share/${APP_NAME_LC}/${dir})
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
    install(CODE "execute_process(COMMAND make -C ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${id}/src/${id}-build install DESTDIR=${DESTDIR})")
  endif()
endforeach()

# Create xbmc -> kodi symlinks
if(NOT EXISTS ${libdir}/xbmc)
  install(CODE "execute_process(COMMAND ln -sf ${APP_NAME_LC}/ xbmc
          WORKING_DIRECTORY ${libdir})")
endif()
if(NOT EXISTS ${includedir}/xbmc)
  install(CODE "execute_process(COMMAND ln -sf ${APP_NAME_LC}/ xbmc
          WORKING_DIRECTORY ${includedir})")
endif()
if(NOT EXISTS ${bindir}/xbmc)
  install(CODE "execute_process(COMMAND ln -sf ${APP_NAME_LC} xbmc
          WORKING_DIRECTORY ${bindir})")
endif()
if(NOT EXISTS ${bindir}/xbmc-standalone)
  install(CODE "execute_process(COMMAND ln -sf ${APP_NAME_LC}-standalone xbmc-standalone
          WORKING_DIRECTORY ${bindir})")
endif()
if(NOT EXISTS ${datarootdir}/xsessions/xbmc.desktop)
  install(CODE "execute_process(COMMAND ln -sf ${APP_NAME_LC}.desktop xbmc.desktop
          WORKING_DIRECTORY ${datarootdir}/xsessions/)")
endif()
if(NOT EXISTS ${datarootdir}/xbmc)
  install(CODE "execute_process(COMMAND ln -sf ${APP_NAME_LC}/ xbmc
          WORKING_DIRECTORY ${datarootdir})")
endif()

