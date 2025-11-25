# sqlite-vec - Vector Search Extension for SQLite
# Repository: https://github.com/asg017/sqlite-vec
# License: Apache 2.0 / MIT

# Check if source files exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sqlite-vec.c")
  message(WARNING "sqlite-vec.c not found. Please download from https://github.com/asg017/sqlite-vec/releases")
  message(WARNING "The semantic search feature will not be available without sqlite-vec source files.")
  return()
endif()

set(SOURCES sqlite-vec.c)
set(HEADERS sqlite-vec.h)

# Create static library
add_library(sqlite-vec STATIC ${SOURCES})

# Public interface includes
target_include_directories(sqlite-vec SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(sqlite-vec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# sqlite-vec requires SQLite3
find_package(SQLite3 REQUIRED)
target_link_libraries(sqlite-vec PRIVATE SQLite3::SQLite3)

# Compile definitions for sqlite-vec
target_compile_definitions(sqlite-vec PRIVATE
  SQLITE_CORE=1
  SQLITE_VEC_ENABLE_AVX2=1
  SQLITE_VEC_ENABLE_NEON=1
)

# Disable code analysis tools for third-party library
set_target_properties(sqlite-vec PROPERTIES
  CXX_CLANG_TIDY ""
  CXX_CPPCHECK ""
  CXX_INCLUDE_WHAT_YOU_USE ""
  FOLDER lib
)

# Organize source files
source_group_by_folder(sqlite-vec)

# Link this target lib to core
target_link_libraries(lib${APP_NAME_LC} PUBLIC sqlite-vec)
