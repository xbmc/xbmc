include(${CORE_SOURCE_DIR}/cmake/scripts/common/CheckTargetPlatform.cmake)

# handle addon depends
function(add_addon_depends addon searchpath)
  # input: string addon string searchpath

  set(OUTPUT_DIR ${ADDON_DEPENDS_PATH})
  # look for platform-specific dependencies
  file(GLOB_RECURSE cmake_input_files ${searchpath}/${CORE_SYSTEM_NAME}/*.txt)
  # backward compatibility
  if(NOT cmake_input_files AND CORE_SYSTEM_NAME STREQUAL windowsstore)
    file(GLOB_RECURSE cmake_input_files ${searchpath}/windows/*.txt)
  endif()
  file(GLOB_RECURSE cmake_input_files2 ${searchpath}/common/*.txt)
  list(APPEND cmake_input_files ${cmake_input_files2})

  foreach(file ${cmake_input_files})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${file})
    if(NOT (file MATCHES CMakeLists.txt OR
            file MATCHES install.txt OR
            file MATCHES noinstall.txt OR
            file MATCHES "flags.*[.]txt" OR
            file MATCHES deps.txt OR
            file MATCHES "[a-z]+-deps[.]txt" OR
            file MATCHES platforms.txt))
      message(STATUS "Processing ${file}")
      file(STRINGS ${file} def)
      string(REPLACE " " ";" def ${def})
      list(LENGTH def deflength)
      get_filename_component(dir ${file} DIRECTORY)

      # get the id of the dependency
      if(NOT "${def}" STREQUAL "")
        # read the id from the file
        list(GET def 0 id)
      else()
        # read the id from the filename
        get_filename_component(id ${file} NAME_WE)
      endif()

      # check if the dependency has a platforms.txt
      set(platform_found FALSE)
      check_target_platform(${dir} ${CORE_SYSTEM_NAME} platform_found)

      if(${platform_found} AND NOT TARGET ${id})
        # determine the download URL of the dependency
        set(url "")
        if(deflength GREATER 1)
          list(GET def 1 url)
          message(STATUS "${id} url: ${url}")
        endif()

        # check if there are any library specific flags that need to be passed on
        if(EXISTS ${dir}/flags.txt)
          set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${dir}/flags.txt)
          file(STRINGS ${dir}/flags.txt extraflags)
          string(REPLACE " " ";" extraflags ${extraflags})

          message(STATUS "${id} extraflags: ${extraflags}")
        endif()

        if(EXISTS ${dir}/flags-${CPU}.txt)
          set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${dir}/flags-${CPU}.txt)
          file(STRINGS ${dir}/flags-${CPU}.txt archextraflags)
          string(REPLACE " " ";" archextraflags ${archextraflags})

          message(STATUS "${id} ${CPU} extraflags: ${archextraflags}")
          list(APPEND extraflags ${archextraflags})
        endif()

        set(BUILD_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
                       -DOUTPUT_DIR=${OUTPUT_DIR}
                       -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                       -DCMAKE_USER_MAKE_RULES_OVERRIDE=${CMAKE_USER_MAKE_RULES_OVERRIDE}
                       -DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX=${CMAKE_USER_MAKE_RULES_OVERRIDE_CXX}
                       -DCMAKE_INSTALL_PREFIX=${OUTPUT_DIR}
                       -DCORE_SYSTEM_NAME=${CORE_SYSTEM_NAME}
                       -DENABLE_STATIC=1
                       -DBUILD_SHARED_LIBS=0)
        # windows args
        if (CMAKE_SYSTEM_NAME STREQUAL WindowsStore OR CMAKE_SYSTEM_NAME STREQUAL Windows)
          list(APPEND BUILD_ARGS -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
                                 -DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION})
        endif()
        # if there are no make rules override files available take care of manually passing on ARCH_DEFINES
        if(NOT CMAKE_USER_MAKE_RULES_OVERRIDE AND NOT CMAKE_USER_MAKE_RULES_OVERRIDE_CXX)
          # make sure we create strings, not lists
          set(TMP_C_FLAGS "${CMAKE_C_FLAGS} ${ARCH_DEFINES}")
          set(TMP_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_DEFINES}")
          set(TMP_EXE_LINKER_FLAGS "-L${OUTPUT_DIR}/lib ${CMAKE_EXE_LINKER_FLAGS}")
          list(APPEND BUILD_ARGS -DCMAKE_C_FLAGS=${TMP_C_FLAGS}
                                 -DCMAKE_CXX_FLAGS=${TMP_CXX_FLAGS}
                                 -DCMAKE_EXE_LINKER_FLAGS=${TMP_EXE_LINKER_FLAGS})
        endif()

        if(CMAKE_TOOLCHAIN_FILE)
          list(APPEND BUILD_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
          message("toolchain specified")
          message(${BUILD_ARGS})
        endif()

        if(ADDON_EXTRA_ARGS)
          string(REPLACE " " ";" ADDON_EXTRA_ARGS ${ADDON_EXTRA_ARGS})
          list(APPEND BUILD_ARGS ${ADDON_EXTRA_ARGS})
          message("Addon Extra Args: ${ADDON_EXTRA_ARGS}")
        endif()

        # used for addons where need special folders to store there content (if
        # not set the addon define it byself).
        # e.g. Google Chromium addon where his git bring:
        # - "unable to create file" ... "Filename too long"
        # see also WARNING by Windows on: https://bitbucket.org/chromiumembedded/cef/wiki/MasterBuildQuickStart
        if(THIRD_PARTY_PATH)
          message(STATUS "Third party lib path specified")
          message(STATUS ${THIRD_PARTY_PATH})
          list(APPEND BUILD_ARGS -DTHIRD_PARTY_PATH=${THIRD_PARTY_PATH})
        endif()

        set(PATCH_COMMAND)

        # if there's a CMakeLists.txt use it to prepare the build
        if(EXISTS ${dir}/CMakeLists.txt)
          set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${dir}/CMakeLists.txt)
          list(APPEND PATCH_COMMAND COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dir}/CMakeLists.txt ${BUILD_DIR}/${id}/src/${id})
        endif()

        # check if we have patches to apply
        file(GLOB patches ${dir}/*.patch)
        list(SORT patches)
        foreach(patch ${patches})
          if(NOT PATCH_PROGRAM OR "${PATCH_PROGRAM}" STREQUAL "")
            if(NOT PATCH_EXECUTABLE)
              # find the path to the patch executable

              if(WIN32)
                # On Windows prioritize Git patch.exe
                find_package(Git)
                if(Git_FOUND)
                  get_filename_component(GIT_DIR ${GIT_EXECUTABLE} DIRECTORY)
                  get_filename_component(GIT_DIR ${GIT_DIR} DIRECTORY)
                endif()
                find_program(PATCH_EXECUTABLE NAMES patch.exe HINTS ${GIT_DIR} PATH_SUFFIXES usr/bin)
              else()
                find_program(PATCH_EXECUTABLE NAMES patch)
              endif()
              if(NOT PATCH_EXECUTABLE)
                message(FATAL_ERROR "Missing patch command (we looked in ${CMAKE_PREFIX_PATH})")
              endif()
            endif()

            set(PATCH_PROGRAM ${PATCH_EXECUTABLE})

            # On Windows "patch.exe" can only handle CR-LF line-endings.
            # Our patches have LF-only line endings - except when they
            # have been checked out as part of a dependency hosted on Git
            # and core.autocrlf=true.
            if(WIN32)
              file(READ ${patch} patch_content_hex HEX)
              # Force handle LF-only line endings
              if(NOT patch_content_hex MATCHES "0d0a")
                list(APPEND PATCH_PROGRAM --binary)
              endif()
            endif()
          endif()

          set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${patch})
          list(APPEND PATCH_COMMAND COMMAND ${PATCH_PROGRAM} -p1 -i ${patch})
        endforeach()


        # if there's an install.txt use it to properly install the built files
        set(INSTALL_COMMAND "")
        if(EXISTS ${dir}/install.txt)
          set(INSTALL_COMMAND INSTALL_COMMAND ${CMAKE_COMMAND}
                                              -DINPUTDIR=${BUILD_DIR}/${id}/src/${id}-build/
                                              -DINPUTFILE=${dir}/install.txt
                                              -DDESTDIR=${OUTPUT_DIR}
                                              -DENABLE_STATIC=1
                                              "${extraflags}"
                                              -P ${PROJECT_SOURCE_DIR}/install.cmake)
        elseif(EXISTS ${dir}/noinstall.txt)
          set(INSTALL_COMMAND INSTALL_COMMAND "")
        endif()

        # check if there's a platform-specific or generic deps.txt containing dependencies on other libraries
        if(EXISTS ${dir}/${CORE_SYSTEM_NAME}-deps.txt)
          file(STRINGS ${dir}/${CORE_SYSTEM_NAME}-deps.txt deps)
          message(STATUS "${id} depends: ${deps}")
        # backward compatibility
        elseif(CORE_SYSTEM_NAME STREQUAL windowsstore AND EXISTS ${dir}/windows-deps.txt)
          file(STRINGS ${dir}/windows-deps.txt deps)
          message(STATUS "${id} depends: ${deps}")
        elseif(EXISTS ${dir}/deps.txt)
          set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${dir}/deps.txt)
          file(STRINGS ${dir}/deps.txt deps)
          message(STATUS "${id} depends: ${deps}")
        else()
          set(deps)
        endif()

        if(CROSS_AUTOCONF AND AUTOCONF_FILES)
          foreach(afile ${AUTOCONF_FILES})
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${afile})
            list(APPEND PATCH_COMMAND COMMAND ${CMAKE_COMMAND} -E echo "AUTOCONF: copying ${afile} to ${BUILD_DIR}/${id}/src/${id}")
            list(APPEND PATCH_COMMAND COMMAND ${CMAKE_COMMAND} -E copy_if_different ${afile} ${BUILD_DIR}/${id}/src/${id})
          endforeach()
        endif()

        # prepare the setup of the call to externalproject_add()
        set(EXTERNALPROJECT_SETUP PREFIX ${BUILD_DIR}/${id}
                                  CMAKE_ARGS ${extraflags} ${BUILD_ARGS}
                                  PATCH_COMMAND ${PATCH_COMMAND}
                                  ${INSTALL_COMMAND})

        if(CMAKE_VERSION VERSION_GREATER 3.5.9)
          list(APPEND EXTERNALPROJECT_SETUP GIT_SHALLOW 1)
        endif()

        # if there's an url defined we need to pass that to externalproject_add()
        if(DEFINED url AND NOT "${url}" STREQUAL "")
          # check if there's a third parameter in the file
          if(deflength GREATER 2)
            # the third parameter is considered as a revision of a git repository
            list(GET def 2 revision)

            externalproject_add(${id}
                                GIT_REPOSITORY ${url}
                                GIT_TAG ${revision}
                                ${EXTERNALPROJECT_SETUP})

            # For patchfiles to work, disable (users globally set) autocrlf=true
            if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_GREATER 3.7)
              message(AUTHOR_WARNING "Make use of GIT_CONFIG")
            endif()
            if(WIN32 AND patches)
              externalproject_add_step(${id} gitconfig
                                       COMMAND git config core.autocrlf false
                                       COMMAND git rm -rf --cached .
                                       COMMAND git reset --hard HEAD
                                       COMMENT "Performing gitconfig step: Disabling autocrlf to enable patching for '${id}'"
                                       DEPENDERS patch
                                       WORKING_DIRECTORY <SOURCE_DIR>)
            endif()
          else()
            set(CONFIGURE_COMMAND "")
            if(NOT WIN32)
              # manually specify the configure command to be able to pass in the custom PKG_CONFIG_PATH
              set(CONFIGURE_COMMAND PKG_CONFIG_PATH=${OUTPUT_DIR}/lib/pkgconfig
                                    ${CMAKE_COMMAND} -DCMAKE_LIBRARY_PATH=${OUTPUT_DIR}/lib ${extraflags} ${BUILD_ARGS}
                                    ${BUILD_DIR}/${id}/src/${id}
                                    -DPACKAGE_CONFIG_PATH=${OUTPUT_DIR}/lib/pkgconfig
                                    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                                    -DOUTPUT_DIR=${OUTPUT_DIR}
                                    -DCMAKE_PREFIX_PATH=${OUTPUT_DIR}
                                    -DCMAKE_INSTALL_PREFIX=${OUTPUT_DIR}
                                    -DCMAKE_INCLUDE_PATH=${OUTPUT_DIR}/include)
            endif()

            set(DOWNLOAD_DIR ${BUILD_DIR}/download)
            if(EXISTS ${dir}/${id}.sha256)
              file(STRINGS ${dir}/${id}.sha256 sha256sum)
              list(GET sha256sum 0 sha256sum)
              set(URL_HASH_COMMAND URL_HASH SHA256=${sha256sum})
              if(TARBALL_DIR)
                set(DOWNLOAD_DIR ${TARBALL_DIR})
              endif()
            else()
              unset(URL_HASH_COMMAND)
              message(AUTHOR_WARNING "${dir}/${id}.sha256 is missing")
            endif()

            externalproject_add(${id}
                                URL ${url}
                                "${URL_HASH_COMMAND}"
                                DOWNLOAD_DIR ${DOWNLOAD_DIR}
                                CONFIGURE_COMMAND ${CONFIGURE_COMMAND}
                                ${EXTERNALPROJECT_SETUP})
          endif()
        else()
          externalproject_add(${id}
                              SOURCE_DIR ${dir}
                              ${EXTERNALPROJECT_SETUP})
        endif()

        if(deps)
          add_dependencies(${id} ${deps})
        endif()
      endif()

      # if the dependency is available for the target platform add it to the list of the addon's dependencies
      # (even if the target already exists as it still has to be built before the addon)
      if(${platform_found})
        list(APPEND ${addon}_DEPS ${id})
      endif()
    endif()
  endforeach()

  # make the ${addon}_DEPS variable available to the calling script
  set(${addon}_DEPS "${${addon}_DEPS}" PARENT_SCOPE)
endfunction()

