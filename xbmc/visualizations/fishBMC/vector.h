#ifndef F_VECTOR_H
#define F_VECTOR_H

#include <math.h>
#include <stdint.h>

struct _fische__vector_ {
    double x;
    double y;
};

typedef struct _fische__vector_ fische__vector;
typedef struct _fische__vector_ fische__point;

enum {_FISCHE__VECTOR_LEFT_, _FISCHE__VECTOR_RIGHT_};

double          fische__vector_length (fische__vector* self);
fische__vector  fische__vector_normal (fische__vector* self);
fische__vector  fische__vector_single (fische__vector* self);
double          fische__vector_angle (fische__vector* self);
uint16_t        fische__vector_to_uint16 (fische__vector* self);
fische__vector  fische__vector_from_uint16 (uint16_t val);
void            fische__vector_add (fische__vector* self, fische__vector* other);
void            fische__vector_sub (fische__vector* self, fische__vector* other);
void            fische__vector_mul (fische__vector* self, double val);
void            fische__vector_div (fische__vector* self, double val);

fische__vector  fische__vector_intersect_border (fische__vector* self,
        fische__vector* normal_vec,
        uint_fast16_t width,
        uint_fast16_t height,
        int_fast8_t direction);

#endif
