if(TARGET ${APP_NAME_LC}::Bluray)
  set(SOURCES M2TSParser.cpp
              MPLSParser.cpp
              StreamParser.cpp)
  set(HEADERS BitReader.h
              M2TSParser.h
              MPLSParser.h
              PlaylistStructure.h
              StreamParser.h)

  core_add_library(bluray)
endif()