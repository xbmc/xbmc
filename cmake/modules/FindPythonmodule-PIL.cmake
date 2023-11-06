# FindPythonModule-Pillow
# --------
# Finds/Builds Pillow Python package
#
# This module will build the python module on the system
#
# --------
#
# This module will define the following variables:
#
# Python::PIL - The python Pillow module
#
# --------
#

if(NOT TARGET Python::PIL)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC pythonmodule-pil)

  SETUP_BUILD_VARS()

  if(${CORE_SYSTEM_NAME} MATCHES "windows")
    if(EXISTS "${DEPENDS_PATH}/bin/Python/Lib/site-packages/PIL/_version.py")
      file(READ "${DEPENDS_PATH}/bin/Python/Lib/site-packages/PIL/_version.py" pil_ver_file)
    endif()
  else()
    if(EXISTS "${PYTHON_SITE_PKG}/PIL/_version.py")
      file(READ "${PYTHON_SITE_PKG}/PIL/_version.py" pil_ver_file)
    elseif(EXISTS "${DEPENDS_PATH}/share/${APP_NAME_LC}/addons/script.module.pil/lib/PIL/_version.py")
      file(READ "${DEPENDS_PATH}/share/${APP_NAME_LC}/addons/script.module.pil/lib/PIL/_version.py" pil_ver_file)
    endif()
  endif()

  if(pil_ver_file)
     string(REGEX REPLACE "^.*__version__ = \"([0-9]+\.[0-9]+\.[0-9]+)\".*" "\\1" PIL_VER ${pil_ver_file})
  endif()

  if(PIL_VER VERSION_LESS ${${MODULE}_VER})

    if(${CORE_SYSTEM_NAME} MATCHES "windows")
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/10-win-cmake.patch"
                  "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/11-win-uwp.patch")
    else()
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/pillow-crosscompile.patch")
    endif()

    generate_patchcommand("${patches}")

    if(${CORE_SYSTEM_NAME} MATCHES "windows")
      if(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
        set(ADDITIONAL_ARGS "-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}" "-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}")
      endif()

      # Force as RelWithDebInfo due to prebuilt dependencies. If all dependencies are ever
      # built, this can be changed to reflect build type.
      set(PYTHONMODULE-PIL_BUILD_TYPE RelWithDebInfo)
      # Due to the above forced build type, we need to use NMake Makefiles generator
      set(PYTHONMODULE-PIL_GENERATOR CMAKE_GENERATOR "NMake Makefiles")
      set(PYTHONMODULE-PIL_GENERATOR_PLATFORM "")

      set(CMAKE_ARGS -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
                     -DDEPENDS_PATH=${DEPENDS_PATH}
                     -DCMAKE_INSTALL_PREFIX=${DEPENDS_PATH}
                     ${ADDITIONAL_ARGS})

      set(BUILD_BYPRODUCTS "${DEPENDS_PATH}/bin/Python/Lib/site-packages/PIL/${${MODULE}_BYPRODUCT}")
    else()
      # ToDo: Dont hardcode *_ROOT in PYMOD_TARGETENV. We should find the libraries
      #       and then populate the ROOT paths based off that information.
      set(LDFLAGS ${CMAKE_EXE_LINKER_FLAGS})
      set(LDSHARED "${CMAKE_C_COMPILER} -shared")
      set(ZLIB_ROOT "${DEPENDS_PATH}")

      find_package(PythonInterpreter REQUIRED)

      if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        if(CPU STREQUAL arm64)
          set(CFLAGS "${CMAKE_C_FLAGS} -target arm64-apple-darwin")
          set(LDFLAGS "${CMAKE_EXE_LINKER_FLAGS} -target arm64-apple-darwin")
        endif()

        set(LDSHARED "${CMAKE_C_COMPILER} -bundle -undefined dynamic_lookup ${LDFLAGS}")
        set(ZLIB_ROOT "${CMAKE_OSX_SYSROOT}/usr")
      elseif(CORE_SYSTEM_NAME STREQUAL android)
        find_package(Pythonmodule-dummylib REQUIRED)
        # ToDo: ideally we want to get the -L link path from the Python::dummylib TARGET
        #       For now, we just expect the dummy lib to be built as part of tools/depends
        set(LDFLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${DEPENDS_PATH}/lib/dummy-lib${APP_NAME_LC} -l${APP_NAME_LC} -lm")
      endif()

      # Prepare buildenv - we need custom CFLAGS/LDFLAGS not in Toolchain.cmake
      set(PYMOD_TARGETENV "AS=${CMAKE_AS}"
                          "AR=${CMAKE_AR}"
                          "CC=${CMAKE_C_COMPILER}"
                          "CXX=${CMAKE_CXX_COMPILER}"
                          "NM=${CMAKE_NM}"
                          "LD=${CMAKE_LINKER}"
                          "STRIP=${CMAKE_STRIP}"
                          "RANLIB=${CMAKE_RANLIB}"
                          "OBJDUMP=${CMAKE_OBJDUMP}"
                          "CPPFLAGS=${CMAKE_CPP_FLAGS}"
                          "PKG_CONFIG_LIBDIR=${PKG_CONFIG_LIBDIR}"
                          "PKG_CONFIG_PATH="
                          "PKG_CONFIG_SYSROOT_DIR=${SDKROOT}"
                          "AUTOM4TE=${AUTOM4TE}"
                          "AUTOMAKE=${AUTOMAKE}"
                          "AUTOCONF=${AUTOCONF}"
                          "ACLOCAL=${ACLOCAL}"
                          "ACLOCAL_PATH=${ACLOCAL_PATH}"
                          "AUTOPOINT=${AUTOPOINT}"
                          "AUTOHEADER=${AUTOHEADER}"
                          "LIBTOOL=${LIBTOOL}"
                          "LIBTOOLIZE=${LIBTOOLIZE}"
                          # These are additional/changed compared to PROJECT_TARGETENV
                          "CFLAGS=${CFLAGS}"
                          "LDFLAGS=${LDFLAGS}"
                          "LDSHARED=${LDSHARED}"
                          "JPEG_ROOT=${DEPENDS_PATH}"
                          "FREETYPE_ROOT=${DEPENDS_PATH}"
                          "HARFBUZZ_ROOT=${DEPENDS_PATH}"
                          "FRIBIDI_ROOT=${DEPENDS_PATH}"
                          "ZLIB_ROOT=${ZLIB_ROOT}"
                          "PYTHONXINCLUDE=${DEPENDS_PATH}/include/python${PYTHON_VERSION}"
                          "SETUPTOOLS_EXT_SUFFIX=.so"
                          )

      set(BYPASS_DEP_BUILDENV ON)

      # Set Target Configure command
      # Must be "" if no step required otherwise will try and use cmake command
      set(CONFIGURE_COMMAND COMMAND ${CMAKE_COMMAND} -E touch <SOURCE_DIR>/.separate_namespace)

      set(BUILD_COMMAND COMMAND ${CMAKE_COMMAND} -E touch <SOURCE_DIR>/.separate_namespace
                        COMMAND ${CMAKE_COMMAND} -E env ${PYMOD_TARGETENV} ${PROJECT_BUILDENV}
                                ${PYTHON_EXECUTABLE} setup.py build_ext --plat-name ${OS}-${CPU} 
                                                                        --disable-jpeg2000
                                                                        --disable-webp
                                                                        --disable-imagequant
                                                                        --disable-tiff
                                                                        --disable-webp
                                                                        --disable-webpmux
                                                                        --disable-xcb
                                                                        --disable-lcms
                                                                        --disable-platform-guessing
                                                              install --install-lib ${PYTHON_SITE_PKG})
 
      # ToDo: empty install_command possible?
      set(INSTALL_COMMAND COMMAND ${CMAKE_COMMAND} -E env ${PYMOD_TARGETENV} ${PROJECT_BUILDENV} echo Empty Install_command)
      set(BUILD_IN_SOURCE 1)

      if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
        set(PIL_OUTPUT_DIR ${DEPENDS_PATH}/share/${APP_NAME_LC}/addons/script.module.pil/lib)
      else()
        set(PIL_OUTPUT_DIR ${PYTHON_SITE_PKG})
      endif()

      set(BUILD_BYPRODUCTS "${PIL_OUTPUT_DIR}/PIL/${${MODULE}_BYPRODUCT}")
    endif()

    BUILD_DEP_TARGET()

    if(NOT ${CORE_SYSTEM_NAME} MATCHES "windows")
      add_custom_command(TARGET ${MODULE_LC} POST_BUILD
                         COMMAND unzip -o ${PYTHON_SITE_PKG}/Pillow-${${MODULE}_VER}-*.egg -d ${PIL_OUTPUT_DIR} && rm -rf ${PYTHON_SITE_PKG}/Pillow-${${MODULE}_VER}-*.egg || (exit 0))

      if(CORE_SYSTEM_NAME STREQUAL android)

        if(CMAKE_HOST_APPLE)
          set(SED_FLAG "-i \'\'")
        else()
          set(SED_FLAG "-i")
        endif()

        add_custom_command(TARGET ${MODULE_LC} POST_BUILD
                           COMMAND sed ${SED_FLAG} -e \'s\/import sys\/import os, sys \/\' 
                                                   -e \'\/__file__\/ s\/_imaging\/lib_imaging\/g\'
                                                   -e \'s\/pkg_resources.resource_filename\(__name__\,\/os.path.join\(os.environ\[\"KODI_ANDROID_LIBS\"\], \/\'
                                                      ${PYTHON_SITE_PKG}/PIL/_imaging*.py)
      endif()
    endif()

    add_library(Python::PIL UNKNOWN IMPORTED)
    if(TARGET ${MODULE_LC})
      add_dependencies(Python::PIL ${MODULE_LC})
      add_dependencies(Python::PIL Python::Python3)
      if(TARGET Python::Setuptools)
        add_dependencies(${MODULE_LC} Python::Setuptools)
      endif()
    endif()

    message(STATUS "Building python binary module Pillow internally")

    if(NOT TARGET python-binarymodules)
      add_custom_target(python-binarymodules)
      set_target_properties(python-binarymodules PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()

    add_dependencies(python-binarymodules Python::PIL)
  endif()
endif()
