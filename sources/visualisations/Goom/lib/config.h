#define VERSION "1.99.4"

// target
//#define XMMS_PLUGIN

// for pc users with mmx processors.
#ifndef HAVE_MMX
#define HAVE_MMX
#endif
//#define VERBOSE

#ifndef guint32
#define guint8 unsigned char
#define guin16 unsigned short
#define guint32 unsigned int
#define gint8 signed char
#define gint16 signed short int
#define gint32 signed int
#endif
//#define COLOR_BGRA
#define COLOR_ARGB

#ifdef COLOR_BGRA
/** position des composantes **/
    #define ROUGE 2
    #define BLEU 0
    #define VERT 1
    #define ALPHA 3
#else
    #define ROUGE 1
    #define BLEU 3
    #define VERT 2
    #define ALPHA 0
#endif


#ifndef M_PI
#define M_PI 3.14159265358979
#endif

#ifdef __VC__
#ifndef __cplusplus
#define inline /* */
#endif
#endif

#ifdef WIN32
// Kris: Changed from windows.h to xtl.h
//#include <windows.h>
#include <xtl.h>

#define bzero ZeroMemory
#endif
