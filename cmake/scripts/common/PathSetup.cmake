# Platform path setup
include(cmake/scripts/${CORE_SYSTEM_NAME}/PathSetup.cmake)

# Fallback install location for dependencies built
if(NOT DEPENDS_PATH)
  set(DEPENDS_PATH "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}")
endif()

# If a platform sets a Depends_path for libs, prepend to cmake prefix path
# for when searching for libs (eg find_package)
if(DEPENDS_PATH)
  list(PREPEND CMAKE_PREFIX_PATH ${DEPENDS_PATH})
endif()
