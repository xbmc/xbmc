/*
    Copyright(C) 1996-1999 Takuya OOURA
    email: ooura@mmm.t.u-tokyo.ac.jp
    download: http://momonga.t.u-tokyo.ac.jp/~ooura/fft.html
    You may use, copy, modify this code for any purpose and
    without fee. You may distribute this ORIGINAL package.
*/
extern void cdft(int, int, float *, int *, float *);
extern void rdft(int, int, float *, int *, float *);
extern void ddct(int, int, float *, int *, float *);
extern void ddst(int, int, float *, int *, float *);
extern void dfct(int, float *, float *, int *, float *);
extern void dfst(int, float *, float *, int *, float *);
