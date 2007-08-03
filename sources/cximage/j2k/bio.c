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

#include "bio.h"
#include <setjmp.h>

unsigned char *bio_start, *bio_end, *bio_bp;
unsigned int bio_buf;
int bio_ct;

extern jmp_buf j2k_error;

/// <summary>
/// Number of bytes written.
/// </summary>
int bio_numbytes() {
    return bio_bp-bio_start;
}

/// <summary>
/// Init encoder.
/// </summary>
/// <param name="bp">Output buffer</param>
/// <param name="len">Output buffer length</param>
void bio_init_enc(unsigned char *bp, int len) {
    bio_start=bp;
    bio_end=bp+len;
    bio_bp=bp;
    bio_buf=0;
    bio_ct=8;
}

/// <summary>
/// Init decoder.
/// </summary>
/// <param name="bp">Input buffer</param>
/// <param name="len">Input buffer length</param>
void bio_init_dec(unsigned char *bp, int len) {
    bio_start=bp;
    bio_end=bp+len;
    bio_bp=bp;
    bio_buf=0;
    bio_ct=0;
}

/// <summary>
/// Write byte.
/// </summary>
void bio_byteout() {
    bio_buf=(bio_buf<<8)&0xffff;
    bio_ct=bio_buf==0xff00?7:8;
    if (bio_bp>=bio_end) longjmp(j2k_error, 1);
    *bio_bp++=bio_buf>>8;
}

/// <summary>
/// Read byte. 
/// </summary>
void bio_bytein() {
    bio_buf=(bio_buf<<8)&0xffff;
    bio_ct=bio_buf==0xff00?7:8;
    if (bio_bp>=bio_end) longjmp(j2k_error, 1);
    bio_buf|=*bio_bp++;
}

/// <summary>
/// Write bit.
/// </summary>
/// <param name="b">Bit to write (0 or 1)</param>
void bio_putbit(int b) {
    if (bio_ct==0) {
        bio_byteout();
    }
    bio_ct--;
    bio_buf|=b<<bio_ct;
}

/// <summary>
/// Read bit.
/// </summary>
int bio_getbit() {
    if (bio_ct==0) {
        bio_bytein();
    }
    bio_ct--;
    return (bio_buf>>bio_ct)&1;
}

/// <summary>
/// Write bits.
/// </summary>
/// <param name="v">Value of bits</param>
/// <param name="n">Number of bits to write</param>
void bio_write(int v, int n) {
    int i;
    for (i=n-1; i>=0; i--) {
        bio_putbit((v>>i)&1);
    }
}

/// <summary>
/// Read bits.
/// </summary>
/// <param name="n">Number of bits to read</param>
int bio_read(int n) {
    int i, v;
    v=0;
    for (i=n-1; i>=0; i--) {
        v+=bio_getbit()<<i;
    }
    return v;
}

/// <summary>
/// Flush bits.
/// </summary>
void bio_flush() {
    bio_ct=0;
    bio_byteout();
    if (bio_ct==7) {
        bio_ct=0;
        bio_byteout();
    }
}

/// <summary>
/// </summary>
void bio_inalign() {
    bio_ct=0;
    if ((bio_buf&0xff)==0xff) {
        bio_bytein();
        bio_ct=0;
    }
}
