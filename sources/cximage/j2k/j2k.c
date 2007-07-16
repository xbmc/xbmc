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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <math.h>

#include "j2k.h"
#include "cio.h"
#include "tcd.h"
#include "dwt.h"
#include "int.h"

#define J2K_MS_SOC 0xff4f
#define J2K_MS_SOT 0xff90
#define J2K_MS_SOD 0xff93
#define J2K_MS_EOC 0xffd9
#define J2K_MS_SIZ 0xff51
#define J2K_MS_COD 0xff52
#define J2K_MS_COC 0xff53
#define J2K_MS_RGN 0xff5e
#define J2K_MS_QCD 0xff5c
#define J2K_MS_QCC 0xff5d
#define J2K_MS_POC 0xff5f
#define J2K_MS_TLM 0xff55
#define J2K_MS_PLM 0xff57
#define J2K_MS_PLT 0xff58
#define J2K_MS_PPM 0xff60
#define J2K_MS_PPT 0xff61
#define J2K_MS_SOP 0xff91
#define J2K_MS_EPH 0xff92
#define J2K_MS_CRG 0xff63
#define J2K_MS_COM 0xff64

#define J2K_STATE_MHSOC 0x0001
#define J2K_STATE_MHSIZ 0x0002
#define J2K_STATE_MH 0x0004
#define J2K_STATE_TPHSOT 0x0008
#define J2K_STATE_TPH 0x0010
#define J2K_STATE_MT 0x0020

jmp_buf j2k_error;

int j2k_state;
int j2k_curtileno;
j2k_tcp_t j2k_default_tcp;
unsigned char *j2k_eot;
int j2k_sot_start;

j2k_image_t *j2k_img;
j2k_cp_t *j2k_cp;

unsigned char **j2k_tile_data;
int *j2k_tile_len;

#if J2K_DUMP_ENABLED
void j2k_dump_image(j2k_image_t *img) {
    int compno;
    fprintf(stderr, "image {\n");
    fprintf(stderr, "  x0=%d, y0=%d, x1=%d, y1=%d\n", img->x0, img->y0, img->x1, img->y1);
    fprintf(stderr, "  numcomps=%d\n", img->numcomps);
    for (compno=0; compno<img->numcomps; compno++) {
        j2k_comp_t *comp=&img->comps[compno];
        fprintf(stderr, "  comp %d {\n", compno);
        fprintf(stderr, "    dx=%d, dy=%d\n", comp->dx, comp->dy);
        fprintf(stderr, "    prec=%d\n", comp->prec);
        fprintf(stderr, "    sgnd=%d\n", comp->sgnd);
        fprintf(stderr, "  }\n");
    }
    fprintf(stderr, "}\n");
}

