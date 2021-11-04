cmake_minimum_required(VERSION 3.13)

project(sqlite3 VERSION 3.30.1 LANGUAGES C)

add_library(${PROJECT_NAME}
	sqlite3.h
	sqlite3ext.h
	sqlite3.c
)

if(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
	target_compile_definitions(sqlite3
		PRIVATE
		SQLITE_OS_WINRT
	)
endif()

target_include_directories(
	sqlite3 PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	INTERFACE
	$<INSTALL_INTERFACE:include>
)

install(TARGETS sqlite3 EXPORT sqlite3
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)

install(FILES
	${CMAKE_CURRENT_SOURCE_DIR}/sqlite3.h
	${CMAKE_CURRENT_SOURCE_DIR}/sqlite3ext.h
	DESTINATION include
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
