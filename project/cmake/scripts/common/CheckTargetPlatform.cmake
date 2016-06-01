# handle target platforms
function(check_target_platform dir target_platform build)
  # param[in] dir path/directory of the addon/dependency
  # param[in] target_platform target platform of the build
  # param[out] build Result whether the addon/dependency should be built for the specified target platform

  set(${build} FALSE)
  # check if the given directory exists and contains a platforms.txt
  if(EXISTS ${dir} AND EXISTS ${dir}/platforms.txt)
    # get all the specified platforms
    file(STRINGS ${dir}/platforms.txt platforms)
    separate_arguments(platforms)

    # check if the addon/dependency should be built for the current platform
    foreach(platform ${platforms})
      if(${platform} STREQUAL "all" OR ${platform} STREQUAL ${target_platform})
        set(${build} TRUE)
      else()
        # check if the platform is defined as "!<platform>"
        string(SUBSTRING ${platform} 0 1 platform_first)
        if(${platform_first} STREQUAL "!")
          # extract the platform
          string(LENGTH ${platform} platform_length)
          math(EXPR platform_length "${platform_length} - 1")
          string(SUBSTRING ${platform} 1 ${platform_length} platform)

          # check if the current platform does not match the extracted platform
          if(NOT ${platform} STREQUAL ${target_platform})
            set(${build} TRUE)
          endif()
        endif()
      endif()
    endforeach()
  else()
    set(${build} TRUE)
  endif()

  # make the ${build} variable available to the calling script
  set(${build} "${${build}}" PARENT_SCOPE)
endfunction()

function(check_install_permissions install_dir have_perms)
  # param[in] install_dir directory to check for write permissions
  # param[out] have_perms wether we have permissions to install to install_dir

  set(${have_perms} TRUE)
  execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${install_dir}/lib/kodi
                  COMMAND ${CMAKE_COMMAND} -E make_directory ${install_dir}/share/kodi
                  COMMAND ${CMAKE_COMMAND} -E touch ${install_dir}/lib/kodi/.cmake-inst-test ${install_dir}/share/kodi/.cmake-inst-test
                  RESULT_VARIABLE permtest)

  if(${permtest} GREATER 0)
    message(STATUS "check_install_permissions: ${permtest}")
    set(${have_perms} FALSE)
  endif()
  set(${have_perms} "${${have_perms}}" PARENT_SCOPE)

  if(EXISTS ${install_dir}/lib/kodi/.cmake-inst-test OR EXISTS ${install_dir}/share/kodi/.cmake-inst-test)
    file(REMOVE ${install_dir}/lib/kodi/.cmake-inst-test ${install_dir}/share/kodi/.cmake-inst-test)
  endif()
endfunction()
