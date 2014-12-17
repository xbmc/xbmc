#ifndef FISCHE_H
#define FISCHE_H

/* int types */
#include <stdint.h>
/* size_t */
#include <stdlib.h>

typedef struct fische {

    /* 16 <= width <= 2048
     * DEFAULT: 512
     * constant after fische_start() */
    uint16_t    width;

    /* 16 <= height <= 2048
     * DEFAULT: 256
     * constant after fische_start() */
    uint16_t    height;

    /* 1 <= used_cpus <= 8
     * DEFAULT: all available (autodetect)
     * constant after fische_start() */
    uint8_t     used_cpus;

    /* true (!=0) or false (0)
     * DEFAULT: 0 */
    uint8_t     nervous_mode;

    /* see below (audio format enum)
     * DEFAULT: FISCHE_AUDIOFORMAT_FLOAT
     * constant after fische_start() */
    uint8_t     audio_format;

    /* see below (pixel format enum)
     * DEFAULT: FISCHE_PIXELFORMAT_0xAABBGGRR
     * constant after fische_start() */
    uint8_t     pixel_format;

    /* see below (blur mode enum)
     * DEFAULT: FISCHE_BLUR_SLICK
     * constant after fische_start() */
    uint8_t     blur_mode;

    /* see below (line style enum)
     * DEFAULT: FISCHE_LINESTYLE_ALPHA_SIMULATION */
    uint8_t     line_style;

    /* 0.5 <= scale <= 2.0
     * DEFAULT: 1.0
     * constant after fische_start() */
    double      scale;

    /* -10 <= amplification <= 10
     * DEFAULT: 0 */
    double      amplification;

    /* if non-NULL,
     * fische calls this to read vector fields from an external source
     * takes a void** for data placement
     * returns the number of bytes read */
    size_t (*read_vectors) (void**);

    /* if non-NULL,
     * fische calls this to write vector field data to an external sink
     * takes a void* and the number of bytes to be written */
    void (*write_vectors) (const void*, size_t);

    /* if non-NULL,
     * fische calls this on major beats that are not handled internally
     * takes frames per beat */
    void (*on_beat) (double);

    /* read only */
    uint32_t    frame_counter;

    /* read only */
    char*       error_text;

    void*       priv;

} FISCHE;



#ifdef __cplusplus
extern "C" {
#endif



    /* creates a new FISCHE object
     * and initialzes it with default values */
    FISCHE*     fische_new();

    /* starts FISCHE */
    int         fische_start (FISCHE* handle);

    /* makes the next frame available */
    uint32_t*   fische_render (FISCHE* handle);

    /* destructs the FISCHE object */
    void        fische_free (FISCHE* handle);

    /* inserts audio data */
    void        fische_audiodata (FISCHE* handle, const void* data, size_t data_size);



#ifdef __cplusplus
}
#endif



/* audio sample formats */
enum {FISCHE_AUDIOFORMAT_U8,
      FISCHE_AUDIOFORMAT_S8,
      FISCHE_AUDIOFORMAT_U16,
      FISCHE_AUDIOFORMAT_S16,
      FISCHE_AUDIOFORMAT_U32,
      FISCHE_AUDIOFORMAT_S32,
      FISCHE_AUDIOFORMAT_FLOAT,
      FISCHE_AUDIOFORMAT_DOUBLE,
      _FISCHE__AUDIOFORMAT_LAST_
     };

/* pixel formats */
enum {FISCHE_PIXELFORMAT_0xRRGGBBAA,
      FISCHE_PIXELFORMAT_0xAABBGGRR,
      FISCHE_PIXELFORMAT_0xAARRGGBB,
      FISCHE_PIXELFORMAT_0xBBGGRRAA,
      _FISCHE__PIXELFORMAT_LAST_
     };

/* blur style */
enum {FISCHE_BLUR_SLICK,
      FISCHE_BLUR_FUZZY,
      _FISCHE__BLUR_LAST_
     };

/* line style */
enum {FISCHE_LINESTYLE_THIN,
      FISCHE_LINESTYLE_THICK,
      FISCHE_LINESTYLE_ALPHA_SIMULATION,
      _FISCHE__LINESTYLE_LAST_
     };

#endif
