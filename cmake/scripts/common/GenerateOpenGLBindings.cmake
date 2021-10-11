if(NOT OPENGL_REGISTRY_DIR)
  find_package(OpenGL-Registry)
  set(OPENGL_GENERATOR_DEPENDS opengl-registry)
else()
  set(OPENGL_GENERATOR_DEPENDS)
endif()

set(GENERATE_FILE_GL RenderingGL.hpp)
set(GENERATE_FILE_GLX RenderingGLX.hpp)

if(CLANGFORMAT_FOUND)
  set(CLANG_FORMAT_COMMAND_GL COMMAND ${CLANG_FORMAT_EXECUTABLE} ARGS -i ${GENERATE_FILE_GL})
  set(CLANG_FORMAT_COMMAND_GLX COMMAND ${CLANG_FORMAT_EXECUTABLE} ARGS -i ${GENERATE_FILE_GLX})
endif()

set(OPENGL_GENERATOR_DIR ${CMAKE_SOURCE_DIR}/tools/OpenGL-Header-Generator)

set(PLATFORM_SWITCH)
if("ios" IN_LIST CORE_PLATFORM_NAME_LC OR
   "tvos" IN_LIST CORE_PLATFORM_NAME_LC OR
   "osx" IN_LIST CORE_PLATFORM_NAME_LC)
  set(PLATFORM_SWITCH --gl-apple)
endif()

add_custom_command(OUTPUT ${GENERATE_FILE_GL}
                   COMMAND ${PYTHON_EXECUTABLE}
                   ARGS ${OPENGL_GENERATOR_DIR}/GeneratorGL.py --gl=${OPENGL_REGISTRY_DIR}/gl.xml ${PLATFORM_SWITCH} ${GENERATE_FILE_GL}
                   DEPENDS ${OPENGL_GENERATOR_DIR}/GeneratorGL.py
                           ${OPENGL_GENERATOR_DEPENDS}
                   ${CLANG_FORMAT_COMMAND_GL})

add_custom_command(OUTPUT ${GENERATE_FILE_GLX}
                   COMMAND ${PYTHON_EXECUTABLE}
                   ARGS ${OPENGL_GENERATOR_DIR}/GeneratorGL.py --glx=${OPENGL_REGISTRY_DIR}/glx.xml ${GENERATE_FILE_GLX}
                   DEPENDS ${OPENGL_GENERATOR_DIR}/GeneratorGL.py
                           ${OPENGL_GENERATOR_DEPENDS}
                   ${CLANG_FORMAT_COMMAND_GLX})


add_custom_target(opengl-bindings DEPENDS ${GENERATE_FILE_GL} ${GENERATE_FILE_GLX})
set(PLATFORM_GLOBAL_TARGET_DEPS ${PLATFORM_GLOBAL_TARGET_DEPS} opengl-bindings PARENT_SCOPE)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
