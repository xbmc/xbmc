if(ENABLE_TESTING)
  include_directories(gmock gmock/include)
  add_library(gmock STATIC gmock/src/gmock-all.cc)
 endif(ENABLE_TESTING)
