#.rst:
# FindKissFFT
# ------------
# Finds the KissFFT as a Fast Fourier Transformation (FFT) library
#
# This will define the following variables:
#
# KISSFFT_FOUND        - System has KissFFT
# KISSFFT_INCLUDE_DIRS - the KissFFT include directory
# KISSFFT_LIBRARIES    - the KissFFT libraries
#

if(ENABLE_INTERNAL_KISSFFT)
  # KissFFT is located in xbmc/contrib/kissfft
  set(KISSFFT_FOUND TRUE)
  set(KISSFFT_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/xbmc/contrib")
  message(STATUS "Found KissFFT: ${KISSFFT_INCLUDE_DIRS}")
else()
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_KISSFFT kissfft QUIET)
  endif()

  find_path(KISSFFT_INCLUDE_DIR kissfft/kiss_fft.h kissfft/kiss_fftr.h
                            HINTS ${PC_KISSFFT_INCLUDEDIR})
  find_library(KISSFFT_LIBRARY NAMES kissfft-float kissfft-int32 kissfft-int16 kissfft-simd
                            HINTS ${PC_KISSFFT_LIBDIR})

  # Check if all REQUIRED_VARS are satisfied and set KISSFFT_FOUND
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(KissFFT REQUIRED_VARS KISSFFT_INCLUDE_DIR KISSFFT_LIBRARY)

  if(KISSFFT_FOUND)
    set(KISSFFT_INCLUDE_DIRS ${KISSFFT_INCLUDE_DIR})
    set(KISSFFT_LIBRARIES ${KISSFFT_LIBRARY})

    if(NOT TARGET kissfft)
      add_library(kissfft UNKNOWN IMPORTED)
      set_target_properties(kissfft PROPERTIES
                            IMPORTED_LOCATION "${KISSFFT_LIBRARY}"
                            INTERFACE_INCLUDE_DIRECTORIES "${KISSFFT_INCLUDE_DIR}")
    endif()
  endif()

  mark_as_advanced(KISSFFT_INCLUDE_DIR KISSFFT_LIBRARY)
endif()
