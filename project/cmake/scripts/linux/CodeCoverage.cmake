# - CodeCoverage
# Generate code coverage reports with LCOV and GCovr.
#
# Configuration:
#  COVERAGE_SOURCE_DIR      - Source root directory (default ${CMAKE_SOURCE_DIR}).
#  COVERAGE_BINARY_DIR      - Directory where the coverage reports (and intermediate files)
#                             are generated to.
#  COVERAGE_EXCLUDES        - List of exclude patterns (for example '*/tests/*').
#
# The following targets will be generated:
#  coverage                 - Builds an html report. Requires LCOV.
#  coverage_xml             - Builds an xml report (in Cobertura format for Jenkins).
#                             Requires Gcovr.
#
# Inspired by https://github.com/bilke/cmake-modules/blob/master/CodeCoverage.cmake

# Comiler and linker setup
set(CMAKE_C_FLAGS_COVERAGE "-g -O0 --coverage" CACHE STRING
  "Flags used by the C compiler during coverage builds." FORCE)
set(CMAKE_CXX_FLAGS_COVERAGE "-g -O0 --coverage" CACHE STRING
  "Flags used by the C++ compiler during coverage builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "--coverage" CACHE STRING
  "Flags used for linking binaries during coverage builds." FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "--coverage" CACHE STRING
  "Flags used by the shared libraries linker during coverage builds." FORCE)
mark_as_advanced(
  CMAKE_C_FLAGS_COVERAGE CMAKE_CXX_FLAGS_COVERAGE CMAKE_EXE_LINKER_FLAGS_COVERAGE
  CMAKE_SHARED_LINKER_FLAGS_COVERAGE CMAKE_STATIC_LINKER_FLAGS_COVERAGE
)

find_program(LCOV_EXECUTABLE lcov)
find_program(GENINFO_EXECUTABLE geninfo)
find_program(GENHTML_EXECUTABLE genhtml)
find_program(GCOVR_EXECUTABLE gcovr)
mark_as_advanced(LCOV_EXECUTABLE GENINFO_EXECUTABLE GENHTML_EXECUTABLE GCOVR_EXECUTABLE)

# Default options
if(NOT COVERAGE_SOURCE_DIR)
  set(COVERAGE_SOURCE_DIR ${CMAKE_SOURCE_DIR})
endif()
if(NOT COVERAGE_BINARY_DIR)
  set(COVERAGE_BINARY_DIR ${CMAKE_BINARY_DIR}/coverage)
endif()
if(NOT COVERAGE_EXCLUDES)
  set(COVERAGE_EXCLUDES)
endif()

# Allow variables in COVERAGE_DEPENDS that are not evaluated before this file is included.
string(CONFIGURE "${COVERAGE_DEPENDS}" COVERAGE_DEPENDS)

# Add coverage target that generates an HTML report using LCOV
if(LCOV_EXECUTABLE AND GENINFO_EXECUTABLE AND GENHTML_EXECUTABLE)
  file(MAKE_DIRECTORY ${COVERAGE_BINARY_DIR})
  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_BINARY_DIR}
    COMMAND ${LCOV_EXECUTABLE} -z -q -d ${CMAKE_BINARY_DIR}
    COMMAND ${LCOV_EXECUTABLE} -c -q -i -d ${CMAKE_BINARY_DIR} -b ${COVERAGE_SOURCE_DIR}
                               -o ${COVERAGE_BINARY_DIR}/${PROJECT_NAME}.coverage_base.info
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test || true
    COMMAND ${LCOV_EXECUTABLE} -c -q -d ${CMAKE_BINARY_DIR} -b ${COVERAGE_SOURCE_DIR}
                               -o ${COVERAGE_BINARY_DIR}/${PROJECT_NAME}.coverage_test.info
    COMMAND ${LCOV_EXECUTABLE} -a ${COVERAGE_BINARY_DIR}/${PROJECT_NAME}.coverage_base.info
                               -a ${COVERAGE_BINARY_DIR}/${PROJECT_NAME}.coverage_test.info
                               -o ${COVERAGE_BINARY_DIR}/${PROJECT_NAME}.coverage.info -q
    COMMAND ${LCOV_EXECUTABLE} -q -r ${COVERAGE_BINARY_DIR}/${PROJECT_NAME}.coverage.info
                               /usr/include/* ${CMAKE_BINARY_DIR}/* ${COVERAGE_EXCLUDES}
                               -o ${COVERAGE_BINARY_DIR}/${PROJECT_NAME}.coverage.info
    COMMAND ${GENHTML_EXECUTABLE} ${COVERAGE_BINARY_DIR}/${PROJECT_NAME}.coverage.info
                               -o ${COVERAGE_BINARY_DIR}/html -s --legend --highlight --demangle-cpp
    COMMAND ${CMAKE_COMMAND} -E echo "Coverage report: file://${COVERAGE_BINARY_DIR}/html/index.html"
    WORKING_DIRECTORY ${COVERAGE_BINARY_DIR}
    VERBATIM
    DEPENDS ${COVERAGE_DEPENDS}
    COMMENT "Generate code coverage html report"
  )
else()
  message(WARNING "Target coverage not available (lcov, geninfo and genhtml needed).")
endif()

# Add coverage target that generates an XML report using Gcovr
if(GCOVR_EXECUTABLE)
  file(MAKE_DIRECTORY ${COVERAGE_BINARY_DIR})
  string(REGEX REPLACE "([^;]+)" "--exclude=\"\\1\"" _gcovr_excludes "${COVERAGE_EXCLUDES}")
  string(REPLACE "*" ".*" _gcovr_excludes "${_gcovr_excludes}")
  add_custom_target(coverage_xml
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test || true
    COMMAND ${GCOVR_EXECUTABLE} -x -r ${COVERAGE_SOURCE_DIR} -o ${COVERAGE_BINARY_DIR}/coverage.xml
                                --object-directory ${CMAKE_BINARY_DIR} ${_gcovr_excludes} ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Coverage report: file://${COVERAGE_BINARY_DIR}/coverage.xml"
    WORKING_DIRECTORY ${COVERAGE_BINARY_DIR}
    DEPENDS ${COVERAGE_DEPENDS}
    COMMENT "Generate code coverage xml report"
  )
  unset(_gcovr_excludes)
else()
  message(WARNING "Target coverage_xml not available (gcovr needed).")
endif()
