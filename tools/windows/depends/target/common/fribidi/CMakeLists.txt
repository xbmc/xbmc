cmake_minimum_required(VERSION 3.13)

set(FRIBIDI_MAJOR_VERSION 1)
set(FRIBIDI_MINOR_VERSION 0)
set(FRIBIDI_MICRO_VERSION 8)
set(FRIBIDI_INTERFACE_VERSION 4)
set(FRIBIDY_INTERFACE_AGE 0)
set(FRIBIDI_BINARY_AGE 4)
set(FRIBIDI_VERSION ${FRIBIDI_MAJOR_VERSION}.${FRIBIDI_MINOR_VERSION}.${FRIBIDI_MICRO_VERSION})

project(fribidi LANGUAGES C VERSION ${FRIBIDI_VERSION})

include(CheckTypeSize)
check_type_size(int SIZEOF_INT)

if(MSVC)
	set(FRIBIDI_MSVC_BUILD_PLACEHOLDER "#define FRIBIDI_BUILT_WITH_MSVC")
else()
	set(FRIBIDI_MSVC_BUILD_PLACEHOLDER "#undef FRIBIDI_BUILT_WITH_MSVC")
endif()

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/lib/fribidi-config.h.in
	${CMAKE_CURRENT_SOURCE_DIR}/lib/fribidi-config.h
	@ONLY
)

add_library(${PROJECT_NAME}
	lib/fribidi-arabic.c
	lib/fribidi-arabic.h
	lib/fribidi-begindecls.h
	lib/fribidi-bidi-types-list.h
	lib/fribidi-bidi-types.c
	lib/fribidi-bidi-types.h
	lib/fribidi-bidi.c
	lib/fribidi-bidi.h
	lib/fribidi-brackets.c
	lib/fribidi-brackets.h
	lib/fribidi-char-sets-cap-rtl.c
	lib/fribidi-char-sets-cp1255.c
	lib/fribidi-char-sets-cp1256.c
	lib/fribidi-char-sets-iso8859-6.c
	lib/fribidi-char-sets-iso8859-8.c
	lib/fribidi-char-sets-list.h
	lib/fribidi-char-sets-utf8.c
	lib/fribidi-char-sets.c
	lib/fribidi-char-sets.h
	lib/fribidi-common.h
	lib/fribidi-config.h
	lib/fribidi-config.h.in
	lib/fribidi-deprecated.c
	lib/fribidi-deprecated.h
	lib/fribidi-enddecls.h
	lib/fribidi-flags.h
	lib/fribidi-joining-types-list.h
	lib/fribidi-joining-types.c
	lib/fribidi-joining-types.h
	lib/fribidi-joining.c
	lib/fribidi-joining.h
	lib/fribidi-mirroring.c
	lib/fribidi-mirroring.h
	lib/fribidi-run.c
	lib/fribidi-shape.c
	lib/fribidi-shape.h
	lib/fribidi-types.h
	lib/fribidi-unicode-version.h
	lib/fribidi-unicode.h
	lib/fribidi.c
	lib/fribidi.h
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
	INTERFACE
		$<INSTALL_INTERFACE:include/fribidi>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
)

target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		HAVE_MEMORY_H
		HAVE_STDLIB_H
		HAVE_STRING_H
		HAVE_STRINGIZE
)

if(NOT BUILD_SHARED_LIBS)
	target_compile_definitions(${PROJECT_NAME} PUBLIC FRIBIDI_STATIC)
endif()

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)

install(FILES
	lib/fribidi-arabic.h
	lib/fribidi-begindecls.h
	lib/fribidi-bidi-types-list.h
	lib/fribidi-bidi-types.h
	lib/fribidi-bidi.h
	lib/fribidi-brackets.h
	lib/fribidi-char-sets-list.h
	lib/fribidi-char-sets.h
	lib/fribidi-common.h
	lib/fribidi-config.h
	lib/fribidi-deprecated.h
	lib/fribidi-enddecls.h
	lib/fribidi-flags.h
	lib/fribidi-joining-types-list.h
	lib/fribidi-joining-types.h
	lib/fribidi-joining.h
	lib/fribidi-mirroring.h
	lib/fribidi-shape.h
	lib/fribidi-types.h
	lib/fribidi-unicode-version.h
	lib/fribidi-unicode.h
	lib/fribidi.h
	DESTINATION include/fribidi
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
