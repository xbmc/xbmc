#ifndef VECTORFIELD_H
#define VECTORFIELD_H

#include <stdint.h>

struct fische;
struct _fische__vectorfield_;
struct fische__vectorfield;



struct fische__vectorfield* fische__vectorfield_new (struct fische* parent, double* progress, uint_fast8_t* cancel);
void                        fische__vectorfield_free (struct fische__vectorfield* self);

void                        fische__vectorfield_change (struct fische__vectorfield* self);



struct _fische__vectorfield_ {
    uint16_t*	        fields;
    uint_fast32_t       fieldsize;
    uint_fast16_t       width;
    uint_fast16_t       height;
    uint_fast16_t       dimension;
    uint_fast16_t       center_x;
    uint_fast16_t       center_y;
    uint_fast8_t        threads;
    uint_fast8_t        n_fields;
    uint_fast8_t        cancelled;

    struct fische*    fische;
};



struct fische__vectorfield {
    uint16_t* field;

    struct _fische__vectorfield_* priv;
};

#endif
