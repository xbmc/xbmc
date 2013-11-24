
#add_definitions(-D__STDC_CONSTANT_MACROS)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS")
if(TARGET_RPI)
  find_all_sources(. omxplayer_SRCS)
  add_sources(${omxplayer_SRCS})
endif()
