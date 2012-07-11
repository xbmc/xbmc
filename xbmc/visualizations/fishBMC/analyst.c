#include "fische_internal.h"

#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#endif

enum {_FISCHE__WAITING_, _FISCHE__MAYBEWAITING_, _FISCHE__BEAT_};

int
_fische__compare_int_fast16_t_ (void const* value1, void const* value2)
{
    return (* (int_fast16_t*) value1 - * (int_fast16_t*) value2);
}

double
_fische__guess_frames_per_beat_ (uint_fast16_t* beat_gap_history)
{
    uint_fast16_t gap_history_sorted[30];

    memcpy (gap_history_sorted, beat_gap_history, 30 * sizeof (uint_fast16_t));
    qsort (gap_history_sorted, 30, sizeof (uint_fast16_t), _fische__compare_int_fast16_t_);

    uint_fast16_t guess = gap_history_sorted[14];

    double result = 0;
    int count = 0;

    uint_fast8_t i;
    for (i = 0; i < 30; ++ i) {
        if (abs (gap_history_sorted[i] - guess) <= 2) {
            result += gap_history_sorted[i];
            ++ count;
        }
    }

    return result / count;
}

double
_fische__get_audio_level_ (double* data, uint_fast32_t data_size)
{
    double E = 0;

    uint_fast32_t i;
    for (i = 0; i < data_size; ++ i) {
        E += fabs (* (data + i));
    }

    if (E <= 0) E = 1e-9;
    E /= data_size;

    return log10 (E) * 10;
}

struct fische__analyst*
fische__analyst_new (struct fische* parent) {

    struct fische__analyst* retval = malloc (sizeof (struct fische__analyst));
    retval->priv = malloc (sizeof (struct _fische__analyst_));

    struct _fische__analyst_* P = retval->priv;

    P->fische = parent;
    P->bghist_head = 0;
    P->intensity_moving_avg = 0;
    P->intensity_std_dev = 0;
    P->last_beat_frame = 0;
    P->moving_avg_03 = 0;
    P->moving_avg_30 = 0;
    P->state = _FISCHE__WAITING_;
    P->std_dev = 0;

    P->beat_gap_history = malloc (30 * sizeof (uint_fast16_t));
    memset (P->beat_gap_history, '\0', 30 * sizeof (uint_fast16_t));

    retval->frames_per_beat = 0;
    retval->relative_energy = 1;

    return retval;
}

void
fische__analyst_free (struct fische__analyst* self)
{
    if (!self)
        return;

    free (self->priv->beat_gap_history);
    free (self->priv);
    free (self);
}

int_fast8_t
fische__analyst_analyse (struct fische__analyst* self,
                         double* data,
                         uint_fast16_t size)
{
    if (!size)
        return -1;

    struct _fische__analyst_* P = self->priv;

    double dezibel = _fische__get_audio_level_ (data, size * 2);

    if (P->moving_avg_30 == 0)
        P->moving_avg_30 = dezibel;
    else
        P->moving_avg_30 = P->moving_avg_30 * 0.9667 + dezibel * 0.0333;

    P->std_dev = P->std_dev * 0.9667 + fabs (dezibel - P->moving_avg_30) * 0.0333;

    uint_fast32_t frameno = P->fische->frame_counter;
    if ( (frameno - P->last_beat_frame) > 90) {
        self->frames_per_beat = 0;
        memset (P->beat_gap_history, '\0', 30 * sizeof (uint_fast16_t));
        P->bghist_head = 0;
    }

    self->relative_energy = P->moving_avg_03 / P->moving_avg_30;

    double relative_intensity = 0;
    double new_frames_per_beat;

    switch (P->state) {
        case _FISCHE__WAITING_:
            // don't bother if intensity too low
            if (dezibel < P->moving_avg_30 + P->std_dev)
                break;

            // initialisation fallbacks
            if (P->std_dev == 0)
                relative_intensity = 1; // avoid div by 0
            else
                relative_intensity = (dezibel - P->moving_avg_30) / P->std_dev;

            if (P->intensity_moving_avg == 0)
                P->intensity_moving_avg = relative_intensity; // initial assignment
            else
                P->intensity_moving_avg = P->intensity_moving_avg * 0.95 + relative_intensity * 0.05;

            // update intensity standard deviation
            P->intensity_std_dev = P->intensity_std_dev * 0.95 + fabs (P->intensity_moving_avg - relative_intensity) * 0.05;

            // we DO have a beat
            P->state = _FISCHE__BEAT_;

            // update beat gap history
            P->beat_gap_history[P->bghist_head++] = frameno - P->last_beat_frame;
            if (P->bghist_head == 30)
                P->bghist_head = 0;

            // remember this as the last beat
            P->last_beat_frame = frameno;

            // reset the short-term moving average
            P->moving_avg_03 = dezibel;

            // try a guess at the tempo
            new_frames_per_beat = _fische__guess_frames_per_beat_ (P->beat_gap_history);
            if ( (self->frames_per_beat) && (self->frames_per_beat / new_frames_per_beat < 1.2) && (new_frames_per_beat / self->frames_per_beat < 1.2))
                self->frames_per_beat = (self->frames_per_beat * 2 + new_frames_per_beat) / 3;
            else
                self->frames_per_beat = new_frames_per_beat;

            // return based on relative beat intensity
            if (relative_intensity > P->intensity_moving_avg + 3 * P->intensity_std_dev)
                return 4;
            if (relative_intensity > P->intensity_moving_avg + 2 * P->intensity_std_dev)
                return 3;
            if (relative_intensity > P->intensity_moving_avg + 1 * P->intensity_std_dev)
                return 2;

            return 1;

        case _FISCHE__BEAT_:
        case _FISCHE__MAYBEWAITING_:
            // update short term moving average
            P->moving_avg_03 = P->moving_avg_03 * 0.6667 + dezibel * 0.3333;

            // needs to be low enough twice to exit BEAT state
            if (P->moving_avg_03 < P->moving_avg_30 + P->std_dev) {
                P->state = P->state == _FISCHE__MAYBEWAITING_ ? _FISCHE__WAITING_ : _FISCHE__MAYBEWAITING_;
                return 0;
            }
    }

    // report level too low
    if (dezibel < -45) return -1;
    return 0;
}
