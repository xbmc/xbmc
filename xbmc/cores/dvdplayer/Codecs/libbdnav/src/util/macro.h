
#ifndef MACRO_H_
#define MACRO_H_

#include <stdio.h>

#define HEX_PRINT(X,Y) { int zz; for(zz = 0; zz < Y; zz++) fprintf(stderr, "%02X", X[zz]); fprintf(stderr, "\n"); }
#define MKINT_BE16(X) ( (X)[0] << 8 | (X)[1] )
#define MKINT_BE24(X) ( (X)[0] << 16 | (X)[1] << 8 | (X)[2] )
#define MKINT_BE32(X) ( (X)[0] << 24 | (X)[1] << 16 |  (X)[2] << 8 | (X)[3] )
#define X_FREE(X) do { if (X) free(X); X = NULL; } while(0)

#endif /* MACRO_H_ */
