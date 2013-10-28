
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/..")

if (APPLE)
  set(HELPER_SHARED_SOURCES ../StlSymbolsLeopard.cpp)
endif()

# Create helper binaries for unit tests
add_executable(oldapp
  old_app.cpp
  ${HELPER_SHARED_SOURCES}
)
add_executable(newapp
  new_app.cpp
  ${HELPER_SHARED_SOURCES}
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
macro(ADD_UPDATER_TEST CLASS)
  set(TEST_TARGET updater_${CLASS})
  add_executable(${TEST_TARGET} ${CLASS}.cpp)
  target_link_libraries(${TEST_TARGET} updatershared)
  add_test(${TEST_TARGET} ${TEST_TARGET})
  if (APPLE)
	  set_target_properties(${TEST_TARGET} PROPERTIES LINK_FLAGS "-framework Security -framework Cocoa")
  endif()
endmacro()

add_updater_test(TestUpdateScript)
add_updater_test(TestUpdaterOptions)
add_updater_test(TestFileUtils)

# Add updater that that performs a complete update install
# and checks the result
find_program(RUBY_BIN ruby)
add_test(updater_TestUpdateInstall ${RUBY_BIN} test-update.rb)
  
