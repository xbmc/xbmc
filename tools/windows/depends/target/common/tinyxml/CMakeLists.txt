cmake_minimum_required(VERSION 3.13)

project(tinyXML VERSION 2.8.2 LANGUAGES CXX)

add_library(${PROJECT_NAME}
	tinyxml.h
	tinyxml.cpp
	tinyxmlerror.cpp
	tinyxmlparser.cpp
)

target_compile_definitions(${PROJECT_NAME}
	PUBLIC
		TIXML_USE_STL
)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)

install(FILES
	tinystr.h
	tinyxml.h
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
