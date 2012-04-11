#include "fische_internal.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#endif

struct fische__audiobuffer*
fische__audiobuffer_new (struct fische* parent) {

    struct fische__audiobuffer* retval = malloc (sizeof (struct fische__audiobuffer));
    retval->priv = malloc (sizeof (struct _fische__audiobuffer_));
    struct _fische__audiobuffer_* P = retval->priv;

    P->fische = parent;
    P->buffer = 0;
    P->buffer_size = 0;
    P->format = parent->audio_format;
    P->is_locked = 0;
    P->puts = 0;
    P->gets = 0;
    P->last_get = 0;

    retval->front_sample_count = 0;
    retval->front_samples = 0;
    retval->back_sample_count = 0;
    retval->back_samples = 0;

    return retval;
}

void
fische__audiobuffer_free (struct fische__audiobuffer* self)
{
    if (!self)
        return;

    fische__audiobuffer_lock( self );

    free (self->priv->buffer);
    free (self->priv);
    free (self);
}

void
fische__audiobuffer_insert (struct fische__audiobuffer* self, const void* data, uint_fast32_t size)
{
    struct _fische__audiobuffer_* P = self->priv;

    if (P->buffer_size > 44100)
        return;

    uint_fast8_t width = 1;

    switch (P->format) {
        case FISCHE_AUDIOFORMAT_DOUBLE:
            width = 8;
            break;
        case FISCHE_AUDIOFORMAT_FLOAT:
        case FISCHE_AUDIOFORMAT_S32:
        case FISCHE_AUDIOFORMAT_U32:
            width = 4;
            break;
        case FISCHE_AUDIOFORMAT_S16:
        case FISCHE_AUDIOFORMAT_U16:
            width = 2;
    }

    uint_fast32_t old_bufsize = P->buffer_size;
    P->buffer_size += size / width;
    P->buffer = realloc (P->buffer, P->buffer_size * sizeof (double));

    uint_fast32_t i;
    for (i = 0; i < size / width; ++ i) {
        double* dest = (P->buffer + old_bufsize + i);

        switch (P->format) {
            case FISCHE_AUDIOFORMAT_FLOAT:
                *dest = * ( (float*) data + i);
                break;
            case FISCHE_AUDIOFORMAT_DOUBLE:
                *dest = * ( (double*) data + i);
                break;
            case FISCHE_AUDIOFORMAT_S32:
                *dest = * ( (int32_t*) data + i);
                *dest /= INT32_MAX;
                break;
            case FISCHE_AUDIOFORMAT_U32:
                *dest = * ( (uint32_t*) data + i);
                *dest -= INT32_MAX;
                *dest /= INT32_MAX;
                break;
            case FISCHE_AUDIOFORMAT_S16:
                *dest = * ( (int16_t*) data + i);
                *dest /= INT16_MAX;
                break;
            case FISCHE_AUDIOFORMAT_U16:
                *dest = * ( (uint16_t*) data + i);
                *dest -= INT16_MAX;
                *dest /= INT16_MAX;
                break;
            case FISCHE_AUDIOFORMAT_S8:
                *dest = * ( (int8_t*) data + i);
                *dest /= INT8_MAX;
                break;
            case FISCHE_AUDIOFORMAT_U8:
                *dest = * ( (uint8_t*) data + i);
                *dest /= INT8_MAX;
                *dest /= INT8_MAX;
                break;
        }
    }

    ++ P->puts;
}

void
fische__audiobuffer_get (struct fische__audiobuffer* self)
{
    struct _fische__audiobuffer_* P = self->priv;

    if (P->buffer_size == 0)
        return;

    double* new_start = P->buffer + P->last_get * 2;
    P->buffer_size -= P->last_get * 2;

    // pop used data off front
    memmove (P->buffer, new_start, P->buffer_size * sizeof (double));
    P->buffer = realloc (P->buffer, P->buffer_size * sizeof (double));

    if (!P->puts)
        return;

    // fallback for first get
    if (P->gets == 0) {
        P->gets = 3;
        P->puts = 1;
    }

    // get/put ratio
    double d_ratio = ( (double) P->gets ) / P->puts;
    uint_fast8_t ratio = ceil( d_ratio );

    // how many samples to return
    uint_fast32_t n_samples = P->buffer_size / 2 / ratio;

    // set return data size and remember
    self->front_sample_count = n_samples;
    self->back_sample_count = n_samples;
    P->last_get = n_samples;

    // set export buffer
    self->front_samples = P->buffer;
    self->back_samples = P->buffer + P->buffer_size - n_samples * 2;

    // increment get counter
    ++ P->gets;
}

void
fische__audiobuffer_lock (struct fische__audiobuffer* self)
{
    #ifdef __GNUC__
    while ( !__sync_bool_compare_and_swap( &self->priv->is_locked, 0, 1 ) )
        usleep( 1 );
    #else
    while( self->priv->is_locked )
        usleep( 1 );
    self->priv->is_locked = 1;
    #endif
}

void
fische__audiobuffer_unlock (struct fische__audiobuffer* self)
{
    self->priv->is_locked = 0;
}
