#ifndef PREPOSTCHUNKPROCESSING_H_
#define PREPOSTCHUNKPROCESSING_H_

MONGO_EXTERN_C_START

#include "bson.h"

#define GRIDFILE_COMPRESS 2

MONGO_EXPORT int initPrepostChunkProcessing( int flags );

MONGO_EXTERN_C_END
#endif