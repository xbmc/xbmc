cmake_minimum_required(VERSION 3.13)

project(crossguid VERSION 0.2.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

add_library(${PROJECT_NAME}
	Guid.hpp
	Guid.cpp
)

target_compile_definitions(${PROJECT_NAME}
	PUBLIC
		GUID_WINDOWS
)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)

install(FILES Guid.hpp DESTINATION include)
