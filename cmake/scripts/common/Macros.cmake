# This script holds the main functions used to construct the build system

# Include system specific macros but only if this file is included from
# kodi main project. It's not needed for kodi-addons project
# If CORE_SOURCE_DIR is set, it was called from kodi-addons project
# TODO: drop check if we ever integrate kodi-addons into kodi project
if(NOT CORE_SOURCE_DIR)
  include(${CMAKE_SOURCE_DIR}/cmake/scripts/${CORE_SYSTEM_NAME}/Macros.cmake)
endif()

# IDEs: Group source files in target in folders (file system hierarchy)
# Source: http://blog.audio-tk.com/2015/09/01/sorting-source-files-and-projects-in-folders-with-cmake-and-visual-studioxcode/
# Arguments:
#   target The target that shall be grouped by folders.
# Optional Arguments:
#   RELATIVE allows to specify a different reference folder.
function(source_group_by_folder target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "There is no target named '${target}'")
  endif()

  set(SOURCE_GROUP_DELIMITER "/")

  cmake_parse_arguments(arg "" "RELATIVE" "" ${ARGN})
  if(arg_RELATIVE)
    set(relative_dir ${arg_RELATIVE})
  else()
    set(relative_dir ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  get_property(files TARGET ${target} PROPERTY SOURCES)
  if(files)
    list(SORT files)

    if(CMAKE_GENERATOR STREQUAL Xcode)
      set_target_properties(${target} PROPERTIES SOURCES "${files}")
    endif()
  endif()
  foreach(file ${files})
    if(NOT IS_ABSOLUTE ${file})
      set(file ${CMAKE_CURRENT_SOURCE_DIR}/${file})
    endif()
    file(RELATIVE_PATH relative_file ${relative_dir} ${file})
    get_filename_component(dir "${relative_file}" DIRECTORY)
    if(NOT dir STREQUAL "${last_dir}")
      if(files)
        source_group("${last_dir}" FILES ${files})
      endif()
      set(files "")
    endif()
    set(files ${files} ${file})
    set(last_dir "${dir}")
  endforeach(file)
  if(files)
    source_group("${last_dir}" FILES ${files})
  endif()
endfunction()

# Add sources to main application
# Arguments:
#   name name of the library to add
# Implicit arguments:
#   ENABLE_STATIC_LIBS Build static libraries per directory
#   SOURCES the sources of the library
#   HEADERS the headers of the library (only for IDE support)
#   OTHERS  other library related files (only for IDE support)
# On return:
#   Library will be built, optionally added to ${core_DEPENDS}
#   Sets CORE_LIBRARY for calls for setting target specific options
function(core_add_library name)
  if(ENABLE_STATIC_LIBS)
    add_library(${name} STATIC ${SOURCES} ${HEADERS} ${OTHERS})
    set_target_properties(${name} PROPERTIES PREFIX "")
    set(core_DEPENDS ${name} ${core_DEPENDS} CACHE STRING "" FORCE)
    add_dependencies(${name} ${GLOBAL_TARGET_DEPS})

    # Adds global target to library. This propagates dep lib info (eg include_dir locations)
    target_link_libraries(${name} PRIVATE ${GLOBAL_TARGET_DEPS})

    set(CORE_LIBRARY ${name} PARENT_SCOPE)

    if(NOT MSVC)
      target_compile_options(${name} PUBLIC ${CORE_COMPILE_OPTIONS})
    endif()

    # Add precompiled headers to Kodi main libraries
    if(CORE_SYSTEM_NAME MATCHES windows)
      add_precompiled_header(${name} pch.h ${CMAKE_SOURCE_DIR}/xbmc/platform/win32/pch.cpp PCH_TARGET kodi)
      set_language_cxx(${name})
      target_link_libraries(${name} PUBLIC effects11)
    endif()
  else()
    foreach(src IN LISTS SOURCES HEADERS OTHERS)
      get_filename_component(src_path "${src}" ABSOLUTE)
      list(APPEND FILES ${src_path})
    endforeach()
    target_sources(lib${APP_NAME_LC} PRIVATE ${FILES})
    set(CORE_LIBRARY lib${APP_NAME_LC} PARENT_SCOPE)
  endif()
endfunction()

# Add a test library, and add sources to list for gtest integration macros
function(core_add_test_library name)
  if(ENABLE_STATIC_LIBS)
    add_library(${name} STATIC ${SOURCES} ${SUPPORTED_SOURCES} ${HEADERS} ${OTHERS})
    set_target_properties(${name} PROPERTIES PREFIX ""
                                             EXCLUDE_FROM_ALL 1
                                             FOLDER "Build Utilities/tests")
    add_dependencies(${name} ${GLOBAL_TARGET_DEPS})
    set(test_archives ${test_archives} ${name} CACHE STRING "" FORCE)

    if(NOT MSVC)
      target_compile_options(${name} PUBLIC ${CORE_COMPILE_OPTIONS})
    endif()

  endif()
  foreach(src IN LISTS SOURCES SUPPORTED_SOURCES HEADERS OTHERS)
    get_filename_component(src_path "${src}" ABSOLUTE)
    set(test_sources "${src_path}" ${test_sources} CACHE STRING "" FORCE)
  endforeach()
endfunction()

# Add addon dev kit headers to main application
# Arguments:
#   name name of the header part to add
function(core_add_devkit_header name)
  if(NOT ENABLE_STATIC_LIBS)
    core_add_library(addons_kodi-dev-kit_include_${name})
  endif()
endfunction()

# Add an dl-loaded shared library
# Arguments:
#   name name of the library to add
# Optional arguments:
#   WRAPPED wrap this library on POSIX platforms to add VFS support for
#           libraries that would otherwise not support it.
#   OUTPUT_DIRECTORY where to create the library in the build dir
#           (default: system)
# Implicit arguments:
#   SOURCES the sources of the library
#   HEADERS the headers of the library (only for IDE support)
#   OTHERS  other library related files (only for IDE support)
# On return:
#   Library target is defined and added to LIBRARY_FILES
function(core_add_shared_library name)
  cmake_parse_arguments(arg "WRAPPED" "OUTPUT_DIRECTORY" "" ${ARGN})
  if(arg_OUTPUT_DIRECTORY)
    set(OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY})
  else()
    if(NOT CORE_SYSTEM_NAME STREQUAL windows)
      set(OUTPUT_DIRECTORY system)
    endif()
  endif()
  if(CORE_SYSTEM_NAME STREQUAL windows)
    set(OUTPUT_NAME lib${name})
  else()
    set(OUTPUT_NAME lib${name}-${ARCH})
  endif()

  if(NOT arg_WRAPPED OR CORE_SYSTEM_NAME STREQUAL windows)
    add_library(${name} SHARED ${SOURCES} ${HEADERS} ${OTHERS})
    set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${OUTPUT_DIRECTORY}
                                             RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${OUTPUT_DIRECTORY}
                                             OUTPUT_NAME ${OUTPUT_NAME} PREFIX "")
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
      string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
      set_target_properties(${name} PROPERTIES  LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${OUTPUT_DIRECTORY}
                                                RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${OUTPUT_DIRECTORY})
    endforeach()

    set(LIBRARY_FILES ${LIBRARY_FILES} ${CMAKE_BINARY_DIR}/${OUTPUT_DIRECTORY}/${OUTPUT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX} CACHE STRING "" FORCE)
    add_dependencies(${APP_NAME_LC}-libraries ${name})
  else()
    add_library(${name} STATIC ${SOURCES} ${HEADERS} ${OTHERS})
    set_target_properties(${name} PROPERTIES POSITION_INDEPENDENT_CODE 1)
    core_link_library(${name} ${OUTPUT_DIRECTORY}/lib${name})

    if(NOT MSVC)
      target_compile_options(${name} PUBLIC ${CORE_COMPILE_OPTIONS})
    endif()
  endif()
