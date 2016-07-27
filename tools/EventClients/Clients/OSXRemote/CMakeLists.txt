list(APPEND CMAKE_MODULE_PATH ${CORE_SOURCE_DIR}/project/cmake/modules)

set(SOURCES xbmchelper_main.mm
            XBMCHelper.m
            xbmcclientwrapper.mm
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
