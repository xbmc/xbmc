set(libdir ${CMAKE_INSTALL_PREFIX}/lib)
set(bindir ${CMAKE_INSTALL_PREFIX}/bin)

configure_file(${CORE_SOURCE_DIR}/tools/Linux/xbmc.sh.in
               ${CORE_BUILD_DIR}/scripts/xbmc @ONLY)
configure_file(${CORE_SOURCE_DIR}/tools/Linux/xbmc-standalone.sh.in
               ${CORE_BUILD_DIR}/scripts/xbmc-standalone @ONLY)

install(TARGETS xbmc-xrandr DESTINATION lib/xbmc)
install(FILES ${addon_bindings} DESTINATION include/xbmc)
install(FILES ${cmake_files} ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/modules/xbmc-config.cmake
        DESTINATION lib/xbmc)
install(PROGRAMS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/scripts/xbmc
                 ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/scripts/xbmc-standalone
        DESTINATION bin)
install(FILES ${CORE_SOURCE_DIR}/tools/Linux/FEH.py
        DESTINATION share/xbmc)

install(FILES ${CORE_SOURCE_DIR}/tools/Linux/xbmc-xsession.desktop
        RENAME XBMC.desktop
        DESTINATION share/xsessions)
                
install(FILES ${CORE_SOURCE_DIR}/LICENSE.GPL
              ${CORE_SOURCE_DIR}/docs/README.freebsd
        DESTINATION share/doc/xbmc)

foreach(texture ${XBT_FILES})
  string(REPLACE "${CMAKE_BINARY_DIR}/" "" dir ${texture})
  get_filename_component(dir ${dir} PATH)
  install(FILES ${texture}
          DESTINATION share/xbmc/${dir})
endforeach()

foreach(wraplib ${WRAP_FILES})
  get_filename_component(dir ${wraplib} PATH)
  install(PROGRAMS ${CMAKE_BINARY_DIR}/${wraplib}
          DESTINATION lib/xbmc/${dir})
endforeach()

foreach(file ${install_data})
  get_filename_component(dir ${file} PATH)
  install(FILES ${CMAKE_BINARY_DIR}/${file}
          DESTINATION share/xbmc/${dir})
endforeach()

install(CODE "file(STRINGS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/extra-installs dirs)
              foreach(dir \${dirs})
                file(GLOB_RECURSE FILES RELATIVE ${CMAKE_BINARY_DIR} \${dir}/*)
                foreach(file \${FILES})
                  get_filename_component(dir \${file} PATH)
                  file(INSTALL \${file} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/xbmc/\${dir})
                endforeach()
              endforeach()")
foreach(subdir ${build_dirs})
  string(REPLACE " " ";" subdir ${subdir})
  list(GET subdir 0 id)
  install(CODE "execute_process(COMMAND make -C ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${id}/src/${id}-build install)")
endforeach()
