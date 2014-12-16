#include "fische_internal.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

struct fische__screenbuffer*
fische__screenbuffer_new (struct fische* parent) {
    struct fische__screenbuffer* retval = malloc (sizeof (struct fische__screenbuffer));
    retval->priv = malloc (sizeof (struct _fische__screenbuffer_));
    struct _fische__screenbuffer_* P = retval->priv;

    P->fische = parent;
    P->width = parent->width;
    P->height = parent->height;
    P->is_locked = 0;

    retval->pixels = malloc (P->width * P->height * sizeof (uint32_t));
    memset (retval->pixels, '\0', P->width * P->height * sizeof (uint32_t));

    switch (parent->pixel_format) {

        case FISCHE_PIXELFORMAT_0xAABBGGRR:
            P->alpha_shift = 24;
            P->blue_shift = 16;
            P->green_shift = 8;
            P->red_shift = 0;
            break;

        case FISCHE_PIXELFORMAT_0xAARRGGBB:
            P->alpha_shift = 24;
            P->blue_shift = 0;
            P->green_shift = 8;
            P->red_shift = 16;
            break;

        case FISCHE_PIXELFORMAT_0xBBGGRRAA:
            P->alpha_shift = 0;
            P->blue_shift = 24;
            P->green_shift = 16;
            P->red_shift = 8;
            break;

        case FISCHE_PIXELFORMAT_0xRRGGBBAA:
            P->alpha_shift = 0;
            P->blue_shift = 8;
            P->green_shift = 16;
            P->red_shift = 24;
            break;
    }

    return retval;
}

void
fische__screenbuffer_free (struct fische__screenbuffer* self)
{
    if (!self)
        return;

    fische__screenbuffer_lock( self );

    free (self->priv);
    free (self->pixels);
    free (self);
}

void
fische__screenbuffer_lock (struct fische__screenbuffer* self)
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
fische__screenbuffer_unlock (struct fische__screenbuffer* self)
{
    self->priv->is_locked = 0;
}

void
fische__screenbuffer_line (struct fische__screenbuffer* self,
                           int_fast16_t x1,
                           int_fast16_t y1,
                           int_fast16_t x2,
                           int_fast16_t y2,
                           uint32_t color)
{
    struct _fische__screenbuffer_* P = self->priv;

    double diff_x     = (x1 > x2) ? (x1 - x2) : (x2 - x1);
    double diff_y     = (y1 > y2) ? (y1 - y2) : (y2 - y1);
    double dir_x      = (x2 < x1) ? -1 : 1;
    double dir_y      = (y2 < y1) ? -1 : 1;

    if (!diff_x && !diff_y)
        return;

    uint32_t half_alpha_mask;

    if (P->fische->line_style == FISCHE_LINESTYLE_ALPHA_SIMULATION)
        half_alpha_mask = (0x7f << P->red_shift)
                          + (0x7f << P->green_shift)
                          + (0x7f << P->blue_shift)
                          + (0x7f << P->alpha_shift);
    else
        half_alpha_mask = (0xff << P->red_shift)
                          + (0xff << P->green_shift)
                          + (0xff << P->blue_shift)
                          + (0x7f << P->alpha_shift);


    if (diff_x > diff_y) {

        int_fast16_t x;
        for (x = x1; x * dir_x <= x2 * dir_x; x += dir_x) {

            int_fast16_t y = (y1 + diff_y / diff_x * dir_y * abs (x - x1) + 0.5);

            if ( (x < 0) || (x >= P->width) || (y < 0) || (y >= P->height))
                continue;

            if (P->fische->line_style != FISCHE_LINESTYLE_THIN) {
                y ++;
                if (! (y < 0) && ! (y >= P->height))
                    * (self->pixels + y * P->width + x) = color & half_alpha_mask;

                y -= 2;
                if ( (y < 0) || (y >= P->height)) continue;
                * (self->pixels + y * P->width + x) = color & half_alpha_mask;

                y ++;
            }

            * (self->pixels + y * P->width + x) = color;
        }
    } else {

        int_fast16_t y;
        for (y = y1; y * dir_y <= y2 * dir_y; y += dir_y) {

            int_fast16_t x = (x1 + diff_x / diff_y * dir_x * abs (y - y1) + 0.5);

            if ( (x < 0) || (x >= P->width) || (y < 0) || (y >= P->height))
                continue;

            if (P->fische->line_style != FISCHE_LINESTYLE_THIN) {
                x ++;
                if (! (x < 0) && ! (x >= P->width))
                    * (self->pixels + y * P->width + x) = color & half_alpha_mask;

                x -= 2;
                if ( (x < 0) || (x >= P->width)) continue;
                * (self->pixels + y * P->width + x) = color & half_alpha_mask;

                x ++;
            }

            * (self->pixels + y * P->width + x) = color;
        }
    }
}