endfunction()

# Sets the compile language for all C source files in a target to CXX.
# Needs to be called from the CMakeLists.txt that defines the target.
# Arguments:
#   target   target
function(set_language_cxx target)
  get_property(sources TARGET ${target} PROPERTY SOURCES)
  foreach(file IN LISTS sources)
    if(file MATCHES "\.c$")
      set_source_files_properties(${file} PROPERTIES LANGUAGE CXX)
    endif()
  endforeach()
endfunction()

# Add a data file to installation list with a mirror in build tree
# Mirroring files in the buildtree allows to execute the app from there.
# Arguments:
#   file        full path to file to mirror
# Optional Arguments:
#   NO_INSTALL: exclude file from installation target (only mirror)
#   DIRECTORY:  directory where the file should be mirrored to
#               (default: preserve tree structure relative to CMAKE_SOURCE_DIR)
#   KEEP_DIR_STRUCTURE: preserve tree structure even when DIRECTORY is set
# On return:
#   Files is mirrored to the build tree and added to ${install_data}
#   (if NO_INSTALL is not given).
function(copy_file_to_buildtree file)
  # Exclude autotools build artifacts and other blacklisted files in source tree.
  if(file MATCHES "(Makefile|\\.in|\\.xbt|\\.so|\\.dylib|\\.gitignore)$")
    if(VERBOSE)
      message(STATUS "copy_file_to_buildtree - ignoring file: ${file}")
    endif()
    return()
  endif()

  cmake_parse_arguments(arg "NO_INSTALL" "DIRECTORY;KEEP_DIR_STRUCTURE" "" ${ARGN})
  if(arg_DIRECTORY)
    set(outdir ${arg_DIRECTORY})
    if(arg_KEEP_DIR_STRUCTURE)
      get_filename_component(srcdir ${arg_KEEP_DIR_STRUCTURE} DIRECTORY)
      string(REPLACE "${CMAKE_SOURCE_DIR}/${srcdir}/" "" outfile ${file})
      if(NOT IS_DIRECTORY ${file})
        set(outdir ${outdir}/${outfile})
      endif()
    else()
      get_filename_component(outfile ${file} NAME)
      set(outfile ${outdir}/${outfile})
    endif()
  else()
    string(REPLACE "${CMAKE_SOURCE_DIR}/" "" outfile ${file})
    get_filename_component(outdir ${outfile} DIRECTORY)
  endif()

  if(NOT TARGET export-files)
    if(${CORE_SYSTEM_NAME} MATCHES "windows")
      set(_bundle_dir $<TARGET_FILE_DIR:${APP_NAME_LC}>)
    else()
      set(_bundle_dir ${CMAKE_BINARY_DIR})
    endif()
    file(REMOVE ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ExportFiles.cmake)
    add_custom_target(export-files ALL COMMENT "Copying files into build tree"
                                       COMMAND ${CMAKE_COMMAND} -DBUNDLEDIR=${_bundle_dir}
                                                                -P ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ExportFiles.cmake)
    set_target_properties(export-files PROPERTIES FOLDER "Build Utilities")
    # Add comment to ensure ExportFiles.cmake is created even if not used.
    file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ExportFiles.cmake "# Export files to build tree\n")
  endif()

  if(${CORE_SYSTEM_NAME} MATCHES "windows")
    # if DEPENDS_PATH in fille
    if(${file} MATCHES ${DEPENDS_PATH})
      file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ExportFiles.cmake
