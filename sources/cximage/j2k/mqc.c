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

#include "mqc.h"

/// <summary>
/// This struct defines the state of a context.
/// </summary>
typedef struct mqc_state_s {
	unsigned int qeval;
	int mps;
	struct mqc_state_s *nmps;
	struct mqc_state_s *nlps;
} mqc_state_t;

/// <summary>
/// This array defines all the possible states for a context.
/// </summary>
mqc_state_t mqc_states[47*2]={
	{0x5601, 0, &mqc_states[2], &mqc_states[3]},
	{0x5601, 1, &mqc_states[3], &mqc_states[2]},
	{0x3401, 0, &mqc_states[4], &mqc_states[12]},
	{0x3401, 1, &mqc_states[5], &mqc_states[13]},
	{0x1801, 0, &mqc_states[6], &mqc_states[18]},
	{0x1801, 1, &mqc_states[7], &mqc_states[19]},
	{0x0ac1, 0, &mqc_states[8], &mqc_states[24]},
	{0x0ac1, 1, &mqc_states[9], &mqc_states[25]},
	{0x0521, 0, &mqc_states[10], &mqc_states[58]},
	{0x0521, 1, &mqc_states[11], &mqc_states[59]},
	{0x0221, 0, &mqc_states[76], &mqc_states[66]},
	{0x0221, 1, &mqc_states[77], &mqc_states[67]},
	{0x5601, 0, &mqc_states[14], &mqc_states[13]},
	{0x5601, 1, &mqc_states[15], &mqc_states[12]},
	{0x5401, 0, &mqc_states[16], &mqc_states[28]},
	{0x5401, 1, &mqc_states[17], &mqc_states[29]},
	{0x4801, 0, &mqc_states[18], &mqc_states[28]},
	{0x4801, 1, &mqc_states[19], &mqc_states[29]},
	{0x3801, 0, &mqc_states[20], &mqc_states[28]},
	{0x3801, 1, &mqc_states[21], &mqc_states[29]},
	{0x3001, 0, &mqc_states[22], &mqc_states[34]},
	{0x3001, 1, &mqc_states[23], &mqc_states[35]},
	{0x2401, 0, &mqc_states[24], &mqc_states[36]},
	{0x2401, 1, &mqc_states[25], &mqc_states[37]},
	{0x1c01, 0, &mqc_states[26], &mqc_states[40]},
	{0x1c01, 1, &mqc_states[27], &mqc_states[41]},
	{0x1601, 0, &mqc_states[58], &mqc_states[42]},
	{0x1601, 1, &mqc_states[59], &mqc_states[43]},
	{0x5601, 0, &mqc_states[30], &mqc_states[29]},
	{0x5601, 1, &mqc_states[31], &mqc_states[28]},
	{0x5401, 0, &mqc_states[32], &mqc_states[28]},
	{0x5401, 1, &mqc_states[33], &mqc_states[29]},
	{0x5101, 0, &mqc_states[34], &mqc_states[30]},
	{0x5101, 1, &mqc_states[35], &mqc_states[31]},
	{0x4801, 0, &mqc_states[36], &mqc_states[32]},
	{0x4801, 1, &mqc_states[37], &mqc_states[33]},
	{0x3801, 0, &mqc_states[38], &mqc_states[34]},
	{0x3801, 1, &mqc_states[39], &mqc_states[35]},
	{0x3401, 0, &mqc_states[40], &mqc_states[36]},
	{0x3401, 1, &mqc_states[41], &mqc_states[37]},
	{0x3001, 0, &mqc_states[42], &mqc_states[38]},
	{0x3001, 1, &mqc_states[43], &mqc_states[39]},
	{0x2801, 0, &mqc_states[44], &mqc_states[38]},
	{0x2801, 1, &mqc_states[45], &mqc_states[39]},
	{0x2401, 0, &mqc_states[46], &mqc_states[40]},
	{0x2401, 1, &mqc_states[47], &mqc_states[41]},
	{0x2201, 0, &mqc_states[48], &mqc_states[42]},
	{0x2201, 1, &mqc_states[49], &mqc_states[43]},
	{0x1c01, 0, &mqc_states[50], &mqc_states[44]},
	{0x1c01, 1, &mqc_states[51], &mqc_states[45]},
	{0x1801, 0, &mqc_states[52], &mqc_states[46]},
	{0x1801, 1, &mqc_states[53], &mqc_states[47]},
	{0x1601, 0, &mqc_states[54], &mqc_states[48]},
	{0x1601, 1, &mqc_states[55], &mqc_states[49]},
	{0x1401, 0, &mqc_states[56], &mqc_states[50]},
	{0x1401, 1, &mqc_states[57], &mqc_states[51]},
	{0x1201, 0, &mqc_states[58], &mqc_states[52]},
	{0x1201, 1, &mqc_states[59], &mqc_states[53]},
	{0x1101, 0, &mqc_states[60], &mqc_states[54]},
	{0x1101, 1, &mqc_states[61], &mqc_states[55]},
	{0x0ac1, 0, &mqc_states[62], &mqc_states[56]},
	{0x0ac1, 1, &mqc_states[63], &mqc_states[57]},
	{0x09c1, 0, &mqc_states[64], &mqc_states[58]},
	{0x09c1, 1, &mqc_states[65], &mqc_states[59]},
	{0x08a1, 0, &mqc_states[66], &mqc_states[60]},
	{0x08a1, 1, &mqc_states[67], &mqc_states[61]},
	{0x0521, 0, &mqc_states[68], &mqc_states[62]},
	{0x0521, 1, &mqc_states[69], &mqc_states[63]},
	{0x0441, 0, &mqc_states[70], &mqc_states[64]},
	{0x0441, 1, &mqc_states[71], &mqc_states[65]},
	{0x02a1, 0, &mqc_states[72], &mqc_states[66]},
	{0x02a1, 1, &mqc_states[73], &mqc_states[67]},
	{0x0221, 0, &mqc_states[74], &mqc_states[68]},
	{0x0221, 1, &mqc_states[75], &mqc_states[69]},
	{0x0141, 0, &mqc_states[76], &mqc_states[70]},
	{0x0141, 1, &mqc_states[77], &mqc_states[71]},
	{0x0111, 0, &mqc_states[78], &mqc_states[72]},
	{0x0111, 1, &mqc_states[79], &mqc_states[73]},
	{0x0085, 0, &mqc_states[80], &mqc_states[74]},
	{0x0085, 1, &mqc_states[81], &mqc_states[75]},
	{0x0049, 0, &mqc_states[82], &mqc_states[76]},
	{0x0049, 1, &mqc_states[83], &mqc_states[77]},
	{0x0025, 0, &mqc_states[84], &mqc_states[78]},
	{0x0025, 1, &mqc_states[85], &mqc_states[79]},
	{0x0015, 0, &mqc_states[86], &mqc_states[80]},
	{0x0015, 1, &mqc_states[87], &mqc_states[81]},
	{0x0009, 0, &mqc_states[88], &mqc_states[82]},
	{0x0009, 1, &mqc_states[89], &mqc_states[83]},
	{0x0005, 0, &mqc_states[90], &mqc_states[84]},
	{0x0005, 1, &mqc_states[91], &mqc_states[85]},
	{0x0001, 0, &mqc_states[90], &mqc_states[86]},
	{0x0001, 1, &mqc_states[91], &mqc_states[87]},
	{0x5601, 0, &mqc_states[92], &mqc_states[92]},
	{0x5601, 1, &mqc_states[93], &mqc_states[93]},
};

