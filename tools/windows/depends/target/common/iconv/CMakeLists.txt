cmake_minimum_required(VERSION 3.13)

project(iconv VERSION 1.16 LANGUAGES C)

set(HAVE_VISIBILITY 0)
set(HAVE_WCHAR_T 1)
set(ICONV_CONST const)
set(USE_MBSTATE_T 0)

configure_file(libcharset/include/localcharset.h.build.in libcharset/include/localcharset.h)
configure_file(include/iconv.h.build.in include/iconv.h)

add_library(${PROJECT_NAME}
	lib/iconv.c
	libcharset/lib/localcharset.c
)

target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		ICONV_CONST=const
		LIBDIR
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/libcharset/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libcharset/include>
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/iconv.h DESTINATION include)

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
