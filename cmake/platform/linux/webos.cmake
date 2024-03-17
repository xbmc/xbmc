include(${CMAKE_SOURCE_DIR}/cmake/platform/${CORE_SYSTEM_NAME}/wayland.cmake)

# add wayland as platform, as we require it.
# saves reworking other assumptions for linux windowing as the platform name.
list(APPEND CORE_PLATFORM_NAME_LC wayland)

list(APPEND PLATFORM_REQUIRED_DEPS WaylandProtocolsWebOS PlayerAPIs>=1.0.0 PlayerFactory>=1.0.0 WebOSHelpers>=2.0.0 AcbAPI)
list(APPEND PLATFORM_OPTIONAL_DEPS LibDovi)
list(APPEND ARCH_DEFINES -DTARGET_WEBOS)

set(PLATFORM_OPTIONAL_DEPS_EXCLUDE CEC)
set(ENABLE_PULSEAUDIO OFF CACHE BOOL "" FORCE)
set(TARGET_WEBOS TRUE)
set(PREFER_TOOLCHAIN_PATH ${TOOLCHAIN}/${HOST}/sysroot)