"file(GLOB filenames ${file})
foreach(filename \$\{filenames\})
  file(COPY \"\$\{filename\}\" DESTINATION \"\$\{BUNDLEDIR\}/${outdir}\")
endforeach()\n"
      )
    else()
      file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ExportFiles.cmake
               "file(COPY \"${file}\" DESTINATION \"\$\{BUNDLEDIR\}/${outdir}\")\n" )
    endif()
  else()
    if(NOT file STREQUAL ${CMAKE_BINARY_DIR}/${outfile})
      if(NOT IS_SYMLINK "${file}")
        if(VERBOSE)
          message(STATUS "copy_file_to_buildtree - copying file: ${file} -> ${CMAKE_BINARY_DIR}/${outfile}")
        endif()
        file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ExportFiles.cmake
             "file(COPY \"${file}\" DESTINATION \"${CMAKE_BINARY_DIR}/${outdir}\")\n" )
      else()
        if(VERBOSE)
          message(STATUS "copy_file_to_buildtree - copying symlinked file: ${file} -> ${CMAKE_BINARY_DIR}/${outfile}")
        endif()
        file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ExportFiles.cmake
             "execute_process(COMMAND \"\${CMAKE_COMMAND}\" -E copy_if_different \"${file}\" \"${CMAKE_BINARY_DIR}/${outfile}\")\n")
      endif()
    endif()

    if(NOT arg_NO_INSTALL)
      list(APPEND install_data ${outfile})
      set(install_data ${install_data} PARENT_SCOPE)
    endif()
  endif()
endfunction()

