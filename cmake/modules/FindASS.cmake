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

  macro(buildmacroASS)

    find_package(FreeType REQUIRED ${SEARCH_QUIET})
    find_package(HarfBuzz REQUIRED ${SEARCH_QUIET})
    find_package(FriBidi REQUIRED ${SEARCH_QUIET})
    find_package(Iconv REQUIRED ${SEARCH_QUIET})

    # Posix platforms (except Apple) use fontconfig
    if(NOT (WIN32 OR WINDOWS_STORE) AND
       NOT (CMAKE_SYSTEM_NAME STREQUAL Darwin))
      find_package(Fontconfig REQUIRED ${SEARCH_QUIET})
    endif()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

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
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES ${APP_NAME_LC}::FriBidi ${APP_NAME_LC}::Iconv ${APP_NAME_LC}::HarfBuzz ${APP_NAME_LC}::FreeType)

    if(WIN32 OR WINDOWS_STORE)
      # Directwrite dependency
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES dwrite.lib)
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
      # Coretext dependencies
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES "-framework CoreText"
                                                                      "-framework CoreFoundation")
    else()
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES Fontconfig::Fontconfig)
      add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} Fontconfig::Fontconfig)
    endif()

    # Add dependencies to build target
    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} ${APP_NAME_LC}::FriBidi
                                                                        ${APP_NAME_LC}::Iconv
                                                                        ${APP_NAME_LC}::HarfBuzz
                                                                        ${APP_NAME_LC}::FreeType)
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libass)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_ASS) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_ASS))
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    if(TARGET libass::libass)
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      get_target_property(_ASS_CONFIGURATIONS libass::libass IMPORTED_CONFIGURATIONS)
      if(_ASS_CONFIGURATIONS)
        foreach(_ass_config IN LISTS _ASS_CONFIGURATIONS)
          # Some non standard config (eg None on Debian)
          # Just set to RELEASE var so select_library_configurations can continue to work its magic
          string(TOUPPER ${_ass_config} _ass_config_UPPER)
          if((NOT ${_ass_config_UPPER} STREQUAL "RELEASE") AND
             (NOT ${_ass_config_UPPER} STREQUAL "DEBUG"))
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE libass::libass IMPORTED_LOCATION_${_ass_config_UPPER})
          else()
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_${_ass_config_UPPER} libass::libass IMPORTED_LOCATION_${_ass_config_UPPER})
          endif()
        endforeach()
      else()
        get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE libass::libass IMPORTED_LOCATION)
      endif()

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR libass::libass INTERFACE_INCLUDE_DIRECTORIES)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${libass_VERSION})
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      # INTERFACE_LINK_OPTIONS is incorrectly populated when cmake generation is executed
      # when an existing build generation is already done. Just set this to blank
      set_target_properties(PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} PROPERTIES INTERFACE_LINK_OPTIONS "")
  
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_LINK_LIBRARIES 0 ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)
      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} INTERFACE_INCLUDE_DIRECTORIES)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION})
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARIES)

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(ASS
                                    REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(ASS_FOUND)
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    elseif(TARGET libass::libass AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      # Kodi custom libass target used for windows platforms
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libass::libass)
    else()
      SETUP_BUILD_TARGET()
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(ASS_FIND_REQUIRED)
      message(FATAL_ERROR "Ass libraries were not found.")
    endif()
  endif()
endif()
