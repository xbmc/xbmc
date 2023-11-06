# FindPythonModule-Pycryptodome
# --------
# Finds/Builds Pycryptodome Python package
#
# This module will build the python module on the system
#
# --------
#
# This module will define the following variables:
#
# Python::Pycryptodome - The python Cryptodome module
#
# --------
#

if(NOT TARGET Python::Pycryptodome)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC pythonmodule-pycryptodome)

  SETUP_BUILD_VARS()

  if(${CORE_SYSTEM_NAME} MATCHES "windows")
    if(EXISTS "${DEPENDS_PATH}/bin/Python/Lib/site-packages/Cryptodome/__init__.py")
      file(READ "${DEPENDS_PATH}/bin/Python/Lib/site-packages/Cryptodome/__init__.py" pycrypto_ver_file)
    endif()
  else()
    if(EXISTS "${PYTHON_SITE_PKG}/Cryptodome/__init__.py")
      file(READ "${PYTHON_SITE_PKG}/Cryptodome/__init__.py" pycrypto_ver_file)
    endif()
  endif()

  if(pycrypto_ver_file)
     string(REGEX REPLACE "^.*version_info = .*([0-9]+), ([0-9]+), .*([0-9]+).*" "\\1\.\\2\.\\3" PYCRYPTO_VER ${pycrypto_ver_file})
  endif()

  if(PYCRYPTO_VER VERSION_LESS ${${MODULE}_VER})

    if(${CORE_SYSTEM_NAME} MATCHES "windows")
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/10-win-cmake.patch"
                  "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/11-win-arm64-buildfix.patch")
    else()
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/01-nosetuptool.patch"
                  "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/02-revert-ctype.pythonapi-use.patch")

      if(CORE_SYSTEM_NAME STREQUAL android)
        list(APPEND patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/03-android-dlopen.patch")
      endif()                             
    endif()

    generate_patchcommand("${patches}")

    if(${CORE_SYSTEM_NAME} MATCHES "windows")
      if(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
        set(ADDITIONAL_ARGS "-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}" "-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}")
      endif()

      # Force as RelWithDebInfo due to prebuilt dependencies. If all dependencies are ever
      # built, this can be changed to reflect build type.
      set(PYTHONMODULE-PYCRYPTODOME_BUILD_TYPE RelWithDebInfo)
      # Due to the above forced build type, we need to use NMake Makefiles generator
      set(PYTHONMODULE-PYCRYPTODOME_GENERATOR CMAKE_GENERATOR "NMake Makefiles")
      set(PYTHONMODULE-PYCRYPTODOME_GENERATOR_PLATFORM "")

      set(CMAKE_ARGS -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
                     -DDEPENDS_PATH=${DEPENDS_PATH}
                     -DCMAKE_INSTALL_PREFIX=${DEPENDS_PATH}
                     -DSEPARATE_NAMESPACE=ON
                     ${ADDITIONAL_ARGS})

      set(BUILD_BYPRODUCTS "${DEPENDS_PATH}/bin/Python/Lib/site-packages/Cryptodome/Cipher/${${MODULE}_BYPRODUCT}")
    else()

      set(LDFLAGS ${CMAKE_EXE_LINKER_FLAGS})
      set(LDSHARED "${CMAKE_C_COMPILER} -shared")

      find_package(PythonInterpreter REQUIRED)

      if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        if(CPU STREQUAL arm64)
          set(CFLAGS "${CMAKE_C_FLAGS} -target arm64-apple-darwin")
          set(LDFLAGS "${CMAKE_EXE_LINKER_FLAGS} -target arm64-apple-darwin")
        endif()

        set(LDSHARED "${CMAKE_C_COMPILER} -bundle -undefined dynamic_lookup ${LDFLAGS}")
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
                          "SETUPTOOLS_EXT_SUFFIX=.so"
                          )

      set(BYPASS_DEP_BUILDENV ON)

      # Set Target Configure command
      set(CONFIGURE_COMMAND COMMAND ${CMAKE_COMMAND} -E touch <SOURCE_DIR>/.separate_namespace)

      set(BUILD_COMMAND COMMAND ${CMAKE_COMMAND} -E touch <SOURCE_DIR>/.separate_namespace
                        COMMAND ${CMAKE_COMMAND} -E env ${PYMOD_TARGETENV} ${PROJECT_BUILDENV}
                                ${PYTHON_EXECUTABLE} setup.py build_ext --plat-name ${OS}-${CPU})

      set(INSTALL_COMMAND COMMAND ${CMAKE_COMMAND} -E env ${PYMOD_TARGETENV} ${PROJECT_BUILDENV}
                                  ${PYTHON_EXECUTABLE} setup.py install --prefix=${DEPENDS_PATH})
      set(BUILD_IN_SOURCE 1)

      if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
        set(PIL_OUTPUT_DIR ${DEPENDS_PATH}/share/${APP_NAME_LC}/addons/script.module.pil/lib)
      else()
        set(PIL_OUTPUT_DIR ${PYTHON_SITE_PKG})
      endif()

      set(BUILD_BYPRODUCTS "${PIL_OUTPUT_DIR}/Cryptodome/Cipher/${${MODULE}_BYPRODUCT}")

    endif()

    BUILD_DEP_TARGET()

    if(NOT ${CORE_SYSTEM_NAME} MATCHES "windows")


      add_custom_command(TARGET ${MODULE_LC} POST_BUILD
                         COMMAND mv -f ${PYTHON_SITE_PKG}/pycryptodomex-${${MODULE}_VER}-*.egg/Cryptodome ${PYTHON_SITE_PKG}/Cryptodome && rm -rf ${PYTHON_SITE_PKG}/pycryptodomex-${${MODULE}_VER}-*.egg || (exit 0))

    endif()

    add_library(Python::Pycryptodome UNKNOWN IMPORTED)
    if(TARGET ${MODULE_LC})
      add_dependencies(Python::Pycryptodome ${MODULE_LC})
      add_dependencies(Python::Pycryptodome Python::Python3)
      if(TARGET Python::Setuptools)
        add_dependencies(${MODULE_LC} Python::Setuptools)
      endif()
    endif()

    message(STATUS "Building python binary module PyCryptodome internally")

    if(NOT TARGET python-binarymodules)
      add_custom_target(python-binarymodules)
      set_target_properties(python-binarymodules PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()

    add_dependencies(python-binarymodules Python::Pycryptodome)
  endif()
endif()