void j2k_dump_cp(j2k_image_t *img, j2k_cp_t *cp) {
    int tileno, compno, layno, bandno, resno, numbands;
    fprintf(stderr, "coding parameters {\n");
    fprintf(stderr, "  tx0=%d, ty0=%d\n", cp->tx0, cp->ty0);
    fprintf(stderr, "  tdx=%d, tdy=%d\n", cp->tdx, cp->tdy);
    fprintf(stderr, "  tw=%d, th=%d\n", cp->tw, cp->th);
    for (tileno=0; tileno<cp->tw*cp->th; tileno++) {
        j2k_tcp_t *tcp=&cp->tcps[tileno];
        fprintf(stderr, "  tile %d {\n", tileno);
        fprintf(stderr, "    csty=%x\n", tcp->csty);
        fprintf(stderr, "    prg=%d\n", tcp->prg);
        fprintf(stderr, "    numlayers=%d\n", tcp->numlayers);
        fprintf(stderr, "    mct=%d\n", tcp->mct);
        fprintf(stderr, "    rates=");
        for (layno=0; layno<tcp->numlayers; layno++) {
            fprintf(stderr, "%d ", tcp->rates[layno]);
        }
        fprintf(stderr, "\n");
        for (compno=0; compno<img->numcomps; compno++) {
            j2k_tccp_t *tccp=&tcp->tccps[compno];
            fprintf(stderr, "    comp %d {\n", compno);
            fprintf(stderr, "      csty=%x\n", tccp->csty);
            fprintf(stderr, "      numresolutions=%d\n", tccp->numresolutions);
            fprintf(stderr, "      cblkw=%d\n", tccp->cblkw);
            fprintf(stderr, "      cblkh=%d\n", tccp->cblkh);
            fprintf(stderr, "      cblksty=%x\n", tccp->cblksty);
            fprintf(stderr, "      qmfbid=%d\n", tccp->qmfbid);
            fprintf(stderr, "      qntsty=%d\n", tccp->qntsty);
            fprintf(stderr, "      numgbits=%d\n", tccp->numgbits);
            fprintf(stderr, "      roishift=%d\n", tccp->roishift);
            fprintf(stderr, "      stepsizes=");
            numbands=tccp->qntsty==J2K_CCP_QNTSTY_SIQNT?1:tccp->numresolutions*3-2;
            for (bandno=0; bandno<numbands; bandno++) {
                fprintf(stderr, "(%d,%d) ", tccp->stepsizes[bandno].mant, tccp->stepsizes[bandno].expn);
            }
            fprintf(stderr, "\n");
            if (tccp->csty&J2K_CCP_CSTY_PRT) {
                fprintf(stderr, "      prcw=");
                for (resno=0; resno<tccp->numresolutions; resno++) {
                    fprintf(stderr, "%d ", tccp->prcw[resno]);
                }
                fprintf(stderr, "\n");
                fprintf(stderr, "      prch=");
                for (resno=0; resno<tccp->numresolutions; resno++) {
                    fprintf(stderr, "%d ", tccp->prch[resno]);
                }
                fprintf(stderr, "\n");
            }
            fprintf(stderr, "    }\n");
        }
        fprintf(stderr, "  }\n");
    }
    fprintf(stderr, "}\n");
}
#endif //J2K_DUMP_ENABLED
void j2k_write_soc() {
	J2KDUMP("%.8x: SOC\n", cio_tell())
    cio_write(J2K_MS_SOC, 2);
}

void j2k_read_soc() {
    J2KDUMP("%.8x: SOC\n", cio_tell()-2)
    j2k_state=J2K_STATE_MHSIZ;
}

void j2k_write_siz() {
    int i;
    int lenp, len;
    J2KDUMP("%.8x: SIZ\n", cio_tell())
    cio_write(J2K_MS_SIZ, 2);
    lenp=cio_tell();
    cio_skip(2);
    cio_write(0, 2);
    cio_write(j2k_img->x1, 4);
    cio_write(j2k_img->y1, 4);
    cio_write(j2k_img->x0, 4);
    cio_write(j2k_img->y0, 4);
    cio_write(j2k_cp->tdx, 4);
    cio_write(j2k_cp->tdy, 4);
    cio_write(j2k_cp->tx0, 4);
    cio_write(j2k_cp->ty0, 4);
    cio_write(j2k_img->numcomps, 2);
    for (i=0; i<j2k_img->numcomps; i++) {
        cio_write(j2k_img->comps[i].prec-1+(j2k_img->comps[i].sgnd<<7), 1);
        cio_write(j2k_img->comps[i].dx, 1);
        cio_write(j2k_img->comps[i].dy, 1);
    }
    len=cio_tell()-lenp;
    cio_seek(lenp);
    cio_write(len, 2);
    cio_seek(lenp+len);
}

