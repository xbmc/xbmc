/******************************************************************
 * CopyPolicy: GNU Public License 2 applies
 * Copyright (C) 1998 Monty xiphmont@mit.edu
 *
 * FFT implementation from OggSquish, minus cosine transforms.
 * Only convenience functions exposed
 *
 ******************************************************************/

extern void fft_forward(int n, float *buf, float *trigcache, int *sp);
extern void fft_backward(int n, float *buf, float *trigcache, int *sp);
extern void fft_i(int n, float **trigcache, int **splitcache);
