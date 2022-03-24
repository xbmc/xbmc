set(SOURCES SavestateDatabase.cpp
            SavestateFlatBuffer.cpp
)

set(HEADERS ISavestate.h
            SavestateDatabase.h
            SavestateFlatBuffer.h
            SavestateTypes.h
)

core_add_library(retroplayer_savestates)

set(DEPENDS retroplayer_messages)

if(ENABLE_STATIC_LIBS)
  add_dependencies(retroplayer_savestates ${DEPENDS})
else()
  add_dependencies(lib${APP_NAME_LC} ${DEPENDS})
endif()
