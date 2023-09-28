include(${CMAKE_SOURCE_DIR}/cmake/platform/${CORE_SYSTEM_NAME}/wayland.cmake)

# add wayland as platform, as we require it.
# saves reworking other assumptions for linux windowing as the platform name.
list(APPEND CORE_PLATFORM_NAME_LC wayland)

list(APPEND PLATFORM_REQUIRED_DEPS WaylandProtocolsWebOS PlayerAPIs PlayerFactory WebOSHelpers AcbAPI)
list(APPEND ARCH_DEFINES -DTARGET_WEBOS)
set(ENABLE_PULSEAUDIO OFF CACHE BOOL "" FORCE)
set(TARGET_WEBOS TRUE)
set(PREFER_TOOLCHAIN_PATH ${TOOLCHAIN}/${HOST}/sysroot)
