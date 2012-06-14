#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

#include <stdint.h>

struct fische;
struct _fische__audiobuffer_;
struct fische__audiobuffer;



struct fische__audiobuffer* fische__audiobuffer_new (struct fische* parent);
void                        fische__audiobuffer_free (struct fische__audiobuffer* self);

void                        fische__audiobuffer_insert (struct fische__audiobuffer* self, const void* data, uint_fast32_t size);
void                        fische__audiobuffer_lock (struct fische__audiobuffer* self);
void                        fische__audiobuffer_unlock (struct fische__audiobuffer* self);
void                        fische__audiobuffer_get (struct fische__audiobuffer* self);



struct _fische__audiobuffer_ {
    double*         buffer;
    uint_fast32_t   buffer_size;
    uint_fast8_t    format;
    uint_fast8_t    is_locked;
    uint_fast32_t   puts;
    uint_fast32_t   gets;
    uint_fast32_t   last_get;

    struct fische*    fische;
};

struct fische__audiobuffer {
    double* front_samples;
    uint_fast16_t front_sample_count;
    double* back_samples;
    uint_fast16_t back_sample_count;

    struct _fische__audiobuffer_* priv;
};

#endif
