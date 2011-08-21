
add_executable(oldapp
  old_app.cpp
)

add_executable(newapp
  new_app.cpp
)

set(TEST_FILES
  file_list.xml
  test-update.rb
)

foreach(TEST_FILE ${TEST_FILES})
  execute_process(
    COMMAND
	"${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_FILE}" "${CMAKE_CURRENT_BINARY_DIR}"
  )
endforeach()

