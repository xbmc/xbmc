#ifndef _RAR_DEFS_
#define _RAR_DEFS_

#define  Min(x,y) (((x)<(y)) ? (x):(y))
#define  Max(x,y) (((x)>(y)) ? (x):(y))

#define  MAXPASSWORD       128

#define  DefSFXName        "default.sfx"
#define  DefSortListName   "rarfiles.lst"

#ifndef FA_RDONLY
  #define FA_RDONLY   0x01
  #define FA_HIDDEN   0x02
  #define FA_SYSTEM   0x04
  #define FA_LABEL    0x08
  #define FA_DIREC    0x10
  #define FA_ARCH     0x20
#endif

#endif
