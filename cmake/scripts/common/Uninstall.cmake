macro(remove_empty_dirs)
  list(REMOVE_DUPLICATES DIRECTORIES)
  unset(PDIRECTORIES)
  foreach(dir IN LISTS DIRECTORIES)
    if(EXISTS $ENV{DESTDIR}${dir})
      file(GLOB _res $ENV{DESTDIR}${dir}/*)
      list(LENGTH _res _len)
      if(_len EQUAL 0 AND EXISTS $ENV{DESTDIR}${dir})
        message(STATUS "Removing empty dir: ${dir}")
        execute_process(
          COMMAND ${CMAKE_COMMAND} -E remove_directory $ENV{DESTDIR}${dir}
          OUTPUT_VARIABLE rm_out
          RESULT_VARIABLE rm_retval
        )
        if(NOT "${rm_retval}" STREQUAL 0)
          message(FATAL_ERROR "Failed to remove directory: $ENV{DESTDIR}${dir}")
        endif()
        get_filename_component(_pdir $ENV{DESTDIR}${dir} DIRECTORY)
        list(APPEND PDIRECTORIES ${_pdir})
      endif()
    endif()
  endforeach()
  list(LENGTH PDIRECTORIES _plen)
  if(_plen GREATER 0)
    set(DIRECTORIES ${PDIRECTORIES})
    remove_empty_dirs()
  endif()
endmacro()

# Uninstall target
set(MANIFEST ${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt)
if(EXISTS ${MANIFEST})
  file(STRINGS ${MANIFEST} files)
    foreach(file IN LISTS files)
      if(EXISTS $ENV{DESTDIR}${file})
        get_filename_component(_dir $ENV{DESTDIR}${file} DIRECTORY)
        list(APPEND DIRECTORIES $ENV{DESTDIR}${_dir})
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

    # delete empty dirs
    if(DIRECTORIES)
      remove_empty_dirs()
    endif()
else()
  message(STATUS "Cannot find install manifest: '${MANIFEST}'")
endif()
