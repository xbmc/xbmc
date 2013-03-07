if(TARGET_COMMON_DARWIN)
  add_subdirectory(CoreAudio)
else(TARGET_COMMON_DARWIN)
  add_subdirectory(SoftAE)
endif(TARGET_COMMON_DARWIN)

if(DEFINED HAVE_LIBPULSE)
  add_subdirectory(PulseAE)
endif()


