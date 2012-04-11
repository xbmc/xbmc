#ifndef FISCHE_INTERNAL_H
#define FISCHE_INTERNAL_H

#include "fische.h"
#include "wavepainter.h"
#include "screenbuffer.h"
#include "analyst.h"
#include "vector.h"
#include "vectorfield.h"
#include "blurengine.h"
#include "audiobuffer.h"

#ifdef WIN32
#define rand_r(_seed) (_seed == _seed ? rand() : rand())
#endif

#define FISCHE_PRIVATE(P) ((struct _fische__internal_*) P->fische->priv)

uint_fast8_t _fische__cpu_detect_();


struct _fische__internal_ {
    struct fische__screenbuffer*    screenbuffer;
    struct fische__wavepainter*     wavepainter;
    struct fische__analyst*         analyst;
    struct fische__blurengine*      blurengine;
    struct fische__vectorfield*     vectorfield;
    struct fische__audiobuffer*     audiobuffer;
    double                          init_progress;
    uint_fast8_t                    init_cancel;
    uint_fast8_t                    audio_valid;
};

#endif
