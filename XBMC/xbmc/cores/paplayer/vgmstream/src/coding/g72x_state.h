/*
 * streamtypes.h - widely used type definitions
 */


#ifndef _G72X_STATE_H
#define _G72X_STATE_H

struct g72x_state {
    long yl;    /* Locked or steady state step size multiplier. */
    short yu;   /* Unlocked or non-steady state step size multiplier. */
    short dms;  /* Short term energy estimate. */
    short dml;  /* Long term energy estimate. */
    short ap;   /* Linear weighting coefficient of 'yl' and 'yu'. */

    short a[2]; /* Coefficients of pole portion of prediction filter. */
    short b[6]; /* Coefficients of zero portion of prediction filter. */
    short pk[2];    /*
             * Signs of previous two samples of a partially
             * reconstructed signal.
             */
    short dq[6];    /*
             * Previous 6 samples of the quantized difference
             * signal represented in an internal floating point
             * format.
             */
    short sr[2];    /*
             * Previous 2 samples of the quantized difference
             * signal represented in an internal floating point
             * format.
             */
    char td;    /* delayed tone detect, new in 1988 version */
};

#endif
