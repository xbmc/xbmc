set(SOURCES
)

set(HEADERS ICecInputProvider.h
)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(input_cec)
endif()
