cmake_minimum_required(VERSION 3.13)

project(TexturePacker)

add_subdirectory(${PROJECT_SOURCE_DIR}/../../../../../depends/native/TexturePacker TexturePacker)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)
