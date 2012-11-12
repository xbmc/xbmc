find_all_sources(. ae_SRCS)

if(WIN32)
  list(REMOVE_ITEM ae_SRCS ./AEELDParser.cpp)
  list(REMOVE_ITEM ae_SRCS ./AEELDParser.h)
endif()
  
add_sources(${ae_SRCS})
