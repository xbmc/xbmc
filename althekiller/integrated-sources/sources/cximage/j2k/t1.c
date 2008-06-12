// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif
/*
 * Copyright (c) 2001-2002, David Janssens
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "t1.h"
#include "j2k.h"
#include "mqc.h"
#include "int.h"
#include "mct.h"
#include "dwt.h"
#include "int.h"
#include "fix.h"
#include <stdio.h>
#include <math.h>

#define T1_MAXCBLKW 64
#define T1_MAXCBLKH 64

#define T1_SIG_NE 0x0001
#define T1_SIG_SE 0x0002
#define T1_SIG_SW 0x0004
#define T1_SIG_NW 0x0008
#define T1_SIG_N 0x0010
#define T1_SIG_E 0x0020
#define T1_SIG_S 0x0040
#define T1_SIG_W 0x0080
#define T1_SIG_OTH (T1_SIG_N|T1_SIG_NE|T1_SIG_E|T1_SIG_SE|T1_SIG_S|T1_SIG_SW|T1_SIG_W|T1_SIG_NW)
#define T1_SIG_PRIM (T1_SIG_N|T1_SIG_E|T1_SIG_S|T1_SIG_W)

#define T1_SGN_N 0x0100
#define T1_SGN_E 0x0200
#define T1_SGN_S 0x0400
#define T1_SGN_W 0x0800
#define T1_SGN (T1_SGN_N|T1_SGN_E|T1_SGN_S|T1_SGN_W)

#define T1_SIG 0x1000
#define T1_REFINE 0x2000
#define T1_VISIT 0x4000

#define T1_NUMCTXS_AGG 1
#define T1_NUMCTXS_ZC 9
#define T1_NUMCTXS_MAG 3
#define T1_NUMCTXS_SC 5
#define T1_NUMCTXS_UNI 1

#define T1_CTXNO_AGG 0
#define T1_CTXNO_ZC (T1_CTXNO_AGG+T1_NUMCTXS_AGG)
#define T1_CTXNO_MAG (T1_CTXNO_ZC+T1_NUMCTXS_ZC)
#define T1_CTXNO_SC (T1_CTXNO_MAG+T1_NUMCTXS_MAG)
#define T1_CTXNO_UNI (T1_CTXNO_SC+T1_NUMCTXS_SC)
#define T1_NUMCTXS (T1_CTXNO_UNI+T1_NUMCTXS_UNI)

#define T1_NMSEDEC_BITS 7
#define T1_NMSEDEC_FRACBITS (T1_NMSEDEC_BITS-1)

int t1_lut_ctxno_zc[1024];
int t1_lut_ctxno_sc[256];
int t1_lut_ctxno_mag[4096];
int t1_lut_spb[256];
int t1_lut_nmsedec_sig[1<<T1_NMSEDEC_BITS];
int t1_lut_nmsedec_sig0[1<<T1_NMSEDEC_BITS];
int t1_lut_nmsedec_ref[1<<T1_NMSEDEC_BITS];
int t1_lut_nmsedec_ref0[1<<T1_NMSEDEC_BITS];

int t1_data[T1_MAXCBLKH][T1_MAXCBLKH];
int t1_flags[T1_MAXCBLKH+2][T1_MAXCBLKH+2];

int t1_getctxno_zc(int f, int orient) {
    return t1_lut_ctxno_zc[(orient<<8)|(f&T1_SIG_OTH)];
}

int t1_getctxno_sc(int f) {
    return t1_lut_ctxno_sc[(f&(T1_SIG_PRIM|T1_SGN))>>4];
}

int t1_getctxno_mag(int f) {
    return t1_lut_ctxno_mag[(f&T1_SIG_OTH)|(((f&T1_REFINE)!=0)<<11)];
}

int t1_getspb(int f) {
    return t1_lut_spb[(f&(T1_SIG_PRIM|T1_SGN))>>4];
}

int t1_getnmsedec_sig(int x, int bitpos) {
    if (bitpos>T1_NMSEDEC_FRACBITS) {
        return t1_lut_nmsedec_sig[(x>>(bitpos-T1_NMSEDEC_FRACBITS))&((1<<T1_NMSEDEC_BITS)-1)];
    } else {
        return t1_lut_nmsedec_sig0[x&((1<<T1_NMSEDEC_BITS)-1)];
    }
}

int t1_getnmsedec_ref(int x, int bitpos) {
    if (bitpos>T1_NMSEDEC_FRACBITS) {
        return t1_lut_nmsedec_ref[(x>>(bitpos-T1_NMSEDEC_FRACBITS))&((1<<T1_NMSEDEC_BITS)-1)];
    } else {
        return t1_lut_nmsedec_ref0[x&((1<<T1_NMSEDEC_BITS)-1)];
    }
}

void t1_updateflags(int *fp, int s) {
    int *np=fp-(T1_MAXCBLKW+2);
    int *sp=fp+(T1_MAXCBLKW+2);
    np[-1]|=T1_SIG_SE;
    np[1]|=T1_SIG_SW;
    sp[-1]|=T1_SIG_NE;
    sp[1]|=T1_SIG_NW;
    *np|=T1_SIG_S;
    *sp|=T1_SIG_N;
    fp[-1]|=T1_SIG_E;
    fp[1]|=T1_SIG_W;
    if (s) {
        *np|=T1_SGN_S;
        *sp|=T1_SGN_N;
        fp[-1]|=T1_SGN_E;
        fp[1]|=T1_SGN_W;
    }
}

void t1_enc_sigpass_step(int *fp, int *dp, int orient, int bpno, int one, int *nmsedec) {
    int v;
    if ((*fp&T1_SIG_OTH) && !(*fp&(T1_SIG|T1_VISIT))) {
        mqc_setcurctx(t1_getctxno_zc(*fp, orient));
        v=int_abs(*dp)&one?1:0;
        mqc_encode(v);
        if (v) {
            *nmsedec+=t1_getnmsedec_sig(int_abs(*dp), bpno+T1_NMSEDEC_FRACBITS);
            mqc_setcurctx(t1_getctxno_sc(*fp));
            v=*dp<0?1:0;
            mqc_encode(v^t1_getspb(*fp));
            t1_updateflags(fp, v);
            *fp|=T1_SIG;
        }
        *fp|=T1_VISIT;
    }
}

void t1_dec_sigpass_step(int *fp, int *dp, int orient, int oneplushalf) {
    int v;
    if ((*fp&T1_SIG_OTH) && !(*fp&(T1_SIG|T1_VISIT))) {
        mqc_setcurctx(t1_getctxno_zc(*fp, orient));
        if (mqc_decode()) {
            mqc_setcurctx(t1_getctxno_sc(*fp));
            v=mqc_decode()^t1_getspb(*fp);
            *dp=v?-oneplushalf:oneplushalf;
            t1_updateflags(fp, v);
            *fp|=T1_SIG;
        }
        *fp|=T1_VISIT;
    }
}

void t1_enc_sigpass(int w, int h, int bpno, int orient, int *nmsedec) {
    int i, j, k, one;
    *nmsedec=0;
    one=1<<(bpno+T1_NMSEDEC_FRACBITS);
    for (k=0; k<h; k+=4) {
        for (i=0; i<w; i++) {
            for (j=k; j<k+4 && j<h; j++) {
                t1_enc_sigpass_step(&t1_flags[1+j][1+i], &t1_data[j][i], orient, bpno, one, nmsedec);
            }
        }
    }
}

void t1_dec_sigpass(int w, int h, int bpno, int orient) {
    int i, j, k, one, half, oneplushalf;
    one=1<<bpno;
    half=one>>1;
    oneplushalf=one|half;
    for (k=0; k<h; k+=4) {
        for (i=0; i<w; i++) {
            for (j=k; j<k+4 && j<h; j++) {
                t1_dec_sigpass_step(&t1_flags[1+j][1+i], &t1_data[j][i], orient, oneplushalf);
            }
        }
    }
}

void t1_enc_refpass_step(int *fp, int *dp, int bpno, int one, int *nmsedec) {
    int v;
    if ((*fp&(T1_SIG|T1_VISIT))==T1_SIG) {
        *nmsedec+=t1_getnmsedec_ref(int_abs(*dp), bpno+T1_NMSEDEC_FRACBITS);
        mqc_setcurctx(t1_getctxno_mag(*fp));
        v=int_abs(*dp)&one?1:0;
        mqc_encode(v);
        *fp|=T1_REFINE;
    }
}

void t1_dec_refpass_step(int *fp, int *dp, int poshalf, int neghalf) {
    int v, t;
    if ((*fp&(T1_SIG|T1_VISIT))==T1_SIG) {
        mqc_setcurctx(t1_getctxno_mag(*fp));
        v=mqc_decode();
        t=v?poshalf:neghalf;
        *dp+=*dp<0?-t:t;
        *fp|=T1_REFINE;
    }
}

void t1_enc_refpass(int w, int h, int bpno, int *nmsedec) {
    int i, j, k, one;
    *nmsedec=0;
    one=1<<(bpno+T1_NMSEDEC_FRACBITS);
    for (k=0; k<h; k+=4) {
        for (i=0; i<w; i++) {
            for (j=k; j<k+4 && j<h; j++) {
                t1_enc_refpass_step(&t1_flags[1+j][1+i], &t1_data[j][i], bpno, one, nmsedec);
            }
        }
    }
}

void t1_dec_refpass(int w, int h, int bpno) {
    int i, j, k, one, poshalf, neghalf;
    one=1<<bpno;
    poshalf=one>>1;
    neghalf=bpno>0?-poshalf:-1;
    for (k=0; k<h; k+=4) {
        for (i=0; i<w; i++) {
            for (j=k; j<k+4 && j<h; j++) {
                t1_dec_refpass_step(&t1_flags[1+j][1+i], &t1_data[j][i], poshalf, neghalf);
            }
        }
    }
}

void t1_enc_clnpass_step(int *fp, int *dp, int orient, int bpno, int one, int *nmsedec, int partial) {
    int v;
    if (partial) goto label_partial;
    if (!(*fp&(T1_SIG|T1_VISIT))) {
        mqc_setcurctx(t1_getctxno_zc(*fp, orient));
        v=int_abs(*dp)&one?1:0;
        mqc_encode(v);
        if (v) {
label_partial:
            *nmsedec+=t1_getnmsedec_sig(int_abs(*dp), bpno+T1_NMSEDEC_FRACBITS);
            mqc_setcurctx(t1_getctxno_sc(*fp));
            v=*dp<0?1:0;
            mqc_encode(v^t1_getspb(*fp));
            t1_updateflags(fp, v);
            *fp|=T1_SIG;
        }
    }
    *fp&=~T1_VISIT;
}

void t1_dec_clnpass_step(int *fp, int *dp, int orient, int oneplushalf, int partial) {
    int v;
    if (partial) goto label_partial;
    if (!(*fp&(T1_SIG|T1_VISIT))) {
        mqc_setcurctx(t1_getctxno_zc(*fp, orient));
        if (mqc_decode()) {
label_partial:
            mqc_setcurctx(t1_getctxno_sc(*fp));
            v=mqc_decode()^t1_getspb(*fp);
            *dp=v?-oneplushalf:oneplushalf;
            t1_updateflags(fp, v);
            *fp|=T1_SIG;
        }
    }
    *fp&=~T1_VISIT;
}

void t1_enc_clnpass(int w, int h, int bpno, int orient, int *nmsedec) {
    int i, j, k, one, agg, runlen;
    *nmsedec=0;
    one=1<<(bpno+T1_NMSEDEC_FRACBITS);
    for (k=0; k<h; k+=4) {
        for (i=0; i<w; i++) {
            if (k+3<h) {
                agg=!(t1_flags[1+k][1+i]&(T1_SIG|T1_VISIT|T1_SIG_OTH) ||
                        t1_flags[1+k+1][1+i]&(T1_SIG|T1_VISIT|T1_SIG_OTH) ||
                        t1_flags[1+k+2][1+i]&(T1_SIG|T1_VISIT|T1_SIG_OTH) ||
                        t1_flags[1+k+3][1+i]&(T1_SIG|T1_VISIT|T1_SIG_OTH));
            } else {
                agg=0;
            }
            if (agg) {
                for (runlen=0; runlen<4; runlen++) {
                    if (int_abs(t1_data[k+runlen][i])&one) break;
                }
                mqc_setcurctx(T1_CTXNO_AGG);
                mqc_encode(runlen!=4);
                if (runlen==4) {
                    continue;
                }
                mqc_setcurctx(T1_CTXNO_UNI);
                mqc_encode(runlen>>1);
                mqc_encode(runlen&1);
            } else {
                runlen=0;
            }
            for (j=k+runlen; j<k+4 && j<h; j++) {
                t1_enc_clnpass_step(&t1_flags[1+j][1+i], &t1_data[j][i], orient, bpno, one, nmsedec, agg && (j==k+runlen));
            }
        }
    }
}

void t1_dec_clnpass(int w, int h, int bpno, int orient, int segsym) {
    int i, j, k, one, half, oneplushalf, agg, runlen;
    one=1<<bpno;
    half=one>>1;
    oneplushalf=one|half;
    for (k=0; k<h; k+=4) {
        for (i=0; i<w; i++) {
            if (k+3<h) {
                agg=!(t1_flags[1+k][1+i]&(T1_SIG|T1_VISIT|T1_SIG_OTH) ||
                        t1_flags[1+k+1][1+i]&(T1_SIG|T1_VISIT|T1_SIG_OTH) ||
                        t1_flags[1+k+2][1+i]&(T1_SIG|T1_VISIT|T1_SIG_OTH) ||
                        t1_flags[1+k+3][1+i]&(T1_SIG|T1_VISIT|T1_SIG_OTH));
            } else {
                agg=0;
            }
            if (agg) {
                mqc_setcurctx(T1_CTXNO_AGG);
                if (!mqc_decode()) {
                    continue;
                }
                mqc_setcurctx(T1_CTXNO_UNI);
                runlen=mqc_decode();
                runlen=(runlen<<1)|mqc_decode();
            } else {
                runlen=0;
            }
            for (j=k+runlen; j<k+4 && j<h; j++) {
                t1_dec_clnpass_step(&t1_flags[1+j][1+i], &t1_data[j][i], orient, oneplushalf, agg && (j==k+runlen));
            }
        }
    }
    if (segsym) {
        int v=0;
        mqc_setcurctx(T1_CTXNO_UNI);
        v=mqc_decode();
        v=(v<<1)|mqc_decode();
        v=(v<<1)|mqc_decode();
        v=(v<<1)|mqc_decode();
        if (v!=0xa) {
            J2KWARNING("warning: bad segmentation symbol\n")
        }
    }
}

double t1_getwmsedec(int nmsedec, int compno, int level, int orient, int bpno, int qmfbid, double stepsize) {
    double w1, w2, wmsedec;
    if (qmfbid==1) {
        w1=mct_getnorm(compno);
        w2=dwt_getnorm(level, orient);
    } else if (qmfbid==0) {
        w1=mct_getnorm_real(compno);
        w2=dwt_getnorm_real(level, orient);
    }
    wmsedec=w1*w2*stepsize*(1<<bpno);
    wmsedec*=wmsedec*nmsedec/8192.0;
    return wmsedec;
}

void t1_encode_cblk(tcd_cblk_t *cblk, int orient, int compno, int level, int qmfbid, double stepsize) {
    int i, j;
    int w, h;
    int passno;
    int bpno, passtype;
    int max;
    int nmsedec;
    double cumwmsedec=0;

    w=cblk->x1-cblk->x0;
    h=cblk->y1-cblk->y0;

    max=0;
    for (j=0; j<h; j++) {
        for (i=0; i<w; i++) {
            max=int_max(max, int_abs(t1_data[j][i]));
        }
    }
    cblk->numbps=max?(int_floorlog2(max)+1)-T1_NMSEDEC_FRACBITS:0;

    for (i=0; i<sizeof(t1_flags)/sizeof(int); i++) ((int*)t1_flags)[i]=0;

    bpno=cblk->numbps-1;
    passtype=2;

    mqc_resetstates();
    mqc_setstate(T1_CTXNO_UNI, 0, 46);
    mqc_setstate(T1_CTXNO_AGG, 0, 3);
    mqc_setstate(T1_CTXNO_ZC, 0, 4);
    mqc_init_enc(cblk->data);

    for (passno=0; bpno>=0; passno++) {
        tcd_pass_t *pass=&cblk->passes[passno];
        switch (passtype) {
            case 0:
                t1_enc_sigpass(w, h, bpno, orient, &nmsedec);
                break;
            case 1:
                t1_enc_refpass(w, h, bpno, &nmsedec);
                break;
            case 2:
                t1_enc_clnpass(w, h, bpno, orient, &nmsedec);
                break;
        }
        cumwmsedec+=t1_getwmsedec(nmsedec, compno, level, orient, bpno, qmfbid, stepsize);
        pass->distortiondec=cumwmsedec;
        pass->rate=mqc_numbytes()+3; // FIXME
        if (++passtype==3) {
            passtype=0;
            bpno--;
        }
    }
    mqc_flush();

    cblk->totalpasses=passno;
}

void t1_decode_cblk(tcd_cblk_t *cblk, int orient, int roishift, int cblksty) {
    int i;
    int w, h;
    int bpno, passtype;
    int segno, passno;

    for (i=0; i<sizeof(t1_data)/sizeof(int); i++) ((int*)t1_data)[i]=0;
    for (i=0; i<sizeof(t1_flags)/sizeof(int); i++) ((int*)t1_flags)[i]=0;

    w=cblk->x1-cblk->x0;
    h=cblk->y1-cblk->y0;
    bpno=roishift+cblk->numbps-1;
    passtype=2;

    mqc_resetstates();
    mqc_setstate(T1_CTXNO_UNI, 0, 46);
    mqc_setstate(T1_CTXNO_AGG, 0, 3);
    mqc_setstate(T1_CTXNO_ZC, 0, 4);

    for (segno=0; segno<cblk->numsegs; segno++) {
        tcd_seg_t *seg=&cblk->segs[segno];
        //fprintf(stderr, "t1_decode_cblk: segno=%d, seg->len=%d, seg->numpasses=%d\n", segno, seg->len, seg->numpasses);
        mqc_init_dec(seg->data, seg->len);
        for (passno=0; passno<seg->numpasses; passno++) {
            switch (passtype) {
                case 0:
                    t1_dec_sigpass(w, h, bpno, orient);
                    break;
                case 1:
                    t1_dec_refpass(w, h, bpno);
                    break;
                case 2:
                    t1_dec_clnpass(w, h, bpno, orient, cblksty&J2K_CCP_CBLKSTY_SEGSYM);
                    break;
            }
            if (++passtype==3) {
                passtype=0;
                bpno--;
            }
        }
    }
}

void t1_encode_cblks(tcd_tile_t *tile, j2k_tcp_t *tcp) {
    int compno, resno, bandno, precno, cblkno;
    for (compno=0; compno<tile->numcomps; compno++) {
        tcd_tilecomp_t *tilec=&tile->comps[compno];
        for (resno=0; resno<tilec->numresolutions; resno++) {
            tcd_resolution_t *res=&tilec->resolutions[resno];
            for (bandno=0; bandno<res->numbands; bandno++) {
                tcd_band_t *band=&res->bands[bandno];
                for (precno=0; precno<res->pw*res->ph; precno++) {
                    tcd_precinct_t *prc=&band->precincts[precno];
                    for (cblkno=0; cblkno<prc->cw*prc->ch; cblkno++) {
                        int x, y, i, j, orient;
                        tcd_cblk_t *cblk=&prc->cblks[cblkno];
                        if (band->bandno==0) {
                            x=res->x0+cblk->x0;
                            y=res->y0+cblk->y0;
                        } else if (band->bandno==1) {
                            x=res->x1-(band->x1-band->x0)+cblk->x0;
                            y=res->y0+cblk->y0;
                        } else if (band->bandno==2) {
                            x=res->x0+cblk->x0;
                            y=res->y1-(band->y1-band->y0)+cblk->y0;
                        } else if (band->bandno==3) {
                            x=res->x1-(band->x1-band->x0)+cblk->x0;
                            y=res->y1-(band->y1-band->y0)+cblk->y0;
                        }
                        if (tcp->tccps[compno].qmfbid==1) {
                            for (j=0; j<cblk->y1-cblk->y0; j++) {
                                for (i=0; i<cblk->x1-cblk->x0; i++) {
                                    t1_data[j][i]=tilec->data[x+i+(y+j)*(tilec->x1-tilec->x0)]<<T1_NMSEDEC_FRACBITS;
                                }
                            }
                        } else if (tcp->tccps[compno].qmfbid==0) {
                            for (j=0; j<cblk->y1-cblk->y0; j++) {
                                for (i=0; i<cblk->x1-cblk->x0; i++) {
                                    t1_data[j][i]=fix_mul(tilec->data[x+i+(y+j)*(tilec->x1-tilec->x0)], 8192*8192/band->stepsize)>>(13-T1_NMSEDEC_FRACBITS);
                                }
                            }
                        }
                        orient=band->bandno; // FIXME
                        if (orient==2) orient=1;
                        else if (orient==1) orient=2;
                        t1_encode_cblk(cblk, orient, compno, tilec->numresolutions-1-resno, tcp->tccps[compno].qmfbid, band->stepsize);
                    }
                }
            }
        }
    }
}

void t1_decode_cblks(tcd_tile_t *tile, j2k_tcp_t *tcp) {
    int compno, resno, bandno, precno, cblkno;
    for (compno=0; compno<tile->numcomps; compno++) {
        tcd_tilecomp_t *tilec=&tile->comps[compno];
        for (resno=0; resno<tilec->numresolutions; resno++) {
            tcd_resolution_t *res=&tilec->resolutions[resno];
            for (bandno=0; bandno<res->numbands; bandno++) {
                tcd_band_t *band=&res->bands[bandno];
                for (precno=0; precno<res->pw*res->ph; precno++) {
                    tcd_precinct_t *prc=&band->precincts[precno];
                    for (cblkno=0; cblkno<prc->cw*prc->ch; cblkno++) {
                        int x, y, i, j, orient;
                        tcd_cblk_t *cblk=&prc->cblks[cblkno];
                        orient=band->bandno; // FIXME
                        if (orient==2) orient=1;
                        else if (orient==1) orient=2;
                        t1_decode_cblk(cblk, orient, tcp->tccps[compno].roishift, tcp->tccps[compno].cblksty);
                        if (band->bandno==0) {
                            x=cblk->x0-band->x0;
                            y=cblk->y0-band->y0;
                        } else if (band->bandno==1) {
                            tcd_resolution_t *pres=&tilec->resolutions[resno-1];
                            x=pres->x1-pres->x0+cblk->x0-band->x0;
                            y=cblk->y0-band->y0;
                        } else if (band->bandno==2) {
                            tcd_resolution_t *pres=&tilec->resolutions[resno-1];
                            x=cblk->x0-band->x0;
                            y=pres->y1-pres->y0+cblk->y0-band->y0;
                        } else if (band->bandno==3) {
                            tcd_resolution_t *pres=&tilec->resolutions[resno-1];
                            x=pres->x1-pres->x0+cblk->x0-band->x0;
                            y=pres->y1-pres->y0+cblk->y0-band->y0;
                        }
                        if (tcp->tccps[compno].roishift) {
                            int thresh, val, mag;
                            thresh=1<<tcp->tccps[compno].roishift;
                            for (j=0; j<cblk->y1-cblk->y0; j++) {
                                for (i=0; i<cblk->x1-cblk->x0; i++) {
                                    val=t1_data[j][i];
                                    mag=int_abs(val);
                                    if (mag>=thresh) {
                                        mag>>=tcp->tccps[compno].roishift;
                                        t1_data[j][i]=val<0?-mag:mag;
                                    }
                                }
                            }
                        }
                        if (tcp->tccps[compno].qmfbid==1) {
                            for (j=0; j<cblk->y1-cblk->y0; j++) {
                                for (i=0; i<cblk->x1-cblk->x0; i++) {
                                    tilec->data[x+i+(y+j)*(tilec->x1-tilec->x0)]=t1_data[j][i];
                                }
                            }
                        } else if (tcp->tccps[compno].qmfbid==0) {
                            for (j=0; j<cblk->y1-cblk->y0; j++) {
                                for (i=0; i<cblk->x1-cblk->x0; i++) {
                                    if (t1_data[j][i]==0) {
                                        tilec->data[x+i+(y+j)*(tilec->x1-tilec->x0)]=0;
                                    } else {
                                        tilec->data[x+i+(y+j)*(tilec->x1-tilec->x0)]=fix_mul(t1_data[j][i]<<13, band->stepsize);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

int t1_init_ctxno_zc(int f, int orient) {
    int h,v,d,n,t,hv;n=0;h=((f&T1_SIG_W)!=0)+((f&T1_SIG_E)!=0);v=((f&T1_SIG_N)!=0)+((f&T1_SIG_S)!=0);d=((f&T1_SIG_NW)!=0)+((f&T1_SIG_NE)!=0)+((f&T1_SIG_SE)!=0)+((f&T1_SIG_SW)!=0);switch(orient){case 2:t=h;h=v;v=t;case 0:case 1:if(!h){if(!v){if(!d){n=0;}else if(d==1){n=1;}else{n=2;}}else if(v==1){n=3;}else{n=4;}}else if(h==1){if(!v){if(!d){n=5;}else{n=6;}}else{n=7;}}else{n=8;}break;case 3:hv=h+v;if(!d){if(!hv){n=0;}else if(hv==1){n=1;}else{n=2;}}else if(d==1){if(!hv){n=3;}else if(hv==1){ n=4;}else{n=5;}}else if(d==2){if(!hv){n=6;}else{n=7;}}else{n=8;}break;}return T1_CTXNO_ZC+n;
}

int t1_init_ctxno_sc(int f) {
    int hc,vc,n;n=0;hc=int_min(((f&(T1_SIG_E|T1_SGN_E))==T1_SIG_E)+((f&(T1_SIG_W|T1_SGN_W))==T1_SIG_W),1)-int_min(((f&(T1_SIG_E|T1_SGN_E))==(T1_SIG_E|T1_SGN_E))+((f&(T1_SIG_W|T1_SGN_W))==(T1_SIG_W|T1_SGN_W)),1);vc=int_min(((f&(T1_SIG_N|T1_SGN_N))==T1_SIG_N)+((f&(T1_SIG_S|T1_SGN_S))==T1_SIG_S),1)-int_min(((f&(T1_SIG_N|T1_SGN_N))==(T1_SIG_N|T1_SGN_N))+((f&(T1_SIG_S|T1_SGN_S))==(T1_SIG_S|T1_SGN_S)),1);if(hc<0){hc=-hc;vc=-vc;}if(!hc){if(vc==-1){n=1;}else if(!vc){n=0;}else{n=1;}}else if(hc==1){if(vc==-1){n=2;}else if(!vc){n=3;}else{n=4;}}return T1_CTXNO_SC+n;
}

int t1_init_ctxno_mag(int f) {
    int n;if(!(f&T1_REFINE)){n=(f&(T1_SIG_OTH))?1:0;}else{n=2;}return T1_CTXNO_MAG+n;
}

int t1_init_spb(int f) {
    int hc,vc,n;hc=int_min(((f&(T1_SIG_E|T1_SGN_E))==T1_SIG_E)+((f&(T1_SIG_W|T1_SGN_W))==T1_SIG_W),1)-int_min(((f&(T1_SIG_E|T1_SGN_E))==(T1_SIG_E|T1_SGN_E))+((f&(T1_SIG_W|T1_SGN_W))==(T1_SIG_W|T1_SGN_W)),1);vc=int_min(((f&(T1_SIG_N|T1_SGN_N))==T1_SIG_N)+((f&(T1_SIG_S|T1_SGN_S))==T1_SIG_S),1)-int_min(((f&(T1_SIG_N|T1_SGN_N))==(T1_SIG_N|T1_SGN_N))+((f&(T1_SIG_S|T1_SGN_S))==(T1_SIG_S|T1_SGN_S)),1);if(!hc&&!vc){n=0;}else{n=(!(hc>0||(!hc&&vc>0)));}return n;
}

void t1_init_luts() {
    int i,j;double u,v,t;for(j=0;j<4;j++){for(i=0;i<256;++i){t1_lut_ctxno_zc[(j<<8)|i]=t1_init_ctxno_zc(i,j);}}for(i=0;i<256;i++){t1_lut_ctxno_sc[i]=t1_init_ctxno_sc(i<<4);}for(j=0;j<2;j++){for(i=0;i<2048;++i){t1_lut_ctxno_mag[(j<<11)+i]=t1_init_ctxno_mag((j?T1_REFINE:0)|i);}}for(i=0;i<256;++i){t1_lut_spb[i]=t1_init_spb(i<<4);}
    // FIXME FIXME FIXME
    //printf("nmsedec luts:\n");
    for (i=0; i<(1<<T1_NMSEDEC_BITS); i++) {
        t=i/pow(2,T1_NMSEDEC_FRACBITS);
        u=t;
        v=t-1.5;
        t1_lut_nmsedec_sig[i]=int_max(0,(int)(floor((u*u-v*v)*pow(2,T1_NMSEDEC_FRACBITS)+ 0.5)/pow(2,T1_NMSEDEC_FRACBITS)*8192.0));
        t1_lut_nmsedec_sig0[i]=int_max(0,(int)(floor((u*u)*pow(2,T1_NMSEDEC_FRACBITS)+0.5)/pow(2,T1_NMSEDEC_FRACBITS)*8192.0));
        u=t-1.0;
        if (i&(1<<(T1_NMSEDEC_BITS-1))) {
            v=t-1.5;
        } else {
            v=t-0.5;
        }
        t1_lut_nmsedec_ref[i]=int_max(0,(int)(floor((u*u-v*v)*pow(2,T1_NMSEDEC_FRACBITS)+0.5)/pow(2,T1_NMSEDEC_FRACBITS)*8192.0));
        t1_lut_nmsedec_ref0[i]=int_max(0,(int)(floor((u*u)*pow(2,T1_NMSEDEC_FRACBITS)+0.5)/pow(2,T1_NMSEDEC_FRACBITS)*8192.0));
        //printf("%d: %d %d %d %d\n", i, t1_lut_nmsedec_sig[i], t1_lut_nmsedec_sig0[i], t1_lut_nmsedec_ref[i], t1_lut_nmsedec_ref0[i]);
    }
}
