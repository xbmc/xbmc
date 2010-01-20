#if WORDS_BIGENDIAN
#define COLOR_ARGB
#else
#define COLOR_BGRA
#endif

#if 1
/* ndef COLOR_BGRA */
/** position des composantes **/
    #define BLEU 2
    #define VERT 1
    #define ROUGE 0
    #define ALPHA 3
#else
    #define ROUGE 1
    #define BLEU 3
    #define VERT 2
    #define ALPHA 0
#endif

#ifndef guint32
#define guint8 unsigned char
#define guin16 unsigned short
#define guint32 unsigned int
#define gint8 signed char
#define gint16 signed short int
#define gint32 signed int
#endif
