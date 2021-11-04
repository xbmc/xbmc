cmake_minimum_required(VERSION 3.13)

project(fstrcmp VERSION 0.7 LANGUAGES C)

include(CheckIncludeFile)
check_include_file(assert.h HAVE_ASSERT_H)
check_include_file(ctype.h HAVE_CTYPE_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(getopt.h HAVE_GETOPT_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(pwd.h HAVE_PWD_H)
check_include_file(stddef.h HAVE_STDDEF_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(wchar.h HAVE_WCHAR_H)

if(HAVE_STRING_H)
	include(CheckFunctionExists)
	set(CMAKE_REQUIRED_INCLUDES string.h)
	check_function_exists(snprintf HAVE_SNPRINTF)
	check_function_exists(strerror HAVE_STRERROR)
	check_function_exists(vsnprintf HAVE_VSNPRINTF)
endif()

configure_file(lib/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/lib/config.h)

add_library(${PROJECT_NAME}
	lib/ac/assert.h
	lib/ac/ctype.h
	lib/ac/errno.h
	lib/ac/fcntl.h
	lib/ac/getopt.h
	lib/ac/pthread.h
	lib/ac/pwd.h
	lib/ac/stdarg.h
	lib/ac/stddef.h
	lib/ac/stdio.h
	lib/ac/stdlib.h
	lib/ac/sys/stat.h
	lib/ac/sys/time.h
	lib/ac/sys/types.h
	lib/ac/unistd.h
	lib/ac/wchar.h
	lib/config.h.in
	lib/config.messy.h
	lib/diffseq.h
	lib/downcase.c
	lib/downcase.h
	lib/fmemcmp.c
	lib/fmemcmpi.c
	lib/fstrcasecmp.c
	lib/fstrcasecmpi.c
	lib/fstrcmp.c
	lib/fstrcmp.h
	lib/fstrcmpi.c
	lib/fstrcoll.c
	lib/fstrcolli.c
	lib/fwcscmp.c
	lib/fwcscmpi.c
	lib/gcc_attributes.h
	lib/mbs_to_wcs.c
	lib/mbs_to_wcs.h
	lib/minmax.h
	lib/nmalloc.c
	lib/nmalloc.h
	lib/patchlevel.h
	lib/version.c
	lib/version.h
	lib/version_print.c
	lib/version_print.h
	lib/warn_unused_result.h
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
	target_compile_definitions(${PROJECT_NAME} PRIVATE -D_CRT_SECURE_NO_WARNINGS -Dssize_t=intptr_t)
endif()

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)

install(FILES lib/fstrcmp.h DESTINATION include)

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
