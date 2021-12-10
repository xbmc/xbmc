#.rst:
# FindOpenGL-Registry
# --------
# Finds the OpenGL-Registry files
#
# This will define the following variable:
#
# OPENGL_REGISTRY_DIR - the OpenGL-Registry location

include(ExternalProject)

set(OPENGL_REGISTRY_VERSION a48c224a2db6edc4f4c610025b529d1c31ee9445)

# allow user to override the download URL with a local tarball
# needed for offline build envs
if(OPENGL_REGISTRY_URL)
  get_filename_component(OPENGL_REGISTRY_URL "${OPENGL_REGISTRY_URL}" ABSOLUTE)
else()
  # todo: use mirror url when uploaded
  set(OPENGL_REGISTRY_URL https://github.com/KhronosGroup/OpenGL-Registry/archive/${OPENGL_REGISTRY_VERSION}.tar.gz)
endif()

if(VERBOSE)
  message(STATUS "OPENGL_REGISTRY_URL: ${OPENGL_REGISTRY_URL}")
endif()

externalproject_add(
  opengl-registry
  URL ${OPENGL_REGISTRY_URL}
  DOWNLOAD_NAME opengl-registry-${OPENGL_REGISTRY_VERSION}.tar.gz
  DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
  PREFIX ${CORE_BUILD_DIR}/opengl-registry
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)

ExternalProject_Get_Property(opengl-registry SOURCE_DIR)
set(OPENGL_REGISTRY_DIR ${SOURCE_DIR}/xml)
