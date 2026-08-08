set(SOURCES SavestateDatabase.cpp
            SavestateBlob.cpp
            SavestateCompression.cpp
            SavestateFlatBuffer.cpp
            SavestateThumbnail.cpp
            SavestateWriteQueue.cpp
)

set(HEADERS ISavestate.h
            SavestateBlob.h
            SavestateCompression.h
            SavestateDatabase.h
            SavestateFlatBuffer.h
            SavestateThumbnail.h
            SavestateTypes.h
            SavestateWriteQueue.h
            SavestateWriteRequest.h
)

core_add_library(retroplayer_savestates)
target_link_libraries(${CORE_LIBRARY} PRIVATE LIBRARY::ZSTD)

set(DEPENDS retroplayer_messages)

if(ENABLE_STATIC_LIBS)
  add_dependencies(retroplayer_savestates ${DEPENDS})
else()
  add_dependencies(lib${APP_NAME_LC} ${DEPENDS})
endif()
