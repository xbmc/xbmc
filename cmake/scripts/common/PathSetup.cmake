# Platform path setup
include(cmake/scripts/${CORE_SYSTEM_NAME}/PathSetup.cmake)

# Fallback install location for dependencies built
if(NOT DEPENDS_PATH)
  set(DEPENDS_PATH "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}")
endif()
