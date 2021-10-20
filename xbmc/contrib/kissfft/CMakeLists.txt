if(ENABLE_INTERNAL_KISSFFT)
  set(SOURCES kiss_fft.c
              kiss_fftr.c)

  set(HEADERS _kiss_fft_guts.h
              kiss_fft.h
              kiss_fftr.h)

  core_add_library(kissfft)
endif()
