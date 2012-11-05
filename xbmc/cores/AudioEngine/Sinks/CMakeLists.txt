set(ae_SRCS AEsinkNULL.cpp AESinkNULL.h AESinkProfiler.cpp AESinkProfiler.h)

if(DEFINED HAVE_LIBALSA)
  set(ae_SRCS ${ae_SRCS} AESinkALSA.cpp AESinkALSA.h)
endif()

if(WINDOWS)
  set(ae_SRCS ${ae_SRCS} AESinkWSAPI.cpp AESinkWSAPI.h AESinkDirectSound.cpp AESinkDirectSound.h)
endif()

add_sources(${ae_SRCS})
