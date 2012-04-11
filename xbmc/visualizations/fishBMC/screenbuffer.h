#ifndef SCREENBUFFER_H
#define SCREENBUFFER_H

#include <stdint.h>

struct fische;
struct _fische__screenbuffer;
struct fische__screenbuffer;



struct fische__screenbuffer*    fische__screenbuffer_new (struct fische* parent);
void                            fische__screenbuffer_free (struct fische__screenbuffer* self);

void                            fische__screenbuffer_lock (struct fische__screenbuffer* self);
void                            fische__screenbuffer_unlock (struct fische__screenbuffer* self);

void fische__screenbuffer_line (struct fische__screenbuffer* self,
                                int_fast16_t x1,
                                int_fast16_t y1,
                                int_fast16_t x2,
                                int_fast16_t y2,
                                uint32_t color);



struct _fische__screenbuffer_ {
    uint_fast8_t        is_locked;
    int_fast16_t        width;
    int_fast16_t        height;
    uint_fast8_t        red_shift;
    uint_fast8_t        blue_shift;
    uint_fast8_t        green_shift;
    uint_fast8_t        alpha_shift;

    struct fische*    fische;
};

struct fische__screenbuffer {
    uint32_t*       pixels;

    struct _fische__screenbuffer_* priv;
};

#endif
