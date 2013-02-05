add_subdirectory(http-api)
add_subdirectory(info)
add_subdirectory(json-rpc)

if(ENABLE_PYTHON)
  add_subdirectory(legacy)
  add_subdirectory(python)
endif()

find_all_sources(. interfaces_SRCS)
add_sources(${interfaces_SRCS})
