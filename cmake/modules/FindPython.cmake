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
# This will define the following targets:
#
#   ${APP_NAME_LC}::Python - The Python library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroPython)

    find_package(BZip2 REQUIRED ${SEARCH_QUIET})
    find_package(EXPAT REQUIRED ${SEARCH_QUIET})

    find_library(FFI_LIBRARY ffi REQUIRED)
    find_library(GMP_LIBRARY gmp REQUIRED)

    find_package(Iconv REQUIRED ${SEARCH_QUIET})
    find_package(Intl REQUIRED ${SEARCH_QUIET})
    find_package(LibLZMA REQUIRED ${SEARCH_QUIET})
    find_package(LibXml2 REQUIRED ${SEARCH_QUIET})
    find_package(OpenSSL REQUIRED ${SEARCH_QUIET})
    find_package(Sqlite3 REQUIRED ${SEARCH_QUIET})

    # ToDo existing build reqs not handled
    #python3:  gettext

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES BZip2::BZip2
                                                            EXPAT::EXPAT
                                                            ${FFI_LIBRARY}
                                                            ${GMP_LIBRARY}
                                                            LIBRARY::Iconv
                                                            Intl::Intl
                                                            LibLZMA::LibLZMA
                                                            LibXml2::LibXml2
                                                            LIBRARY::OpenSSL
                                                            LIBRARY::Sqlite3)


    string(REGEX MATCH "^([0-9]?)\.([0-9]+)\." Python3_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    set(Python3_VERSION_MAJOR ${CMAKE_MATCH_1} CACHE INTERNAL "" FORCE)
    set(Python3_VERSION_MINOR ${CMAKE_MATCH_2} CACHE INTERNAL "" FORCE)

    if(CORE_SYSTEM_NAME STREQUAL linux)
      set(EXTRA_CONFIGURE ac_cv_pthread=yes)
      if("webos" IN_LIST CORE_PLATFORM_NAME_LC)
        list(APPEND EXTRA_CONFIGURE ac_cv_lib_intl_textdomain=yes)
      endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
      set(EXTRA_CONFIGURE ac_cv_lib_intl_textdomain=yes)

      set(PYSDKROOT SDKROOT=${SDKROOT})

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

    if(NOT Iconv_IS_BUILT_IN)
      set(LIBS "LIBS=-liconv")
    endif()

    # Disabled c extension modules for all platforms
    set(PY_MODULES py_cv_module_grp=n/a
                   py_cv_module_syslog=n/a
                   py_cv_module__dbm=n/a
                   py_cv_module__gdbm=n/a
                   py_cv_module__uuid=n/a
                   py_cv_module_readline=n/a
                   py_cv_module__curses=n/a
                   py_cv_module__curses_panel=n/a
                   py_cv_module_xx=n/a
                   py_cv_module_xxlimited=n/a
                   py_cv_module_xxlimited_35=n/a
                   py_cv_module_xxsubtype=n/a
                   py_cv_module__xxsubinterpreters=n/a
                   py_cv_module__tkinter=n/a
                   py_cv_module__curses=n/a
                   py_cv_module__codecs_jp=n/a
                   py_cv_module__codecs_kr=n/a
                   py_cv_module__codecs_tw=n/a)

    # These modules use "internal" libs for building. The required static archives
    # are not installed outside of the cpython build tree, and cause failure in kodi linking
    # If we wish to support them in the future, we should create "system libs" for them
    list(APPEND PY_MODULES py_cv_module__decimal=n/a
                           py_cv_module__sha2=n/a)

    if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
      list(APPEND PY_MODULES py_cv_module__posixsubprocess=n/a
                             py_cv_module__scproxy=n/a)
    endif()

    list(APPEND patches "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/process-vm-ready.patch"
                        "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/static-compile.patch")

    if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
      list(APPEND patches "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/apple.patch")
    endif()

    if(CORE_SYSTEM_NAME STREQUAL android)
      list(APPEND patches "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/11-android-ctypes.patch"
                          "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/12-android-arm.patch")
    endif()

    if(EXISTS "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/10-${CORE_SYSTEM_NAME}-modules.patch")
      list(APPEND patches "${CMAKE_SOURCE_DIR}/tools/depends/target/python3/10-${CORE_SYSTEM_NAME}-modules.patch")
    endif()

    generate_patchcommand("${patches}")
    unset(patches)

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
    find_package(PythonInterpreter REQUIRED ${SEARCH_QUIET})

    # Python uses ax_c_float_words_bigendian.m4 to find autoconf-archive
    # Make sure we can find it as a requirement as well
    find_file(AUTOCONF-ARCHIVE "ax_c_float_words_bigendian.m4" PATHS "${NATIVEPREFIX}/share/aclocal" NO_CMAKE_FIND_ROOT_PATH REQUIRED)
    string(REGEX REPLACE "/ax_c_float_words_bigendian.m4" "" AUTOCONF-ARCHIVE ${AUTOCONF-ARCHIVE})
    set(ACLOCAL_PATH_VAR "ACLOCAL_PATH=${AUTOCONF-ARCHIVE}")

    set(CONFIGURE_COMMAND ${PYSDKROOT} ${LIBS} ${ACLOCAL_PATH_VAR} ${AUTORECONF} -vif
                  COMMAND ${PYSDKROOT} ${LIBS} ./configure
                            --prefix=${DEPENDS_PATH}
                            --disable-shared
                            --without-ensurepip
                            --disable-framework
                            --without-pymalloc
                            --enable-ipv6
                            --with-build-python=${PYTHON_EXECUTABLE}
                            --with-system-expat=yes
                            --disable-test-modules
                            MODULE_BUILDTYPE=static
                            ${PY_MODULES}
                            ${EXTRA_CONFIGURE})

    set(BUILD_COMMAND ${PYSDKROOT} ${LIBS} ${MAKE_EXECUTABLE} libpython${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}.a)

    set(INSTALL_COMMAND ${PYSDKROOT} ${LIBS} ${MAKE_EXECUTABLE} install -j1)

    set(BUILD_IN_SOURCE 1)

    BUILD_DEP_TARGET()

    set(PYTHON_SITE_PKG "${DEPENDS_PATH}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages" CACHE INTERNAL "" FORCE)

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
    # We must create the directory, otherwise we get non-existant path errors
    file(MAKE_DIRECTORY ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR})

    if(UNIX)
      add_custom_command(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} POST_BUILD
                         COMMAND find ${DEPENDS_PATH}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR} -type f -name \"*.pyc\" -delete || (exit 0)
                         COMMAND find ${DEPENDS_PATH}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR} -type d -name \"config-${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}-*\" -exec rm -r {} + || (exit 0))

    endif()

    # Add dependencies to build target
    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} BZip2::BZip2
                                                                        EXPAT::EXPAT
                                                                        LIBRARY::Iconv
                                                                        Intl::Intl
                                                                        LibLZMA::LibLZMA
                                                                        LibXml2::LibXml2
                                                                        LIBRARY::OpenSSL
                                                                        LIBRARY::Sqlite3)

    set(Python3_FOUND TRUE)
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC python3)

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

  find_package(Python3 ${VERSION} ${EXACT_VER} COMPONENTS Development ${SEARCH_QUIET})

  # If there is a potential this library can be built internally
  # Check its dependencies to allow forcing this lib to be built if one of its
  # dependencies requires being rebuilt
  if(ENABLE_INTERNAL_PYTHON)
    # Dependency list of this find module for an INTERNAL build
    # expat gettext libxml2 sqlite3 openssl libffi bzip2 xz $(ICONV)
    # set(${CMAKE_FIND_PACKAGE_NAME}_DEPLIST libs)

    # check_dependency_build(${CMAKE_FIND_PACKAGE_NAME} "${${CMAKE_FIND_PACKAGE_NAME}_DEPLIST}")
  endif()

  if(("${Python3_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} OR
     DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD) AND KODI_DEPENDSBUILD)

    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(Python3_FOUND)
    # We use this all over the place. Maybe it would be nice to keep it as a TARGET property
    # but for now a cached variable will do
    set(PYTHON_VERSION "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}" CACHE INTERNAL "" FORCE)

    if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      SETUP_BUILD_TARGET()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

    else()
      if(KODI_DEPENDSBUILD)
        set(EXPAT_USE_STATIC_LIBS TRUE)
        find_package(EXPAT REQUIRED ${SEARCH_QUIET})

        find_library(FFI_LIBRARY ffi REQUIRED)
        find_library(GMP_LIBRARY gmp REQUIRED)

        find_package(Iconv REQUIRED ${SEARCH_QUIET})
        find_package(Intl REQUIRED ${SEARCH_QUIET})
        find_package(LibLZMA REQUIRED ${SEARCH_QUIET})

        if(NOT CORE_SYSTEM_NAME STREQUAL android)
          set(PYTHON_DEP_LIBRARIES pthread dl util)
          if(CORE_SYSTEM_NAME STREQUAL linux)
            # python archive built via depends requires librt for _posixshmem library
            list(APPEND PYTHON_DEP_LIBRARIES rt)
          endif()
        endif()

        set(Py_LINK_LIBRARIES EXPAT::EXPAT ${FFI_LIBRARY} ${GMP_LIBRARY} LIBRARY::Iconv Intl::Intl LibLZMA::LibLZMA ${PYTHON_DEP_LIBRARIES})
      endif()

      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION "${Python3_LIBRARIES}"
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${Python3_INCLUDE_DIRS}"
                                                                       INTERFACE_LINK_OPTIONS "${Python3_LINK_OPTIONS}")

      if(Py_LINK_LIBRARIES)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         INTERFACE_LINK_LIBRARIES "${Py_LINK_LIBRARIES}")
      endif()
    endif()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_PYTHON)
    ADD_TARGET_COMPILE_DEFINITION()

    # Add python modules
    if(KODI_DEPENDSBUILD)
      if(NOT PYTHON_SITE_PKG)
        set(PYTHON_SITE_PKG "${DEPENDS_PATH}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages" CACHE INTERNAL "" FORCE)
      endif()

      find_package(PythonmodulePIL REQUIRED ${SEARCH_QUIET})
      find_package(PythonmodulePycryptodome REQUIRED ${SEARCH_QUIET})

      ADD_MULTICONFIG_BUILDMACRO()
    endif()
  endif()
endif()
