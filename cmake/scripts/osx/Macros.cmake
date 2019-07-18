function(core_link_library lib wraplib)
  if(CMAKE_GENERATOR MATCHES "Unix Makefiles" OR CMAKE_GENERATOR STREQUAL Ninja)
    set(wrapper_obj cores/dll-loader/exports/CMakeFiles/wrapper.dir/wrapper.c.o)
  elseif(CMAKE_GENERATOR MATCHES "Xcode")
    set(wrapper_obj cores/dll-loader/exports/kodi.build/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)/wrapper.build/Objects-$(CURRENT_VARIANT)/$(CURRENT_ARCH)/wrapper.o)
  else()
    message(FATAL_ERROR "Unsupported generator in core_link_library")
  endif()

  set(export -bundle -undefined dynamic_lookup -read_only_relocs suppress
             -Wl,-alias_list,${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cores/dll-loader/exports/wrapper.def
             ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${wrapper_obj})
  set(extension ${CMAKE_SHARED_MODULE_SUFFIX})
  set(check_arg "")
  if(TARGET ${lib})
    set(target ${lib})
    set(link_lib $<TARGET_FILE:${lib}>)
    set(check_arg ${ARGV2})
    set(data_arg  ${ARGV3})

    # iOS: EFFECTIVE_PLATFORM_NAME is not resolved
    # http://public.kitware.com/pipermail/cmake/2016-March/063049.html
    if(CORE_SYSTEM_NAME STREQUAL darwin_embedded AND CMAKE_GENERATOR STREQUAL Xcode)
      get_target_property(dir ${lib} BINARY_DIR)
      set(link_lib ${dir}/${CORE_BUILD_CONFIG}/${CMAKE_STATIC_LIBRARY_PREFIX}${lib}${CMAKE_STATIC_LIBRARY_SUFFIX})
    endif()
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
    set(extra_libs ${data_arg})
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
                     DEPENDS ${target} wrapper.def wrapper
                     VERBATIM)

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