# Add data files to installation list with a mirror in build tree.
# reads list of files to install from a given list of text files.
# Arguments:
#   pattern globbing pattern for text files to read
# Optional Arguments:
#   NO_INSTALL: exclude files from installation target
# Implicit arguments:
#   CMAKE_SOURCE_DIR - root of source tree
# On return:
#   Files are mirrored to the build tree and added to ${install_data}
#   (if NO_INSTALL is not given).
function(copy_files_from_filelist_to_buildtree pattern)
  # copies files listed in text files to the buildtree
  # Input: [glob pattern: filepattern]
  cmake_parse_arguments(arg "NO_INSTALL" "" "" ${ARGN})
  list(APPEND pattern ${ARGN})
  list(SORT pattern)
  if(VERBOSE)
    message(STATUS "copy_files_from_filelist_to_buildtree - got pattern: ${pattern}")
  endif()
  foreach(pat ${pattern})
    file(GLOB filenames ${pat})
    foreach(filename ${filenames})
      string(STRIP ${filename} filename)
      core_file_read_filtered(fstrings ${filename})
      foreach(dir ${fstrings})
        string(CONFIGURE ${dir} dir)
        string(REPLACE " " ";" dir ${dir})
        list(GET dir 0 src)
        list(LENGTH dir len)
        if(len EQUAL 1)
          set(dest)
        elseif(len EQUAL 3)
          list(GET dir 1 opt)
          if(opt STREQUAL "KEEP_DIR_STRUCTURE")
            set(DIR_OPTION ${opt} ${src})
            if(VERBOSE)
              message(STATUS "copy_files_from_filelist_to_buildtree - DIR_OPTION: ${DIR_OPTION}")
            endif()
          endif()
          list(GET dir -1 dest)
        else()
          list(GET dir -1 dest)
        endif()

        if((${CMAKE_SOURCE_DIR}/${src} MATCHES ${DEPENDS_PATH}) OR
           (EXISTS ${CMAKE_SOURCE_DIR}/${src} AND (NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/${src} OR DIR_OPTION)))
          # If the path is in DEPENDS_PATH, pass through as is. This will be handled in a build time
          # glob of the location. This insures any dependencies built at build time can be bundled if 
          # required.
          # OR If the full path to an existing file is specified then add that single file.
          # Don't recursively add all files with the given name.
          set(files ${src})
        else()
          # Static path contents, so we can just glob at generation time
          file(GLOB_RECURSE files RELATIVE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/${src})
        endif()

        foreach(file ${files})
          if(arg_NO_INSTALL)
            copy_file_to_buildtree(${CMAKE_SOURCE_DIR}/${file} DIRECTORY ${dest} NO_INSTALL ${DIR_OPTION})
          else()
            copy_file_to_buildtree(${CMAKE_SOURCE_DIR}/${file} DIRECTORY ${dest} ${DIR_OPTION})
          endif()
        endforeach()
        set(DIR_OPTION)
      endforeach()
    endforeach()
  endforeach()
  set(install_data ${install_data} PARENT_SCOPE)
endfunction()

# helper macro to set modified variables in parent scope
macro(export_dep)
  set(SYSTEM_INCLUDES ${SYSTEM_INCLUDES} PARENT_SCOPE)
  set(DEPLIBS ${DEPLIBS} PARENT_SCOPE)
  set(DEP_DEFINES ${DEP_DEFINES} PARENT_SCOPE)
  set(${depup}_FOUND ${${depup}_FOUND} PARENT_SCOPE)
  mark_as_advanced(${depup}_LIBRARIES)
endmacro()

# split dependency specification to name and version
# Arguments:
#   depspec dependency specification that can optionally include a required
#           package version
#           syntax: [package name], [package name]>=[version] (minimum version),
#                   or [package name]=[version] (exact version)
#   name_outvar variable that should receive the package name
#   version_outvar variable that should receive the package version part (>=[version])
# On return:
#   ${name_outvar} and ${version_outvar} in caller scope are set to respective values.
#   ${version_outvar} may be unset if there is no specific version requested.
function(split_dependency_specification depspec name_outvar version_outvar)
  if(${depspec} MATCHES "^([^>]*)(>?=[0-9.]+)$")
    set(${name_outvar} ${CMAKE_MATCH_1} PARENT_SCOPE)
    set(${version_outvar} ${CMAKE_MATCH_2} PARENT_SCOPE)
  else()
    set(${name_outvar} ${depspec} PARENT_SCOPE)
    unset(${version_outvar} PARENT_SCOPE)
  endif()
endfunction()

# helper macro to split version info from req and call find_package
macro(find_package_with_ver package)
  set(_find_arguments "${ARGN}")
  if("${ARGV1}" MATCHES "^(>)?=([0-9.]+)$")
    # We have a version spec, parse it
    list(REMOVE_AT _find_arguments 0)
    # ">" not present? -> exact match
    if(NOT CMAKE_MATCH_1)
      list(INSERT _find_arguments 0 "EXACT")
    endif()
    find_package(${package} ${CMAKE_MATCH_2} ${_find_arguments})
  else()
    find_package(${package} ${_find_arguments})
  endif()
  unset(_find_arguments)
endmacro()

# add required dependencies of main application
# Arguments:
#   dep_list One or many dependency specifications (see split_dependency_specification)
#            for syntax). The dependency name is used uppercased as variable prefix.
# On return:
#   dependencies added to ${SYSTEM_INCLUDES}, ${DEPLIBS} and ${DEP_DEFINES}
function(core_require_dep)
  foreach(depspec ${ARGN})
    split_dependency_specification(${depspec} dep version)
    find_package_with_ver(${dep} ${version} REQUIRED)
    string(TOUPPER ${dep} depup)
    list(APPEND SYSTEM_INCLUDES ${${depup}_INCLUDE_DIRS})
    list(APPEND DEPLIBS ${${depup}_LIBRARIES})
    list(APPEND DEP_DEFINES ${${depup}_DEFINITIONS})
    export_dep()
  endforeach()
endfunction()

