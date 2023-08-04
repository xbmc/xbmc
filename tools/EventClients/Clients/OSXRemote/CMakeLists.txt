if(ENABLE_XBMCHELPER)
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

  set(SOURCES xbmcclientwrapper.mm
              xbmchelper_main.mm
              XBMCHelper.m
              HIDRemote/HIDRemote.m)

  set(HEADERS XBMCDebugHelpers.h)

  add_executable(XBMCHelper ${SOURCES} ${HEADERS})
  set_target_properties(XBMCHelper PROPERTIES
                                   RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools/darwin/runtime
                                   RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tools/darwin/runtime
                                   RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tools/darwin/runtime
                                   FOLDER Tools)
  target_link_libraries(XBMCHelper
                        PRIVATE ${SYSTEM_LDFLAGS}
                                "-framework IOKit"
                                "-framework Carbon"
                                "-framework Cocoa")
endif()
