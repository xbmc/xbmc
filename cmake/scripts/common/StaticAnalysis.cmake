include(ProcessorCount)
ProcessorCount(CPU_CORES)

find_program(CPPCHECK_EXECUTABLE cppcheck)

if(CPPCHECK_EXECUTABLE)
  add_custom_target(analyze-cppcheck
    DEPENDS ${APP_NAME_LC} ${APP_NAME_LC}-test
    COMMAND ${CPPCHECK_EXECUTABLE}
            -j${CPU_CORES}
            --project=${CMAKE_BINARY_DIR}/compile_commands.json
            --std=c++${CMAKE_CXX_STANDARD}
            --enable=all
            --xml
            --xml-version=2
            --language=c++
            --relative-paths=${CMAKE_SOURCE_DIR}
            --suppress-xml=${CMAKE_SOURCE_DIR}/tools/static-analysis/cppcheck/cppcheck-suppressions.xml
            --output-file=${CMAKE_BINARY_DIR}/cppcheck-result.xml
    COMMENT "Static code analysis using cppcheck")
endif()
