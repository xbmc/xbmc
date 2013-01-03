if(APPLE)
  add_subdirectory(CoreAudio)
else()
  add_subdirectory(SoftAE)
endif()

if(DEFINED HAVE_LIBPULSE)
  add_subdirectory(PulseAE)
endif()


