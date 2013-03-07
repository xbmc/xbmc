find_all_sources(. au_SRCS)
add_sources(${au_SRCS})

if(TARGET_COMMON_DARWIN)
  add_subdirectory(Mac)
elseif(TARGET_WIN32)
  add_subdirectory(Win)
endif(TARGET_COMMON_DARWIN)
