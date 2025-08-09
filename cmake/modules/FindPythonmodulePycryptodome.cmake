# FindPythonmodulePycryptodome
# --------
# Finds/Builds Pycryptodome Python package
#
# This module will build the python module on the system
#
# --------
#
# This module will define the following variables:
#
# Python::PythonmodulePycryptodome - The python Cryptodome module
#
# --------
#

if(NOT TARGET Python::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroPythonmodulePycryptodome)
    find_package(PythonInterpreter REQUIRED ${SEARCH_QUIET})
    # We explicitly do not search for Python as a required dependency due to recursion
    find_package(PythonmoduleSetuptools REQUIRED ${SEARCH_QUIET})

    if(${CORE_SYSTEM_NAME} MATCHES "windows")
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/10-win-cmake.patch")
    else()
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-revert-ctype.pythonapi-use.patch")

      if(CORE_SYSTEM_NAME STREQUAL android)
        list(APPEND patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/02-android-dlopen.patch")
      endif()
    endif()

    generate_patchcommand("${patches}")
    unset(patches)

    if(${CORE_SYSTEM_NAME} MATCHES "windows")
      if(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
        set(ADDITIONAL_ARGS "-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}" "-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}")
      endif()

      # Force as RelWithDebInfo due to prebuilt dependencies. If all dependencies are ever
      # built, this can be changed to reflect build type.
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_TYPE RelWithDebInfo)
      # Due to the above forced build type, we need to use NMake Makefiles generator
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR CMAKE_GENERATOR "NMake Makefiles")
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR_PLATFORM "")

      set(CMAKE_ARGS -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
                     -DDEPENDS_PATH=${DEPENDS_PATH}
                     -DCMAKE_INSTALL_PREFIX=${DEPENDS_PATH}
                     -DSEPARATE_NAMESPACE=ON
                     ${ADDITIONAL_ARGS})

      set(BUILD_BYPRODUCTS "${PYTHON_SITE_PKG}/Cryptodome/Cipher/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT}")
    else()
      set(CFLAGS "${CMAKE_C_FLAGS}")
      set(LDFLAGS ${CMAKE_EXE_LINKER_FLAGS})
      set(LDSHARED "${CMAKE_C_COMPILER} -shared")

      if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        if(CPU STREQUAL arm64)
          set(CFLAGS "${CMAKE_C_FLAGS} -target arm64-apple-darwin")
          set(LDFLAGS "${CMAKE_EXE_LINKER_FLAGS} -target arm64-apple-darwin")
        endif()

        set(LDSHARED "${CMAKE_C_COMPILER} -bundle -undefined dynamic_lookup ${LDFLAGS}")
      elseif(CORE_SYSTEM_NAME STREQUAL android)
        find_package(Pythonmoduledummylib REQUIRED ${SEARCH_QUIET})
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

      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPASS_DEP_BUILDENV ON)

      # Set Target Configure command. Must be "" if no step required
      set(CONFIGURE_COMMAND COMMAND "")

      set(BUILD_COMMAND COMMAND ${CMAKE_COMMAND} -E touch <SOURCE_DIR>/.separate_namespace
                        COMMAND ${CMAKE_COMMAND} -E env ${PYMOD_TARGETENV} ${PROJECT_BUILDENV} PYTHONPATH=${PYTHON_SITE_PKG}
                                ${PYTHON_EXECUTABLE} setup.py build_ext --plat-name ${OS}-${CPU})

      set(INSTALL_COMMAND COMMAND ${CMAKE_COMMAND} -E env ${PYMOD_TARGETENV} ${PROJECT_BUILDENV} PYTHONPATH=${PYTHON_SITE_PKG}
                                  ${PYTHON_EXECUTABLE} setup.py install --prefix=${DEPENDS_PATH})
      set(BUILD_IN_SOURCE 1)

      set(BUILD_BYPRODUCTS "${PYTHON_SITE_PKG}/Cryptodome/Cipher/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT}")
    endif()

    BUILD_DEP_TARGET()

    if(UNIX)
      add_custom_command(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} POST_BUILD
                         COMMAND mv -f ${PYTHON_SITE_PKG}/pycryptodomex-${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}-*.egg/Cryptodome ${PYTHON_SITE_PKG} && rm -rf ${PYTHON_SITE_PKG}/pycryptodomex-${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}-*.egg || (exit 0))

    endif()

    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} ${APP_NAME_LC}::Python
                                                                        Python::PythonmoduleSetuptools)

  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC pythonmodule-pycryptodome)

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

  add_library(Python::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)

  if(PYCRYPTO_VER VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")

    message(STATUS "Building Python module: PyCryptodome")

    if(NOT TARGET python-modules)
      add_custom_target(python-modules)
      set_target_properties(python-modules PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()

    add_dependencies(Python::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    add_dependencies(python-modules Python::${CMAKE_FIND_PACKAGE_NAME})
  endif()

  ADD_MULTICONFIG_BUILDMACRO()
endif()
