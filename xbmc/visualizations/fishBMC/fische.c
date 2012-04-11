#include "fische_internal.h"

#include <string.h>
#include <pthread.h>
#include <unistd.h>

#ifdef DEBUG
#include <stdio.h>
#endif

void*
create_vectors (void* arg)
{
    struct fische* F = arg;
    struct _fische__internal_ * P = F->priv;
    P->vectorfield = fische__vectorfield_new (F,
                     &P->init_progress,
                     &P->init_cancel);
    return 0;
}

void*
indicate_busy (void* arg)
{
    struct fische* F = arg;
    struct _fische__internal_ * P = F->priv;
    struct fische__screenbuffer* sbuf = P->screenbuffer;

    fische__point center;
    center.x = sbuf->priv->width / 2;
    center.y = sbuf->priv->height / 2;
    double dim = (center.x > center.y) ? center.y / 2 : center.x / 2;

    double last = -1;

    while ( (P->init_progress < 1) && (!P->init_cancel)) {

        if ( (P->init_progress < 0) || (P->init_progress == last)) {
            usleep (10000);
            continue;
        }

        last = P->init_progress;
        double angle = P->init_progress * -2 * 3.1415 + 3.0415;

        fische__vector c1;
        c1.x = sin (angle) * dim;
        c1.y = cos (angle) * dim;

        fische__vector c2;
        c2.x = sin (angle + 0.1) * dim;
        c2.y = cos (angle + 0.1) * dim;

        fische__vector e1 = fische__vector_single (&c1);
        fische__vector_mul (&e1, dim / 2);
        fische__vector e2 = fische__vector_single (&c2);
        fische__vector_mul (&e2, dim / 2);

        fische__vector c3 = c2;
        fische__vector_sub (&c3, &e2);
        fische__vector c4 = c1;
        fische__vector_sub (&c4, &e1);

        fische__vector_mul (&c1, F->scale);
        fische__vector_mul (&c2, F->scale);
        fische__vector_mul (&c3, F->scale);
        fische__vector_mul (&c4, F->scale);

        fische__vector_add (&c1, &center);
        fische__vector_add (&c2, &center);
        fische__vector_add (&c3, &center);
        fische__vector_add (&c4, &center);

        fische__screenbuffer_lock (sbuf);
        fische__screenbuffer_line (sbuf, c1.x, c1.y, c2.x, c2.y, 0xffffffff);
        fische__screenbuffer_line (sbuf, c2.x, c2.y, c3.x, c3.y, 0xffffffff);
        fische__screenbuffer_line (sbuf, c3.x, c3.y, c4.x, c4.y, 0xffffffff);
        fische__screenbuffer_line (sbuf, c4.x, c4.y, c1.x, c1.y, 0xffffffff);
        fische__screenbuffer_unlock (sbuf);
    }

    return 0;
}

struct fische *
fische_new() {
    struct fische* retval = malloc (sizeof (struct fische));

    retval->used_cpus = _fische__cpu_detect_();
    if (retval->used_cpus > 8)
        retval->used_cpus = 8;

    retval->frame_counter = 0;
    retval->audio_format = FISCHE_AUDIOFORMAT_FLOAT;
    retval->pixel_format = FISCHE_PIXELFORMAT_0xAABBGGRR;
    retval->width = 512;
    retval->height = 256;
    retval->read_vectors = 0;
    retval->write_vectors = 0;
    retval->on_beat = 0;
    retval->nervous_mode = 0;
    retval->blur_mode = FISCHE_BLUR_SLICK;
    retval->line_style = FISCHE_LINESTYLE_ALPHA_SIMULATION;
    retval->scale = 1;
    retval->amplification = 0;
    retval->priv = 0;
    retval->error_text = "no error";

    return retval;
}

