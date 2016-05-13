project(kodi-addon-dev-kit)

cmake_minimum_required(VERSION 3.0.0)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)

if(DOXYGEN_FOUND)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
endif(DOXYGEN_FOUND)

add_subdirectory(src/api2/kodi-addon-sharedlibrary)
if(BUILD_DEV_LEVEL)
  add_subdirectory(src/api3/kodi-addon-sharedlibrary)
endif(BUILD_DEV_LEVEL)

install(DIRECTORY include/kodi DESTINATION include)
