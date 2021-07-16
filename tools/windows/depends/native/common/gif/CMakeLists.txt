cmake_minimum_required(VERSION 3.13)

project(gif VERSION 5.2.1 LANGUAGES C)

add_library(${PROJECT_NAME}
	dgif_lib.c
	egif_lib.c
	gif_err.c
	gif_hash.c
	gif_lib.h
	gifalloc.c
	openbsd-reallocarray.c
)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)

install(FILES gif_lib.h DESTINATION include)
