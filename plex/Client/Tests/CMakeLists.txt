include_directories(${root}/lib/gtest ${root}/lib/gtest/include)
add_executable(PlayQueueManager_Tests PlayQueueManager_Tests.cpp $<TARGET_OBJECTS:xbmctest> $<TARGET_OBJECTS:PlexMain>)
target_link_libraries(PlayQueueManager_Tests xbmc pcre gtest ${CONFIG_INTERNAL_LIBS} ${CONFIG_PLEX_LINK_LIBRARIES})
add_test(PlayQueueManager ${EXECUTABLE_OUTPUT_PATH}/PlayQueueManager_Tests)

add_custom_target(all_tests DEPENDS PlayQueueManager_Tests)
add_custom_target(run_tests COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS all_tests)