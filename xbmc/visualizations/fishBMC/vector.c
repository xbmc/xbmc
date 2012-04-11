#include "vector.h"

inline double fische__vector_length (fische__vector* self)
{
    return sqrt (pow (self->x, 2) + pow (self->y, 2));
}

inline fische__vector fische__vector_normal (fische__vector* self)
{
    fische__vector r;
    r.x = self->y;
    r.y = - self->x;
    return r;
}

inline fische__vector fische__vector_single (fische__vector* self)
{
    double l = fische__vector_length (self);
    fische__vector r;
    r.x = self->x / l;
    r.y = self->y / l;
    return r;
}

inline double fische__vector_angle (fische__vector* self)
{
    fische__vector su = fische__vector_single (self);
    double a = acos (su.x);
    if (self->y > 0) return a;
    else return -a;
}

// conversion to 2x int8
inline uint16_t fische__vector_to_uint16 (fische__vector* self)
{
    if (self->x < -127) self->x = -127;
    if (self->x > 127) self->x = 127;
    if (self->y < -127) self->y = -127;
    if (self->y > 127) self->y = 127;

    int8_t ix = (self->x < 0) ? self->x - 0.5 : self->x + 0.5;
    int8_t iy = (self->y < 0) ? self->y - 0.5 : self->y + 0.5;

    uint16_t retval = (uint8_t) ix;
    retval |= ( (uint8_t) iy) << 8;
    return retval;
}

inline fische__vector fische__vector_from_uint16 (uint16_t val)
{
    int8_t ix = val & 0xff;
    int8_t iy = val >> 8;
    fische__vector r;
    r.x = ix;
    r.y = iy;
    return r;
}

inline void fische__vector_add (fische__vector* self, fische__vector* other)
{
    self->x += other->x;
    self->y += other->y;
}

inline void fische__vector_sub (fische__vector* self, fische__vector* other)
{
    self->x -= other->x;
    self->y -= other->y;
}

inline void fische__vector_mul (fische__vector* self, double val)
{
    self->x *= val;
    self->y *= val;
}

inline void fische__vector_div (fische__vector* self, double val)
{
    self->x /= val;
    self->y /= val;
}

fische__vector fische__vector_intersect_border (fische__vector* self,
        fische__vector* normal_vec,
        uint_fast16_t width,
        uint_fast16_t height,
        int_fast8_t direction)
{
    width--;
    height--;

    fische__vector nvec = *normal_vec;
    if (direction == _FISCHE__VECTOR_RIGHT_) {
        fische__vector_mul (&nvec, -1);
    }

    double t1, t2, t3, t4;

    if (nvec.x == 0) {
        t1 = 1e6;
        t2 = 1e6;
    } else {
        t1 = - self->x / nvec.x;
        t2 = (width - self->x) / nvec.x;
    }

    if (nvec.y == 0) {
        t3 = 1e6;
        t4 = 1e6;
    } else {
        t3 = -self->y / nvec.y;
        t4 = (height - self->y) / nvec.y;
    }

    t1 = (t1 < 0) ? 1e6 : t1;
    t2 = (t2 < 0) ? 1e6 : t2;
    t3 = (t3 < 0) ? 1e6 : t3;
    t4 = (t4 < 0) ? 1e6 : t4;

    double a = (t1 < t2) ? t1 : t2;
    double b = (t3 < t4) ? t3 : t4;

    double min_t = (a < b) ? a : b;

    int_fast16_t ret_x = self->x + nvec.x * min_t;
    while (ret_x < 0) ret_x ++;
    while ( (unsigned) ret_x > width) ret_x --;

    int_fast16_t ret_y = self->y + nvec.y * min_t;
    while (ret_y < 0) ret_y ++;
    while ( (unsigned) ret_y > height) ret_y --;

    fische__vector r;
    r.x = ret_x;
    r.y = ret_y;
    return r;
}