# helper macro for optional deps
macro(setup_enable_switch)
  string(TOUPPER ${dep} depup)
  if(${ARGV1})
    set(enable_switch ${ARGV1})
  else()
    set(enable_switch ENABLE_${depup})
  endif()
  # normal options are boolean, so we override set our ENABLE_FOO var to allow "auto" handling
  set(${enable_switch} "AUTO" CACHE STRING "Enable ${depup} support?")
endmacro()

# add optional dependencies of main application
# Arguments:
#   dep_list One or many dependency specifications (see split_dependency_specification)
#            for syntax). The dependency name is used uppercased as variable prefix.
# On return:
#   dependency optionally added to ${SYSTEM_INCLUDES}, ${DEPLIBS} and ${DEP_DEFINES}
function(core_optional_dep)
  foreach(depspec ${ARGN})
    set(_required False)
    split_dependency_specification(${depspec} dep version)
    setup_enable_switch()
    if(${enable_switch} STREQUAL AUTO)
      find_package_with_ver(${dep} ${version})
    elseif(${${enable_switch}})
      find_package_with_ver(${dep} ${version} REQUIRED)
      set(_required True)
    endif()

    if(${depup}_FOUND)
      list(APPEND SYSTEM_INCLUDES ${${depup}_INCLUDE_DIRS})
      list(APPEND DEPLIBS ${${depup}_LIBRARIES})
      list(APPEND DEP_DEFINES ${${depup}_DEFINITIONS})
      set(final_message ${final_message} "${depup} enabled: Yes")
      export_dep()
    elseif(_required)
      message(FATAL_ERROR "${depup} enabled but not found")
    else()
      set(final_message ${final_message} "${depup} enabled: No")
    endif()
  endforeach()
  set(final_message ${final_message} PARENT_SCOPE)
endfunction()

function(core_file_read_filtered result filepattern)
  # Reads STRINGS from text files
  #  with comments filtered out
  # Result: [list: result]
  # Input:  [glob pattern: filepattern]
  file(GLOB filenames ${filepattern})
  list(SORT filenames)
  foreach(filename ${filenames})
    if(VERBOSE)
      message(STATUS "core_file_read_filtered - filename: ${filename}")
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${filename})
    file(STRINGS ${filename} fstrings REGEX "^[^#//]")
    foreach(fstring ${fstrings})
      string(REGEX REPLACE "^(.*)#(.*)" "\\1" fstring ${fstring})
      string(REGEX REPLACE "[ \n\r\t]//.*" "" fstring ${fstring})
      string(STRIP ${fstring} fstring)
      list(APPEND filename_strings ${fstring})
    endforeach()
  endforeach()
  set(${result} ${filename_strings} PARENT_SCOPE)
endfunction()

function(core_add_subdirs_from_filelist files)
  # Adds subdirectories from a sorted list of files
  # Input: [list: filenames] [bool: sort]
  foreach(arg ${ARGN})
    list(APPEND files ${arg})
  endforeach()
  list(SORT files)
  if(VERBOSE)
    message(STATUS "core_add_subdirs_from_filelist - got pattern: ${files}")
  endif()
  foreach(filename ${files})
    string(STRIP ${filename} filename)
    core_file_read_filtered(fstrings ${filename})
    foreach(subdir ${fstrings})
      string(REPLACE " " ";" subdir ${subdir})
      list(GET subdir  0 subdir_src)
      list(GET subdir -1 subdir_dest)
      if(VERBOSE)
        message(STATUS "  core_add_subdirs_from_filelist - adding subdir: ${CMAKE_SOURCE_DIR}/${subdir_src} -> ${CORE_BUILD_DIR}/${subdir_dest}")
      endif()
      add_subdirectory(${CMAKE_SOURCE_DIR}/${subdir_src} ${CORE_BUILD_DIR}/${subdir_dest})
    endforeach()
  endforeach()
endfunction()

