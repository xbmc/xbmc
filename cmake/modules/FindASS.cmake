#.rst:
# FindASS
# -------
# Finds the ASS library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::ASS   - The ASS library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildlibASS)

    find_package(FreeType REQUIRED ${SEARCH_QUIET})
    find_package(HarfBuzz REQUIRED ${SEARCH_QUIET})
    find_package(FriBidi REQUIRED ${SEARCH_QUIET})
    find_package(Iconv REQUIRED ${SEARCH_QUIET})

    # Posix platforms (except Apple) use fontconfig
    if(NOT (WIN32 OR WINDOWS_STORE) AND
       NOT (CMAKE_SYSTEM_NAME STREQUAL Darwin))
      find_package(Fontconfig REQUIRED ${SEARCH_QUIET})
    endif()

    set(ASS_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

    if(WIN32 OR WINDOWS_STORE)
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/libass/01-win-CMakeLists.patch"
                  "${CMAKE_SOURCE_DIR}/tools/depends/target/libass/02-win-dwrite-fontload.patch"
                  "${CMAKE_SOURCE_DIR}/tools/depends/target/libass/03-win-dwrite-prevent-ass_set_fonts_dir.patch"
                  "${CMAKE_SOURCE_DIR}/tools/depends/target/libass/04-win-nullcheck-shared_hdc.patch")

      generate_patchcommand("${patches}")

      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)
    endif()

    if(WIN32 OR WINDOWS_STORE)

      set(CMAKE_ARGS -DNATIVEPREFIX=${NATIVEPREFIX}
                     -DARCH=${ARCH}
                     ${LIBASS_ADDITIONAL_ARGS})
    else()
      find_program(AUTORECONF autoreconf REQUIRED)
      if (CMAKE_HOST_SYSTEM_NAME MATCHES "(Free|Net|Open)BSD")
        find_program(MAKE_EXECUTABLE gmake)
      endif()
      find_program(MAKE_EXECUTABLE make REQUIRED)

      if(ARCH STREQUAL x86_64)
        find_program(NASM nasm HINTS "${NATIVEPREFIX}/bin")
        if(NOT NASM_FOUND)
          set(DISABLE_ASM "--disable-asm")
        endif()
      elseif(NOT ARCH STREQUAL aarch64)
          set(DISABLE_ASM "--disable-asm")
      endif()

      set(CONFIGURE_COMMAND ${AUTORECONF} -vif
                    COMMAND ./configure
                              --prefix=${DEPENDS_PATH}
                              --disable-shared
                              --disable-libunibreak
                              ${DISABLE_ASM})

      set(BUILD_COMMAND ${MAKE_EXECUTABLE})
      set(INSTALL_COMMAND ${MAKE_EXECUTABLE} install)
      set(BUILD_IN_SOURCE 1)
    endif()

    BUILD_DEP_TARGET()

    # Link libraries for target interface
    set(ASS_LINK_LIBRARIES ${APP_NAME_LC}::FriBidi ${APP_NAME_LC}::Iconv ${APP_NAME_LC}::HarfBuzz ${APP_NAME_LC}::FreeType)

    if(WIN32 OR WINDOWS_STORE)
      # Directwrite dependency
      list(APPEND ASS_LINK_LIBRARIES dwrite.lib)
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
      # Coretext dependencies
      list(APPEND ASS_LINK_LIBRARIES "-framework CoreText"
                                     "-framework CoreFoundation")
    else()
      list(APPEND ASS_LINK_LIBRARIES Fontconfig::Fontconfig)
      add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} Fontconfig::Fontconfig)
    endif()

    set(ASS_INCLUDE_DIR ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR})
    set(ASS_LIBRARY_RELEASE ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE})
    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG)
      set(ASS_LIBRARY_DEBUG ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG})
    endif()

    # Add dependencies to build target
    add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} ${APP_NAME_LC}::FriBidi)
    add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} ${APP_NAME_LC}::Iconv)
    add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} ${APP_NAME_LC}::HarfBuzz)
    add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} ${APP_NAME_LC}::FreeType)
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libass)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()


  # Check for cmake config that is used by our windows platforms custom cmake
  find_package(libass ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                      HINTS ${DEPENDS_PATH}/lib/cmake
                      ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # fallback to pkgconfig for non windows platforms
  if(NOT libass_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})
    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(libass libass${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()

  if((libass_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_ASS) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_ASS))
    # build internal module
    buildlibASS()
  else()
    if(TARGET libass::libass)
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      get_target_property(_ASS_CONFIGURATIONS libass::libass IMPORTED_CONFIGURATIONS)
      foreach(_ass_config IN LISTS _ASS_CONFIGURATIONS)
        # Some non standard config (eg None on Debian)
        # Just set to RELEASE var so select_library_configurations can continue to work its magic
        string(TOUPPER ${_ass_config} _ass_config_UPPER)
        if((NOT ${_ass_config_UPPER} STREQUAL "RELEASE") AND
           (NOT ${_ass_config_UPPER} STREQUAL "DEBUG"))
          get_target_property(ASS_LIBRARY_RELEASE libass::libass IMPORTED_LOCATION_${_ass_config_UPPER})
        else()
          get_target_property(ASS_LIBRARY_${_ass_config_UPPER} libass::libass IMPORTED_LOCATION_${_ass_config_UPPER})
        endif()
      endforeach()
  
      get_target_property(ASS_INCLUDE_DIR libass::libass INTERFACE_INCLUDE_DIRECTORIES)
      set(ASS_VERSION ${libass_VERSION})
    elseif(TARGET PkgConfig::libass)
      # INTERFACE_LINK_OPTIONS is incorrectly populated when cmake generation is executed
      # when an existing build generation is already done. Just set this to blank
      set_target_properties(PkgConfig::libass PROPERTIES INTERFACE_LINK_OPTIONS "")
  
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET libass_LINK_LIBRARIES 0 ASS_LIBRARY_RELEASE)
      get_target_property(ASS_INCLUDE_DIR PkgConfig::libass INTERFACE_INCLUDE_DIRECTORIES)
      set(ASS_VERSION ${libass_VERSION})
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(ASS)
  unset(ASS_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(ASS
                                    REQUIRED_VARS ASS_LIBRARY ASS_INCLUDE_DIR
                                    VERSION_VAR ASS_VERSION)

  if(ASS_FOUND)
    if(TARGET PkgConfig::libass AND NOT TARGET libass)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::libass)
    elseif(TARGET libass::libass AND NOT TARGET libass)
      # Kodi custom libass target used for windows platforms
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libass::libass)
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      if(ASS_LIBRARY_RELEASE)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_CONFIGURATIONS RELEASE
                                                                         IMPORTED_LOCATION_RELEASE "${ASS_LIBRARY_RELEASE}")
      endif()
      if(ASS_LIBRARY_DEBUG)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_LOCATION_DEBUG "${ASS_LIBRARY_DEBUG}")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              IMPORTED_CONFIGURATIONS DEBUG)
      endif()
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${ASS_INCLUDE_DIR}")

      if(ASS_LINK_LIBRARIES)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         INTERFACE_LINK_LIBRARIES "${ASS_LINK_LIBRARIES}")  
      endif()
    endif()

    if(TARGET libass)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} libass)
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
      if(NOT TARGET libass)
        buildlibASS()
        set_target_properties(libass PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends libass)
    endif()
  else()
    if(ASS_FIND_REQUIRED)
      message(FATAL_ERROR "Ass libraries were not found.")
    endif()
  endif()
endif()
