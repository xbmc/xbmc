list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(Bluetooth REQUIRED)
find_package(CWiid REQUIRED)
find_package(GLU REQUIRED)

set(SOURCES CWIID_WiiRemote.cpp)

set(HEADERS CWIID_WiiRemote.h)

add_executable(${APP_NAME_LC}-wiiremote ${SOURCES} ${HEADERS})

target_include_directories(${APP_NAME_LC}-wiiremote
                           PRIVATE $<TARGET_PROPERTY:Bluetooth::Bluetooth,INTERFACE_INCLUDE_DIRECTORIES>
                                   ${CWIID_INCLUDE_DIRS})

target_link_libraries(${APP_NAME_LC}-wiiremote
                      PRIVATE ${SYSTEM_LDFLAGS}
                              Bluetooth::Bluetooth
                              ${CWIID_LIBRARIES})

target_compile_options(${APP_NAME_LC}-wiiremote PRIVATE ${ARCH_DEFINES})

