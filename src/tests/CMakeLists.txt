
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/..")

# Create helper binaries for unit tests
add_executable(oldapp
  old_app.cpp
)
add_executable(newapp
  new_app.cpp
)

# Install data files required by unit tests
set(TEST_FILES
  file_list.xml
  v2_file_list.xml
  test-update.rb
)

foreach(TEST_FILE ${TEST_FILES})
  execute_process(
    COMMAND
	"${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_FILE}" "${CMAKE_CURRENT_BINARY_DIR}"
  )
endforeach()

# Add unit test binaries
add_executable(TestUpdateScript
  TestUpdateScript.cpp
)
target_link_libraries(TestUpdateScript
  updatershared
)
add_executable(TestUpdaterOptions
  TestUpdaterOptions.cpp
)
target_link_libraries(TestUpdaterOptions
  updatershared
)
add_executable(TestFileUtils
  TestFileUtils.cpp
)
target_link_libraries(TestFileUtils
  updatershared
)
  
if (APPLE)
	set_target_properties(TestUpdateScript PROPERTIES LINK_FLAGS "-framework Security -framework Cocoa")
	set_target_properties(TestUpdaterOptions PROPERTIES LINK_FLAGS "-framework Security -framework Cocoa")
endif()

