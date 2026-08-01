function(core_link_library lib wraplib)
  set(export -Wl,--unresolved-symbols=ignore-all
             `cat ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cores/dll-loader/exports/wrapper.def`
             ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cores/dll-loader/exports/CMakeFiles/wrapper.dir/wrapper.c.o)
  set(check_arg "")
  if(TARGET ${lib})
    set(target ${lib})

    get_target_property(_ALIASTARGET ${target} ALIASED_TARGET)
    if(_ALIASTARGET)
      set(LIB_TARGET ${_ALIASTARGET})
    else()
      set(LIB_TARGET ${target})
    endif()

    string(FIND "${LIB_TARGET}" "PkgConfig::" pclib_target_found)
    if(${pclib_target_found} GREATER_EQUAL 0)
      # PkgConfig targets are interface targets, and do not have the property to
      # fulfill TARGET_FILE genex. Manually retrieve lib from pop front of INTERFACE_LINK_LIBRARIES

      get_target_property(pc_link_libs_target ${LIB_TARGET} INTERFACE_LINK_LIBRARIES)
      list(POP_FRONT pc_link_libs_target _lib_path)
      set(link_lib ${_lib_path})
    else()
      set(link_lib $<TARGET_FILE:${LIB_TARGET}>)
    endif()

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
  elseif(check_arg STREQUAL archives)
    foreach(_data_arg ${data_arg})
      if(TARGET ${_data_arg})
        get_target_property(_extra_ALIASTARGET ${_data_arg} ALIASED_TARGET)
        if(_extra_ALIASTARGET)
          set(DATA_TARGET ${_extra_ALIASTARGET})
        else()
          set(DATA_TARGET ${_data_arg})
        endif()

        string(FIND "${DATA_TARGET}" "PkgConfig::" pctarget_found)
        if(${pctarget_found} GREATER_EQUAL 0)
          # PkgConfig targets are interface targets, and do not have the property to
          # fulfill TARGET_FILE genex. Manually retrieve lib from pop front of INTERFACE_LINK_LIBRARIES

          get_target_property(pc_link_libs ${DATA_TARGET} INTERFACE_LINK_LIBRARIES)
          list(POP_FRONT pc_link_libs _data_lib_path)
          list(APPEND extra_libs ${_data_lib_path})
        else()
          list(APPEND extra_libs $<TARGET_FILE:${DATA_TARGET}>)
        endif()
      else()
        list(APPEND extra_libs ${_data_arg})
      endif()
    endforeach()
  endif()

  string(REGEX REPLACE "[ ]+" ";" _flags "${CMAKE_SHARED_LINKER_FLAGS}")
  get_filename_component(dir ${wraplib} DIRECTORY)
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX}
                     COMMAND ${CMAKE_COMMAND} -E make_directory ${dir}
                     COMMAND ${CMAKE_C_COMPILER}
                     ARGS    ${_flags} -Wl,--whole-archive
                             "${link_lib}" ${extra_libs}
                             -Wl,--no-whole-archive -lm
                             -Wl,-soname,${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX}
                             -shared -o ${CMAKE_BINARY_DIR}/${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX}
                             ${export}
                     DEPENDS ${target} wrapper.def wrapper)

  get_filename_component(libname ${wraplib} NAME_WE)
  add_custom_target(wrap_${libname} ALL DEPENDS ${CMAKE_BINARY_DIR}/${wraplib}-${ARCH}${CMAKE_SHARED_MODULE_SUFFIX})
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
                    ERROR_QUIET
                    OUTPUT_VARIABLE _TMP_FILENAME)
    string(REGEX MATCH ".*lib${liblow}.so" ${lib}_FILENAME ${_TMP_FILENAME})
  endif()
  if(${lib}_FILENAME)
    execute_process(COMMAND ${CMAKE_OBJDUMP} -p ${${lib}_FILENAME}
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
