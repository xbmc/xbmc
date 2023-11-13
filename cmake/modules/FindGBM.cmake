# FindGBM
# ----------
# Finds the GBM library
#
# This will define the following target:
#
#   GBM::GBM   - The GBM library

if(NOT TARGET GBM::GBM)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GBM gbm QUIET)
  endif()

  find_path(GBM_INCLUDE_DIR NAMES gbm.h
                            HINTS ${PC_GBM_INCLUDEDIR}
                            NO_CACHE)
  find_library(GBM_LIBRARY NAMES gbm
                           HINTS ${PC_GBM_LIBDIR}
                           NO_CACHE)

  set(GBM_VERSION ${PC_GBM_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(GBM
                                    REQUIRED_VARS GBM_LIBRARY GBM_INCLUDE_DIR
                                    VERSION_VAR GBM_VERSION)

  include(CheckCSourceCompiles)
  set(CMAKE_REQUIRED_LIBRARIES ${GBM_LIBRARY})
  check_c_source_compiles("#include <gbm.h>

                           int main()
                           {
                             gbm_bo_map(NULL, 0, 0, 0, 0, GBM_BO_TRANSFER_WRITE, NULL, NULL);
                           }
                           " GBM_HAS_BO_MAP)

  check_c_source_compiles("#include <gbm.h>

                           int main()
                           {
                             gbm_surface_create_with_modifiers(NULL, 0, 0, 0, NULL, 0);
                           }
                           " GBM_HAS_MODIFIERS)

  if(GBM_FOUND)
    add_library(GBM::GBM UNKNOWN IMPORTED)
    set_target_properties(GBM::GBM PROPERTIES
                                   IMPORTED_LOCATION "${GBM_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${GBM_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS "HAVE_GBM=1")
    if(GBM_HAS_BO_MAP)
      set_property(TARGET GBM::GBM APPEND PROPERTY
                                          INTERFACE_COMPILE_DEFINITIONS HAS_GBM_BO_MAP=1)
    endif()
    if(GBM_HAS_MODIFIERS)
      set_property(TARGET GBM::GBM APPEND PROPERTY
                                          INTERFACE_COMPILE_DEFINITIONS HAS_GBM_MODIFIERS=1)
    endif()
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP GBM::GBM)
  endif()
endif()
