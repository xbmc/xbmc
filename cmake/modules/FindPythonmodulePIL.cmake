# FindPythonmodulePIL
# --------
# Finds/Builds Pillow Python package
#
# This module will build the python module on the system
#
# --------
#
# This module will define the following variables:
#
# Python::PythonmodulePIL - The python Pillow module
#
# --------
#

if(NOT TARGET Python::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroPythonmodulePIL)
    find_package(PythonInterpreter REQUIRED ${SEARCH_QUIET})
    # We explicitly do not search for Python as a required dependency due to recursion
    find_package(PythonmoduleSetuptools REQUIRED ${SEARCH_QUIET})

    find_package(FreeType REQUIRED ${SEARCH_QUIET})
    find_package(FriBidi REQUIRED ${SEARCH_QUIET})
    find_package(HarfBuzz REQUIRED ${SEARCH_QUIET})
    find_package(ZLIB REQUIRED ${SEARCH_QUIET})

    # ToDo: libjpeg-turbo

    if(${CORE_SYSTEM_NAME} MATCHES "windows")
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/10-win-cmake.patch"
                  "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/11-win-uwp.patch")
    else()
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/pillow-crosscompile.patch")
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
                     ${ADDITIONAL_ARGS})

      set(BUILD_BYPRODUCTS "${PYTHON_SITE_PKG}/PIL/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT}")
    else()
      set(CFLAGS ${CMAKE_C_FLAGS})
      set(LDFLAGS ${CMAKE_EXE_LINKER_FLAGS})
      set(LDSHARED "${CMAKE_C_COMPILER} -shared")
      get_filename_component(ZLIB_ROOT ${ZLIB_INCLUDE_DIRS} DIRECTORY)

      if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        if(CPU STREQUAL arm64)
          set(CFLAGS "${CMAKE_C_FLAGS} -target arm64-apple-darwin")
          set(LDFLAGS "${CMAKE_EXE_LINKER_FLAGS} -target arm64-apple-darwin")
        endif()

        set(LDSHARED "${CMAKE_C_COMPILER} -bundle -undefined dynamic_lookup ${LDFLAGS}")
      elseif(${CORE_SYSTEM_NAME} STREQUAL android)
        find_package(Pythonmoduledummylib REQUIRED ${SEARCH_QUIET})
        # ToDo: ideally we want to get the -L link path from the Python::dummylib TARGET
        #       For now, we just expect the dummy lib to be built as part of tools/depends
        set(LDFLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${DEPENDS_PATH}/lib/dummy-lib${APP_NAME_LC} -l${APP_NAME_LC} -lm")
      endif()

      if(${CORE_SYSTEM_NAME} STREQUAL darwin_embedded OR ${CORE_SYSTEM_NAME} STREQUAL android)
        set(PIL_OUTPUT_DIR ${DEPENDS_PATH}/share/${APP_NAME_LC}/addons/script.module.pil/lib)
      else()
        set(PIL_OUTPUT_DIR ${PYTHON_SITE_PKG})
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

      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPASS_DEP_BUILDENV ON)

      # Set Target Configure command. Must be "" if no step required
      set(CONFIGURE_COMMAND COMMAND "")

      set(BUILD_COMMAND ${CMAKE_COMMAND} -E env ${PYMOD_TARGETENV} ${PROJECT_BUILDENV} PYTHONPATH=${PIL_OUTPUT_DIR}
                        ${PYTHON_EXECUTABLE} setup.py build
                                                      build_py
                                                      build_ext
                                                      --plat-name ${OS}-${CPU}
                                                      --disable-jpeg2000
                                                      --disable-webp
                                                      --disable-imagequant
                                                      --disable-tiff
                                                      --disable-xcb
                                                      --disable-lcms
                                                      --disable-platform-guessing
                                                      install --install-lib ${PIL_OUTPUT_DIR}
                                                      bdist_egg)

      set(INSTALL_COMMAND ${CMAKE_COMMAND} -E true)

      set(BUILD_IN_SOURCE 1)

      set(BUILD_BYPRODUCTS "${PIL_OUTPUT_DIR}/PIL/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT}")
    endif()

    BUILD_DEP_TARGET()

    if("webos" IN_LIST CORE_PLATFORM_NAME_LC OR
       ${CORE_SYSTEM_NAME} STREQUAL android OR
       ${CORE_SYSTEM_NAME} STREQUAL darwin_embedded)

      # Extraction only adds stub loaders for _imaging* libraries.
      # We install the whole lib as part of the BUILD_COMMAND, and as we have to patch
      # android loaders anyway, we will skip extraction and just copy in stub py files.
      # For webos/darwin_embedded, we will just extract to get the standard stub py files.
      if(${CORE_SYSTEM_NAME} STREQUAL android)
        find_program(SED_EXECUTABLE sed REQUIRED)
        if(CMAKE_HOST_APPLE)
          set(SED_FLAG -i \"\")
        else()
          set(SED_FLAG -i)
        endif()

        # creates a stub loader .py file corresponding to each _imaging*.so lib and fill
        # android lib names (eg. lib_imaging.so)
        set(POST_COMMAND find ${PIL_OUTPUT_DIR}/PIL/ -maxdepth 1 -type f -name '_imaging*.so' -execdir sh -c 'libfile=$$\{1\#\#*\/\}\\\; cp -f ${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/12-android-loader.patch ${PIL_OUTPUT_DIR}/PIL/$$\{libfile%.so\}.py\\\; sed ${SED_FLAG} -e s,PYFILENAME,lib$$libfile, $$\{libfile%.so}.py' sh \{\} \\\\\;)

        add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} Python::Pythonmoduledummylib)
      else()
        find_program(UNZIP_EXECUTABLE unzip REQUIRED)
        set(POST_COMMAND ${UNZIP_EXECUTABLE} -o ${CMAKE_BINARY_DIR}/build/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/src/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/dist/pillow-${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}-*.egg -d ${PIL_OUTPUT_DIR})
      endif()

      add_custom_command(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} POST_BUILD
                                COMMAND ${POST_COMMAND})
    endif()

    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} ${APP_NAME_LC}::Python
                                                                        Python::PythonmoduleSetuptools
                                                                        LIBRARY::FreeType
                                                                        LIBRARY::FriBidi
                                                                        LIBRARY::HarfBuzz
                                                                        LIBRARY::ZLIB)
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC pythonmodule-pil)

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
     string(REGEX REPLACE "^.*__version__ = \"([0-9]+\.[0-9]+\.[0-9]+)\".*" "\\1" ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION ${pil_ver_file})
  endif()

  if("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  # Set TARGET namespace to Python
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE Python)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INTERFACE_LIB TRUE)

  SETUP_BUILD_TARGET()

  if(NOT TARGET python-modules)
    add_custom_target(python-modules)
    set_target_properties(python-modules PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()

  add_dependencies(python-modules ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE}::${CMAKE_FIND_PACKAGE_NAME})

  if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
  endif()

  ADD_MULTICONFIG_BUILDMACRO()
endif()
