if(TARGET_COMMON_DARWIN)
  plex_find_header(libcrystalhd/libcrystalhd_if.h ${dependdir}/include)

  if(DEFINED HAVE_LIBCRYSTALHD_LIBCRYSTALHD_IF_H)
    CHECK_C_SOURCE_COMPILES("
      #include <libcrystalhd/bc_dts_types.h>
      #include <libcrystalhd/bc_dts_defs.h>
      PBC_INFO_CRYSTAL bCrystalInfo;
      int main() {}
    " CHECK_CRYSTALHD_VERSION)
    if(CHECK_CRYSTALHD_VERSION)
      set(HAVE_LIBCRYSTALHD 2)
    else(CHECK_CRYSTALHD_VERSION)
      set(HAVE_LIBCRYSTALHD 1)
    endif(CHECK_CRYSTALHD_VERSION)
  endif(DEFINED HAVE_LIBCRYSTALHD_LIBCRYSTALHD_IF_H)
endif(TARGET_COMMON_DARWIN)
