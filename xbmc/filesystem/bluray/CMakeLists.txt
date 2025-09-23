set(SOURCES)
set(HEADERS)

if(TARGET ${APP_NAME_LC}::Bluray)
  list(APPEND SOURCES M2TSParser.cpp
			MPLSParser.cpp
			StreamParser.cpp)
  list(APPEND HEADERS BitReader.h
			M2TSParser.h
			MPLSParser.h
			PlaylistStructure.h
			StreamParser.h)
endif()

core_add_library(bluray)
