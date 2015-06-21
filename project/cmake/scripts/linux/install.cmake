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

configure_file(${CORE_SOURCE_DIR}/tools/Linux/kodi.sh.in
               ${CORE_BUILD_DIR}/scripts/${APP_NAME_LC} @ONLY)
configure_file(${CORE_SOURCE_DIR}/tools/Linux/kodi-standalone.sh.in
                ${CORE_BUILD_DIR}/scripts/${APP_NAME_LC}-standalone @ONLY)

install(TARGETS ${APP_NAME_LC} DESTINATION ${libdir}/kodi)
if(ENABLE_X11 AND XRANDR_FOUND)
  install(TARGETS ${APP_NAME_LC}-xrandr DESTINATION ${libdir}/${APP_NAME_LC})
endif()
install(FILES ${addon_bindings} DESTINATION ${includedir}/kodi)
install(FILES ${cmake-files}
        DESTINATION ${libdir}/kodi)
install(PROGRAMS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/scripts/${APP_NAME_LC}
                ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/scripts/${APP_NAME_LC}-standalone
        DESTINATION ${bindir})

configure_file(${CORE_SOURCE_DIR}/tools/Linux/kodi-xsession.desktop.in
               ${CORE_BUILD_DIR}/${APP_NAME_LC}-xsession.desktop)
install(FILES ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${APP_NAME_LC}-xsession.desktop
        DESTINATION ${datarootdir}/xsessions)

install(FILES ${CORE_SOURCE_DIR}/LICENSE.GPL
              ${CORE_SOURCE_DIR}/docs/README.linux
        DESTINATION ${datarootdir}/doc/kodi)

install(FILES ${CORE_SOURCE_DIR}/tools/Linux/kodi.desktop
        DESTINATION ${datarootdir}/applications)

foreach(texture ${XBT_FILES})
  string(REPLACE "${CMAKE_BINARY_DIR}/" "" dir ${texture})
  get_filename_component(dir ${dir} PATH)
  install(FILES ${texture}
          DESTINATION ${datarootdir}/kodi/${dir})
endforeach()

foreach(wraplib ${WRAP_FILES})
  get_filename_component(dir ${wraplib} PATH)
  install(PROGRAMS ${CMAKE_BINARY_DIR}/${wraplib}
          DESTINATION ${libdir}/kodi/${dir})
endforeach()

foreach(file ${install_data})
  get_filename_component(dir ${file} PATH)
  install(FILES ${CMAKE_BINARY_DIR}/${file}
          DESTINATION ${datarootdir}/kodi/${dir})
endforeach()

if(EXISTS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/extra-installs)
  install(CODE "file(STRINGS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/extra-installs dirs)
              foreach(dir \${dirs})
                file(GLOB_RECURSE FILES RELATIVE ${CMAKE_BINARY_DIR} \${dir}/*)
                foreach(file \${FILES})
                  get_filename_component(dir \${file} PATH)
                  file(INSTALL \${file} DESTINATION ${datarootdir}/kodi/\${dir})
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

install(FILES ${CORE_SOURCE_DIR}/media/icon48x48.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION ${datarootdir}/icons/hicolor/48x48/apps)
install(FILES ${CORE_SOURCE_DIR}/media/icon256x256.png
        RENAME ${APP_NAME_LC}.png
        DESTINATION ${datarootdir}/icons/hicolor/256x256/apps)

install(CODE "execute_process(COMMAND gtk-update-icon-cache -f -q -t $ENV{DESTDIR}${datarootdir}/icons/hicolor ERROR_QUIET)")
