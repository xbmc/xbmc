list(APPEND CMAKE_MODULE_PATH ${CORE_SOURCE_DIR}/project/cmake/modules)

find_package(Bluetooth REQUIRED)
find_package(CWiid REQUIRED)

set(SOURCES CWIID_WiiRemote.cpp)

set(HEADERS CWIID_WiiRemote.h)

add_executable(${APP_NAME_LC}-wiiremote ${SOURCES} ${HEADERS})

target_include_directories(${APP_NAME_LC}-wiiremote
                           PRIVATE ${BLUETOOTH_INCLUDE_DIRS}
                                   ${CWIID_INCLUDE_DIRS})

target_link_libraries(${APP_NAME_LC}-wiiremote
                      PRIVATE ${SYSTEM_LDFLAGS}
                              ${BLUETOOTH_LIBRARIES}
                              ${CWIID_LIBRARIES})

target_compile_options(${APP_NAME_LC}-wiiremote PRIVATE ${ARCH_DEFINES})

