#include "fische_internal.h"

#include <math.h>
#include <stdlib.h>

#ifdef DEBUG
#include <stdio.h>
#endif

struct fische__wavepainter*
fische__wavepainter_new (struct fische* parent) {

    struct fische__wavepainter* retval = malloc (sizeof (struct fische__wavepainter));
    retval->priv = malloc (sizeof (struct _fische__wavepainter_));
    struct _fische__wavepainter_* P = retval->priv;

    P->fische = parent;

    uint32_t full_alpha = 0xff << FISCHE_PRIVATE (P)->screenbuffer->priv->alpha_shift;

    P->width = parent->width;
    P->height = parent->height;
    P->angle = 0;
    P->center_x = P->width / 2;
    P->center_y = P->height / 2;
    P->color_1 = (rand() % 0xffffffff) | full_alpha;
    P->color_2 = (~P->color_1) | full_alpha;
    P->direction = 1;
    P->is_rotating = 0;
    P->n_shapes = 2;
    P->rotation_increment = 0;
    P->shape = 0;

    return retval;
}

void
fische__wavepainter_free (struct fische__wavepainter* self)
{
    if (!self)
        return;

    free (self->priv);
    free (self);
}

void
fische__wavepainter_paint (struct fische__wavepainter* self,
                           double* data,
                           uint_fast16_t size)
{
    if (!size)
        return;

    struct _fische__wavepainter_* P = self->priv;

    // rotation
    if (P->is_rotating) {
        P->angle += P->rotation_increment;
        if ( (P->angle > 2 * M_PI) || (P->angle < -2 * M_PI)) {
            P->angle = 0;
            P->is_rotating = 0;
        }
    }

    // only init fische scale once
    static double f_scale = 0;
    if (f_scale == 0)
        f_scale = P->fische->scale;

    // necessary parameters
    double dim = (P->height < P->width) ? P->height : P->width;
    dim *= f_scale;
    double factor = pow (10, P->fische->amplification / 10);
    double scale = 6 / dim / factor;

    // alpha saturation fix
    struct fische__screenbuffer* sbuf = FISCHE_PRIVATE (P)->screenbuffer;
    fische__screenbuffer_line (sbuf, 0, 0, P->width - 1, 0, 0);
    fische__screenbuffer_line (sbuf, P->width - 1, 0, P->width - 1, P->height - 1, 0);
    fische__screenbuffer_line (sbuf, P->width - 1, P->height - 1, 0, P->height - 1, 0);
    fische__screenbuffer_line (sbuf, 0, P->height - 1, 0, 0, 0);

    switch (P->shape) {

        case 0: {
            fische__point center;
            center.x = P->center_x;
            center.y = P->center_y;

            // base will be the middle of a line,
            // normally horizontal (angle = 0), but could be rotating
            fische__point base1;
            base1.x = center.x + (dim / 6) * sin (P->angle);
            base1.y = center.y + (dim / 6) * cos (P->angle);

            fische__point base2;
            base2.x = P->width / 2 - (dim / 6) * sin (P->angle);
            base2.y = P->height / 2 - (dim / 6) * cos (P->angle);

            // create vectors perpendicular to the line center->base
            fische__vector _nvec1 = base1;
            fische__vector_sub (&_nvec1, &center);
            fische__vector nvec1 = fische__vector_normal (&_nvec1);

            fische__vector _nvec2 = base2;
            fische__vector_sub (&_nvec2, &center);
            fische__vector nvec2 = fische__vector_normal (&_nvec2);

            // find the points where the line would exit the screen
            fische__point start1 = fische__vector_intersect_border (&base1, &nvec1, P->width, P->height, _FISCHE__VECTOR_LEFT_);
            fische__point end1 = fische__vector_intersect_border (&base1, &nvec1, P->width, P->height, _FISCHE__VECTOR_RIGHT_);

            fische__point start2 = fische__vector_intersect_border (&base2, &nvec2, P->width, P->height, _FISCHE__VECTOR_LEFT_);
            fische__point end2 = fische__vector_intersect_border (&base2, &nvec2, P->width, P->height, _FISCHE__VECTOR_RIGHT_);

            // determine the direction and length (i.e. vector)
            // of the increment between two sound samples
            fische__vector v1 = end1;
            fische__vector_sub (&v1, &start1);
            fische__vector_div (&v1, size);

            fische__vector v2 = end2;
            fische__vector_sub (&v2, &start2);
            fische__vector_div (&v2, size);

            // determine the normal vectors
            // for calculating the sound sample offset (amplitude)
            fische__vector _n1 = fische__vector_normal (&v1);
            fische__vector n1 = fische__vector_single (&_n1);
            fische__vector _n2 = fische__vector_normal (&v2);
            fische__vector n2 = fische__vector_single (&_n2);

            // draw both lines
            fische__point base_p1 = start1;
            fische__point base_p2 = start2;

            uint_fast16_t i;
            for (i = 0; i < size - 1; i ++) {
                fische__point pt11 = base_p1;
                fische__vector offset11 = n1;
                fische__vector_mul (&offset11, (* (data + 2 * i)));
                fische__vector_div (&offset11, scale);
                fische__vector_add (&pt11, &offset11);

                fische__point pt21 = base_p2;
                fische__vector offset21 = n2;
                fische__vector_mul (&offset21, (* (data + 1 + 2 * i)));
                fische__vector_div (&offset21, scale);
                fische__vector_add (&pt21, &offset21);

                fische__vector_add (&base_p1, &v1);
                fische__vector_add (&base_p2, &v2);

                fische__point pt12 = base_p1;
                fische__vector offset12 = n1;
                fische__vector_mul (&offset12, (* (data + 2 * (i + 1))));
                fische__vector_div (&offset12, scale);
                fische__vector_add (&pt12, &offset12);

                fische__point pt22 = base_p2;
                fische__vector offset22 = n2;
                fische__vector_mul (&offset22, (* (data + 1 + 2 * (i + 1))));
                fische__vector_div (&offset22, scale);
                fische__vector_add (&pt22, &offset22);

                fische__screenbuffer_line (sbuf, pt11.x, pt11.y, pt12.x, pt12.y, P->color_1);
                fische__screenbuffer_line (sbuf, pt21.x, pt21.y, pt22.x, pt22.y, P->color_2);
            }
            return;
        }

        // circular shape
        case 1: {
            double f = cos (M_PI / 3 + 2 * P->angle) + 0.5;
            double e = 1;

            uint_fast16_t i;
            for (i = 0; i < size - 1; i ++) {

                double incr = i;

                // calculate angles for this and the next sound sample
                double phi1 = M_PI * (0.25 + incr / size) + P->angle;
                double phi2 = phi1 + M_PI / size;

                // calculate the corresponding radius
                double r1 = dim / 4 + * (data + 2 * i) / scale;
                double r2 = dim / 4 + * (data + 2 * (i + 1)) / scale;

                uint_fast16_t x1 = floor ( (P->center_x + f * r1 * sin (phi1)) + 0.5);
                uint_fast16_t x2 = floor ( (P->center_x + f * r2 * sin (phi2)) + 0.5);
                uint_fast16_t y1 = floor ( (P->center_y + e * r1 * cos (phi1)) + 0.5);
                uint_fast16_t y2 = floor ( (P->center_y + e * r2 * cos (phi2)) + 0.5);

                fische__screenbuffer_line (sbuf, x1, y1, x2, y2, P->color_1);

                // the second line will be exactly on the
                // opposite side of the circle
                phi1 += M_PI;
                phi2 += M_PI;

                r1 = dim / 4 + * (data + 1 + 2 * i) / scale;
                r2 = dim / 4 + * (data + 1 + 2 * (i + 1)) / scale;

                x1 = floor ( (P->center_x + f * r1 * sin (phi1)) + 0.5);
                x2 = floor ( (P->center_x + f * r2 * sin (phi2)) + 0.5);
                y1 = floor ( (P->center_y + e * r1 * cos (phi1)) + 0.5);
                y2 = floor ( (P->center_y + e * r2 * cos (phi2)) + 0.5);

                fische__screenbuffer_line (sbuf, x1, y1, x2, y2, P->color_2);
            }
            return;
        }
    }

}

