plex_find_header(shairport/shairport.h ${dependdir}/include)

if(DEFINED HAVE_SHAIRPORT_SHAIRPORT_H)
  CHECK_C_SOURCE_COMPILES("
    #include <shairport/shairport.h>
    int main(int argc, char *argv[])
    { 
      static struct AudioOutput test;
      if(sizeof(test.ao_set_metadata))
        return 0;
      return 0;
    }
  "
  HAVE_STRUCT_AUDIOOUTPUT_AO_SET_METADATA)
endif()