macro(core_add_optional_subdirs_from_filelist pattern)
  # Adds subdirectories from text files
  #  if the option(s) in the 3rd field are enabled
  # Input: [glob pattern: filepattern]
  foreach(arg ${ARGN})
    list(APPEND pattern ${arg})
  endforeach()
  foreach(elem ${pattern})
    string(STRIP ${elem} elem)
    list(APPEND filepattern ${elem})
  endforeach()

  file(GLOB filenames ${filepattern})
  list(SORT filenames)
  if(VERBOSE)
    message(STATUS "core_add_optional_subdirs_from_filelist - got pattern: ${filenames}")
  endif()

  foreach(filename ${filenames})
    if(VERBOSE)
      message(STATUS "core_add_optional_subdirs_from_filelist - reading file: ${filename}")
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${filename})
    file(STRINGS ${filename} fstrings REGEX "^[^#//]")
    foreach(line ${fstrings})
      string(REPLACE " " ";" line "${line}")
      list(GET line 0 subdir_src)
      list(GET line 1 subdir_dest)
      list(GET line 3 opts)
      foreach(opt ${opts})
        if(ENABLE_${opt})
          if(VERBOSE)
            message(STATUS "  core_add_optional_subdirs_from_filelist - adding subdir: ${CMAKE_SOURCE_DIR}/${subdir_src} -> ${CORE_BUILD_DIR}/${subdir_dest}")
          endif()
          add_subdirectory(${CMAKE_SOURCE_DIR}/${subdir_src} ${CORE_BUILD_DIR}/${subdir_dest})
        else()
          if(VERBOSE)
            message(STATUS "  core_add_optional_subdirs_from_filelist: OPTION ${opt} not enabled for ${subdir_src}, skipping subdir")
          endif()
        endif()
      endforeach()
    endforeach()
  endforeach()
endmacro()

# Generates an RFC2822 timestamp
#
# The following variable is set:
#   RFC2822_TIMESTAMP
function(rfc2822stamp)
  execute_process(COMMAND date -R
                  OUTPUT_VARIABLE RESULT)
  set(RFC2822_TIMESTAMP ${RESULT} PARENT_SCOPE)
endfunction()

# Generates an user stamp from git config info
#
# The following variable is set:
#   PACKAGE_MAINTAINER - user stamp in the form of "username <username@example.com>"
#                        if no git tree is found, value is set to "nobody <nobody@example.com>"
function(userstamp)
  find_package(Git)
  if(GIT_FOUND AND EXISTS ${CMAKE_SOURCE_DIR}/.git)
    execute_process(COMMAND ${GIT_EXECUTABLE} config user.name
                    OUTPUT_VARIABLE username
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} config user.email
                    OUTPUT_VARIABLE useremail
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(PACKAGE_MAINTAINER "${username} <${useremail}>" PARENT_SCOPE)
  else()
    set(PACKAGE_MAINTAINER "nobody <nobody@example.com>" PARENT_SCOPE)
  endif()
endfunction()

