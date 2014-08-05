if(NOT TARGET_COMMON_DARWIN)
  set(main_SRCS main.cpp)
  add_library(xbmcmain OBJECT ${main_SRCS})
endif(NOT TARGET_COMMON_DARWIN)
