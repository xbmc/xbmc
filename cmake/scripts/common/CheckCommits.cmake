find_package(Git REQUIRED)

macro(sanity_check message)
  if(status_code)
    message(FATAL_ERROR "${message}")
  endif()
endmacro()

# Check that there are no changes in working-tree
execute_process(COMMAND ${GIT_EXECUTABLE} diff --quiet
                RESULT_VARIABLE status_code)
sanity_check("Cannot run with working tree changes. Commit, stash or drop them.")

# Setup base of tests
set(check_base $ENV{CHECK_BASE})
if(NOT check_base)
  set(check_base origin/master)
endif()

# Setup end of tests
set(check_head $ENV{CHECK_HEAD})
if(NOT check_head)
  set(check_head HEAD)
endif()

# Setup target to build
set(check_target $ENV{CHECK_TARGET})
if(NOT check_target)
  set(check_target check)
endif()

# Build threads
set(build_threads $ENV{CHECK_THREADS})
if(NOT build_threads)
  if(UNIX)
    execute_process(COMMAND nproc
                    OUTPUT_VARIABLE build_threads)
    string(REGEX REPLACE "(\r?\n)+$" "" build_threads "${build_threads}")
  endif()
endif()

# Record current HEAD
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
                OUTPUT_VARIABLE current_branch)

string(REGEX REPLACE "(\r?\n)+$" "" current_branch "${current_branch}")

# Grab revision list
execute_process(COMMAND ${GIT_EXECUTABLE} rev-list ${check_base}..${check_head} --reverse
                OUTPUT_VARIABLE rev_list)

string(REPLACE "\n" ";" rev_list ${rev_list})
foreach(rev ${rev_list})
  # Checkout
  message("Testing revision ${rev}")
  execute_process(COMMAND ${GIT_EXECUTABLE} checkout ${rev}
                  RESULT_VARIABLE status_code)
  sanity_check("Failed to checkout ${rev}")

  # Build
  if(build_threads GREATER 2)
    execute_process(COMMAND ${CMAKE_COMMAND} "--build" "${CMAKE_BINARY_DIR}" "--target" "${check_target}" "--use-stderr" "--" "-j${build_threads}"
                    RESULT_VARIABLE status_code)
  else()
    execute_process(COMMAND ${CMAKE_COMMAND} "--build" "${CMAKE_BINARY_DIR}" "--target" "${check_target}" "--use-stderr"
                    RESULT_VARIABLE status_code)
  endif()
  if(status_code)
    execute_process(COMMAND ${GIT_EXECUTABLE} checkout ${current_branch})
  endif()
  sanity_check("Failed to build target for revision ${rev}")
endforeach()

message("Everything checks out fine")
execute_process(COMMAND ${GIT_EXECUTABLE} checkout ${current_branch})
