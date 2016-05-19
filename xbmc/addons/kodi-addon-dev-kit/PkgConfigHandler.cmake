# translate a list of libraries into a command-line that can be passed to the
# compiler/linker. first parameter is the name of the variable that will
# receive this list, the rest is considered the list of libraries
function (linker_cmdline what INTO outvar FROM)
  # if we are going to put these in regexps, we must escape period
  string (REPLACE "." "\\." esc_dl_pref "${CMAKE_SHARED_LIBRARY_PREFIX}")
  string (REPLACE "." "\\." esc_dl_suff "${CMAKE_SHARED_LIBRARY_SUFFIX}")
  string (REPLACE "." "\\." esc_ar_pref "${CMAKE_STATIC_LIBRARY_PREFIX}")
  string (REPLACE "." "\\." esc_ar_suff "${CMAKE_STATIC_LIBRARY_PREFIX}")

  # CMake loves absolute paths, whereas libtool won't have any of it!
  # (you get an error message about argument not parsed). translate each
  # of the libraries into a linker option
  set (deplib_list "")
  foreach (deplib IN LISTS ARGN)
        # starts with a hyphen already? then just add it
        string (SUBSTRING ${deplib} 0 1 dash)
        if (${dash} STREQUAL "-")
          list (APPEND deplib_list ${deplib})
        else (${dash} STREQUAL "-")
          # otherwise, parse the name into a directory and a name
          get_filename_component (deplib_dir ${deplib} PATH)
          get_filename_component (deplib_orig ${deplib} NAME)
          string (REGEX REPLACE
                "^${esc_dl_pref}(.*)${esc_dl_suff}$"
                "\\1"
                deplib_name
                ${deplib_orig}
                )
          string (REGEX REPLACE
                "^${esc_ar_pref}(.*)${esc_ar_suff}$"
                "\\1"
                deplib_name
                ${deplib_name}
                )
          # directory and name each on their own; this is somewhat
          # unsatisfactory because it may be that a system dir is specified
          # by an earlier directory and you start picking up libraries from
          # there instead of the "closest" path here. also, the soversion
          # is more or less lost. remove system default path, to lessen the
          # chance that we pick the wrong library
          if (NOT ((deplib_dir STREQUAL "/usr/lib") OR
                           (deplib_dir STREQUAL "/usr/${CMAKE_INSTALL_LIBDIR}")))
                   list (APPEND deplib_list "-L${deplib_dir}")
          endif (NOT ((deplib_dir STREQUAL "/usr/lib") OR
                              (deplib_dir STREQUAL "/usr/${CMAKE_INSTALL_LIBDIR}")))
          # if there was no translation of the name, the library is named
          # unconventionally (.so.3gf, I'm looking at you), so pass this
          # name unmodified to the linker switch
          if (deplib_orig STREQUAL deplib_name)
                list (APPEND deplib_list "-l:${deplib_orig}")
          else (deplib_orig STREQUAL deplib_name)
                list (APPEND deplib_list "-l${deplib_name}")
          endif (deplib_orig STREQUAL deplib_name)
        endif (${dash} STREQUAL "-")
  endforeach (deplib)
  # caller determines whether we want it returned as a list or a string
  if ("${what}" STREQUAL "LIST")
        set (${outvar} ${deplib_list})
  else ("${what}" STREQUAL "LIST")
        set (${outvar} "${deplib_list}")
        string (REPLACE ";" " " ${outvar} "${${outvar}}")
  endif ("${what}" STREQUAL "LIST")
  set (${outvar} "${${outvar}}" PARENT_SCOPE)
endfunction (linker_cmdline what INTO outvar FROM)

# convert a list back to a command-line string
function (unseparate_args var_name prefix value)
  separate_arguments (value)
  foreach (item IN LISTS value)
        set (prefixed_item "${prefix}${item}")
        if (${var_name})
          set (${var_name} "${${var_name}} ${prefixed_item}")
        else (${var_name})
          set (${var_name} "${prefixed_item}")
        endif (${var_name})
  endforeach (item)
  set (${var_name} "${${var_name}}" PARENT_SCOPE)
endfunction (unseparate_args var_name prefix value)

# wrapper to set variables in pkg-config file
function (configure_pc_file name source dest prefix libdir includedir)
  # escape set of standard strings
  unseparate_args (includes "-I" "${${name}_INCLUDE_DIRS}")
  unseparate_args (defs "" "${${name}_DEFINITIONS}")
  linker_cmdline (STRING INTO libs FROM ${${name}_LIBRARIES})

  # necessary to make these variables visible to configure_file
  set (name "${${name}_NAME}")
  set (description "${${name}_DESCRIPTION}")
  set (major "${${name}_VERSION_MAJOR}")
  set (minor "${${name}_VERSION_MINOR}")
  set (target "${name}")
  linker_cmdline (STRING INTO target from ${target})

  configure_file (${source} ${dest} @ONLY)
endfunction (configure_pc_file name source dist prefix libdir includedir)
