#ifndef ANALYST_H
#define ANALYST_H

#include <stdint.h>

struct fische;
struct _fische__analyst_;
struct fische__analyst;



struct fische__analyst* fische__analyst_new (struct fische* parent);
void                    fische__analyst_free (struct fische__analyst* self);

int_fast8_t             fische__analyst_analyse (struct fische__analyst* self, double* data, uint_fast16_t size);



struct _fische__analyst_ {
    uint_fast8_t    state;
    double          moving_avg_30;
    double          moving_avg_03;
    double          std_dev;
    double          intensity_moving_avg;
    double          intensity_std_dev;
    uint_fast32_t   last_beat_frame;
    uint_fast16_t*  beat_gap_history;
    uint_fast8_t    bghist_head;

    struct fische*    fische;
};

struct fische__analyst {
    double relative_energy;
    double frames_per_beat;

    struct _fische__analyst_* priv;
};

#endif
