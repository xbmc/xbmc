if(TARGET ${APP_NAME_LC}::Bluray)
  set(SOURCES BlurayPlaylistHints.cpp
              M2TSParser.cpp
              MPLSParser.cpp
              ProjectParser.cpp
              StreamParser.cpp)
  set(HEADERS BitReader.h
              BlurayPlaylistHints.h
              M2TSParser.h
              MPLSParser.h
              PlaylistStructure.h
              ProjectParser.h
              StreamParser.h)

  core_add_library(bluray)
endif()