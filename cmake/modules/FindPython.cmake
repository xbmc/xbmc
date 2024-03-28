# FindPython
# --------
# Finds Python3 libraries
#
# This module will search for the required python libraries on the system
# If multiple versions are found, the highest version will be used.
#
# --------
#
# the following variables influence behaviour:
#
# PYTHON_PATH - use external python not found in system paths
#               usage: -DPYTHON_PATH=/path/to/python/lib
# PYTHON_VER - use exact python version, fail if not found
#               usage: -DPYTHON_VER=3.8
#
# --------
#
# This module will define the following variables:
#
# Python::Python3 - The python3 library
#
# --------
#

if(NOT TARGET Python::Python3)

  macro(buildPython3)

    if(WIN32 OR WINDOWS_STORE)
      # Windows is built as a dll, only require linking to zlib
      find_package(zlib NO_MODULE REQUIRED)

      # These packages are all required by cpython modules, therefore do checks now.
      find_package(expat NO_MODULE REQUIRED)
      find_package(openssl NO_MODULE REQUIRED)
      find_package(bzip2 NO_MODULE REQUIRED)
      find_package(libffi NO_MODULE REQUIRED)
      find_package(xz NO_MODULE REQUIRED)
      find_package(sqlite3 NO_MODULE REQUIRED)

      list(APPEND PYTHON_DEP_LIBRARIES zlib::zlibstatic)
    else()
      find_library(FFI_LIBRARY ffi REQUIRED)
      find_library(EXPAT_LIBRARY expat REQUIRED)
      find_library(LZMA_LIBRARY lzma REQUIRED)
      find_library(INTL_LIBRARY intl REQUIRED)
      find_library(GMP_LIBRARY gmp REQUIRED)

      find_package(OpenSSL REQUIRED)
      find_package(Sqlite3 REQUIRED)
      find_package(BZip2 REQUIRED)
      find_package(LibXml2 REQUIRED)

      # ToDo existing build reqs not handled
      #python3:  gettext $(ICONV)

      # ToDo: change openssl/sqlite/bzip/libxml to TARGET
      list(APPEND PYTHON_DEP_LIBRARIES ${FFI_LIBRARY} ${EXPAT_LIBRARY} ${LZMA_LIBRARY} ${INTL_LIBRARY} ${GMP_LIBRARY} ${OPENSSL_LIBRARIES} ${SQLITE3_LIBRARY} ${BZIP2_LIBRARIES} ${LIBXML2_LIBRARY})
    endif()

    string(REGEX MATCH "^([0-9]?)\.([0-9]+)\." Python3_VERSION ${${MODULE}_VER})
    set(Python3_VERSION_MAJOR ${CMAKE_MATCH_1} CACHE INTERNAL "" FORCE)
    set(Python3_VERSION_MINOR ${CMAKE_MATCH_2} CACHE INTERNAL "" FORCE)

    if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
      set(HOSTPLATFORM "_PYTHON_HOST_PLATFORM=\"darwin\"")
    elseif(CMAKE_SYSTEM_NAME STREQUAL Linux)
      set(HOSTPLATFORM "_PYTHON_HOST_PLATFORM=\"linux\"")
    endif()

    if(CORE_SYSTEM_NAME STREQUAL linux)
      set(EXTRA_CONFIGURE ac_cv_pthread=yes)
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
      set(EXTRA_CONFIGURE ac_cv_lib_intl_textdomain=yes)

      set(CONFIG_OPTS "--with-system-ffi")

      if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
        list(APPEND EXTRA_CONFIGURE ac_cv_func_execv=no
                                    ac_cv_func_fexecv=no
                                    ac_cv_func_forkpty=no
                                    ac_cv_func_getentropy=no
                                    ac_cv_func_getgroups=no
                                    ac_cv_func_posix_spawn=no
                                    ac_cv_func_posix_spawnp=no
                                    ac_cv_func_sendfile=no
                                    ac_cv_func_setpriority=no
                                    ac_cv_func_system=no
                                    ac_cv_func_wait3=no
                                    ac_cv_func_wait4=no
                                    ac_cv_func_waitpid=no
                                    ac_cv_header_sched_h=no
                                    ac_cv_header_sched_h=no
                                    ac_cv_lib_util_forkpty=no)
      endif()
    endif()

    if(WIN32 OR WINDOWS_STORE)
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/01-win-cmake.patch"
                  "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/02-win-modules.patch"
                  "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/03-win-PC.patch"
                  "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/04-win-Python.patch"
                  "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/05-win-winregistryfinder.patch")
    else()
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/crosscompile.patch")

      if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        list(APPEND patches "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/apple.patch")

        if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
          list(APPEND patches "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/darwin_embedded.patch")
        endif()
      endif()
    endif()

    if(EXISTS "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/10-${CORE_SYSTEM_NAME}-modules.patch")
      list(APPEND patches "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/10-${CORE_SYSTEM_NAME}-modules.patch")
    endif()

    generate_patchcommand("${patches}")

    # We prepend this after generate_patchcommand as we dont want to run this through that function
    list(PREPEND PATCH_COMMAND ${CMAKE_COMMAND} -E copy
                               ${CMAKE_SOURCE_DIR}/tools/depends/target/python3/modules.setup
                               <SOURCE_DIR>/Modules/Setup
                               COMMAND)

    if(CORE_SYSTEM_NAME MATCHES windows)
      set(CMAKE_ARGS -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
                     -DDEPENDS_PATH=${DEPENDS_PATH}
                     -DNATIVEPREFIX=${NATIVEPREFIX}
                     -DENABLE_MODULES=ON
                     -DCMAKE_INSTALL_PREFIX=${DEPENDS_PATH}
                     -DCMAKE_BUILD_TYPE=RelWithDebInfo
                     -DARCH=${ARCH}
                     ${ADDITIONAL_ARGS})
    else()

      if(NOT CORE_SYSTEM_NAME STREQUAL android)
        list(APPEND PYTHON_DEP_LIBRARIES pthread dl util)
        if(CORE_SYSTEM_NAME STREQUAL linux)
          # python archive built via depends requires librt for _posixshmem library
          list(APPEND PYTHON_DEP_LIBRARIES rt)
        else(CORE_SYSTEM_NAME STREQUAL osx)
          list(APPEND PYTHON_DEP_LIBRARIES "-framework SystemConfiguration" "-framework CoreFoundation")
        endif()
      endif()

      find_program(AUTORECONF autoreconf REQUIRED)
      find_program(MAKE_EXECUTABLE make REQUIRED)
      find_package(PythonInterpreter REQUIRED)

      # Python uses ax_c_float_words_bigendian.m4 to find autoconf-archive
      # Make sure we can find it as a requirement as well
      find_file(AUTOCONF-ARCHIVE "ax_c_float_words_bigendian.m4" PATHS "${NATIVEPREFIX}/share/aclocal" NO_CMAKE_FIND_ROOT_PATH REQUIRED)
      string(REGEX REPLACE "/ax_c_float_words_bigendian.m4" "" AUTOCONF-ARCHIVE ${AUTOCONF-ARCHIVE})
      set(ACLOCAL_PATH_VAR "ACLOCAL_PATH=${AUTOCONF-ARCHIVE}")

      set(PYTHON_TARGETENV ${PROJECT_TARGETENV})
      if("webos" IN_LIST CORE_PLATFORM_NAME_LC)
        # Prepare buildenv - we need custom CFLAGS/LDFLAGS not in Toolchain.cmake
        set(PYTHON_TARGETENV "AS=${CMAKE_AS}"
                             "AR=${CMAKE_AR}"
                             "CC=${CMAKE_C_COMPILER}"
                             "CXX=${CMAKE_CXX_COMPILER}"
                             "NM=${CMAKE_NM}"
                             "LD=${CMAKE_LINKER}"
                             "STRIP=${CMAKE_STRIP}"
                             "RANLIB=${CMAKE_RANLIB}"
                             "OBJDUMP=${CMAKE_OBJDUMP}"
                             "CFLAGS=${CFLAGS}"
                             "CPPFLAGS=${CMAKE_CPP_FLAGS}"
                             # These are additional/changed compared to PROJECT_TARGETENV
                             "LDFLAGS=${LDFLAGS} -liconv"
                             "PKG_CONFIG_LIBDIR=${PKG_CONFIG_LIBDIR}"
                             "PKG_CONFIG_PATH="
                             "PKG_CONFIG_SYSROOT_DIR=${SDKROOT}"
                             "AUTOM4TE=${AUTOM4TE}"
                             "AUTOMAKE=${AUTOMAKE}"
                             "AUTOCONF=${AUTOCONF}"
                             "ACLOCAL=${ACLOCAL}"
                             "AUTOPOINT=${AUTOPOINT}"
                             "AUTOHEADER=${AUTOHEADER}"
                             "LIBTOOL=${LIBTOOL}"
                             "LIBTOOLIZE=${LIBTOOLIZE}"
                             )

        set(BYPASS_DEP_BUILDENV ON)
      endif()

      set(CONFIGURE_COMMAND ${ACLOCAL_PATH_VAR} ${AUTORECONF} -vif
                    COMMAND ${CMAKE_COMMAND} -E env ${PYTHON_TARGETENV}
                            ./configure
                              --prefix=${DEPENDS_PATH}
                              --disable-shared
                              --without-ensurepip
                              --disable-framework
                              --without-pymalloc
                              --enable-ipv6
                              --with-build-python=${PYTHON_EXECUTABLE}
                              --with-system-expat=yes
                              --disable-test-modules
                              ${CONFIG_OPTS}
                              MODULE_BUILDTYPE=static
                              ${EXTRA_CONFIGURE})

      set(BUILD_COMMAND ${CMAKE_COMMAND} -E env ${PYTHON_TARGETENV}
                        ${MAKE_EXECUTABLE} ${HOSTPLATFORM} CROSS_COMPILE_TARGET=yes libpython${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}.a)

      set(INSTALL_COMMAND ${CMAKE_COMMAND} -E env ${PYTHON_TARGETENV}
                          ${MAKE_EXECUTABLE} ${HOSTPLATFORM} CROSS_COMPILE_TARGET=yes install)
      set(BUILD_IN_SOURCE 1)
    endif()

    BUILD_DEP_TARGET()

    if(WIN32)
      set(PYTHON_SITE_PKG "${DEPENDS_PATH}/bin/python/Lib/site-packages" CACHE INTERNAL "" FORCE)
    else()
      set(PYTHON_SITE_PKG "${DEPENDS_PATH}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages" CACHE INTERNAL "" FORCE)
    endif()

    include(SelectLibraryConfigurations)
    select_library_configurations(PYTHON3)
    unset(PYTHON3_LIBRARIES)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Python
                                      REQUIRED_VARS PYTHON3_LIBRARY PYTHON3_INCLUDE_DIR
                                      VERSION_VAR PYTHON3_VERSION)

    set(Python3_LIBRARIES ${PYTHON3_LIBRARY})
    set(Python3_INCLUDE_DIRS "${PYTHON3_INCLUDE_DIR}/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
    # We must create the directory, otherwise we get non-existant path errors
    file(MAKE_DIRECTORY ${Python3_INCLUDE_DIRS})

    set(Python3_FOUND ON)

  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC python3)

  SETUP_BUILD_VARS()

  # for Depends/Windows builds, set search root dir to libdir path
  if(KODI_DEPENDSBUILD
     OR CMAKE_SYSTEM_NAME STREQUAL WINDOWS
     OR CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
    set(Python3_USE_STATIC_LIBS TRUE)
    set(Python3_ROOT_DIR ${libdir})
  endif()

  # Provide root dir to search for Python if provided
  if(PYTHON_PATH)
    set(Python3_ROOT_DIR ${PYTHON_PATH})

    # unset cache var so we can generate again with a different dir (or none) if desired
    unset(PYTHON_PATH CACHE)
  endif()

  # Set specific version of Python to find if provided
  if(PYTHON_VER)
    set(VERSION ${PYTHON_VER})
    set(EXACT_VER "EXACT")

    # unset cache var so we can generate again with a different ver (or none) if desired
    unset(PYTHON_VER CACHE)
  endif()

  find_package(Python3 ${VERSION} ${EXACT_VER} COMPONENTS Development)

  if(Python3_FOUND)
    set(PY3_VER "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}.${Python3_VERSION_PATCH}")
  endif()

  if((PY3_VER VERSION_LESS ${${MODULE}_VER} AND ENABLE_INTERNAL_PYTHON) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_PYTHON))

    buildPython3()
  else()

    if(KODI_DEPENDSBUILD)
      find_library(FFI_LIBRARY ffi REQUIRED)
      find_library(EXPAT_LIBRARY expat REQUIRED)
      find_library(INTL_LIBRARY intl REQUIRED)
      find_library(GMP_LIBRARY gmp REQUIRED)
      find_library(LZMA_LIBRARY lzma REQUIRED)

      if(NOT CORE_SYSTEM_NAME STREQUAL android)
        set(PYTHON_DEP_LIBRARIES pthread dl util)
        if(CORE_SYSTEM_NAME STREQUAL linux)
          # python archive built via depends requires librt for _posixshmem library
          list(APPEND PYTHON_DEP_LIBRARIES rt)
        endif()
      endif()

      list(APPEND PYTHON_DEP_LIBRARIES ${LZMA_LIBRARY} ${FFI_LIBRARY} ${EXPAT_LIBRARY} ${INTL_LIBRARY} ${GMP_LIBRARY})
    endif()
  endif()

  if(Python3_FOUND)
    set(PYTHON_VERSION "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}" CACHE INTERNAL "" FORCE)
    set(PYTHON_FOUND ${Python3_FOUND})

    add_library(Python::Python3 UNKNOWN IMPORTED)
    set_target_properties(Python::Python3 PROPERTIES
                                          INTERFACE_INCLUDE_DIRECTORIES "${Python3_INCLUDE_DIRS}"
                                          INTERFACE_COMPILE_DEFINITIONS HAS_PYTHON=1)

    if(WIN32)
      string(REGEX MATCH "^.*/lib/(.*)\.lib" Python3_DLL ${Python3_LIBRARIES})
      set(Python3_DLL "${DEPENDS_PATH}/bin/${CMAKE_MATCH_1}${CMAKE_SHARED_LIBRARY_SUFFIX}")
      set_target_properties(Python::Python3 PROPERTIES
                                            IMPORTED_IMPLIB "${Python3_DLL}"
                                            IMPORTED_LOCATION "${Python3_LIBRARIES}")
    else()
      set_target_properties(Python::Python3 PROPERTIES
                                            IMPORTED_LOCATION "${Python3_LIBRARIES}")
    endif()

    if(PYTHON_DEP_LIBRARIES)
      set_property(TARGET Python::Python3 APPEND PROPERTY
                                                 INTERFACE_LINK_LIBRARIES "${PYTHON_DEP_LIBRARIES}")
    endif()

    if(Python3_LINK_OPTIONS)
      set_property(TARGET Python::Python3 APPEND PROPERTY
                                                 INTERFACE_LINK_LIBRARIES "${Python3_LINK_OPTIONS}")
    endif()

    if(TARGET python3)
      add_dependencies(Python::Python3 python3)
    endif()

    # Add internal build target when a Multi Config Generator is used
    # We cant add a dependency based off a generator expression for targeted build types,
    # https://gitlab.kitware.com/cmake/cmake/-/issues/19467
    # therefore if the find heuristics only find the library, we add the internal build
    # target to the project to allow user to manually trigger for any build type they need
    # in case only a specific build type is actually available (eg Release found, Debug Required)
    # This is mainly targeted for windows who required different runtime libs for different
    # types, and they arent compatible
    if(_multiconfig_generator)
      if(NOT TARGET python3)
        buildPython3()
        set_target_properties(python3 PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends python3)
    endif()

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Python::Python3)

    if(KODI_DEPENDSBUILD OR WIN32)
      if(NOT WIN32)
        find_package(Pythonmodule-Setuptools)
      endif()
      find_package(Pythonmodule-PIL)
      find_package(Pythonmodule-Pycryptodome)
    endif()
  endif()
endif()