#define MQC_NUMCTXS 32

unsigned int mqc_c;
unsigned int mqc_a;
unsigned int mqc_ct;
unsigned char *mqc_bp;
unsigned char *mqc_start;
unsigned char *mqc_end;
mqc_state_t *mqc_ctxs[MQC_NUMCTXS];
mqc_state_t **mqc_curctx;

/// <summary>
/// Return the number of bytes already encoded.
/// </sumary>
int mqc_numbytes() {
	return mqc_bp-mqc_start;
}

/// <summary>
/// Output a byte.
/// </sumary>
void mqc_byteout()
{
	if (*mqc_bp==0xff) {
		mqc_bp++;
		*mqc_bp=mqc_c>>20;
		mqc_c&=0xfffff;
		mqc_ct=7;
	} else {
		if ((mqc_c&0x8000000)==0) {
			mqc_bp++;
			*mqc_bp=mqc_c>>19;
			mqc_c&=0x7ffff;
			mqc_ct=8;
		} else {
			(*mqc_bp)++;
			if (*mqc_bp==0xff) {
				mqc_c&=0x7ffffff;
				mqc_bp++;
				*mqc_bp=mqc_c>>20;
				mqc_c&=0xfffff;
				mqc_ct=7;
			} else {
				mqc_bp++;
				*mqc_bp=mqc_c>>19;
				mqc_c&=0x7ffff;
				mqc_ct=8;
			}
		}
	}
}

