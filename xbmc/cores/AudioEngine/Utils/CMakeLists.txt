find_all_sources(. ae_SRCS)

if(TARGET_WIN32)
  list(REMOVE_ITEM ae_SRCS ./AEELDParser.cpp)
  list(REMOVE_ITEM ae_SRCS ./AEELDParser.h)
endif(TARGET_WIN32)
  
add_sources(${ae_SRCS})
