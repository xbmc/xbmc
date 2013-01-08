find_all_sources(. au_SRCS)
add_sources(${au_SRCS})

if(APPLE)
  add_subdirectory(Mac)
elseif(WIN32)
  add_subdirectory(Win)
endif()
