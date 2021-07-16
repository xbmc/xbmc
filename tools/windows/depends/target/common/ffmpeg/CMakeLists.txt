cmake_minimum_required(VERSION 3.13)

project(ffmpeg LANGUAGES C)

include(CheckSymbolExists)
check_symbol_exists(_X86_ "Windows.h" _X86_)
check_symbol_exists(_AMD64_ "Windows.h" _AMD64_)
check_symbol_exists(_ARM_ "Windows.h" _ARM_)

if(_X86_)
	set(TARGET_OS win32)
	set(ARCH x86)
elseif(_AMD64_)
	set(TARGET_OS win64)
	set(ARCH x86_64)
elseif(_ARM_)
	set(TARGET_OS win32)
	set(ARCH arm)
else()
	message(FATAL_ERROR "Unsupported target architecture")
endif()

set(EXTRA_CFLAGS "-I${CMAKE_INSTALL_PREFIX}/include -DWIN32_LEAN_AND_MEAN -MD")
set(EXTRA_LDFLAGS "-LIBPATH:${CMAKE_INSTALL_PREFIX}/lib")
# -NODEFAULTLIB:msvcrt

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
	set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -DNTDDI_VERSION=NTDDI_WIN7 -D_WIN32_WINNT=_WIN32_WINNT_WIN7")
elseif(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
	set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -DWINAPI_FAMILY=WINAPI_FAMILY_APP -DNTDDI_VERSION=NTDDI_WIN10 -D_WIN32_WINNT=_WIN32_WINNT_WIN10")
	set(EXTRA_LDFLAGS "${EXTRA_LDFLAGS} -APPCONTAINER WindowsApp.lib")
endif()

set(FFMPEG_OPTS
	--arch=${ARCH}
	--extra-cflags=${EXTRA_CFLAGS}
	--extra-ldflags=${EXTRA_LDFLAGS}
	--prefix=${CMAKE_INSTALL_PREFIX}
	--target-os=${TARGET_OS}
	--toolchain=msvc
	--disable-asm
	--disable-avdevice
	--disable-crystalhd
	--disable-cuda
	--disable-cuvid
	--disable-debug
	--disable-devices
	--disable-doc
	--disable-dxva2
	--disable-gnutls
	--disable-mediafoundation
	--disable-nvenc
	--disable-programs
	--disable-shared
	--enable-encoder=ac3,aac,wmav2,png,mjpeg
	--enable-gpl
	--enable-muxer=spdif,adts,asf,ipod
	--enable-nonfree
	--enable-openssl
	--enable-postproc
	--enable-protocol=http
	--enable-runtime-cpudetect
	--enable-static
	--enable-w32threads
	--enable-zlib
)

if(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
	list(APPEND FFMPEG_OPTS --enable-cross-compile)
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E env "PATH=${MSYS_ROOT_PATH}/usr/bin;$ENV{PATH}"
	${MSYS_ROOT_PATH}/usr/bin/bash ${CMAKE_CURRENT_SOURCE_DIR}/configure ${FFMPEG_OPTS})
add_custom_target(${PROJECT_NAME} ALL
	COMMAND ${CMAKE_COMMAND} -E env "PATH=${MSYS_ROOT_PATH}/usr/bin;$ENV{PATH}"
		${MSYS_ROOT_PATH}/usr/bin/bash --noprofile --norc -c "make -j `getconf _NPROCESSORS_ONLN`")
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E env PATH=${MSYS_ROOT_PATH}/usr/bin ${MSYS_ROOT_PATH}/usr/bin/bash --noprofile --norc -c \"make -j `getconf _NPROCESSORS_ONLN` install\")")
