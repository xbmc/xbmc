#include "fische_internal.h"

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#define N_FIELDS 20

struct field_param {
    uint16_t*           data;
    uint_fast8_t        number;
    uint_fast16_t       start_y;
    uint_fast16_t       end_y;
    struct _fische__vectorfield_* vecfield;
};

unsigned int rand_seed;

inline void
_fische__vectorfield_randomize_ (fische__vector* vec)
{
    vec->x += rand_r (&rand_seed) % 3;
    vec->x -= 1;
    vec->y += rand_r (&rand_seed) % 3;
    vec->y -= 1;
}


inline void
_fische__vectorfield_validate_ (struct _fische__vectorfield_* P,
                                fische__vector* vec,
                                double x,
                                double y)
{
    while (x + vec->x < 2)
        vec->x += 1;
    while (x + vec->x > P->width - 3)
        vec->x -= 1;
    while (y + vec->y < 2)
        vec->y += 1;
    while (y + vec->y > P->height - 2)
        vec->y -= 1;
}

void*
_fische__fill_thread_ (void* arg)
{
    struct field_param* params = arg;
    uint16_t* field = params->data;
    int fieldno = params->number;
    struct _fische__vectorfield_* P = params->vecfield;

    uint_fast16_t y;
    for (y = params->start_y; y < params->end_y; y ++) {

        uint_fast16_t x;
        for (x = 0; x < P->width; x++) {

            uint16_t* vector = field + x + y * P->width;

            // distance and direction relative to center
            fische__vector rvec;
            rvec.x = x;
            rvec.x -= P->center_x;
            rvec.y = y;
            rvec.y -= P->center_y;

            fische__vector e = fische__vector_single (&rvec);
            fische__vector n = fische__vector_normal (&e);

            double r = fische__vector_length (&rvec) / P->dimension;

            // distance and direction relative to left co-center
            fische__vector rvec_left;
            rvec_left.x = (double)x - P->center_x + P->width / 3 * P->fische->scale;
            rvec_left.y = (double)y - P->center_y;

            fische__vector e_left = fische__vector_single (&rvec_left);
            fische__vector n_left = fische__vector_normal (&e_left);

            double r_left = fische__vector_length (&rvec_left) / P->dimension;

            // distance and direction relative to right co-center
            fische__vector rvec_right;
            rvec_right.x = (double)x - P->center_x - P->width / 3 * P->fische->scale;
            rvec_right.y = (double)y - P->center_y;

            fische__vector e_right = fische__vector_single (&rvec_right);
            fische__vector n_right = fische__vector_normal (&e_right);

            double r_right = fische__vector_length (&rvec_right) / P->dimension;

            double speed = P->dimension / 45;

            // correction factors ensure consistent average speeds with all field types
            // double const corr[] = {0.77, 0.92, 1.72, 2.06, 1.45, 1.45, 1.73, 1.18, 3.24, 2.76, 0.82, 1.21, 1.73, 3.55, 0.47, 0.66, 0.96, 0.97, 1.00, 1.00};
            double const corr[] =    {0.83, 0.83, 1.56, 1.56, 1.08, 3.54, 1.56, 1.00, 4.47, 2.77, 0.74, 1.01, 1.56, 3.12, 0.67, 0.67, 0.83, 2.43, 1.21, 0.77};

            fische__vector v;
            switch (fieldno) {

                case 0:
                    // linear vectors showing away from a horizontal mirror axis
                    v.x = 0;
                    v.y = (y < P->center_y) ? speed * corr[fieldno] : -speed * corr[fieldno];
                    break;

                case 1:
                    // linear vectors showing away from a vertical mirror axis
                    v.x = (x < P->center_x) ? speed * corr[fieldno] : -speed * corr[fieldno];
                    v.y = 0;
                    break;

                case 2:
                    // radial vectors showing away from the center
                    v = e;
                    fische__vector_mul (&v, -r * speed * corr[fieldno]);
                    break;

                case 3:
                    // tangential vectors (right)
                    v = n;
                    fische__vector_mul (&v, r * speed * corr[fieldno]);
                    break;

                case 4: {
                    // tangential-radial vectors (left)
                    fische__vector _v1 = n;
                    fische__vector_mul (&_v1, -r * speed * corr[fieldno]);
                    v = e;
                    fische__vector_mul (&v, -r * speed * corr[fieldno]);
                    fische__vector_add (&v, &_v1);
                    break;
                }

                case 5: {
                    // tree rings
                    double dv = cos (M_PI * 24 * r);
                    v = e;
                    fische__vector_mul (&v, speed * 0.33 * corr[fieldno] * dv);
                    break;
                }

                case 6: {
                    // hyperbolic vectors
                    v.x = e.y;
                    v.y = e.x;
                    fische__vector_mul (&v, -r * speed * corr[fieldno]);
                    break;
                }

                case 7:
                    // purely random
                    v.x = rand_r (&rand_seed) % (int_fast32_t) (2 * speed * corr[fieldno] + 1) - (speed * corr[fieldno]);
                    v.y = rand_r (&rand_seed) % (int_fast32_t) (2 * speed * corr[fieldno] + 1) - (speed * corr[fieldno]);
                    break;

                case 8: {
                    // sphere
                    double dv = cos (M_PI * r);
                    v = e;
                    fische__vector_mul (&v, -r * dv * speed * corr[fieldno]);
                    break;
                }

                case 9: {
                    // sine distortion
                    double dv = sin (M_PI * 8 * r);
                    v = e;
                    fische__vector_mul (&v, -r * dv * speed * corr[fieldno]);
                    break;
                }

                case 10: {
                    // black hole
                    fische__vector _v1 = n;
                    fische__vector_mul (&_v1, speed * corr[fieldno]);
                    v = e;
                    fische__vector_mul (&v, r * speed * corr[fieldno]);
                    fische__vector_add (&v, &_v1);
                    if (r * P->dimension < 10) {
                        v.x = 0;
                        v.y = 0;
                    }
                    break;
                }

                case 11: {
                    // circular waves
                    double dim = pow (11 * M_PI, 2);
                    double _r = r * dim;
                    v = e;
                    fische__vector_mul (&v, -speed * corr[fieldno] * sqrt (1.04 - pow (cos (sqrt (_r)), 2) + 0.25 * pow (sin (sqrt (_r)), 2)));
                    break;
                }

                case 12:
                    // spinning CD
                    v = n;
                    if (fabs (r - 0.25) < 0.15)
                        fische__vector_mul (&v, r * speed * corr[fieldno]);
                    else
                        fische__vector_mul (&v, -r * speed * corr[fieldno]);
                    break;

                case 13: {
                    // three spinning disks
                    double rt = 0.3;
                    if (r < rt * 1.2) {
                        v = n;
                        fische__vector_mul (&v, -r * speed * corr[fieldno]);
                    } else if (r_left < rt) {
                        v = n_left;
                        fische__vector_mul (&v, r_left * speed * corr[fieldno]);
                    } else if (r_right < rt) {
                        v = n_right;
                        fische__vector_mul (&v, r_right * speed * corr[fieldno]);
                    } else {
                        v.x = 0;
                        v.y = 0;
                    }
                    break;
                }

                case 14: {
                    // 3-centered fields - radial
                    fische__vector _v1 = e_left;
                    fische__vector_mul (&_v1, (2 - r_left) * speed * corr[fieldno]);
                    fische__vector _v2 = e_right;
                    fische__vector_mul (&_v2, (2 - r_right)  * speed * corr[fieldno]);
                    v = e;
                    fische__vector_mul (&v, (2 - r) * -speed * corr[fieldno]);
                    fische__vector_add (&v, &_v1);
                    fische__vector_add (&v, &_v2);
                    break;
                }

                case 15: {
                    // 3-centered fields - tangential
                    fische__vector _v1 = n_left;
                    fische__vector_mul (&_v1, (2 - r_left) * -speed * corr[fieldno]);
                    fische__vector _v2 = n_right;
                    fische__vector_mul (&_v2, (2 - r_right)  * -speed * corr[fieldno]);
                    v = n;
                    fische__vector_mul (&v, (2 - r) * speed * corr[fieldno]);
                    fische__vector_add (&v, &_v1);
                    fische__vector_add (&v, &_v2);
                    break;
                }

                case 16: {
                    // lenses effect
                    double _r = r *	8 * M_PI;
                    fische__vector _v1 = e;
                    fische__vector_mul (&_v1, sin (_r) * -speed * corr[fieldno]);
                    v = n;
                    fische__vector_mul (&v,  sin (8 * fische__vector_angle (&e)) * -speed * corr[fieldno]);
                    fische__vector_add (&v, &_v1);
                    break;
                }

                case 17: {
                    // lenses effect
                    double _r = r *	24 * M_PI;
                    fische__vector _v1 = e;
                    fische__vector_mul (&_v1, sin (_r) * -speed * 0.33 * corr[fieldno]);
                    v = n;
                    fische__vector_mul (&v,  sin (24 * fische__vector_angle (&e)) * -speed * 0.33 * corr[fieldno]);
                    fische__vector_add (&v, &_v1);
                    break;
                }

                case 18: {
                    // fan 1
                    v = e;
                    double angle = fische__vector_angle (&e);
                    fische__vector_mul (&v, -speed * corr[fieldno] * sin (8 * angle));
                    break;
                }

                case 19: {
                    // fan 1
                    v = e;
                    double angle = fische__vector_angle (&e);
                    fische__vector_mul (&v, -speed * corr[fieldno] * (1.1 + sin (8 * angle)));
                    break;
                }

                default:
                    // index too high. return nothing.
                    return 0;
            }

            if (P->fische->blur_mode == FISCHE_BLUR_FUZZY)
                _fische__vectorfield_randomize_ (&v);

            _fische__vectorfield_validate_ (P, &v, x, y);
            *vector = fische__vector_to_uint16 (&v);
        }
    }
    return 0;
}

