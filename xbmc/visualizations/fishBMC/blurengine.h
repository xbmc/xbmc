#ifndef BLURENGINE_H
#define BLURENGINE_H

#include <stdint.h>
#include <pthread.h>

struct fische;

struct _fische__blurworker_;
struct _fische__blurengine_;
struct fische__blurengine;



struct fische__blurengine*  fische__blurengine_new (struct fische* parent);
void                        fische__blurengine_free (struct fische__blurengine* self);

void                        fische__blurengine_blur (struct fische__blurengine* self, uint16_t* vectors);
void                        fische__blurengine_swapbuffers (struct fische__blurengine* self);



struct _fische__blurworker_ {
    pthread_t       thread_id;
    uint32_t*       source;
    uint32_t*       destination;
    uint_fast16_t   width;
    uint_fast16_t   y_start;
    uint_fast16_t   y_end;
    uint16_t*       vectors;
    uint_fast8_t    work;
    uint_fast8_t    kill;
};

struct _fische__blurengine_ {
    int_fast16_t        width;
    int_fast16_t        height;
    uint_fast8_t        threads;
    uint32_t*           sourcebuffer;
    uint32_t*           destinationbuffer;

    struct _fische__blurworker_ worker[8];

    struct fische*    fische;
};

struct fische__blurengine {
    struct _fische__blurengine_* priv;
};

#endif
