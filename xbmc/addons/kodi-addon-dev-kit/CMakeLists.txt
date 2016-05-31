project(kodi-addon-dev-kit)

cmake_minimum_required(VERSION 3.1.0)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)

if(DOXYGEN_FOUND)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
endif()

add_subdirectory(src/api2/kodi-addon-sharedlibrary)

install(DIRECTORY include/kodi DESTINATION include)
