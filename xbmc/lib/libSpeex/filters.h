/* Copyright (C) 2002 Jean-Marc Valin 
   File: filters.h
   Various analysis/synthesis filters

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef FILTERS_H
#define FILTERS_H

#include "misc.h"

spx_word16_t compute_rms(const spx_sig_t *x, int len);
void signal_mul(const spx_sig_t *x, spx_sig_t *y, spx_word32_t scale, int len);
void signal_div(const spx_sig_t *x, spx_sig_t *y, spx_word32_t scale, int len);

#ifdef FIXED_POINT

int normalize16(const spx_sig_t *x, spx_word16_t *y, int max_scale, int len);

#endif

typedef struct CombFilterMem {
   int   last_pitch;
   spx_word16_t last_pitch_gain[3];
   spx_word16_t smooth_gain;
} CombFilterMem;


void qmf_decomp(const short *xx, const spx_word16_t *aa, spx_sig_t *y1, spx_sig_t *y2, int N, int M, spx_word16_t *mem, char *stack);
void fir_mem_up(const spx_sig_t *x, const spx_word16_t *a, spx_sig_t *y, int N, int M, spx_word32_t *mem, char *stack);


void filter_mem2(const spx_sig_t *x, const spx_coef_t *num, const spx_coef_t *den, spx_sig_t *y, int N, int ord, spx_mem_t *mem);
void fir_mem2(const spx_sig_t *x, const spx_coef_t *num, spx_sig_t *y, int N, int ord, spx_mem_t *mem);
void iir_mem2(const spx_sig_t *x, const spx_coef_t *den, spx_sig_t *y, int N, int ord, spx_mem_t *mem);

/* Apply bandwidth expansion on LPC coef */
void bw_lpc(spx_word16_t gamma, const spx_coef_t *lpc_in, spx_coef_t *lpc_out, int order);



void syn_percep_zero(const spx_sig_t *x, const spx_coef_t *ak, const spx_coef_t *awk1, const spx_coef_t *awk2, spx_sig_t *y, int N, int ord, char *stack);

void residue_percep_zero(const spx_sig_t *xx, const spx_coef_t *ak, const spx_coef_t *awk1, const spx_coef_t *awk2, spx_sig_t *y, int N, int ord, char *stack);

void comb_filter_mem_init (CombFilterMem *mem);

void comb_filter(
spx_sig_t *exc,          /*decoded excitation*/
spx_sig_t *new_exc,      /*enhanced excitation*/
spx_coef_t *ak,           /*LPC filter coefs*/
int p,               /*LPC order*/
int nsf,             /*sub-frame size*/
int pitch,           /*pitch period*/
spx_word16_t *pitch_gain,   /*pitch gain (3-tap)*/
spx_word16_t  comb_gain,    /*gain of comb filter*/
CombFilterMem *mem
);


#endif