void
fische__wavepainter_beat (struct fische__wavepainter* self, double frames_per_beat)
{
    struct _fische__wavepainter_* P = self->priv;
    if (!P->is_rotating) {
        if (frames_per_beat != 0) {
            P->direction = 1 - 2 * (rand() % 2);
            P->rotation_increment = M_PI / frames_per_beat / 2 * P->direction;
            P->angle = 0;
            P->is_rotating = 1;
        }
    }

}

void
fische__wavepainter_change_color (struct fische__wavepainter* self,
                                  double frames_per_beat,
                                  double energy)
{
    struct _fische__wavepainter_* P = self->priv;

    uint32_t full_alpha = 0xff << FISCHE_PRIVATE (P)->screenbuffer->priv->alpha_shift;

    if (!frames_per_beat && !energy) {
        P->color_1 = (rand() % 0xffffffff) | full_alpha;
        P->color_2 = (~P->color_1) | full_alpha;
    }

    if (!frames_per_beat)
        return;

    double hue = frames_per_beat / 2;
    while (hue >= 6)
        hue -= 6;

    double sv = (energy > 1) ? 1 : pow (energy, 4);
    double x = sv * (1 - fabs ( (int_fast32_t) hue % 2 - 1));

    double r, g, b;

    switch ( (int_fast32_t) hue) {
        case 0:
            r = sv;
            g = x;
            b = 0;
            break;
        case 1:
            r = x;
            g = sv;
            b = 0;
            break;
        case 2:
            r = 0;
            g = sv;
            b = x;
            break;
        case 3:
            r = 0;
            g = x;
            b = sv;
            break;
        case 4:
            r = x;
            g = 0;
            b = sv;
            break;
        default:
        case 5:
            r = sv;
            g = 0;
            b = x;
    }

    uint32_t red = floor (r * 255 + 0.5);
    uint32_t green = floor (b * 255 + 0.5);
    uint32_t blue = floor (g * 255 + 0.5);

    P->color_1 = (blue << FISCHE_PRIVATE (P)->screenbuffer->priv->blue_shift)
                 + (green << FISCHE_PRIVATE (P)->screenbuffer->priv->green_shift)
                 + (red << FISCHE_PRIVATE (P)->screenbuffer->priv->red_shift)
                 + (0xff << FISCHE_PRIVATE (P)->screenbuffer->priv->alpha_shift);

    P->color_2 = (~P->color_1) | full_alpha;
}

void
fische__wavepainter_change_shape (struct fische__wavepainter* self)
{
    struct _fische__wavepainter_* P = self->priv;

    if (P->is_rotating) return;
    int_fast8_t n = P->shape;
    while (n == P->shape) n = rand() % P->n_shapes;
    P->shape = n;
}
