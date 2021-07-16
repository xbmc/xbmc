cmake_minimum_required(VERSION 3.13)

project(ass LANGUAGES C VERSION 0.14.0)

find_package(iconv REQUIRED NO_MODULE)
find_package(freetype 2.6 REQUIRED NO_MODULE)
find_package(fribidi REQUIRED NO_MODULE)

configure_file(config.h.in include/config.h)

add_library(${PROJECT_NAME}
	libass/ass.c
	libass/ass.h
	libass/ass_bitmap.c
	libass/ass_bitmap.h
	libass/ass_blur.c
	libass/ass_cache.c
	libass/ass_cache.h
	libass/ass_cache_template.h
	libass/ass_compat.h
	libass/ass_drawing.c
	libass/ass_drawing.h
	libass/ass_font.c
	libass/ass_font.h
	libass/ass_fontselect.c
	libass/ass_fontselect.h
	libass/ass_func_template.h
	libass/ass_library.c
	libass/ass_library.h
	libass/ass_outline.c
	libass/ass_outline.h
	libass/ass_parse.c
	libass/ass_parse.h
	libass/ass_rasterizer.c
	libass/ass_rasterizer.h
	libass/ass_rasterizer_c.c
	libass/ass_render.c
	libass/ass_render.h
	libass/ass_render_api.c
	libass/ass_shaper.c
	libass/ass_shaper.h
	libass/ass_string.c
	libass/ass_string.h
	libass/ass_strtod.c
	libass/ass_types.h
	libass/ass_utils.c
	libass/ass_utils.h
	)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		freetype::freetype
		fribidi::fribidi
		iconv::iconv
)

target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		CONFIG_RASTERIZER
		BUILDING_LIBASS
		CONFIG_ICONV
)

if(NOT BUILD_SHARED_LIBS)
	target_compile_definitions(${PROJECT_NAME} PRIVATE ASS_STATIC)
endif()

target_include_directories(${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/include
		freetype::freetype
	INTERFACE
		$<INSTALL_INTERFACE:include/ass>
)

if(MSVC AND OFF)
	target_sources(${PROJECT_NAME}
		PRIVATE
			libass/ass_directwrite.h
			libass/ass_directwrite.cpp
	)
	target_compile_definitions(${PROJECT_NAME} PRIVATE CONFIG_DIRECTWRITE)
	target_link_libraries(${PROJECT_NAME} PRIVATE dwrite.lib)
endif()

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)

install(FILES
	libass/ass_render.h
	libass/ass_shaper.h
	libass/ass_types.h
	libass/ass_utils.h
	libass/ass.h
	libass/ass_bitmap.h
	libass/ass_cache.h
	libass/ass_cache_template.h
	libass/ass_drawing.h
	libass/ass_font.h
	libass/ass_fontconfig.h
	libass/ass_library.h
	libass/ass_parse.h
	libass/ass_rasterizer.h
	DESTINATION include/ass
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
