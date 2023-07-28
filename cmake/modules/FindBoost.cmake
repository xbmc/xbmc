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
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

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

    # Darwin platforms use clang for ASM compilation with the context lib
    # we need to pass -arch flag in particular, but we will just pass CFLAGS
    # generically.
    if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
      set(CMAKE_EXTRA_ARGS "-DCMAKE_ASM_FLAGS=${CMAKE_C_FLAGS}")
    endif()

    # Ensure the ARM assembler sees the correct FPU/ABI settings for
    # Boost.Context on hard-float toolchains.
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|armv[7-8]|armel|armhf)$")
      set(_boost_arm_asm_flags "${CMAKE_ASM_FLAGS}")
      if(CMAKE_LIBRARY_ARCHITECTURE MATCHES "gnueabihf"
         OR CMAKE_C_COMPILER_TARGET MATCHES "gnueabihf"
         OR CMAKE_C_COMPILER MATCHES "gnueabihf")
        string(APPEND _boost_arm_asm_flags " -mfpu=vfp -mfloat-abi=hard")
      endif()
      list(APPEND CMAKE_EXTRA_ARGS "-DCMAKE_ASM_FLAGS=${_boost_arm_asm_flags}")
      unset(_boost_arm_asm_flags)
    endif()

    # For our own sanity, just use unversioned system layout. Lib names then also
    # dont append toolset info eg.
    #   versioned: libboost_container-vc142-mt-x64-1_85.lib
    #   system:    libboost_container.lib
    if(WIN32 OR WINDOWS_STORE)
      list(APPEND CMAKE_EXTRA_ARGS "-DBOOST_INSTALL_LAYOUT=system")
    endif()

    if(WINDOWS_STORE)
      # ToDo: warning LNK4264: archiving object file compiled with /ZW into a static library; note
      #       that when authoring Windows Runtime types it is not recommended to link with a static library 
      #       that contains Windows Runtime metadata unless /WHOLEARCHIVE is specified to include everything 
      #       from the static library
      set(BOOST_CXX_FLAGS "-DWINAPI_FAMILY=WINAPI_FAMILY_APP -D_WIN32_WINNT=0x0A00 /ZW /Zc:twoPhase-")
    endif()

    # Define Boost.Context implementation to use 'fcontext'
    list(APPEND CMAKE_EXTRA_ARGS "-DBOOST_CONTEXT_IMPLEMENTATION=fcontext")

    # Add BOOST_CONTEXT_ARCHITECTURE to CMake arguments
    if(WIN32 OR WINDOWS_STORE)
      # If the generator platform is set (VS multi-arch), use it to determine
      # the boost context architecture. Normalize the platform string to
      # uppercase first so the check is case-insensitive.
      if(CMAKE_GENERATOR_PLATFORM)
        string(TOUPPER "${CMAKE_GENERATOR_PLATFORM}" _gen_platform)
        if(_gen_platform STREQUAL "WIN32")
          set(BOOST_CONTEXT_ARCHITECTURE "i386")
        elseif(_gen_platform STREQUAL "X64")
          set(BOOST_CONTEXT_ARCHITECTURE "x86_64")
        elseif(_gen_platform STREQUAL "ARM")
          set(BOOST_CONTEXT_ARCHITECTURE "arm")
        elseif(_gen_platform STREQUAL "ARM64")
          set(BOOST_CONTEXT_ARCHITECTURE "arm64")
        else()
          message(FATAL_ERROR "Unrecognized generator platform: ${CMAKE_GENERATOR_PLATFORM}")
        endif()
      else()
        # Fallback for generators that do not specify a platform. Determine the
        # architecture from CMAKE_SYSTEM_PROCESSOR instead.
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86)$")
          set(BOOST_CONTEXT_ARCHITECTURE "i386")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
          set(BOOST_CONTEXT_ARCHITECTURE "x86_64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|armv[7-8]|armel|armhf|ARM)$")
          set(BOOST_CONTEXT_ARCHITECTURE "arm")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
          set(BOOST_CONTEXT_ARCHITECTURE "arm64")
        else()
          message(FATAL_ERROR "Unknown CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
        endif()
      endif()
    else()
      # Dynamically determine the BOOST_CONTEXT_ARCHITECTURE based on the
      # desired system architecture
      if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86)$")
        set(BOOST_CONTEXT_ARCHITECTURE "i386")
      elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
        set(BOOST_CONTEXT_ARCHITECTURE "x86_64")
      elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|armv[7-8]|armel|armhf)$")
        set(BOOST_CONTEXT_ARCHITECTURE "arm")
      elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
        set(BOOST_CONTEXT_ARCHITECTURE "arm64")
      else()
        message(FATAL_ERROR "Unknown CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
      endif()
    endif()
    list(APPEND CMAKE_EXTRA_ARGS "-DBOOST_CONTEXT_ARCHITECTURE=${BOOST_CONTEXT_ARCHITECTURE}")

    # On Windows ARM/ARM64, Boost.Context uses the AAPCS ABI (file names *_aapcs_pe_armasm.asm)
    if((WIN32 OR WINDOWS_STORE) AND BOOST_CONTEXT_ARCHITECTURE MATCHES "^(arm|arm64)$")
      list(APPEND CMAKE_EXTRA_ARGS "-DBOOST_CONTEXT_ABI=aapcs")
    endif()

    # Force-disable embedded GDB scripts to avoid build failures on certain
    # toolchains. This is a workaround for issues related to inline assembly
    # errors with .debug_gdb_scripts.
    list(APPEND CMAKE_EXTRA_ARGS "-DBOOST_ALL_NO_EMBEDDED_GDB_SCRIPTS=ON")

    set(CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
                   "-DBOOST_INCLUDE_LIBRARIES=${BOOST_INCLUDE_LIBS}"
                   -DBUILD_SHARED_LIBS=OFF
                   ${CMAKE_EXTRA_ARGS})

    BUILD_DEP_TARGET()

    # create target for each boost_lib for linking purposes
    foreach(lib ${BOOST_LIBS})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME}::${lib} UNKNOWN IMPORTED)

      if(WIN32 OR WINDOWS_STORE)
        set(_lib_extension "lib")
      else()
        set(_lib_extension "a")
      endif()

      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME}::${lib} PROPERTIES
                                                                               IMPORTED_CONFIGURATIONS RELEASE
                                                                               IMPORTED_LOCATION_RELEASE "${DEP_LOCATION}/lib/libboost_${lib}.${_lib_extension}")

      if(DEFINED BOOST_DEBUG_POSTFIX)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME}::${lib} PROPERTIES
                                                                                 IMPORTED_LOCATION_DEBUG "${DEP_LOCATION}/lib/libboost_${lib}${BOOST_DEBUG_POSTFIX}.${_lib_extension}")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME}::${lib} APPEND PROPERTY
                                                                                      IMPORTED_CONFIGURATIONS DEBUG)
      endif()

      # Create "standard" target aliases for Boost components
      add_library(Boost::${lib} ALIAS ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME}::${lib})
    endforeach()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC boost)

  SETUP_BUILD_VARS()

  # TODO: Check for existing boost. If version >= BOOST-VERSION file version, dont build
  if(ENABLE_INTERNAL_BOOST)
    if(NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      # Build lib
      buildBoost()
    endif()
  else()
    # TODO
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Boost
                                    REQUIRED_VARS
                                      ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR
                                      ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(Boost_FOUND)
    find_package(OpenSSL REQUIRED ${SEARCH_QUIET})

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)

    set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                          INTERFACE_INCLUDE_DIRECTORIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR};${OPENSSL_INCLUDE_DIR}")

    # Skip linking OpenSSL::Crypto on Windows due to duplicate symbols in the
    # shairplay library
    if(NOT WIN32 AND NOT WINDOWS_STORE)
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            INTERFACE_LINK_LIBRARIES "OpenSSL::Crypto")
    endif()

    if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
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
      if(NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
        buildBoost()
        set_target_properties(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()
  else()
    if(Boost_FIND_REQUIRED)
      message(FATAL_ERROR "boost libraries were not found. You may want to try -DENABLE_INTERNAL_BOOST=ON")
    endif()
  endif()
endif()
