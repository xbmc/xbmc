find_all_sources(. au_SRCS)
add_sources(${au_SRCS})

if(APPLE)
  add_subdirectory(Mac)
endif()