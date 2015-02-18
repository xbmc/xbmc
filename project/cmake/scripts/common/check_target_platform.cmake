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
          MATH(EXPR platform_length "${platform_length} - 1")
          string(SUBSTRING ${platform} 1 ${platform_length} platform)

          # check if the current platform does not match the extracted platform
          if (NOT ${platform} STREQUAL ${target_platform})
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
