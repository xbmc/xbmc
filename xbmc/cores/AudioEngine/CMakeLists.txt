add_subdirectory(Encoders)
add_subdirectory(Engines)
if(NOT APPLE)
  add_subdirectory(Sinks)
endif()
add_subdirectory(Interfaces)
add_subdirectory(Utils)

add_sources(AEFactory.cpp AEFactory.h AEAudioFormat.h)
