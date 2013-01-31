if(NOT WIN32)
  include(TestBigEndian)
  TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

  add_subdirectory(libdvdnav)
  add_subdirectory(libdvdcss)
  add_subdirectory(libdvdread)
endif()
