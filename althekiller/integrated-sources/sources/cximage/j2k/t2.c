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

#include "t2.h"
#include "tcd.h"
#include "bio.h"
#include "j2k.h"
#include "pi.h"
#include "tgt.h"
#include "int.h"
#include <memory.h> 
#include <stdlib.h> 
#include <stdio.h>
#include <setjmp.h>

extern jmp_buf j2k_error;

void t2_putcommacode(int n)
{
    while (--n>=0) {
        bio_write(1, 1);
    }
    bio_write(0, 1);
}

int t2_getcommacode() {
    int n;
    for (n=0; bio_read(1); n++) {}
    return n;
}

void t2_putnumpasses(int n)
{
    if (n==1) {
        bio_write(0, 1);
    } else if (n==2) {
        bio_write(2, 2);
    } else if (n<=5) {
        bio_write(0xc|(n-3), 4);
    } else if (n<=36) {
        bio_write(0x1e0|(n-6), 9);
    } else if (n<=164) {
        bio_write(0xff80|(n-37), 16);
    }
}

int t2_getnumpasses()
{
    int n;
    if (!bio_read(1)) return 1;
    if (!bio_read(1)) return 2;
    if ((n=bio_read(2))!=3) return 3+n;
    if ((n=bio_read(5))!=31) return 6+n;
    return 37+bio_read(7);
}

int t2_encode_packet(tcd_tile_t *tile, j2k_tcp_t *tcp, int compno, int resno, int precno, int layno, unsigned char *dest, int len) {
    int bandno, cblkno;
    tcd_tilecomp_t *tilec=&tile->comps[compno];
    tcd_resolution_t *res=&tilec->resolutions[resno];
    unsigned char *c=dest;
    if (!layno) {
        for (bandno=0; bandno<res->numbands; bandno++) {
            tcd_band_t *band=&res->bands[bandno];
            tcd_precinct_t *prc=&band->precincts[precno];
            tgt_reset(prc->incltree);
            tgt_reset(prc->imsbtree);
            for (cblkno=0; cblkno<prc->cw*prc->ch; cblkno++) {
                tcd_cblk_t *cblk=&prc->cblks[cblkno];
                cblk->numpasses=0;
                tgt_setvalue(prc->imsbtree, cblkno, band->numbps-cblk->numbps);
            }
        }
    }
    bio_init_enc(c, len);
    bio_write(1, 1);
    for (bandno=0; bandno<res->numbands; bandno++) {
        tcd_band_t *band=&res->bands[bandno];
        tcd_precinct_t *prc=&band->precincts[precno];
        for (cblkno=0; cblkno<prc->cw*prc->ch; cblkno++) {
            tcd_cblk_t *cblk=&prc->cblks[cblkno];
            tcd_layer_t *layer=&cblk->layers[layno];
            if (!cblk->numpasses && layer->numpasses) {
                tgt_setvalue(prc->incltree, cblkno, layno);
            }
        }
        for (cblkno=0; cblkno<prc->cw*prc->ch; cblkno++) {
            tcd_cblk_t *cblk=&prc->cblks[cblkno];
            tcd_layer_t *layer=&cblk->layers[layno];
            int increment;
            if (!cblk->numpasses) {
                tgt_encode(prc->incltree, cblkno, layno+1);
            } else {
                bio_write(layer->numpasses!=0, 1);
            }
            if (!layer->numpasses) {
                continue;
            }
            if (!cblk->numpasses) {
                cblk->numlenbits=3;
                tgt_encode(prc->imsbtree, cblkno, 999);
            }
            t2_putnumpasses(layer->numpasses);
            increment=int_max(0, int_floorlog2(layer->len)+1-(cblk->numlenbits+int_floorlog2(layer->numpasses)));
            t2_putcommacode(increment);
            cblk->numlenbits+=increment;
            bio_write(layer->len, cblk->numlenbits+int_floorlog2(layer->numpasses));
        }
    }
    bio_flush();
    c+=bio_numbytes();
    for (bandno=0; bandno<res->numbands; bandno++) {
        tcd_band_t *band=&res->bands[bandno];
        tcd_precinct_t *prc=&band->precincts[precno];
        for (cblkno=0; cblkno<prc->cw*prc->ch; cblkno++) {
            tcd_cblk_t *cblk=&prc->cblks[cblkno];
            tcd_layer_t *layer=&cblk->layers[layno];
            if (!layer->numpasses) continue;
            if (c+layer->len>dest+len) {
                longjmp(j2k_error, 1);
            }
            memcpy(c, layer->data, layer->len);
            cblk->numpasses+=layer->numpasses;
            c+=layer->len;
        }
    }
    return c-dest;
}

void t2_init_seg(tcd_seg_t *seg, int cblksty) {
    seg->numpasses=0;
    seg->len=0;
    seg->maxpasses=cblksty&J2K_CCP_CBLKSTY_TERMALL?1:100;
}