# Parses git info and sets variables used to identify the build
# Arguments:
#   stamp variable name to return
# Optional Arguments:
#   FULL: generate git HEAD commit in the form of 'YYYYMMDD-hash'
#         if git tree is dirty, value is set in the form of 'YYYYMMDD-hash-dirty'
#         if no git tree is found, value is set in the form of 'YYYYMMDD-nogitfound'
#         if FULL is not given, stamp is generated following the same process as above
#         but without 'YYYYMMDD'
# On return:
#   Variable is set with generated stamp to PARENT_SCOPE
function(core_find_git_rev stamp)
  # allow manual setting GIT_VERSION
  if(GIT_VERSION)
    set(${stamp} ${GIT_VERSION} PARENT_SCOPE)
    string(TIMESTAMP APP_BUILD_DATE "%Y%m%d" UTC)
    set(APP_BUILD_DATE ${APP_BUILD_DATE} PARENT_SCOPE)
  else()
    find_package(Git)
    if(GIT_FOUND AND EXISTS ${CMAKE_SOURCE_DIR}/.git)
      # get tree status i.e. clean working tree vs dirty (uncommitted or unstashed changes, etc.)
      execute_process(COMMAND ${GIT_EXECUTABLE} update-index --ignore-submodules -q --refresh
                      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
      execute_process(COMMAND ${GIT_EXECUTABLE} diff-files --ignore-submodules --quiet --
                      RESULT_VARIABLE status_code
                      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
        if(NOT status_code)
          execute_process(COMMAND ${GIT_EXECUTABLE} diff-index --ignore-submodules --quiet HEAD --
                        RESULT_VARIABLE status_code
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
        endif()
        # get HEAD commit SHA-1
        execute_process(COMMAND ${GIT_EXECUTABLE} log -n 1 --pretty=format:"%h" HEAD
                        OUTPUT_VARIABLE HASH
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
        string(REPLACE "\"" "" HASH ${HASH})

        if(status_code)
          string(CONCAT HASH ${HASH} "-dirty")
        endif()

      # get HEAD commit date
      execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:"%cd" --date=short HEAD
                      OUTPUT_VARIABLE DATE
                      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
      string(REPLACE "\"" "" DATE ${DATE})
      string(REPLACE "-" "" DATE ${DATE})

      # build date
      string(TIMESTAMP APP_BUILD_DATE "%Y%m%d" UTC)
      set(APP_BUILD_DATE ${APP_BUILD_DATE} PARENT_SCOPE)
    else()
      if(EXISTS ${CMAKE_SOURCE_DIR}/BUILDDATE)
        file(STRINGS ${CMAKE_SOURCE_DIR}/BUILDDATE DATE LIMIT_INPUT 8)
      else()
        string(TIMESTAMP DATE "%Y%m%d" UTC)
      endif()
      set(APP_BUILD_DATE ${DATE} PARENT_SCOPE)

      if(EXISTS ${CMAKE_SOURCE_DIR}/VERSION)
        file(STRINGS ${CMAKE_SOURCE_DIR}/VERSION HASH LIMIT_INPUT 16)
      else()
        set(HASH "nogitfound")
      endif()
    endif()
    cmake_parse_arguments(arg "FULL" "" "" ${ARGN})
    if(arg_FULL)
      set(${stamp} ${DATE}-${HASH} PARENT_SCOPE)
    else()
      set(${stamp} ${HASH} PARENT_SCOPE)
    endif()
  endif()
endfunction()

# Parses version.txt and versions.h and sets variables
# used to construct dirs structure, file naming, API version, etc.
#
# The following variables are set from version.txt:
#   APP_NAME - app name
#   APP_NAME_LC - lowercased app name
#   APP_NAME_UC - uppercased app name
#   APP_PACKAGE - Android full package name
#   COMPANY_NAME - company name
#   APP_WEBSITE - site url
#   ABOUT_WEBSITE - url for the about page of the app
#   DEV_NAME - name of the developer of the app
#   DOCS_WEBSITE - url for the documentation of the app
#   FORUM_WEBSITE - url for the forum/discussion board of the app
#   APP_VERSION_MAJOR - the app version major
#   APP_VERSION_MINOR - the app version minor
#   APP_VERSION_TAG - the app version tag
#   APP_VERSION_TAG_LC - lowercased app version tag
#   APP_VERSION - the app version (${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}-${APP_VERSION_TAG})
#   APP_ADDON_API - the addon API version in the form of 16.9.702
#   ADDON_REPOS - official addon repositories and their origin path delimited by pipe
#                 - e.g. repository.xbmc.org|https://mirrors.kodi.tv -
#                 (multiple repo/path-sets are delimited by comma)
#   FILE_VERSION - file version in the form of 16,9,702,0 - Windows only
#   JSONRPC_VERSION - the json api version in the form of 8.3.0
#
# Set various variables defined in "versions.h"
macro(core_find_versions)
  # kodi-addons project also calls this macro and uses CORE_SOURCE_DIR
  # to point to core base dir
  # Set CORE_SOURCE_DIR here, otherwise kodi main project fails
  # TODO: drop this code block and refactor the rest to use CMAKE_SOURCE_DIR
  # if we ever integrate kodi-addons into kodi project
  if(NOT CORE_SOURCE_DIR)
    set(CORE_SOURCE_DIR ${CMAKE_SOURCE_DIR})
  endif()

  include(CMakeParseArguments)
  core_file_read_filtered(version_list ${CORE_SOURCE_DIR}/version.txt)
  core_file_read_filtered(json_version ${CORE_SOURCE_DIR}/xbmc/interfaces/json-rpc/schema/version.txt)
  string(REGEX REPLACE "([^ ;]*) ([^;]*)" "\\1;\\2" version_list "${version_list};${json_version}")
  set(version_props
    ADDON_API
    ADDON_REPOS
    ABOUT_WEBSITE
    APP_NAME
    APP_PACKAGE
    COMPANY_NAME
    COPYRIGHT_YEARS
    DEV_NAME
    DOCS_WEBSITE
    FORUM_WEBSITE
    JSONRPC_VERSION
    PACKAGE_DESCRIPTION
    PACKAGE_IDENTITY
    PACKAGE_PUBLISHER
    VERSION_MAJOR
    VERSION_MINOR
    VERSION_TAG
    VERSION_CODE
    WEBSITE
  )
  cmake_parse_arguments(APP "" "${version_props}" "" ${version_list})

  if(NOT ${APP_VERSION_CODE} MATCHES "^[0-9]+\\.[0-9][0-9]?\\.[0-9][0-9]?[0-9]?$")
    message(FATAL_ERROR "VERSION_CODE was set to ${APP_VERSION_CODE} in version.txt, but it has to match '^\\d+\\.\\d{1,2}\\.\\d{1,3}$'")
  endif()
  set(APP_NAME ${APP_APP_NAME}) # inconsistency but APP_APP_NAME looks weird
  string(TOLOWER ${APP_APP_NAME} APP_NAME_LC)
  string(TOUPPER ${APP_APP_NAME} APP_NAME_UC)
  set(COMPANY_NAME ${APP_COMPANY_NAME})
  set(ABOUT_WEBSITE ${APP_ABOUT_WEBSITE})
  set(DEV_NAME ${APP_DEV_NAME})
  set(DOCS_WEBSITE ${APP_DOCS_WEBSITE})
  set(FORUM_WEBSITE ${APP_FORUM_WEBSITE})
  set(APP_VERSION ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR})
  # Let Flatpak builders etc override APP_PACKAGE
  # NOTE: We cannot declare an option() in top-level CMakeLists.txt
  # because of CMP0077.
  if(NOT APP_PACKAGE)
    set(APP_PACKAGE ${APP_APP_PACKAGE})
  endif()
  list(APPEND final_message "App package: ${APP_PACKAGE}")
  if(APP_VERSION_TAG)
    set(APP_VERSION ${APP_VERSION}-${APP_VERSION_TAG})
    string(TOLOWER ${APP_VERSION_TAG} APP_VERSION_TAG_LC)
  endif()
  string(REPLACE "." "," FILE_VERSION ${APP_ADDON_API}.0)
  set(ADDON_REPOS ${APP_ADDON_REPOS})
  set(JSONRPC_VERSION ${APP_JSONRPC_VERSION})

  # Set defines used in addon.xml.in and read from versions.h to set add-on
  # version parts automatically
  # This part is nearly identical to "AddonHelpers.cmake", except location of versions.h
  file(STRINGS ${CORE_SOURCE_DIR}/xbmc/addons/kodi-dev-kit/include/kodi/versions.h BIN_ADDON_PARTS)
  foreach(loop_var ${BIN_ADDON_PARTS})
    string(FIND "${loop_var}" "#define ADDON_" matchres)
    if("${matchres}" EQUAL 0)
      string(REGEX MATCHALL "[A-Z0-9._]+|[A-Z0-9._]+$" loop_var "${loop_var}")
      list(GET loop_var 0 include_name)
      list(GET loop_var 1 include_version)
      string(REGEX REPLACE ".*\"(.*)\"" "\\1" ${include_name} ${include_version})
    endif()
  endforeach(loop_var)

  # unset variables not used anywhere else
  unset(version_list)
  unset(APP_APP_NAME)
  unset(APP_COMPANY_NAME)
  unset(APP_APP_PACKAGE)
  unset(APP_JSONRPC_VERSION)
  unset(BIN_ADDON_PARTS)

  # bail if we can't parse version.txt
  if(NOT DEFINED APP_VERSION_MAJOR OR NOT DEFINED APP_VERSION_MINOR)
    message(FATAL_ERROR "Could not determine app version! Make sure that ${CORE_SOURCE_DIR}/version.txt exists")
  endif()
  if(NOT DEFINED JSONRPC_VERSION)
    message(FATAL_ERROR "Could not determine json-rpc version! Make sure that ${CORE_SOURCE_DIR}/xbmc/interfaces/json-rpc/schema/version.txt exists")
  endif()
endmacro()

# add-on xml's
# find all folders containing addon.xml.in and used to define
# ADDON_XML_OUTPUTS, ADDON_XML_DEPENDS and ADDON_INSTALL_DATA
macro(find_addon_xml_in_files)
  set(filter ${ARGV0})

  if(filter AND VERBOSE)
    message(STATUS "find_addon_xml_in_files: filtering ${filter}")
  endif()

  file(GLOB ADDON_XML_IN_FILE ${CMAKE_SOURCE_DIR}/addons/*/addon.xml.in)
  foreach(loop_var ${ADDON_XML_IN_FILE})
    list(GET loop_var 0 xml_name)

    string(REPLACE "/addon.xml.in" "" xml_name ${xml_name})
    string(REPLACE "${CORE_SOURCE_DIR}/" "" xml_name ${xml_name})

    list(APPEND ADDON_XML_DEPENDS "${CORE_SOURCE_DIR}/${xml_name}/addon.xml.in")
    if(filter AND NOT xml_name MATCHES ${filter})
      list(APPEND ADDON_XML_OUTPUTS "${CMAKE_BINARY_DIR}/${xml_name}/addon.xml")
    endif()

    # Read content of add-on folder to have on install
    file(GLOB ADDON_FILES "${CORE_SOURCE_DIR}/${xml_name}/*")
    foreach(loop_var ${ADDON_FILES})
      if(loop_var MATCHES "addon.xml.in")
        string(REPLACE "addon.xml.in" "addon.xml" loop_var ${loop_var})

        list(GET loop_var 0 file_name)
        string(REPLACE "${CORE_SOURCE_DIR}/" "" file_name ${file_name})
        list(APPEND ADDON_INSTALL_DATA "${file_name}")

        unset(file_name)
      endif()
    endforeach()
    unset(xml_name)
  endforeach()

  # Append also versions.h to depends
  list(APPEND ADDON_XML_DEPENDS "${CORE_SOURCE_DIR}/xbmc/addons/kodi-dev-kit/include/kodi/versions.h")
endmacro()