int
fische_start (struct fische* handle)
{
    // plausibility checks
    if ( (handle->used_cpus > 8) || (handle->used_cpus < 1)) {
        handle->error_text = "CPU count out of range (1 <= used_cpus <= 8)";
        return 1;
    }

    if (handle->audio_format >= _FISCHE__AUDIOFORMAT_LAST_) {
        handle->error_text = "audio format invalid";
        return 1;
    }

    if (handle->line_style >= _FISCHE__LINESTYLE_LAST_) {
        handle->error_text = "line style invalid";
        return 1;
    }

    if (handle->frame_counter != 0) {
        handle->error_text = "frame counter garbled";
        return 1;
    }

    if ( (handle->amplification < -10) || (handle->amplification > 10)) {
        handle->error_text = "amplification value out of range (-10 <= amplification <= 10)";
        return 1;
    }

    if ( (handle->height < 16) || (handle->height > 2048)) {
        handle->error_text = "height value out of range (16 <= height <= 2048)";
        return 1;
    }

    if ( (handle->width < 16) || (handle->width > 2048)) {
        handle->error_text = "width value out of range (16 <= width <= 2048)";
        return 1;
    }

    if (handle->width % 4 != 0) {
        handle->error_text = "width value invalid (must be a multiple of four)";
        return 1;
    }

    if (handle->pixel_format >= _FISCHE__PIXELFORMAT_LAST_) {
        handle->error_text = "pixel format invalid";
        return 1;
    }

    if ( (handle->scale < 0.5) || (handle->scale > 2)) {
        handle->error_text = "scale value out of range (0.5 <= scale <= 2.0)";
        return 1;
    }

    if (handle->blur_mode >= _FISCHE__BLUR_LAST_) {
        handle->error_text = "blur option invalid";
        return 1;
    }

    // initialize private struct
    handle->priv = malloc (sizeof (struct _fische__internal_));
    memset (handle->priv, '\0', sizeof (struct _fische__internal_));
    struct _fische__internal_* P = handle->priv;

    P->init_progress = -1;

    P->analyst = fische__analyst_new (handle);
    P->screenbuffer = fische__screenbuffer_new (handle);
    P->wavepainter = fische__wavepainter_new (handle);
    P->blurengine = fische__blurengine_new (handle);
    P->audiobuffer = fische__audiobuffer_new (handle);

    // start vector creation and busy indicator threads
    pthread_t vector_thread;
    pthread_create (&vector_thread, NULL, create_vectors, handle);
    pthread_detach (vector_thread);

    pthread_t busy_thread;
    pthread_create (&busy_thread, NULL, indicate_busy, handle);
    pthread_detach (busy_thread);

    return 0;
}

uint32_t*
fische_render (struct fische* handle)
{
    struct _fische__internal_* P = handle->priv;

    // only if init completed
    if (P->init_progress >= 1) {

        // analyse sound data
        fische__audiobuffer_lock (P->audiobuffer);
        fische__audiobuffer_get (P->audiobuffer);
        int_fast8_t analysis = fische__analyst_analyse (P->analyst, P->audiobuffer->back_samples, P->audiobuffer->back_sample_count);

        // act accordingly
        if (handle->nervous_mode) {
            if (analysis >= 2)
                fische__wavepainter_change_shape (P->wavepainter);
            if (analysis >= 1)
                fische__vectorfield_change (P->vectorfield);
        } else {
            if (analysis >= 1)
                fische__wavepainter_change_shape (P->wavepainter);
            if (analysis >= 2)
                fische__vectorfield_change (P->vectorfield);
        }

        if (analysis >= 3) {
            fische__wavepainter_beat (P->wavepainter, P->analyst->frames_per_beat);
        }
        if (analysis >= 4) {
            if (handle->on_beat)
                handle->on_beat (P->analyst->frames_per_beat);
        }

        P->audio_valid = analysis >= 0 ? 1 : 0;

        fische__wavepainter_change_color (P->wavepainter, P->analyst->frames_per_beat, P->analyst->relative_energy);


        // wait for blurring to be finished
        // and swap buffers
        fische__screenbuffer_lock (P->screenbuffer);
        fische__blurengine_swapbuffers (P->blurengine);
        fische__screenbuffer_unlock (P->screenbuffer);

        // draw waves
        if (P->audio_valid)
            fische__wavepainter_paint (P->wavepainter, P->audiobuffer->front_samples, P->audiobuffer->front_sample_count);

        // start blurring for the next frame
        fische__blurengine_blur (P->blurengine, P->vectorfield->field);

        fische__audiobuffer_unlock (P->audiobuffer);
    }

    handle->frame_counter ++;

    return P->screenbuffer->pixels;
}

void
fische_free (struct fische* handle)
{
    if (!handle)
        return;

    struct _fische__internal_* P = handle->priv;

    if (handle->priv) {
        // tell init threads to quit
        P->init_cancel = 1;

        // wait for init threads to quit
        while (P->init_progress < 1)
            usleep (10);

        fische__audiobuffer_free (P->audiobuffer);
        fische__blurengine_free (P->blurengine);
        fische__vectorfield_free (P->vectorfield);
        fische__wavepainter_free (P->wavepainter);
        fische__screenbuffer_free (P->screenbuffer);
        fische__analyst_free (P->analyst);

        free (handle->priv);
    }

    free (handle);
}

void
fische_audiodata (struct fische* handle, const void* data, size_t data_size)
{
    struct _fische__internal_* P = handle->priv;

    if (NULL == P->audiobuffer)
        return;

    fische__audiobuffer_lock (P->audiobuffer);
    fische__audiobuffer_insert (P->audiobuffer, data, data_size);
    fische__audiobuffer_unlock (P->audiobuffer);
}
