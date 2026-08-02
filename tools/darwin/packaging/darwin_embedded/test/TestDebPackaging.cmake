cmake_minimum_required(VERSION 3.15)

if(NOT KODI_SOURCE_DIR OR NOT TEST_BINARY_DIR OR NOT DPKG_DEB_EXECUTABLE)
  message(FATAL_ERROR
    "KODI_SOURCE_DIR, TEST_BINARY_DIR and DPKG_DEB_EXECUTABLE are required")
endif()

set(_test_root "${TEST_BINARY_DIR}/darwin-embedded-deb")
set(_packaging_dir "${_test_root}/tools/darwin/packaging/darwin_embedded")
set(_app_dir "${_test_root}/build/Debug-iphoneos/Kodi.app")
file(REMOVE_RECURSE "${_test_root}")
file(MAKE_DIRECTORY "${_packaging_dir}" "${_app_dir}")
file(WRITE "${_app_dir}/payload.txt" "packaged\n")

get_filename_component(_dpkg_bin_dir "${DPKG_DEB_EXECUTABLE}" DIRECTORY)
get_filename_component(NATIVEPREFIX "${_dpkg_bin_dir}" DIRECTORY)
set(APP_NAME Kodi)
set(APP_VERSION_MAJOR 22)
set(APP_VERSION_MINOR 0)
set(APP_VERSION_TAG_LC alpha1)
set(APP_WEBSITE "https://kodi.tv")
set(PLATFORM iphoneos)
set(PLATFORM_BUNDLE_IDENTIFIER org.xbmc.kodi-ios)

configure_file(
  "${KODI_SOURCE_DIR}/tools/darwin/packaging/darwin_embedded/mkdeb-darwin_embedded.sh.in"
  "${_packaging_dir}/mkdeb-darwin_embedded.sh"
  @ONLY)

execute_process(
  COMMAND sh "${_packaging_dir}/mkdeb-darwin_embedded.sh" Debug
  RESULT_VARIABLE _package_result
  OUTPUT_VARIABLE _package_output
  ERROR_VARIABLE _package_error)
if(NOT _package_result EQUAL 0)
  message(FATAL_ERROR
    "Debian packaging failed (${_package_result}):\n${_package_output}\n${_package_error}")
endif()

set(_package
  "${_packaging_dir}/org.xbmc.kodi-ios64_22.0-0~alpha1_iphoneos-arm.deb")
if(NOT EXISTS "${_package}")
  message(FATAL_ERROR "Expected package was not created: ${_package}")
endif()

execute_process(
  COMMAND "${DPKG_DEB_EXECUTABLE}" --contents "${_package}"
  RESULT_VARIABLE _contents_result
  OUTPUT_VARIABLE _contents
  ERROR_VARIABLE _contents_error)
if(NOT _contents_result EQUAL 0)
  message(FATAL_ERROR "Could not inspect package: ${_contents_error}")
endif()
if(NOT _contents MATCHES "Applications/Kodi\\.app/payload\\.txt")
  message(FATAL_ERROR "Package does not contain the app payload:\n${_contents}")
endif()

execute_process(
  COMMAND "${DPKG_DEB_EXECUTABLE}" --field "${_package}"
          Package Version Architecture
  RESULT_VARIABLE _metadata_result
  OUTPUT_VARIABLE _metadata
  ERROR_VARIABLE _metadata_error)
if(NOT _metadata_result EQUAL 0)
  message(FATAL_ERROR "Could not inspect package metadata: ${_metadata_error}")
endif()
if(NOT _metadata MATCHES
   "Package: org\\.xbmc\\.kodi-ios64.*Version: 22\\.0-0~alpha1.*Architecture: iphoneos-arm")
  message(FATAL_ERROR "Package metadata is incorrect:\n${_metadata}")
endif()