void
_fische__fill_field_ (struct _fische__vectorfield_* P, uint_fast8_t fieldno)
{
    uint16_t* field = P->fields + fieldno * P->fieldsize / 2;

    // threads maximum is 8
    pthread_t vec_threads[8];
    struct field_param params[8];

    uint_fast8_t i;
    for (i = 0; i < P->threads; ++ i) {
        params[i].data = field;
        params[i].number = fieldno;
        params[i].start_y = (i * P->height) / P->threads;
        params[i].end_y = ( (i + 1) * P->height) / P->threads;
        params[i].vecfield = P;

        pthread_create (&vec_threads[i], NULL, _fische__fill_thread_, &params[i]);
    }

    for (i = 0; i < P->threads; ++ i) {
        pthread_join (vec_threads[i], NULL);
    }
}

struct fische__vectorfield*
fische__vectorfield_new (struct fische* parent,
                         double* progress,
                         uint_fast8_t* cancel) {

    struct fische__vectorfield* retval = malloc (sizeof (struct fische__vectorfield));
    retval->priv = malloc (sizeof (struct _fische__vectorfield_));
    struct _fische__vectorfield_* P = retval->priv;

    rand_seed = time (NULL);
    *progress = 0;

    P->fische = parent;
    P->width = parent->width;
    P->height = parent->height;
    P->center_x = P->width / 2;
    P->center_y = P->height / 2;
    P->dimension = P->width < P->height ? P->width * P->fische->scale : P->height * P->fische->scale;
    P->fieldsize = P->width * P->height * sizeof (int16_t);
    P->threads = parent->used_cpus;
    P->cancelled = 0;

    // if we have stored fields, load them
    if (parent->read_vectors) {
        size_t bytes = parent->read_vectors ( (void**) (&P->fields));
        if (bytes) {
            *progress = 1;
            P->n_fields = bytes / P->fieldsize;
            retval->field = P->fields;
            return retval;
        }
    }

    // if not, recalculate everything
    P->fields = malloc (N_FIELDS * P->fieldsize);
    P->n_fields = N_FIELDS;

    uint_fast8_t i;
    for (i = 0; i < N_FIELDS; ++i) {
        if (*cancel) {
            P->cancelled = 1;
            break;
        }
        _fische__fill_field_ (P, i);
        *progress = (i + 1);
        *progress /= N_FIELDS;
    }

    *progress = 1;

    retval->field = P->fields;

    return retval;
}

void
fische__vectorfield_free (struct fische__vectorfield* self)
{
    if (!self)
        return;

    struct _fische__vectorfield_* P = self->priv;

    if (!P->cancelled && P->fische->write_vectors) {
        P->fische->write_vectors (P->fields, P->n_fields * P->fieldsize);
    }

    free (self->priv->fields);
    free (self->priv);
    free (self);
}

void
fische__vectorfield_change (struct fische__vectorfield* self)
{
    struct _fische__vectorfield_* P = self->priv;

    uint16_t* n = self->field;
    while (n == self->field) {
        self->field = P->fields + rand() % P->n_fields * P->width * P->height;
    }
}
