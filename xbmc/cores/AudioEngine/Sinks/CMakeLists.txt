set(ae_SRCS AESinkNULL.cpp AESinkNULL.h AESinkProfiler.cpp AESinkProfiler.h)

if(DEFINED HAS_ALSA)
  set(ae_SRCS ${ae_SRCS} AESinkALSA.cpp AESinkALSA.h)
endif()

if(TARGET_WIN32)
  set(ae_SRCS ${ae_SRCS} AESinkWASAPI.cpp AESinkWASAPI.h AESinkDirectSound.cpp AESinkDirectSound.h)
endif(TARGET_WIN32)

if(TARGET_COMMON_LINUX)
  set(ae_SRCS ${ae_SRCS} AESinkOSS.cpp AESinkOSS.h)
endif(TARGET_COMMON_LINUX)

add_sources(${ae_SRCS})
