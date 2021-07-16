cmake_minimum_required(VERSION 3.13)

project(cdio VERSION 2.1.0 LANGUAGES C)

set(VERSION ${PROJECT_VERSION})
math(EXPR LIBCDIO_VERSION_NUM "${PROJECT_VERSION_MAJOR} * 10000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")
configure_file(
	include/cdio/version.h.in
	${CMAKE_CURRENT_BINARY_DIR}/cdio/version.h
	@ONLY
)

add_library(${PROJECT_NAME}
	${CMAKE_CURRENT_BINARY_DIR}/cdio/version.h
	include/cdio/audio.h
	include/cdio/bytesex.h
	include/cdio/bytesex_asm.h
	include/cdio/cd_types.h
	include/cdio/cdio.h
	include/cdio/cdtext.h
	include/cdio/device.h
	include/cdio/disc.h
	include/cdio/ds.h
	include/cdio/dvd.h
	include/cdio/ecma_167.h
	include/cdio/iso9660.h
	include/cdio/logging.h
	include/cdio/memory.h
	include/cdio/posix.h
	include/cdio/read.h
	include/cdio/rock.h
	include/cdio/sector.h
	include/cdio/track.h
	include/cdio/types.h
	include/cdio/udf.h
	include/cdio/udf_file.h
	include/cdio/udf_time.h
	include/cdio/utf8.h
	include/cdio/util.h
	include/cdio/xa.h
	lib/driver/_cdio_generic.c
	lib/driver/_cdio_stdio.c
	lib/driver/_cdio_stdio.h
	lib/driver/_cdio_stream.c
	lib/driver/_cdio_stream.h
	lib/driver/abs_path.c
	lib/driver/aix.c
	lib/driver/audio.c
	lib/driver/cd_types.c
	lib/driver/cdio.c
	lib/driver/cdio_assert.h
	lib/driver/cdio_private.h
	lib/driver/cdtext.c
	lib/driver/cdtext_private.h
	lib/driver/device.c
	lib/driver/disc.c
	lib/driver/ds.c
	lib/driver/filemode.h
	lib/driver/FreeBSD/freebsd.c
	lib/driver/FreeBSD/freebsd.h
	lib/driver/FreeBSD/freebsd_cam.c
	lib/driver/FreeBSD/freebsd_ioctl.c
	lib/driver/generic.h
	lib/driver/gnu_linux.c
	lib/driver/image.h
	lib/driver/image/bincue.c
	lib/driver/image/cdrdao.c
	lib/driver/image/nrg.c
	lib/driver/image/nrg.h
	lib/driver/image_common.c
	lib/driver/image_common.h
	lib/driver/logging.c
	lib/driver/memory.c
	lib/driver/mmc/mmc.c
	lib/driver/mmc/mmc_cmd_helper.h
	lib/driver/mmc/mmc_hl_cmds.c
	lib/driver/mmc/mmc_ll_cmds.c
	lib/driver/mmc/mmc_private.h
	lib/driver/mmc/mmc_util.c
	lib/driver/MSWindows/aspi32.c
	lib/driver/MSWindows/aspi32.h
	lib/driver/MSWindows/win32.c
	lib/driver/MSWindows/win32.h
	lib/driver/MSWindows/win32_ioctl.c
	lib/driver/netbsd.c
	lib/driver/osx.c
	lib/driver/portable.h
	lib/driver/read.c
	lib/driver/realpath.c
	lib/driver/sector.c
	lib/driver/solaris.c
	lib/driver/track.c
	lib/driver/utf8.c
	lib/driver/util.c
	lib/iso9660/iso9660.c
	lib/iso9660/iso9660_fs.c
	lib/iso9660/iso9660_private.h
	lib/iso9660/rock.c
	lib/iso9660/xa.c
	lib/udf/filemode.c
	lib/udf/udf.c
	lib/udf/udf_file.c
	lib/udf/udf_fs.c
	lib/udf/udf_fs.h
	lib/udf/udf_private.h
	lib/udf/udf_time.c
)

target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		HAVE_CONFIG_H
		fopen_utf8=cdio_fopen_utf8
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		$<BUILD_INTERFACE:include;lib;lib/udf;lib/iso9660;lib/driver>
		$<BUILD_INTERFACE:lib/driver/mmc;lib/driver/image;lib/driver/MSWindows>
		$<BUILD_INTERFACE:MSVC;MSVC/missing;${CMAKE_CURRENT_BINARY_DIR}>
	INTERFACE
		$<INSTALL_INTERFACE:include/cdio>
)

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		Winmm.lib
)

install(TARGETS cdio EXPORT cdio
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)

install(DIRECTORY
	include/
	DESTINATION include
	PATTERN "Makefile.*" EXCLUDE
)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/cdio/version.h
	DESTINATION include/cdio
)

install(EXPORT ${PROJECT_NAME}
	FILE
		${PROJECT_NAME}-config.cmake
	NAMESPACE
		${PROJECT_NAME}::
	DESTINATION
		lib/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
	VERSION ${${PROJECT_NAME}_VERSION}
	COMPATIBILITY AnyNewerVersion
)

install(
	FILES
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
	DESTINATION
		lib/cmake/${PROJECT_NAME}
)
