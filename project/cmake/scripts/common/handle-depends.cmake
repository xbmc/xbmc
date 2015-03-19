include(${APP_ROOT}/project/cmake/scripts/common/check_target_platform.cmake)

# handle addon depends
function(add_addon_depends addon searchpath)
  # input: string addon string searchpath

  set(OUTPUT_DIR ${DEPENDS_PATH})
  file(GLOB_RECURSE cmake_input_files ${searchpath}/${CORE_SYSTEM_NAME}/*.txt)
  file(GLOB_RECURSE cmake_input_files2 ${searchpath}/common/*.txt)
  list(APPEND cmake_input_files ${cmake_input_files2})

  foreach(file ${cmake_input_files})
    if(NOT (file MATCHES CMakeLists.txt OR
            file MATCHES install.txt OR
            file MATCHES noinstall.txt OR
            file MATCHES flags.txt OR
            file MATCHES deps.txt OR
            file MATCHES platforms.txt))
      message(STATUS "Processing ${file}")
      file(STRINGS ${file} def)
      separate_arguments(def)
      list(LENGTH def deflength)
      get_filename_component(dir ${file} PATH)

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
          file(STRINGS ${dir}/flags.txt extraflags)
          separate_arguments(extraflags)
          message(STATUS "${id} extraflags: ${extraflags}")
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
        # if there are no make rules override files available take care of manually passing on ARCH_DEFINES
        if(NOT CMAKE_USER_MAKE_RULES_OVERRIDE AND NOT CMAKE_USER_MAKE_RULES_OVERRIDE_CXX)
          # make sure we create strings, not lists
          set(TMP_C_FLAGS "${CMAKE_C_FLAGS} ${ARCH_DEFINES}")
          set(TMP_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_DEFINES}")
          list(APPEND BUILD_ARGS -DCMAKE_C_FLAGS=${TMP_C_FLAGS}
                                 -DCMAKE_CXX_FLAGS=${TMP_CXX_FLAGS})
        endif()

        if(CMAKE_TOOLCHAIN_FILE)
          list(APPEND BUILD_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
          MESSAGE("toolchain specified")
          MESSAGE(${BUILD_ARGS})
        endif()

        # if there's a CMakeLists.txt use it to prepare the build
        set(PATCH_FILE ${BUILD_DIR}/${id}/tmp/patch.cmake)
        if(EXISTS ${dir}/CMakeLists.txt)
          file(APPEND ${PATCH_FILE}
               "file(COPY ${dir}/CMakeLists.txt
                   DESTINATION ${BUILD_DIR}/${id}/src/${id})\n")
        endif()

        # check if we have patches to apply
        file(GLOB patches ${dir}/*.patch)
        list(SORT patches)
        foreach(patch ${patches})
          if(NOT PATCH_PROGRAM OR "${PATCH_PROGRAM}" STREQUAL "")
            if(NOT PATCH_EXECUTABLE)
              # find the path to the patch executable
              find_program(PATCH_EXECUTABLE NAMES patch)

              if(NOT PATCH_EXECUTABLE)
                message(FATAL_ERROR "Missing patch command (we looked in ${CMAKE_PREFIX_PATH})")
              endif()
            endif()

            # on windows "patch.exe" can only handle CR-LF line-endings so we
            # need to force it to also handle LF-only line endings
            set(PATCH_PROGRAM ${PATCH_EXECUTABLE})
            if(WIN32)
              set(PATCH_PROGRAM "\"${PATCH_PROGRAM}\" --binary")
            endif()
          endif()

          file(APPEND ${PATCH_FILE}
               "execute_process(COMMAND ${PATCH_PROGRAM} -p1 -i \"${patch}\")\n")
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

        # check if there's a deps.txt containing dependencies on other libraries
        if(EXISTS ${dir}/deps.txt)
          file(STRINGS ${dir}/deps.txt deps)
          message(STATUS "${id} depends: ${deps}")
        else()
          set(deps)
        endif()

        if(CROSS_AUTOCONF AND AUTOCONF_FILES)
          foreach(afile ${AUTOCONF_FILES})
            file(APPEND ${PATCH_FILE}
                 "message(STATUS \"AUTOCONF: copying ${afile} to ${BUILD_DIR}/${id}/src/${id}\")\n
                 file(COPY ${afile} DESTINATION ${BUILD_DIR}/${id}/src/${id})\n")
          endforeach()
        endif()

        # if the patch file exists we need to set the PATCH_COMMAND
        set(PATCH_COMMAND "")
        if (EXISTS ${PATCH_FILE})
          set(PATCH_COMMAND ${CMAKE_COMMAND} -P ${PATCH_FILE})
        endif()

        # prepare the setup of the call to externalproject_add()
        set(EXTERNALPROJECT_SETUP PREFIX ${BUILD_DIR}/${id}
                                  CMAKE_ARGS ${extraflags} ${BUILD_ARGS}
                                  PATCH_COMMAND ${PATCH_COMMAND}
                                  "${INSTALL_COMMAND}")

        # if there's an url defined we need to pass that to externalproject_add()
        if(DEFINED url AND NOT "${url}" STREQUAL "")
          # check if there's a third parameter in the file
          if(deflength GREATER 2)
            # the third parameter is considered as a revision of a git repository
            list(GET def 2 revision)

            externalproject_add(${id}
                                GIT_REPOSITORY ${url}
                                GIT_TAG ${revision}
                                "${EXTERNALPROJECT_SETUP}")
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
                                    -DCMAKE_EXE_LINKER_FLAGS=-L${OUTPUT_DIR}/lib
                                    -DCMAKE_INCLUDE_PATH=${OUTPUT_DIR}/include)
            endif()

            externalproject_add(${id}
                                URL ${url}
                                DOWNLOAD_DIR ${BUILD_DIR}/download
                                CONFIGURE_COMMAND ${CONFIGURE_COMMAND}
                                "${EXTERNALPROJECT_SETUP}")
          endif()
        else()
          externalproject_add(${id}
                              SOURCE_DIR ${dir}
                              "${EXTERNALPROJECT_SETUP}")
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

