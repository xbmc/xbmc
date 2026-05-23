function(core_link_library lib wraplib)
  set(export -bundle -undefined dynamic_lookup -read_only_relocs suppress
             -Wl,-alias_list,${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cores/dll-loader/exports/wrapper.def
             $<TARGET_OBJECTS:wrapper>)
  set(extension ${CMAKE_SHARED_MODULE_SUFFIX})
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
  get_filename_component(dir ${wraplib} DIRECTORY)

  # We can't simply pass the linker flags to the args section of the custom command
  # because cmake will add quotes around it (and the linker will fail due to those).
  # We need to do this handstand first ...
  string(REPLACE " " ";" CUSTOM_COMMAND_ARGS_LDFLAGS ${CMAKE_SHARED_LINKER_FLAGS})

  add_custom_command(OUTPUT ${wraplib}-${ARCH}${extension}
                     COMMAND ${CMAKE_COMMAND} -E make_directory ${dir}
                     COMMAND ${CMAKE_C_COMPILER}
                     ARGS    ${CUSTOM_COMMAND_ARGS_LDFLAGS} ${export} -Wl,-force_load ${link_lib} ${extra_libs}
                             -o ${CMAKE_BINARY_DIR}/${wraplib}-${ARCH}${extension}
                     DEPENDS ${target} wrapper.def wrapper)

  get_filename_component(libname ${wraplib} NAME_WE)
  add_custom_target(wrap_${libname} ALL DEPENDS ${wraplib}-${ARCH}${extension})
  set_target_properties(wrap_${libname} PROPERTIES FOLDER lib/wrapped)
  add_dependencies(${APP_NAME_LC}-libraries wrap_${libname})

  set(LIBRARY_FILES ${LIBRARY_FILES} ${CMAKE_BINARY_DIR}/${wraplib}-${ARCH}${extension} CACHE STRING "" FORCE)
endfunction()

function(find_soname lib)
  cmake_parse_arguments(arg "REQUIRED" "" "" ${ARGN})

  string(TOLOWER ${lib} liblow)
  if(${lib}_LDFLAGS)
    set(link_lib "${${lib}_LDFLAGS}")
  else()
    set(link_lib "${${lib}_LIBRARIES}")
  endif()

  execute_process(COMMAND ${CMAKE_C_COMPILER} -print-search-dirs
                  COMMAND fgrep libraries:
                  COMMAND sed "s/[^=]*=\\(.*\\)/\\1/"
                  COMMAND sed "s/:/ /g"
                  ERROR_QUIET
                  OUTPUT_VARIABLE cc_lib_path
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND echo ${link_lib}
                  COMMAND sed "s/-L[ ]*//g"
                  COMMAND sed "s/-l[^ ]*//g"
                  ERROR_QUIET
                  OUTPUT_VARIABLE env_lib_path
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

  foreach(path ${cc_lib_path} ${env_lib_path})
    if(IS_DIRECTORY ${path})
      execute_process(COMMAND ls -- ${path}/lib${liblow}.dylib
                      ERROR_QUIET
                      OUTPUT_VARIABLE lib_file
                      OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
      set(lib_file ${path})
    endif()
    if(lib_file)
      # we want the path/name that is embedded in the dylib
      execute_process(COMMAND otool -L ${lib_file}
                      COMMAND grep -v lib${liblow}.dylib
                      COMMAND grep ${liblow}
                      COMMAND awk "{V=1; print $V}"
                      ERROR_QUIET
                      OUTPUT_VARIABLE filename
                      OUTPUT_STRIP_TRAILING_WHITESPACE)
      get_filename_component(${lib}_SONAME "${filename}" NAME)
      if(VERBOSE)
        message(STATUS "${lib} soname: ${${lib}_SONAME}")
      endif()
    endif()
  endforeach()
  if(arg_REQUIRED AND NOT ${lib}_SONAME)
    message(FATAL_ERROR "Could not find dynamically loadable library ${lib}")
  endif()
  set(${lib}_SONAME ${${lib}_SONAME} PARENT_SCOPE)
endfunction()
