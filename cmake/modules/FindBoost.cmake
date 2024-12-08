#.rst:
# FindBoost
# -------
# Finds the required boost libraries
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::Boost - The boost library
#   ${APP_NAME_LC}::Boost::container
#   ${APP_NAME_LC}::Boost::context
#   ${APP_NAME_LC}::Boost::coroutine
#   ${APP_NAME_LC}::Boost::date_time
#   ${APP_NAME_LC}::Boost::exception
#   ${APP_NAME_LC}::Boost::json
#   ${APP_NAME_LC}::Boost::random
#   Boost::container
#   Boost::context
#   Boost::coroutine
#   Boost::date_time
#   Boost::exception
#   Boost::json
#   Boost::random
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  macro(buildBoost)
    set(BOOST_VERSION ${${MODULE}_VER})

    # Boost libs used by libtorrent
    set(BOOST_LIBS container
                   context
                   coroutine
                   date_time
                   exception
                   json
                   random)

    # Header only libs required
    set(BOOST_HEADER_ONLY asio
                          beast
                          crc
                          config
                          core
                          logic
                          multi_index
                          optional
                          system
                          multiprecision
                          pool)

    set(BOOST_INCLUDE_LIBS_LIST ${BOOST_LIBS} ${BOOST_HEADER_ONLY})

    # We use the semicolon genex to pass through the semicolon separated list to the
    # externalproject_add call
    STRING(REPLACE ";" "\$\<SEMICOLON\>" BOOST_INCLUDE_LIBS "${BOOST_INCLUDE_LIBS_LIST}")

    set(BOOST_INCLUDE_LIBRARIES "-DBOOST_INCLUDE_LIBRARIES=${BOOST_INCLUDE_LIBS}")

    # Darwin platforms use clang for ASM compilation with the context lib
    # we need to pass -arch flag in particular, but we will just pass CFLAGS generically
    if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
      set(EXTRA_ARGS "-DCMAKE_ASM_FLAGS=${CMAKE_C_FLAGS}")
    endif()

    # For our own sanity, just use unversioned system layout. Lib names then also
    # dont append toolset info eg.
    #   versioned: libboost_container-vc142-mt-x64-1_85.lib
    #   system:    libboost_container.lib
    if(WIN32 OR WINDOWS_STORE)
      list(APPEND EXTRA_ARGS "-DBOOST_INSTALL_LAYOUT=system")
    endif()

    if(WINDOWS_STORE)
      # ToDo: warning LNK4264: archiving object file compiled with /ZW into a static library; note
      #       that when authoring Windows Runtime types it is not recommended to link with a static library 
      #       that contains Windows Runtime metadata unless /WHOLEARCHIVE is specified to include everything 
      #       from the static library
      set(BOOST_CXX_FLAGS "-DWINAPI_FAMILY=WINAPI_FAMILY_APP -D_WIN32_WINNT=0x0A00 /ZW /Zc:twoPhase-")
    endif()

    set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                   "-DBOOST_INCLUDE_LIBRARIES=${BOOST_INCLUDE_LIBS}"
                   -DBUILD_SHARED_LIBS=OFF
                   ${EXTRA_ARGS})

    BUILD_DEP_TARGET()

    # create target for each boost_lib for linking purposes
    foreach(lib ${BOOST_LIBS})
      add_library(${APP_NAME_LC}::Boost::${lib} UNKNOWN IMPORTED)

      if(WIN32 OR WINDOWS_STORE)
        set(_lib_extension "lib")
      else()
        set(_lib_extension "a")
      endif()

      set_target_properties(${APP_NAME_LC}::Boost::${lib} PROPERTIES
                                                          IMPORTED_CONFIGURATIONS RELEASE
                                                          IMPORTED_LOCATION_RELEASE "${DEP_LOCATION}/lib/libboost_${lib}.${_lib_extension}")

      if(DEFINED BOOST_DEBUG_POSTFIX)
        set_target_properties(${APP_NAME_LC}::Boost::${lib} PROPERTIES
                                                            IMPORTED_LOCATION_DEBUG "${DEP_LOCATION}/lib/libboost_${lib}${BOOST_DEBUG_POSTFIX}.${_lib_extension}")
        set_property(TARGET ${APP_NAME_LC}::Boost::${lib} APPEND PROPERTY
                                                                 IMPORTED_CONFIGURATIONS DEBUG)
      endif()

      # Create "standard" target aliases for Boost components
      add_library(Boost::${lib} ALIAS ${APP_NAME_LC}::Boost::${lib})
    endforeach()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC boost)

  SETUP_BUILD_VARS()

  # TODO: Check for existing boost. If version >= BOOST-VERSION file version, dont build
  if(ENABLE_INTERNAL_BOOST)
    # Build lib
    buildBoost()
  else()
    # TODO
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Boost
                                    REQUIRED_VARS BOOST_INCLUDE_DIR
                                    VERSION_VAR BOOST_VERSION)

  if(Boost_FOUND)

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     # May need this for UWP re LNK4264
                                                                     # causes duplicate symbols however.
                                                                     #INTERFACE_LINK_LIBRARIES "$<LINK_LIBRARY:WHOLE_ARCHIVE,${APP_NAME_LC}::Boost::container>;$<LINK_LIBRARY:WHOLE_ARCHIVE,${APP_NAME_LC}::Boost::context>;$<LINK_LIBRARY:WHOLE_ARCHIVE,${APP_NAME_LC}::Boost::exception>;$<LINK_LIBRARY:WHOLE_ARCHIVE,${APP_NAME_LC}::Boost::coroutine>;$<LINK_LIBRARY:WHOLE_ARCHIVE,${APP_NAME_LC}::Boost::date_time>;$<LINK_LIBRARY:WHOLE_ARCHIVE,${APP_NAME_LC}::Boost::json>;$<LINK_LIBRARY:WHOLE_ARCHIVE,${APP_NAME_LC}::Boost::random>"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${BOOST_INCLUDE_DIR}")

    if(TARGET boost)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} boost)
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
      if(NOT TARGET boost)
        buildBoost()
        set_target_properties(boost PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends boost)
    endif()
  else()
    if(Boost_FIND_REQUIRED)
      message(FATAL_ERROR "boost libraries were not found. You may want to try -DENABLE_INTERNAL_BOOST=ON")
    endif()
  endif()
endif()