void j2k_read_siz() {
    int len, i;
    J2KDUMP("%.8x: SIZ\n", cio_tell()-2)
    len=cio_read(2);
    cio_read(2);
    j2k_img->x1=cio_read(4);
    j2k_img->y1=cio_read(4);
    j2k_img->x0=cio_read(4);
    j2k_img->y0=cio_read(4);
    j2k_cp->tdx=cio_read(4);
    j2k_cp->tdy=cio_read(4);
    j2k_cp->tx0=cio_read(4);
    j2k_cp->ty0=cio_read(4);
    j2k_img->numcomps=cio_read(2);
    j2k_img->comps=(j2k_comp_t*)malloc(j2k_img->numcomps*sizeof(j2k_comp_t));
    for (i=0; i<j2k_img->numcomps; i++) {
        int tmp, w, h;
        tmp=cio_read(1);
        j2k_img->comps[i].prec=(tmp&0x7f)+1;
        j2k_img->comps[i].sgnd=tmp>>7;
        j2k_img->comps[i].dx=cio_read(1);
        j2k_img->comps[i].dy=cio_read(1);
        w=int_ceildiv(j2k_img->x1-j2k_img->x0, j2k_img->comps[i].dx);
        h=int_ceildiv(j2k_img->y1-j2k_img->y0, j2k_img->comps[i].dy);
        j2k_img->comps[i].data=(int*)malloc(sizeof(int)*w*h);
    }
    j2k_cp->tw=int_ceildiv(j2k_img->x1-j2k_img->x0, j2k_cp->tdx);
    j2k_cp->th=int_ceildiv(j2k_img->y1-j2k_img->y0, j2k_cp->tdy);
    j2k_cp->tcps=(j2k_tcp_t*)calloc(sizeof(j2k_tcp_t), j2k_cp->tw*j2k_cp->th);
    j2k_default_tcp.tccps=(j2k_tccp_t*)calloc(sizeof(j2k_tccp_t), j2k_img->numcomps);
    for (i=0; i<j2k_cp->tw*j2k_cp->th; i++) {
        j2k_cp->tcps[i].tccps=(j2k_tccp_t*)calloc(sizeof(j2k_tccp_t), j2k_img->numcomps);
    }
    j2k_tile_data=(unsigned char**)calloc(j2k_cp->tw*j2k_cp->th, sizeof(char*));
    j2k_tile_len=(int*)calloc(j2k_cp->tw*j2k_cp->th, sizeof(int));
    j2k_state=J2K_STATE_MH;
}

void j2k_write_com() {
    unsigned int i;
    int lenp, len;
    char str[256];
    sprintf(str, "Creator: J2000 codec");
    J2KDUMP("%.8x: COM\n", cio_tell())
    cio_write(J2K_MS_COM, 2);
    lenp=cio_tell();
    cio_skip(2);
    cio_write(0, 2);
    for (i=0; i<strlen(str); i++) {
        cio_write(str[i], 1);
    }
    len=cio_tell()-lenp;
    cio_seek(lenp);
    cio_write(len, 2);
    cio_seek(lenp+len);
}

void j2k_read_com() {
    int len;
    J2KDUMP("%.8x: COM\n", cio_tell()-2)
    len=cio_read(2);
    cio_skip(len-2);
}

void j2k_write_cox(int compno) {
    int i;
    j2k_tcp_t *tcp;
    j2k_tccp_t *tccp;
    tcp=&j2k_cp->tcps[j2k_curtileno];
    tccp=&tcp->tccps[compno];
    cio_write(tccp->numresolutions-1, 1);
    cio_write(tccp->cblkw-2, 1);
    cio_write(tccp->cblkh-2, 1);
    cio_write(tccp->cblksty, 1);
    cio_write(tccp->qmfbid, 1);
    if (tccp->csty&J2K_CCP_CSTY_PRT) {
        for (i=0; i<tccp->numresolutions; i++) {
            cio_write(tccp->prcw[i]+(tccp->prch[i]<<4), 1);
        }
    }
}

