get_filename_component(file ${URL} NAME)
file(DOWNLOAD ${URL} ${DEST}/${file})
execute_process(COMMAND ${7ZIP_EXECUTABLE} -y x ${DEST}/${file}
                WORKING_DIRECTORY ${DESTDIR})
if(${file} MATCHES .tar)
  string(REPLACE ".7z" "" tarball ${file})
  string(REPLACE ".lzma" "" tarball ${file})
  execute_process(COMMAND ${7ZIP_EXECUTABLE} -y x ${DESTDIR}/${tarball}
                  WORKING_DIRECTORY ${DESTDIR})
endif()