/// <summary>
/// Renormalize mqc_a and mqc_c while encoding.
/// </sumary>
void mqc_renorme()
{
	do {
		mqc_a<<=1;
		mqc_c<<=1;
		mqc_ct--;
		if (mqc_ct==0) {
			mqc_byteout();
		}
	} while ((mqc_a&0x8000)==0);
}

/// <summary>
/// Encode the most probable symbol.
/// </sumary>
void mqc_codemps()
{
	mqc_a-=(*mqc_curctx)->qeval;
	if ((mqc_a&0x8000)==0) {
		if (mqc_a<(*mqc_curctx)->qeval) {
			mqc_a=(*mqc_curctx)->qeval;
		} else {
			mqc_c+=(*mqc_curctx)->qeval;
		}
		*mqc_curctx=(*mqc_curctx)->nmps;
		mqc_renorme();
	} else {
		mqc_c+=(*mqc_curctx)->qeval;
	}
}

/// <summary>
/// Encode the most least symbol.
/// </sumary>
void mqc_codelps()
{
	mqc_a-=(*mqc_curctx)->qeval;
	if (mqc_a<(*mqc_curctx)->qeval) {
		mqc_c+=(*mqc_curctx)->qeval;
	} else {
		mqc_a=(*mqc_curctx)->qeval;
	}
	*mqc_curctx=(*mqc_curctx)->nlps;
	mqc_renorme();
}

/// <summary>
/// Initialize encoder.
/// </sumary>
/// <param name="bp">Output buffer.</param>
void mqc_init_enc(unsigned char *bp)
{
	mqc_setcurctx(0);
	mqc_a=0x8000;
	mqc_c=0;
	mqc_bp=bp-1;
	mqc_ct=12;
	if (*mqc_bp==0xff) {
		mqc_ct=13;
	}
	mqc_start=bp;
}

/// <summary>
/// Set current context.
/// </sumary>
/// <param name="ctxno">Context number.</param>
void mqc_setcurctx(int ctxno)
{
	mqc_curctx=&mqc_ctxs[ctxno];
}

/// <summary>
/// Encode a symbol using the MQ-coder.
/// </summary>
/// <param name="d"> The symbol to be encoded (0 or 1).</param>
void mqc_encode(int d)
{
	if ((*mqc_curctx)->mps==d) {
		mqc_codemps();
	} else {
		mqc_codelps();
	}
}

/// <summary>
/// </sumary>
void mqc_setbits()
{
	unsigned int tempc=mqc_c+mqc_a;
	mqc_c|=0xffff;
	if (mqc_c>=tempc) {
		mqc_c-=0x8000;
	}
}