void j2k_read_cox(int compno) {
    int i;
    j2k_tcp_t *tcp;
    j2k_tccp_t *tccp;
    tcp=j2k_state==J2K_STATE_TPH?&j2k_cp->tcps[j2k_curtileno]:&j2k_default_tcp;
    tccp=&tcp->tccps[compno];
    tccp->numresolutions=cio_read(1)+1;
    tccp->cblkw=cio_read(1)+2;
    tccp->cblkh=cio_read(1)+2;
    tccp->cblksty=cio_read(1);
    tccp->qmfbid=cio_read(1);
    if (tccp->csty&J2K_CP_CSTY_PRT) {
        for (i=0; i<tccp->numresolutions; i++) {
            int tmp=cio_read(1);
            tccp->prcw[i]=tmp&0xf;
            tccp->prch[i]=tmp>>4;
        }
    }
}

void j2k_write_cod() {
    j2k_tcp_t *tcp;
    int lenp, len;
    J2KDUMP("%.8x: COD\n", cio_tell())
    cio_write(J2K_MS_COD, 2);
    lenp=cio_tell();
    cio_skip(2);
    tcp=&j2k_cp->tcps[j2k_curtileno];
    cio_write(tcp->csty, 1);
    cio_write(tcp->prg, 1);
    cio_write(tcp->numlayers, 2);
    cio_write(tcp->mct, 1);
    j2k_write_cox(0);
    len=cio_tell()-lenp;
    cio_seek(lenp);
    cio_write(len, 2);
    cio_seek(lenp+len);
}

void j2k_read_cod() {
    int len, i, pos;
    j2k_tcp_t *tcp;
    J2KDUMP("%.8x: COD\n", cio_tell()-2)
    tcp=j2k_state==J2K_STATE_TPH?&j2k_cp->tcps[j2k_curtileno]:&j2k_default_tcp;
    len=cio_read(2);
    tcp->csty=cio_read(1);
    tcp->prg=cio_read(1);
    tcp->numlayers=cio_read(2);
    tcp->mct=cio_read(1);
    pos=cio_tell();
    for (i=0; i<j2k_img->numcomps; i++) {
        tcp->tccps[i].csty=tcp->csty&J2K_CP_CSTY_PRT;
        cio_seek(pos);
        j2k_read_cox(i);
    }
}

void j2k_write_coc(int compno) {
    j2k_tcp_t *tcp;
    int lenp, len;
    J2KDUMP("%.8x: COC\n", cio_tell())
    cio_write(J2K_MS_COC, 2);
    lenp=cio_tell();
    cio_skip(2);
    tcp=&j2k_cp->tcps[j2k_curtileno];
    cio_write(compno, j2k_img->numcomps<=256?1:2);
    cio_write(tcp->tccps[compno].csty, 1);
    j2k_write_cox(compno);
    len=cio_tell()-lenp;
    cio_seek(lenp);
    cio_write(len, 2);
    cio_seek(lenp+len);
}

void j2k_read_coc() {
    int len, compno;
    j2k_tcp_t *tcp;
    J2KDUMP("%.8x: COC\n", cio_tell()-2)
    tcp=j2k_state==J2K_STATE_TPH?&j2k_cp->tcps[j2k_curtileno]:&j2k_default_tcp;
    len=cio_read(2);
    compno=cio_read(j2k_img->numcomps<=256?1:2);
    tcp->tccps[compno].csty=cio_read(1);
    j2k_read_cox(compno);
}

void j2k_write_qcx(int compno) {
    j2k_tcp_t *tcp;
    j2k_tccp_t *tccp;
    int bandno, numbands;
    tcp=&j2k_cp->tcps[j2k_curtileno];
    tccp=&tcp->tccps[compno];
    cio_write(tccp->qntsty+(tccp->numgbits<<5), 1);
    numbands=tccp->qntsty==J2K_CCP_QNTSTY_SIQNT?1:tccp->numresolutions*3-2;
    for (bandno=0; bandno<numbands; bandno++) {
        int expn, mant;
        expn=tccp->stepsizes[bandno].expn;
        mant=tccp->stepsizes[bandno].mant;
        if (tccp->qntsty==J2K_CCP_QNTSTY_NOQNT) {
            cio_write(expn<<3, 1);
        } else {
            cio_write((expn<<11)+mant, 2);
        }
    }
}

