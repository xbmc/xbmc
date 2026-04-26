#.rst:
# FindLibJXL
# ----------
# Finds the libjxl library
#
# This will define the following target ALIAS:
#
#   LIBRARY::LibJXL    - The libjxl library
#

if(NOT TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildmacroLibJXL)

    find_package(Brotli REQUIRED ${SEARCH_QUIET})
    find_package(Highway REQUIRED ${SEARCH_QUIET})
    find_package(LCMS2 REQUIRED ${SEARCH_QUIET})

    if(WIN32 OR WINDOWS_STORE)
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-win-remove_mlib.patch")

      generate_patchcommand("${patches}")
      unset(patches)
    endif()

    set(CMAKE_ARGS -DBUILD_TESTING=OFF
                   -DBUILD_SHARED_LIBS=OFF
                   -DJPEGXL_ENABLE_TOOLS=OFF
                   -DJPEGXL_ENABLE_FUZZERS=OFF
                   -DJPEGXL_ENABLE_JPEGLI=OFF
                   -DJPEGXL_ENABLE_JPEGLI_LIBJPEG=OFF
                   -DJPEGXL_ENABLE_DOXYGEN=OFF
                   -DJPEGXL_ENABLE_MANPAGES=OFF
                   -DJPEGXL_ENABLE_BENCHMARK=OFF
                   -DJPEGXL_ENABLE_EXAMPLES=OFF
                   -DJPEGXL_BUNDLE_LIBPNG=OFF
                   -DJPEGXL_ENABLE_JNI=OFF
                   -DJPEGXL_ENABLE_SJPEG=OFF
                   -DJPEGXL_ENABLE_OPENEXR=OFF
                   -DJPEGXL_ENABLE_SKCMS=OFF
                   -DJPEGXL_ENABLE_TCMALLOC=OFF
                   -DJPEGXL_FORCE_SYSTEM_BROTLI=ON
                   -DJPEGXL_FORCE_SYSTEM_LCMS2=ON
                   -DJPEGXL_FORCE_SYSTEM_HWY=ON)

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR ${DEPENDS_PATH}/include/jxl)

    # We need the dir created to avoid non-existant path errors
    file(MAKE_DIRECTORY ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR})

    BUILD_DEP_TARGET()

    if(NOT (WIN32 OR WINDOWSSTORE))
      include(CheckCXXSymbolExists)
      set(JXL_CXX_LIB "")
      check_cxx_symbol_exists(__GLIBCXX__ iostream LIBSTDCXX)
      check_cxx_symbol_exists(_LIBCPP_VERSION iostream LIBCXX)
      if(LIBSTDCXX)
        set(JXL_CXX_LIB "-lstdc++")
      elseif(LIBCXX)
        set(JXL_CXX_LIB "-lc++")
      endif()

      include(CheckLibraryExists)
      
      CHECK_LIBRARY_EXISTS(m sin "" HAVE_LIB_M)                                                                                                
                                                                                                                                               
      if(HAVE_LIB_M)                                                                                                                          
          set(JXL_MATH_LIB m)                                                                                                      
      endif()

      if("${CORE_SYSTEM_NAME}" STREQUAL android)
        set(EXTRA_LIBS log)
      endif()
    endif()

    # Retrieve suffix of platform byproduct to apply to additional jxl libraries
    string(REGEX REPLACE "^.*\\." "" _LIBEXT ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT})
    if(NOT (WIN32 OR WINDOWS_STORE))
      set(_PREFIX "lib")
    endif()

    set(LIBJXLTHREADS_LIBRARY_RELEASE "${DEP_LOCATION}/lib/${_PREFIX}jxl_threads.${_LIBEXT}")
    set(LIBJXLCMS_LIBRARY_RELEASE "${DEP_LOCATION}/lib/${_PREFIX}jxl_cms.${_LIBEXT}")

    # Link libraries for target interface
    list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES LIBRARY::Brotli
                                                                    LIBRARY::brotlienc
                                                                    LIBRARY::Highway
                                                                    LIBRARY::LCMS2
                                                                    ${JXL_CXX_LIB}
                                                                    ${JXL_MATH_LIB}
                                                                    ${EXTRA_LIBS})

    # Add dependencies to build target
    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} LIBRARY::Brotli
                                                                        LIBRARY::Highway
                                                                        LIBRARY::LCMS2)
  endmacro()

  # If there is a potential this library can be built internally
  # Check its dependencies to allow forcing this lib to be built if one of its
  # dependencies requires being rebuilt
  if(ENABLE_INTERNAL_LIBJXL)
    # Dependency list of this find module for an INTERNAL build
    set(${CMAKE_FIND_PACKAGE_NAME}_DEPLIST Brotli
                                           Highway
                                           LCMS2)

    check_dependency_build(${CMAKE_FIND_PACKAGE_NAME} "${${CMAKE_FIND_PACKAGE_NAME}_DEPLIST}")
  endif()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libjxl)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_LIBJXL) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_LIBJXL) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})

      # libjxl_cms does not have a pkgconfig file shipped in some distros.
      pkg_check_modules(LibJXL-cms libjxl_cms ${SEARCH_QUIET} IMPORTED_TARGET)

      if(TARGET PkgConfig::LibJXL-cms)
        add_library(LIBRARY::LibJXL-cms ALIAS PkgConfig::LibJXL-cms)
      endif()

      pkg_check_modules(LibJXL-threads libjxl_threads ${SEARCH_QUIET} REQUIRED IMPORTED_TARGET)

      add_library(LIBRARY::LibJXL-threads ALIAS PkgConfig::LibJXL-threads)
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE LIBRARY)
      SETUP_BUILD_TARGET()

      add_dependencies(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # LibJXL creates two additional libraries. As we are building in this path, we
      # manually create the additional libs, and there data
      add_library(LIBRARY::LibJXL-threads UNKNOWN IMPORTED)
      set_target_properties(LIBRARY::LibJXL-threads PROPERTIES
                                                    IMPORTED_LOCATION "${LIBJXLTHREADS_LIBRARY_RELEASE}"
                                                    INTERFACE_COMPILE_DEFINITIONS JXL_THREADS_STATIC_DEFINE
                                                    INTERFACE_INCLUDE_DIRECTORIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}")
  
      add_library(LIBRARY::LibJXL-cms UNKNOWN IMPORTED)
      set_target_properties(LIBRARY::LibJXL-cms PROPERTIES
                                                IMPORTED_LOCATION "${LIBJXLCMS_LIBRARY_RELEASE}"
                                                INTERFACE_COMPILE_DEFINITIONS JXL_CMS_STATIC_DEFINE
                                                INTERFACE_INCLUDE_DIRECTORIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}")

      if(JXL_CXX_LIB)
        set_property(TARGET LIBRARY::LibJXL-cms APPEND PROPERTY
                                                 INTERFACE_LINK_LIBRARIES "${JXL_CXX_LIB}")
      endif()
      
      if(JXL_MATH_LIB)
        set_property(TARGET LIBRARY::LibJXL-cms APPEND PROPERTY
                                                       INTERFACE_LINK_LIBRARIES "${JXL_MATH_LIB}")
        set_property(TARGET LIBRARY::LibJXL-threads APPEND PROPERTY
                                                           INTERFACE_LINK_LIBRARIES "${JXL_MATH_LIB}")
      endif()

      set_property(TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                     INTERFACE_LINK_LIBRARIES LIBRARY::LibJXL-cms)

      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS JXL_STATIC_DEFINE)
      ADD_TARGET_COMPILE_DEFINITION()

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      # Property must be set on actual TARGET and not the ALIAS
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(LibJXL_FIND_REQUIRED)
      message(FATAL_ERROR "libjxl library was not found.")
    endif()
  endif()
endif()