int t2_decode_packet(unsigned char *src, int len, tcd_tile_t *tile, j2k_tcp_t *tcp, int compno, int resno, int precno, int layno) {
    int bandno, cblkno;
    tcd_tilecomp_t *tilec=&tile->comps[compno];
    tcd_resolution_t *res=&tilec->resolutions[resno];
    unsigned char *c=src;
    int present;
    if (layno==0) {
        for (bandno=0; bandno<res->numbands; bandno++) {
            tcd_band_t *band=&res->bands[bandno];
            tcd_precinct_t *prc=&band->precincts[precno];
            tgt_reset(prc->incltree);
            tgt_reset(prc->imsbtree);
            for (cblkno=0; cblkno<prc->cw*prc->ch; cblkno++) {
                tcd_cblk_t *cblk=&prc->cblks[cblkno];
                cblk->numsegs=0;
            }
        }
    }
    if (tcp->csty&J2K_CP_CSTY_SOP) {
        c+=6;
    }
    bio_init_dec(c, src+len-c);
    present=bio_read(1);
    if (!present) {
        bio_inalign();
        c+=bio_numbytes();
        return c-src;
    }
    for (bandno=0; bandno<res->numbands; bandno++) {
        tcd_band_t *band=&res->bands[bandno];
        tcd_precinct_t *prc=&band->precincts[precno];
        for (cblkno=0; cblkno<prc->cw*prc->ch; cblkno++) {
            int included, increment, n;
            tcd_cblk_t *cblk=&prc->cblks[cblkno];
            tcd_seg_t *seg;
            if (!cblk->numsegs) {
                included=tgt_decode(prc->incltree, cblkno, layno+1);
            } else {
                included=bio_read(1);
            }
            if (!included) {
                cblk->numnewpasses=0;
                continue;
            }
            if (!cblk->numsegs) {
                int i, numimsbs;
                for (i=0; !tgt_decode(prc->imsbtree, cblkno, i); i++) {}
                numimsbs=i-1;
                cblk->numbps=band->numbps-numimsbs;
                cblk->numlenbits=3;
            }
            cblk->numnewpasses=t2_getnumpasses();
            increment=t2_getcommacode();
            cblk->numlenbits+=increment;
            if (!cblk->numsegs) {
                seg=&cblk->segs[0];
                t2_init_seg(seg, tcp->tccps[compno].cblksty);
            } else {
                seg=&cblk->segs[cblk->numsegs-1];
                if (seg->numpasses==seg->maxpasses) {
                    t2_init_seg(++seg, tcp->tccps[compno].cblksty);
                }
            }
            n=cblk->numnewpasses;
            do {
                seg->numnewpasses=int_min(seg->maxpasses-seg->numpasses, n);
                seg->newlen=bio_read(cblk->numlenbits+int_floorlog2(seg->numnewpasses));
                n-=seg->numnewpasses;
                if (n>0) {
                    t2_init_seg(++seg, tcp->tccps[compno].cblksty);
                }
            } while (n>0);
       }
    }


    bio_inalign();
    c+=bio_numbytes();
    if (tcp->csty&J2K_CP_CSTY_EPH) {
        c+=2;
    }
    for (bandno=0; bandno<res->numbands; bandno++) {
        tcd_band_t *band=&res->bands[bandno];
        tcd_precinct_t *prc=&band->precincts[precno];
        for (cblkno=0; cblkno<prc->cw*prc->ch; cblkno++) {
            tcd_cblk_t *cblk=&prc->cblks[cblkno];
            tcd_seg_t *seg;
            if (!cblk->numnewpasses) continue;
            if (!cblk->numsegs) {
                seg=&cblk->segs[cblk->numsegs++];
                cblk->len=0;
            } else {
                seg=&cblk->segs[cblk->numsegs-1];
                if (seg->numpasses==seg->maxpasses) {
                    seg++;
                    cblk->numsegs++;
                }
            }
            do {
                if (c+seg->newlen>src+len) longjmp(j2k_error, 1);
                memcpy(cblk->data+cblk->len, c, seg->newlen);
                if (seg->numpasses==0) {
                    seg->data=cblk->data+cblk->len;
                }
                c+=seg->newlen;
                cblk->len+=seg->newlen;
                seg->len+=seg->newlen;
                seg->numpasses+=seg->numnewpasses;
                cblk->numnewpasses-=seg->numnewpasses;
                if (cblk->numnewpasses>0) {
                    seg++;
                    cblk->numsegs++;
                }
            } while (cblk->numnewpasses>0);
        }
    }

 
	return c-src;
}

int t2_encode_packets(j2k_image_t *img, j2k_cp_t *cp, int tileno, tcd_tile_t *tile, int maxlayers, unsigned char *dest, int len) {
    unsigned char *c=dest;
    pi_iterator_t *pi;
    pi=pi_create(img, cp, tileno);
    while (pi_next(pi)) {
        if (pi->layno<maxlayers) {
            //fprintf(stderr, "compno=%d, resno=%d, precno=%d, layno=%d\n", pi->compno, pi->resno, pi->precno, pi->layno);
            c+=t2_encode_packet(tile, &cp->tcps[tileno], pi->compno, pi->resno, pi->precno, pi->layno, c, dest+len-c);
        }
    }
	pi_destroy(pi);
    return c-dest;
}

int t2_decode_packets(unsigned char *src, int len, j2k_image_t *img, j2k_cp_t *cp, int tileno, tcd_tile_t *tile) {
    unsigned char *c=src;
    pi_iterator_t *pi;
    pi=pi_create(img, cp, tileno);
    while (pi_next(pi)) {
        //fprintf(stderr, "compno=%d, resno=%d, precno=%d, layno=%d\n", pi->compno, pi->resno, pi->precno, pi->layno);
        c+=t2_decode_packet(c, src+len-c, tile, &cp->tcps[tileno], pi->compno, pi->resno, pi->precno, pi->layno);
    }
	pi_destroy(pi);
    return c-src;
}