void j2k_read_qcx(int compno, int len) {
    int tmp;
    j2k_tcp_t *tcp;
    j2k_tccp_t *tccp;
    int bandno, numbands;
    tcp=j2k_state==J2K_STATE_TPH?&j2k_cp->tcps[j2k_curtileno]:&j2k_default_tcp;
    tccp=&tcp->tccps[compno];
    tmp=cio_read(1);
    tccp->qntsty=tmp&0x1f;
    tccp->numgbits=tmp>>5;
    numbands=tccp->qntsty==J2K_CCP_QNTSTY_SIQNT?1:(tccp->qntsty==J2K_CCP_QNTSTY_NOQNT?len-1:(len-1)/2);
    for (bandno=0; bandno<numbands; bandno++) {
        int expn, mant;
        if (tccp->qntsty==J2K_CCP_QNTSTY_NOQNT) { // WHY STEPSIZES WHEN NOQNT ?
            expn=cio_read(1)>>3;
            mant=0;
        } else {
            tmp=cio_read(2);
            expn=tmp>>11;
            mant=tmp&0x7ff;
        }
        tccp->stepsizes[bandno].expn=expn;
        tccp->stepsizes[bandno].mant=mant;
    }
}

void j2k_write_qcd() {
    int lenp, len;
    J2KDUMP("%.8x: QCD\n", cio_tell())
    cio_write(J2K_MS_QCD, 2);
    lenp=cio_tell();
    cio_skip(2);
    j2k_write_qcx(0);
    len=cio_tell()-lenp;
    cio_seek(lenp);
    cio_write(len, 2);
    cio_seek(lenp+len);
}

void j2k_read_qcd() {
    int len, i, pos;
    J2KDUMP("%.8x: QCD\n", cio_tell()-2)
    len=cio_read(2);
    pos=cio_tell();
    for (i=0; i<j2k_img->numcomps; i++) {
        cio_seek(pos);
        j2k_read_qcx(i, len-2);
    }
}

void j2k_write_qcc(int compno) {
    int lenp, len;
    J2KDUMP("%.8x: QCC\n", cio_tell())
    cio_write(J2K_MS_QCC, 2);
    lenp=cio_tell();
    cio_skip(2);
    cio_write(compno, j2k_img->numcomps<=256?1:2);
    j2k_write_qcx(compno);
    len=cio_tell()-lenp;
    cio_seek(lenp);
    cio_write(len, 2);
    cio_seek(lenp+len);
}

void j2k_read_qcc() {
    int len, compno;
    J2KDUMP("%.8x: QCC\n", cio_tell()-2)
    len=cio_read(2);
    compno=cio_read(j2k_img->numcomps<=256?1:2);
    j2k_read_qcx(compno, len-2-(j2k_img->numcomps<=256?1:2));
}

void j2k_read_poc() {
    int len, numpchgs, i;
    j2k_tcp_t *tcp;
    J2KWARNING("WARNING: POC marker segment processing not fully implemented\n")
    tcp=j2k_state==J2K_STATE_TPH?&j2k_cp->tcps[j2k_curtileno]:&j2k_default_tcp;
    len=cio_read(2);
    numpchgs=(len-2)/(5+2*(j2k_img->numcomps<=256?1:2));
    for (i=0; i<numpchgs; i++) {
        int resno0, compno0, layerno1, resno1, compno1, prg;
        resno0=cio_read(1);
        compno0=cio_read(j2k_img->numcomps<=256?1:2);
        layerno1=cio_read(2);
        resno1=cio_read(1);
        compno1=cio_read(j2k_img->numcomps<=256?1:2);
        prg=cio_read(1);
        tcp->prg=prg;
    }
}

