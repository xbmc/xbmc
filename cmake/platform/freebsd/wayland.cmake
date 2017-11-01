set(PLATFORM_REQUIRED_DEPS EGL Waylandpp LibDRM Xkbcommon)
set(PLATFORM_OPTIONAL_DEPS VAAPI)

set(WAYLAND_RENDER_SYSTEM "" CACHE STRING "Render system to use with Wayland: \"gl\" or \"gles\"")

if(WAYLAND_RENDER_SYSTEM STREQUAL "gl")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGl)
elseif(WAYLAND_RENDER_SYSTEM STREQUAL "gles")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGLES)
else()
  message(SEND_ERROR "You need to decide whether you want to use GL- or GLES-based rendering in combination with the Wayland windowing system. Please set WAYLAND_RENDER_SYSTEM to either \"gl\" or \"gles\". For normal desktop systems, you will usually want to use \"gl\".")
endif()

set(PLATFORM_GLOBAL_TARGET_DEPS generate-wayland-extra-protocols)
set(WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}")
# for wayland-extra-protocols.hpp
include_directories("${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}")