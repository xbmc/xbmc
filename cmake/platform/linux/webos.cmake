include(${CMAKE_SOURCE_DIR}/cmake/platform/${CORE_SYSTEM_NAME}/wayland.cmake)

# add wayland as platform, as we require it.
# saves reworking other assumptions for linux windowing as the platform name.
list(APPEND CORE_PLATFORM_NAME_LC wayland)

list(APPEND PLATFORM_REQUIRED_DEPS WaylandProtocolsWebOS PlayerAPIs PlayerFactory WebOSHelpers AcbAPI)
list(APPEND ARCH_DEFINES -DTARGET_WEBOS)
set(PLATFORM_OPTIONAL_DEPS_EXCLUDE CEC Alsa)
set(ENABLE_PULSEAUDIO ON CACHE BOOL "" FORCE)
set(TARGET_WEBOS TRUE)
set(PREFER_TOOLCHAIN_PATH ${TOOLCHAIN}/${HOST}/sysroot)
set(ICONV_LIBRARY ${CMAKE_INSTALL_PREFIX}/lib/libiconv.a CACHE FILEPATH "Iconv library" FORCE)
set(ICONV_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include CACHE PATH "Iconv include directory" FORCE)