void j2k_read_crg() {
    int len;
    len=cio_read(2);
    J2KWARNING("WARNING: CRG marker segment processing not implemented\n")
    cio_skip(len-2);
}

void j2k_read_tlm() {
    int len;
    len=cio_read(2);
    J2KWARNING("WARNING: TLM marker segment processing not implemented\n")
    cio_skip(len-2);
}

void j2k_read_plm() {
    int len;
    len=cio_read(2);
    J2KWARNING("WARNING: PLM marker segment processing not implemented\n")
    cio_skip(len-2);
}

void j2k_read_plt() {
    int len;
    len=cio_read(2);
    J2KWARNING("WARNING: PLT marker segment processing not implemented\n")
    cio_skip(len-2);
}

void j2k_read_ppm() {
    int len;
    len=cio_read(2);
    J2KWARNING("WARNING: PPM marker segment processing not implemented\n")
    cio_skip(len-2);
}

void j2k_read_ppt() {
    int len;
    len=cio_read(2);
    J2KWARNING("WARNING: PPT marker segment processing not implemented\n")
    cio_skip(len-2);
}

void j2k_write_sot() {
    int lenp, len;
    J2KDUMP("%.8x: SOT\n", cio_tell())
    j2k_sot_start=cio_tell();
    cio_write(J2K_MS_SOT, 2);
    lenp=cio_tell();
    cio_skip(2);
    cio_write(j2k_curtileno, 2);
    cio_skip(4);
    cio_write(0, 1);
    cio_write(1, 1);
    len=cio_tell()-lenp;
    cio_seek(lenp);
    cio_write(len, 2);
    cio_seek(lenp+len);
}

void j2k_read_sot() {
    int len, tileno, totlen, partno, numparts, i;
    j2k_tcp_t *tcp;
    j2k_tccp_t *tmp;
    J2KDUMP("%.8x: SOT\n", cio_tell()-2)
    len=cio_read(2);
    tileno=cio_read(2);
    totlen=cio_read(4);
    partno=cio_read(1);
    numparts=cio_read(1);
    j2k_curtileno=tileno;
    j2k_eot=cio_getbp()-12+totlen;
    j2k_state=J2K_STATE_TPH;
    tcp=&j2k_cp->tcps[j2k_curtileno];
    tmp=tcp->tccps;
    *tcp=j2k_default_tcp;
    tcp->tccps=tmp;
    for (i=0; i<j2k_img->numcomps; i++) {
        tcp->tccps[i]=j2k_default_tcp.tccps[i];
    }
}

void j2k_write_sod() {
    int l, layno;
    int totlen;
    j2k_tcp_t *tcp;
    J2KDUMP("%.8x: SOD\n", cio_tell())
    cio_write(J2K_MS_SOD, 2);
    tcp=&j2k_cp->tcps[j2k_curtileno];
    for (layno=0; layno<tcp->numlayers; layno++) {
        tcp->rates[layno]-=cio_tell();
        J2KDUMP2("tcp->rates[%d]=%d\n", layno, tcp->rates[layno])
    }
    J2KDUMP("cio_numbytesleft=%d\n", cio_numbytesleft())
    tcd_init(j2k_img, j2k_cp);
    l=tcd_encode_tile(j2k_curtileno, cio_getbp(), cio_numbytesleft()-2);
    totlen=cio_tell()+l-j2k_sot_start;
    cio_seek(j2k_sot_start+6);
    cio_write(totlen, 4);
    cio_seek(j2k_sot_start+totlen);
}

