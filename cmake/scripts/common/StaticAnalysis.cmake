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
            --rule-file=${CMAKE_SOURCE_DIR}/tools/static-analysis/cppcheck/cppcheck-rules.xml
            --suppress-xml=${CMAKE_SOURCE_DIR}/tools/static-analysis/cppcheck/cppcheck-suppressions.xml
            --output-file=${CMAKE_BINARY_DIR}/cppcheck-result.xml
    COMMENT "Static code analysis using cppcheck")
endif()

find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
find_program(RUN_CLANG_TIDY NAMES run-clang-tidy.py run-clang-tidy
                            PATHS /usr/share/clang /usr/bin)

if(RUN_CLANG_TIDY AND CLANG_TIDY_EXECUTABLE)
  add_custom_target(analyze-clang-tidy
    DEPENDS ${APP_NAME_LC} ${APP_NAME_LC}-test
    COMMAND ${RUN_CLANG_TIDY}
            -j${CPU_CORES}
            -clang-tidy-binary=${CLANG_TIDY_EXECUTABLE}
            -p=${CMAKE_BINARY_DIR}
            -header-filter='${CMAKE_BINARY_DIR}/.*/include/.*'
            | tee ${CMAKE_BINARY_DIR}/clangtidy-result.xml
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Static code analysis using clang-tidy")
endif()
