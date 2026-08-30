if(TARGET ${APP_NAME_LC}::Bluray)
  set(SOURCES IndexParser.cpp
              M2TSParser.cpp
              MovieObjectParser.cpp
              MPLSParser.cpp
              StreamParser.cpp)
  set(HEADERS BitReader.h
              IndexParser.h
              M2TSParser.h
              MovieObjectParser.h
              MPLSParser.h
              NavigationCommand.h
              PlaylistStructure.h
              StreamParser.h)

  core_add_library(bluray)
endif()