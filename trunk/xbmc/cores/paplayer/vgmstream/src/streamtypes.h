/*
 * streamtypes.h - widely used type definitions
 */


#ifndef _STREAMTYPES_H
#define _STREAMTYPES_H

#ifdef _MSC_VER
#include <pstdint.h>
#define inline _inline
#define strcasecmp _stricmp
#define snprintf _snprintf
#else
#include <stdint.h>
#endif

typedef int16_t sample;

#endif
