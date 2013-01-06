set(ae_SRCS AESinkNULL.cpp AESinkNULL.h AESinkProfiler.cpp AESinkProfiler.h)

if(DEFINED HAS_ALSA)
  set(ae_SRCS ${ae_SRCS} AESinkALSA.cpp AESinkALSA.h)
endif()

if(WIN32)
  set(ae_SRCS ${ae_SRCS} AESinkWASAPI.cpp AESinkWASAPI.h AESinkDirectSound.cpp AESinkDirectSound.h)
endif()

if(UNIX)
  set(ae_SRCS ${ae_SRCS} AESinkOSS.cpp AESinkOSS.h)
endif()

add_sources(${ae_SRCS})
