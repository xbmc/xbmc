
if(NOT TARGET_COMMON_DARWIN)
  add_subdirectory(Encoders)
endif(NOT TARGET_COMMON_DARWIN)

add_subdirectory(Engines)
if(NOT APPLE)
  add_subdirectory(Sinks)
  add_sources(AESinkFactory.cpp AESinkFactory.h)
endif()
add_subdirectory(Interfaces)
add_subdirectory(Utils)

add_sources(AEFactory.cpp AEFactory.h AEAudioFormat.h)