void j2k_read_sod() {
    int len;
    unsigned char *data;
    J2KDUMP("%.8x: SOD\n", cio_tell()-2)
    len=int_min(j2k_eot-cio_getbp(), cio_numbytesleft());
    j2k_tile_len[j2k_curtileno]+=len;
    data=(unsigned char*)realloc(j2k_tile_data[j2k_curtileno], j2k_tile_len[j2k_curtileno]);
    memcpy(data, cio_getbp(), len);
    j2k_tile_data[j2k_curtileno]=data;
    cio_skip(len);
    j2k_state=J2K_STATE_TPHSOT;
}

void j2k_read_rgn() {
    int len, compno, roisty;
    j2k_tcp_t *tcp;
    J2KDUMP("%.8x: RGN\n", cio_tell()-2)
    tcp=j2k_state==J2K_STATE_TPH?&j2k_cp->tcps[j2k_curtileno]:&j2k_default_tcp;
    len=cio_read(2);
    compno=cio_read(j2k_img->numcomps<=256?1:2);
    roisty=cio_read(1);
    tcp->tccps[compno].roishift=cio_read(1);
}

void j2k_write_eoc() {
    J2KDUMP("%.8x: EOC\n", cio_tell())
    cio_write(J2K_MS_EOC, 2);
}

void j2k_read_eoc() {
    int tileno;
#if J2K_DUMP_ENABLED
    J2KDUMP("%.8x: EOC\n", cio_tell()-2)
    j2k_dump_image(j2k_img);
    j2k_dump_cp(j2k_img, j2k_cp);
#endif //J2K_DUMP_ENABLED
    tcd_init(j2k_img, j2k_cp);
    for (tileno=0; tileno<j2k_cp->tw*j2k_cp->th; tileno++) {
        tcd_decode_tile(j2k_tile_data[tileno], j2k_tile_len[tileno], tileno);
    }
    j2k_state=J2K_STATE_MT;
    longjmp(j2k_error, 1);
}

void j2k_read_unk() {
    J2KWARNING("warning: unknown marker\n")
}

#ifndef XBMC
LIBJ2K_API int j2k_encode(j2k_image_t *img, j2k_cp_t *cp, unsigned char *dest, int len) {
    int tileno, compno;
    if (setjmp(j2k_error)) {
        return 0;
    }
    cio_init(dest, len);
    j2k_img=img;
    j2k_cp=cp;
#if J2K_DUMP_ENABLED
    j2k_dump_cp(j2k_img, j2k_cp);
#endif //J2K_DUMP_ENABLED
    j2k_write_soc();
    j2k_write_siz();
    j2k_write_com();
    for (tileno=0; tileno<cp->tw*cp->th; tileno++) {
        j2k_curtileno=tileno;
        j2k_write_sot();
        j2k_write_cod();
        j2k_write_qcd();
        for (compno=1; compno<img->numcomps; compno++) {
            j2k_write_coc(compno);
            j2k_write_qcc(compno);
        }
        j2k_write_sod();
    }
    j2k_write_eoc();
    return cio_tell();
}
#endif

typedef struct {
    int id;
    int states;
    void (*handler)();
} j2k_dec_mstabent_t;

