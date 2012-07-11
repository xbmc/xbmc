#ifndef WAVEPAINTER_H
#define WAVEPAINTER_H

#include <stdint.h>

struct fische;
struct _fische__wavepainter_;
struct fische__wavepainter;



struct fische__wavepainter* fische__wavepainter_new (struct fische* parent);
void                        fische__wavepainter_free (struct fische__wavepainter* self);

void                        fische__wavepainter_paint (struct fische__wavepainter* self, double* data, uint_fast16_t size);
void                        fische__wavepainter_beat (struct fische__wavepainter* self, double bpm);
void                        fische__wavepainter_change_color (struct fische__wavepainter* self, double bpm, double energy);
void                        fische__wavepainter_change_shape (struct fische__wavepainter* self);



struct _fische__wavepainter_ {
    uint_fast16_t   width;
    uint_fast16_t   height;
    uint_fast16_t   center_x;
    uint_fast16_t   center_y;
    int_fast8_t     direction;
    uint_fast8_t    shape;
    uint_fast8_t    n_shapes;
    uint32_t        color_1;
    uint32_t        color_2;
    double          angle;
    uint_fast8_t    is_rotating;
    double          rotation_increment;

    struct fische* fische;
};

struct fische__wavepainter {
    struct _fische__wavepainter_* priv;
};

#endif