/// <summary>
/// Flush encoded data.
/// </sumary>
void mqc_flush()
{
	mqc_setbits();
	mqc_c<<=mqc_ct;
	mqc_byteout();
	mqc_c<<=mqc_ct;
	mqc_byteout();
	if (*mqc_bp!=0xff) {
		mqc_bp++;
	}
}

/// <summary>
/// </sumary>
int mqc_mpsexchange()
{
	int d;
	if (mqc_a<(*mqc_curctx)->qeval) {
		d=1-(*mqc_curctx)->mps;
		*mqc_curctx=(*mqc_curctx)->nlps;
	} else {
		d=(*mqc_curctx)->mps;
		*mqc_curctx=(*mqc_curctx)->nmps;
	}
	return d;
}

/// <summary>
/// </sumary>
int mqc_lpsexchange()
{
	int d;
	if (mqc_a<(*mqc_curctx)->qeval) {
		mqc_a=(*mqc_curctx)->qeval;
		d=(*mqc_curctx)->mps;
		*mqc_curctx=(*mqc_curctx)->nmps;
	} else {
		mqc_a=(*mqc_curctx)->qeval;
		d=1-(*mqc_curctx)->mps;
		*mqc_curctx=(*mqc_curctx)->nlps;
	}
	return d;
}

/// <summary>
/// Input a byte.
/// </sumary>
void mqc_bytein()
{
	if (mqc_bp!=mqc_end) {
		unsigned int c;
		if (mqc_bp+1!=mqc_end) {
			c=*(mqc_bp+1);
		} else {
			c=0xff;
		}
		if (*mqc_bp==0xff) {
			if (c>0x8f) {
				mqc_c+=0xff00;
				mqc_ct=8;
			} else {
				mqc_bp++;
				mqc_c+=c<<9;
				mqc_ct=7;
			}
		} else {
			mqc_bp++;
			mqc_c+=c<<8;
			mqc_ct=8;
		}
	} else {
		mqc_c+=0xff00;
		mqc_ct=8;
	}
}

/// <summary>
/// Renormalize mqc_a and mqc_c while decoding.
/// </sumary>
void mqc_renormd()
{
	do {
		if (mqc_ct==0) {
			mqc_bytein();
		}
		mqc_a<<=1;
		mqc_c<<=1;
		mqc_ct--;
	} while (mqc_a<0x8000);
}

/// <summary>
/// Initialize decoder.
/// </sumary>
void mqc_init_dec(unsigned char *bp, int len)
{
	mqc_setcurctx(0);
	mqc_start=bp;
	mqc_end=bp+len;
	mqc_bp=bp;
	mqc_c=*mqc_bp<<16;
	mqc_bytein();
	mqc_c<<=7;
	mqc_ct-=7;
	mqc_a=0x8000;
}

/// <summary>
/// Decode a symbol.
/// </sumary>
int mqc_decode()
{
	int d;
	mqc_a-=(*mqc_curctx)->qeval;
	if ((mqc_c>>16)<(*mqc_curctx)->qeval) {
		d=mqc_lpsexchange();
		mqc_renormd();
	} else {
		mqc_c-=(*mqc_curctx)->qeval<<16;
		if ((mqc_a&0x8000)==0) {
			d=mqc_mpsexchange();
			mqc_renormd();
		} else {
			d=(*mqc_curctx)->mps;
		}
	}
	return d;
}

/// <summary>
/// Reset states of all contexts.
/// </sumary>
void mqc_resetstates() {
	int i;
	for (i=0; i<MQC_NUMCTXS; i++) {
		mqc_ctxs[i]=mqc_states;
	}
}

/// <summary>
/// Set the state for a context.
/// </sumary>
/// <param name="ctxno">Context number</param>
/// <param name="msb">Most significant bit</param>
/// <param name="prob">Index to the probability of symbols</param>
void mqc_setstate(int ctxno, int msb, int prob) {
	mqc_ctxs[ctxno]=&mqc_states[msb+(prob<<1)];
}
