# Uninstall target
set(MANIFEST ${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt)
if(EXISTS ${MANIFEST})
  file(STRINGS ${MANIFEST} files)
    foreach(file IN LISTS files)
      if(EXISTS $ENV{DESTDIR}${file})
        message(STATUS "Uninstalling: ${file}")
        execute_process(
          COMMAND ${CMAKE_COMMAND} -E remove $ENV{DESTDIR}${file}
          OUTPUT_VARIABLE rm_out
          RESULT_VARIABLE rm_retval
        )
        if(NOT "${rm_retval}" STREQUAL 0)
          message(FATAL_ERROR "Failed to remove file: $ENV{DESTDIR}${file}")
        endif()
      else()
        message(STATUS "File does not exist: $ENV{DESTDIR}${file}")
      endif()
    endforeach(file)
else()
  message(STATUS "Cannot find install manifest: '${MANIFEST}'")
endif()
