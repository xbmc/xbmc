set(HEADERS IRRemote.h
)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(input_remote)
endif()
