function(core_link_library lib wraplib)
  set(export -Wl,--unresolved-symbols=ignore-all
             `cat ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cores/dll-loader/exports/wrapper.def`
             ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cores/dll-loader/exports/CMakeFiles/wrapper.dir/wrapper.c.o)
  set(check_arg "")
  if(TARGET ${lib})
    set(target ${lib})
    set(link_lib $<TARGET_FILE:${lib}>)
    set(check_arg ${ARGV2})
    set(data_arg  ${ARGV3})
  else()
    set(target ${ARGV2})
    set(link_lib ${lib})
    set(check_arg ${ARGV3})
    set(data_arg ${ARGV4})
  endif()

  # wrapper has to be adapted in order to support coverage.
  if(CMAKE_BUILD_TYPE STREQUAL Coverage)
    set(export "")
  endif()

  if(check_arg STREQUAL export)
    set(export ${export}
        -Wl,--version-script=${ARGV3})
  elseif(check_arg STREQUAL extras)
    foreach(arg ${data_arg})
      list(APPEND export ${arg})
    endforeach()
  endif()
  get_filename_component(dir ${wraplib} PATH)
  add_custom_command(OUTPUT ${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX}
                     COMMAND cmake -E make_directory ${dir}
                     COMMAND ${CMAKE_C_COMPILER}
                     ARGS    -Wl,--whole-archive
                             "${link_lib}"
                             -Wl,--no-whole-archive -lm
                             -shared -o ${CMAKE_BINARY_DIR}/${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX}
                             ${export}
                     DEPENDS ${target} wrapper.def wrapper)

  get_filename_component(libname ${wraplib} NAME_WE)
  add_custom_target(wrap_${libname} ALL DEPENDS ${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX})
  set_target_properties(wrap_${libname} PROPERTIES FOLDER lib/wrapped)
  add_dependencies(${APP_NAME_LC}-libraries wrap_${libname})

  set(LIBRARY_FILES ${LIBRARY_FILES} ${CMAKE_BINARY_DIR}/${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX} CACHE STRING "" FORCE)
endfunction()

function(find_soname lib)
  cmake_parse_arguments(arg "REQUIRED" "" "" ${ARGN})

  string(TOLOWER ${lib} liblow)
  if(${lib}_LDFLAGS)
    set(link_lib "${${lib}_LDFLAGS}")
  else()
    if(IS_ABSOLUTE "${${lib}_LIBRARIES}")
      set(link_lib "${${lib}_LIBRARIES}")
    else()
      set(link_lib -l${${lib}_LIBRARIES})
    endif()
  endif()
  execute_process(COMMAND ${CMAKE_C_COMPILER} -nostdlib -o /dev/null -Wl,-M ${link_lib} 
                  COMMAND grep LOAD.*${liblow}
                  ERROR_QUIET
                  OUTPUT_VARIABLE ${lib}_FILENAME)
  string(REPLACE "LOAD " "" ${lib}_FILENAME "${${lib}_FILENAME}")
  string(STRIP "${${lib}_FILENAME}" ${lib}_FILENAME)
  if(NOT ${lib}_FILENAME)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -nostdlib -o /dev/null -Wl,-t ${link_lib}
                    OUTPUT_QUIET
                    ERROR_VARIABLE _TMP_FILENAME)
    string(REGEX MATCH ".*lib${liblow}.so" ${lib}_FILENAME ${_TMP_FILENAME})
  endif()
  if(${lib}_FILENAME)
    execute_process(COMMAND objdump -p ${${lib}_FILENAME}
                    COMMAND grep SONAME.*${liblow}
                    ERROR_QUIET
                    OUTPUT_VARIABLE ${lib}_SONAME)
    string(REPLACE "SONAME " "" ${lib}_SONAME ${${lib}_SONAME})
    string(STRIP ${${lib}_SONAME} ${lib}_SONAME)
    if(VERBOSE)
      message(STATUS "${lib} soname: ${${lib}_SONAME}")
    endif()
    set(${lib}_SONAME ${${lib}_SONAME} PARENT_SCOPE)
  endif()
  if(arg_REQUIRED AND NOT ${lib}_SONAME)
    message(FATAL_ERROR "Could not find dynamically loadable library ${lib}")
  endif()
endfunction()