j2k_dec_mstabent_t j2k_dec_mstab[]={
    {J2K_MS_SOC, J2K_STATE_MHSOC, j2k_read_soc},
    {J2K_MS_SOT, J2K_STATE_MH|J2K_STATE_TPHSOT, j2k_read_sot},
    {J2K_MS_SOD, J2K_STATE_TPH, j2k_read_sod},
    {J2K_MS_EOC, J2K_STATE_TPHSOT, j2k_read_eoc},
    {J2K_MS_SIZ, J2K_STATE_MHSIZ, j2k_read_siz},
    {J2K_MS_COD, J2K_STATE_MH|J2K_STATE_TPH, j2k_read_cod},
    {J2K_MS_COC, J2K_STATE_MH|J2K_STATE_TPH, j2k_read_coc},
    {J2K_MS_RGN, J2K_STATE_MH|J2K_STATE_TPH, j2k_read_rgn},
    {J2K_MS_QCD, J2K_STATE_MH|J2K_STATE_TPH, j2k_read_qcd},
    {J2K_MS_QCC, J2K_STATE_MH|J2K_STATE_TPH, j2k_read_qcc},
    {J2K_MS_POC, J2K_STATE_MH|J2K_STATE_TPH, j2k_read_poc},
    {J2K_MS_TLM, J2K_STATE_MH, j2k_read_tlm},
    {J2K_MS_PLM, J2K_STATE_MH, j2k_read_plm},
    {J2K_MS_PLT, J2K_STATE_TPH, j2k_read_plt},
    {J2K_MS_PPM, J2K_STATE_MH, j2k_read_ppm},
    {J2K_MS_PPT, J2K_STATE_TPH, j2k_read_ppt},
    {J2K_MS_SOP, 0, 0},
    {J2K_MS_CRG, J2K_STATE_MH, j2k_read_crg},
    {J2K_MS_COM, J2K_STATE_MH|J2K_STATE_TPH, j2k_read_com},
    {0, J2K_STATE_MH|J2K_STATE_TPH, j2k_read_unk}
};

j2k_dec_mstabent_t *j2k_dec_mstab_lookup(int id) {
    j2k_dec_mstabent_t *e;
    for (e=j2k_dec_mstab; e->id!=0; e++) {
        if (e->id==id) {
            break;
        }
    }
    return e;
}

#ifndef XBMC
LIBJ2K_API int j2k_decode(unsigned char *src, int len, j2k_image_t **img, j2k_cp_t **cp) {
    if (setjmp(j2k_error)) {
        if (j2k_state!=J2K_STATE_MT) {
            J2KWARNING("WARNING: incomplete bitstream\n")
            return 0;
        }
        return cio_numbytes();
    }
    j2k_img=(j2k_image_t*)malloc(sizeof(j2k_image_t));
    j2k_cp=(j2k_cp_t*)malloc(sizeof(j2k_cp_t));
    *img=j2k_img;
    *cp=j2k_cp;
    j2k_state=J2K_STATE_MHSOC;
    cio_init(src, len);
    for (;;) {
        j2k_dec_mstabent_t *e;
        int id=cio_read(2);
        if (id>>8!=0xff) {
            J2KDUMP2("%.8x: expected a marker instead of %x\n", cio_tell()-2, id);
            return 0;
        }
        e=j2k_dec_mstab_lookup(id);
        if (!(j2k_state & e->states)) {
            J2KDUMP2("%.8x: unexpected marker %x\n", cio_tell()-2, id);
            return 0;
        }
        if (e->handler) {
            (*e->handler)();
        }
    }
}

LIBJ2K_API void j2k_destroy(j2k_image_t **img, j2k_cp_t **cp)
{
	int i;
    tcd_destroy(*img, *cp);
	free(j2k_tile_len); j2k_tile_len =0;
	if (j2k_tile_data) for(i=0;i<((*cp)->tw*(*cp)->th);i++) { free(j2k_tile_data[i]); j2k_tile_data[i]=0;}
	free(j2k_tile_data); j2k_tile_data=0;
	free(j2k_default_tcp.tccps); j2k_default_tcp.tccps = 0;
	if ((*cp)->tcps) for(i=0;i<((*cp)->tw*(*cp)->th);i++) {free((*cp)->tcps[i].tccps); (*cp)->tcps[i].tccps=0;}
	free((*cp)->tcps); (*cp)->tcps=0;

	if ((*img)->comps) for(i=0;i<(*img)->numcomps;i++) {free((*img)->comps[i].data); (*img)->comps[i].data=0;}
	free((*img)->comps); (*img)->comps = 0;

	free(*img); *img=0;
	free(*cp); *cp=0;
}
#endif

/*
#ifdef WIN32
#include <windows.h>

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
#endif
*/
