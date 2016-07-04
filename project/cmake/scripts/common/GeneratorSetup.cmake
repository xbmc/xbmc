# Configure single-/multiconfiguration generators and variables
#
# CORE_BUILD_CONFIG that is set to
#   - CMAKE_BUILD_TYPE for single configuration generators such as make, nmake
#   - a variable that expands on build time to the current configuration for
#     multi configuration generators such as VS or Xcode
if(CMAKE_CONFIGURATION_TYPES)
  if(CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must not be defined for multi-configuration generators")
  endif()
  set(CORE_BUILD_CONFIG ${CMAKE_CFG_INTDIR})
  message(STATUS "Generator: Multi-configuration (${CMAKE_GENERATOR})")
else()
  if(CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}
        CACHE STRING "Choose build type (${CMAKE_BUILD_TYPES})" FORCE)
  else()
    # Set default
    set(CMAKE_BUILD_TYPE Release
        CACHE STRING "Choose build type (${CMAKE_BUILD_TYPES})" FORCE)
  endif()
  set(CORE_BUILD_CONFIG ${CMAKE_BUILD_TYPE})
  message(STATUS "Generator: Single-configuration: ${CMAKE_BUILD_TYPE} (${CMAKE_GENERATOR})")
endif()

# Print CMake version
message(STATUS "CMake Version: ${CMAKE_VERSION}")

# Deal with CMake special cases
if(CMAKE_VERSION VERSION_EQUAL 3.5.1)
  message(WARNING "CMake 3.5.1 introduced a crash during configuration. "
                  "Please consider upgrading to 3.5.2 (cmake.org/Bug/view.php?id=16044)")
endif()

# Darwin needs CMake 3.4
if(APPLE AND CMAKE_VERSION VERSION_LESS 3.4)
  message(WARNING "Build on Darwin requires CMake 3.4 or later (tdb library support) "
                  "or the usage of the patched version in depends.")
endif()

# Ninja needs CMake 3.2 due to ExternalProject BUILD_BYPRODUCTS usage
if(CMAKE_GENERATOR STREQUAL Ninja AND CMAKE_VERSION VERSION_LESS 3.2)
  message(FATAL_ERROR "Generator: Ninja requires CMake 3.2 or later")
endif()
