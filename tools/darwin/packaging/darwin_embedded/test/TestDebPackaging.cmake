cmake_minimum_required(VERSION 3.15)

if(NOT KODI_SOURCE_DIR OR NOT TEST_BINARY_DIR OR NOT DPKG_DEB_EXECUTABLE OR
   NOT TEST_PLATFORM OR NOT TEST_BUNDLE_IDENTIFIER OR NOT TEST_ARCHITECTURE)
  message(FATAL_ERROR
    "Kodi source, test binary, dpkg-deb and platform settings are required")
endif()

set(_test_root "${TEST_BINARY_DIR}/darwin-embedded-deb-${TEST_PLATFORM}")
set(_packaging_dir "${_test_root}/tools/darwin/packaging/darwin_embedded")
set(_app_dir "${_test_root}/build/Debug-${TEST_PLATFORM}/Kodi.app")
set(_dsym_dir "${_test_root}/build/Debug-${TEST_PLATFORM}/Kodi.app.dSYM")
set(DARWIN_EMBEDDED_DSYM_TARGET_DIR "${_test_root}/dsyms")
file(REMOVE_RECURSE "${_test_root}")
file(MAKE_DIRECTORY "${_packaging_dir}" "${_app_dir}" "${_dsym_dir}"
                    "${DARWIN_EMBEDDED_DSYM_TARGET_DIR}")
file(WRITE "${_app_dir}/payload.txt" "packaged\n")
file(WRITE "${_dsym_dir}/symbols" "symbols\n")
file(WRITE "${_packaging_dir}/../gitrev-posix" "#!/bin/sh\nprintf 'safe-revision'\n")
file(CHMOD "${_packaging_dir}/../gitrev-posix"
     PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
                 WORLD_READ WORLD_EXECUTE)

get_filename_component(_dpkg_bin_dir "${DPKG_DEB_EXECUTABLE}" DIRECTORY)
get_filename_component(NATIVEPREFIX "${_dpkg_bin_dir}" DIRECTORY)
set(APP_NAME Kodi)
set(APP_VERSION_MAJOR 22)
set(APP_VERSION_MINOR 0)
set(APP_VERSION_TAG_LC alpha1)
set(APP_WEBSITE "https://kodi.tv")
set(PLATFORM "${TEST_PLATFORM}")
set(PLATFORM_BUNDLE_IDENTIFIER "${TEST_BUNDLE_IDENTIFIER}")

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
  "${_packaging_dir}/${TEST_BUNDLE_IDENTIFIER}64_22.0-0~alpha1_${TEST_PLATFORM}-arm.deb")
if(NOT EXISTS "${_package}")
  message(FATAL_ERROR "Expected package was not created: ${_package}")
endif()
if(NOT EXISTS
   "${DARWIN_EMBEDDED_DSYM_TARGET_DIR}/safe-revision-Kodi.app.dSYM.tar.bz2")
  message(FATAL_ERROR "Expected dSYM archive was not created")
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

foreach(_field IN ITEMS Package Version Architecture)
  execute_process(
    COMMAND "${DPKG_DEB_EXECUTABLE}" --field "${_package}" "${_field}"
    RESULT_VARIABLE _metadata_result
    OUTPUT_VARIABLE _metadata
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE _metadata_error)
  if(NOT _metadata_result EQUAL 0)
    message(FATAL_ERROR "Could not inspect ${_field}: ${_metadata_error}")
  endif()
  if(_field STREQUAL Package)
    set(_expected_metadata "${TEST_BUNDLE_IDENTIFIER}64")
  elseif(_field STREQUAL Version)
    set(_expected_metadata "22.0-0~alpha1")
  else()
    set(_expected_metadata "${TEST_ARCHITECTURE}")
  endif()
  if(NOT _metadata STREQUAL _expected_metadata)
    message(FATAL_ERROR
      "Package ${_field} is '${_metadata}', expected '${_expected_metadata}'")
  endif()
endforeach()

set(_unsafe_dir "${_packaging_dir}/../unsafe-package")
file(MAKE_DIRECTORY "${_unsafe_dir}")
file(WRITE "${_unsafe_dir}/sentinel" "keep\n")
set(PLATFORM_BUNDLE_IDENTIFIER "../unsafe-package")
configure_file(
  "${KODI_SOURCE_DIR}/tools/darwin/packaging/darwin_embedded/mkdeb-darwin_embedded.sh.in"
  "${_packaging_dir}/mkdeb-unsafe.sh"
  @ONLY)
execute_process(
  COMMAND sh "${_packaging_dir}/mkdeb-unsafe.sh" Debug
  RESULT_VARIABLE _unsafe_result
  OUTPUT_VARIABLE _unsafe_output
  ERROR_VARIABLE _unsafe_error)
if(_unsafe_result EQUAL 0)
  message(FATAL_ERROR "Packaging accepted an unsafe package identifier")
endif()
if(NOT "${_unsafe_output}${_unsafe_error}" MATCHES "Invalid package identifier")
  message(FATAL_ERROR "Packaging failed for the wrong reason")
endif()
if(NOT EXISTS "${_unsafe_dir}/sentinel")
  message(FATAL_ERROR "Unsafe package identifier removed a directory outside its staging path")
endif()

set(PLATFORM_BUNDLE_IDENTIFIER "${TEST_BUNDLE_IDENTIFIER}")
file(WRITE "${_packaging_dir}/../gitrev-posix" "#!/bin/sh\nprintf '../unsafe-revision'\n")
configure_file(
  "${KODI_SOURCE_DIR}/tools/darwin/packaging/darwin_embedded/mkdeb-darwin_embedded.sh.in"
  "${_packaging_dir}/mkdeb-unsafe-revision.sh"
  @ONLY)
execute_process(
  COMMAND sh "${_packaging_dir}/mkdeb-unsafe-revision.sh" Debug
  RESULT_VARIABLE _unsafe_revision_result
  OUTPUT_VARIABLE _unsafe_revision_output
  ERROR_VARIABLE _unsafe_revision_error)
if(_unsafe_revision_result EQUAL 0)
  message(FATAL_ERROR "Packaging accepted an unsafe git revision")
endif()
if(NOT "${_unsafe_revision_output}${_unsafe_revision_error}" MATCHES
   "Invalid git revision")
  message(FATAL_ERROR "dSYM packaging failed for the wrong reason")
endif()
if(EXISTS "${DARWIN_EMBEDDED_DSYM_TARGET_DIR}/../unsafe-revision-Kodi.app.dSYM.tar.bz2")
  message(FATAL_ERROR "Unsafe git revision wrote a dSYM archive outside its target directory")
endif()
