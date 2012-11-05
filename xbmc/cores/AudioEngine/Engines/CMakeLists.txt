if(APPLE)
  add_subdirectory(CoreAudio)
endif()

if(DEFINED USE_PULSE)
  add_subdirectory(PulseAE)
endif()

add_subdirectory(SoftAE)
