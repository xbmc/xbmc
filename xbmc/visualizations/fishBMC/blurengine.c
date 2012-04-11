#include "fische_internal.h"

#include <pthread.h>
#include <unistd.h>

#ifdef DEBUG
#include <stdio.h>
#endif

void*
blur_worker (void* arg)
{
    struct _fische__blurworker_* params = arg;

    uint_fast16_t const width = params->width;
    uint_fast16_t const width_x2 = 2 * width;
    uint_fast16_t const y_start = params->y_start;
    uint_fast16_t const y_end = params->y_end;

    uint32_t source_component[4];

    uint_fast16_t const two_lines = 2 * width;
    uint_fast16_t const one_line = width;
    uint_fast16_t const two_columns	= 2;

    uint_fast16_t x, y;
    int_fast8_t vector_x, vector_y;

    while (!params->kill) {

        if (!params->work) {
            usleep (1);
            continue;
        }

        uint32_t* source = params->source;
        uint32_t* source_pixel;

        uint32_t* destination = params->destination;
        uint32_t* destination_pixel = destination + y_start * width;

        int8_t* vectors = (int8_t*) params->vectors;
        int8_t*	vector_pointer = vectors + y_start * width_x2;

        // vertical loop
        for (y = y_start; y < y_end; y ++) {
            // horizontal loop
            for (x = 0; x < width; x ++) {

                // read the motion vector (actually its opposite)
                vector_x = * (vector_pointer + 0);
                vector_y = * (vector_pointer + 1);

                // point to the pixel at [present + motion vector]
                source_pixel = source + (y + vector_y) * width + x + vector_x;

                // read the pixels at [source + (2,1)]   [source + (-2,1)]   [source + (0,-2)]
                // shift them right by 2 and remove the bits that overflow each byte
                source_component[0] = (* (source_pixel + one_line - two_columns) >> 2) & 0x3f3f3f3f;
                source_component[1] = (* (source_pixel + one_line + two_columns) >> 2) & 0x3f3f3f3f;
                source_component[2] = (* (source_pixel - two_lines) >> 2) & 0x3f3f3f3f;
                source_component[3] = (* (source_pixel) >> 2) & 0x3f3f3f3f;

                // add those four components and write to the destination
                // increment destination pointer
                * (destination_pixel ++) = source_component[0]
                                           + source_component[1]
                                           + source_component[2]
                                           + source_component[3];

                // increment vector source pointer
                vector_pointer += 2;
            }
        }

        // mark work as done
        params->work = 0;
    }

    return 0;
}

struct fische__blurengine*
fische__blurengine_new (struct fische* parent) {

    struct fische__blurengine* retval = malloc (sizeof (struct fische__blurengine));
    retval->priv = malloc (sizeof (struct _fische__blurengine_));
    struct _fische__blurengine_* P = retval->priv;

    P->fische = parent;
    P->width = parent->width;
    P->height = parent->height;
    P->threads = parent->used_cpus;
    P->sourcebuffer = FISCHE_PRIVATE(P)->screenbuffer->pixels;
    P->destinationbuffer = malloc (P->width * P->height * sizeof (uint32_t));

    uint_fast8_t i;
    for (i = 0; i < P->threads; ++ i) {
        P->worker[i].source = P->sourcebuffer;
        P->worker[i].destination = P->destinationbuffer;
        P->worker[i].vectors = 0;
        P->worker[i].width = P->width;
        P->worker[i].y_start = (i * P->height) / P->threads;
        P->worker[i].y_end = ( (i + 1) * P->height) / P->threads;
        P->worker[i].kill = 0;
        P->worker[i].work = 0;

        pthread_create (&P->worker[i].thread_id, NULL, blur_worker, &P->worker[i]);
    }

    return retval;
}

void
fische__blurengine_free (struct fische__blurengine* self)
{
    if (!self)
        return;

    struct _fische__blurengine_* P = self->priv;

    uint_fast8_t i;
    for (i = 0; i < P->threads; ++ i) {
        P->worker[i].kill = 1;
        pthread_join (P->worker[i].thread_id, NULL);
    }

    free (self->priv->destinationbuffer);
    free (self->priv);
    free (self);
}

void
fische__blurengine_blur (struct fische__blurengine* self, uint16_t* vectors)
{
    struct _fische__blurengine_* P = self->priv;
    uint_fast8_t i;
    for (i = 0; i < P->threads; ++ i) {
        P->worker[i].source = P->sourcebuffer;
        P->worker[i].destination = P->destinationbuffer;
        P->worker[i].vectors = vectors;
        P->worker[i].work = 1;
    }
}

void
fische__blurengine_swapbuffers (struct fische__blurengine* self)
{
    struct _fische__blurengine_* P = self->priv;

    // wait for all workers to finish
    uint_fast8_t work = 1;
    while (work) {

        work = 0;
        uint_fast8_t i;
        for (i = 0; i < P->threads; ++ i) {
            work += P->worker[i].work;
        }

        if (work)
            usleep (1);
    }

    uint32_t* t = P->destinationbuffer;
    P->destinationbuffer = P->sourcebuffer;
    P->sourcebuffer = t;
    FISCHE_PRIVATE(P)->screenbuffer->pixels = t;
}
