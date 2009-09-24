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

#include "dwt.h"
#include "int.h"
#include "fix.h"
#include <stdlib.h>

#define S(i) a[x*(i)*2]
#define D(i) a[x*(1+(i)*2)]
#define S_(i) ((i)<0?S(0):((i)>=sn?S(sn-1):S(i)))
#define D_(i) ((i)<0?D(0):((i)>=dn?D(dn-1):D(i)))

/// <summary>
/// This table contains the norms of the 5-3 wavelets for different bands. 
/// </summary>
double dwt_norms[4][10]={
    {1.000, 1.500, 2.750, 5.375, 10.68, 21.34, 42.67, 85.33, 170.7, 341.3},
    {1.038, 1.592, 2.919, 5.703, 11.33, 22.64, 45.25, 90.48, 180.9},
    {1.038, 1.592, 2.919, 5.703, 11.33, 22.64, 45.25, 90.48, 180.9},
    {.7186, .9218, 1.586, 3.043, 6.019, 12.01, 24.00, 47.97, 95.93}
};

/// <summary>
/// This table contains the norms of the 9-7 wavelets for different bands. 
/// </summary>
double dwt_norms_real[4][10]={
    {1.000, 1.965, 4.177, 8.403, 16.90, 33.84, 67.69, 135.3, 270.6, 540.9},
    {2.022, 3.989, 8.355, 17.04, 34.27, 68.63, 137.3, 274.6, 549.0},
    {2.022, 3.989, 8.355, 17.04, 34.27, 68.63, 137.3, 274.6, 549.0},
    {2.080, 3.865, 8.307, 17.18, 34.71, 69.59, 139.3, 278.6, 557.2}
};

/// <summary>
/// Forward lazy transform.
/// </summary>
void dwt_deinterleave(int *a, int n, int x) {
    int dn, sn, i;
    int *b;
    dn=n/2;
    sn=(n+1)/2;
    b=(int*)malloc(n*sizeof(int));
    for (i=0; i<sn; i++)
        b[i]=a[2*i*x];
    for (i=0; i<dn; i++)
        b[sn+i]=a[(2*i+1)*x];
    for (i=0; i<n; i++)
        a[i*x]=b[i];
    free(b);
}

/// <summary>
/// Inverse lazy transform.
/// </summary>
void dwt_interleave(int *a, int n, int x) {
    int dn, sn, i;
    int *b;
    dn=n/2;
    sn=(n+1)/2;
    b=(int*)malloc(n*sizeof(int));
    for (i=0; i<sn; i++)
        b[2*i]=a[i*x];
    for (i=0; i<dn; i++)
        b[2*i+1]=a[(sn+i)*x];
    for (i=0; i<n; i++)
        a[i*x]=b[i];
    free(b);
}

/// <summary>
/// Forward 5-3 wavelet tranform in 1-D.
/// </summary>
void dwt_encode_1(int *a, int n, int x)
{
    int dn, sn, i;
    dn=n/2;
    sn=1+(n-1)/2;
    for (i=0; i<dn; i++)
        D(i)-=(S_(i)+S_(i+1))>>1;
    for (i=0; i<sn; i++)
        S(i)+=(D_(i-1)+D_(i)+2)>>2;
    dwt_deinterleave(a, n, x);
}

/// <summary>
/// Inverse 5-3 wavelet tranform in 1-D.
/// </summary>
void dwt_decode_1(int *a, int n, int x)
{
    int dn, sn, i;
    dn=n/2;
    sn=1+(n-1)/2;
    dwt_interleave(a, n, x);
    for (i=0; i<sn; i++)
        S(i)-=(D_(i-1)+D_(i)+2)>>2;
    for (i=0; i<dn; i++)
        D(i)+=(S_(i)+S_(i+1))>>1;
}

/// <summary>
/// Forward 5-3 wavelet tranform in 2-D.
/// </summary>
void dwt_encode(int* a, int w, int h, int l)
{
    int i, j, rw, rh;
    for (i=0; i<l; i++) {
        rw=int_ceildivpow2(w, i);
        rh=int_ceildivpow2(h, i);
        for (j=0; j<rw; j++)
            dwt_encode_1(a+j, rh, w);
        for (j=0; j<rh; j++)
            dwt_encode_1(a+j*w, rw, 1);
    }
}

/// <summary>
/// Inverse 5-3 wavelet tranform in 2-D.
/// </summary>
void dwt_decode(int* a, int w, int h, int l)
{
    int i, j, rw, rh;
    for (i=l-1; i>=0; i--) {
        rw=int_ceildivpow2(w, i);
        rh=int_ceildivpow2(h, i);
        for (j=0; j<rh; j++)
            dwt_decode_1(a+j*w, rw, 1);
        for (j=0; j<rw; j++)
            dwt_decode_1(a+j, rh, w);
    }
}

/// <summary>
/// Get gain of 5-3 wavelet transform.
/// </summary>
int dwt_getgain(int orient) {
    if (orient==0) return 0;
    if (orient==1 || orient==2) return 1;
    return 2;
}

/// <summary>
/// Get norm of 5-3 wavelet.
/// </summary>
double dwt_getnorm(int level, int orient) {
    return dwt_norms[orient][level];
}

/// <summary>
/// Forward 9-7 wavelet transform in 1-D.
/// </summary>
void dwt_encode_1_real(int *a, int n, int x)
{
    int dn, sn, i;
    dn=n/2;
    sn=1+(n-1)/2;
    for (i=0; i<dn; i++)
        D(i)-=fix_mul(S_(i)+S_(i+1), 12993);
    for (i=0; i<sn; i++)
        S(i)-=fix_mul(D_(i-1)+D_(i), 434);
    for (i=0; i<dn; i++)
        D(i)+=fix_mul(S_(i)+S_(i+1), 7233);
    for (i=0; i<sn; i++)
        S(i)+=fix_mul(D_(i-1)+D_(i), 3633);
    for (i=0; i<dn; i++)
        D(i)=fix_mul(D(i), 5038);
    for (i=0; i<sn; i++)
        S(i)=fix_mul(S(i), 6660);
    dwt_deinterleave(a, n, x);
}

/// <summary>
/// Inverse 9-7 wavelet transform in 1-D.
/// </summary>
void dwt_decode_1_real(int *a, int n, int x)
{
    int dn, sn, i;
    dn=n/2;
    sn=1+(n-1)/2;
    dwt_interleave(a, n, x);
    for (i=0; i<sn; i++)
        S(i)=fix_mul(S(i), 10076);
    for (i=0; i<dn; i++)
        D(i)=fix_mul(D(i), 13320);
    for (i=0; i<sn; i++)
        S(i)-=fix_mul(D_(i-1)+D_(i), 3633);
    for (i=0; i<dn; i++)
        D(i)-=fix_mul(S_(i)+S_(i+1), 7233);
    for (i=0; i<sn; i++)
        S(i)+=fix_mul(D_(i-1)+D_(i), 434);
    for (i=0; i<dn; i++)
        D(i)+=fix_mul(S_(i)+S_(i+1), 12993);
}

/// <summary>
/// Forward 9-7 wavelet transform in 2-D.
/// </summary>
void dwt_encode_real(int* a, int w, int h, int l)
{
    int i, j, rw, rh;
    for (i=0; i<l; i++) {
        rw=int_ceildivpow2(w, i);
        rh=int_ceildivpow2(h, i);
        for (j=0; j<rw; j++)
            dwt_encode_1_real(a+j, rh, w);
        for (j=0; j<rh; j++)
            dwt_encode_1_real(a+j*w, rw, 1);
    }
}

/// <summary>
/// Inverse 9-7 wavelet transform in 2-D.
/// </summary>
void dwt_decode_real(int* a, int w, int h, int l)
{
    int i, j, rw, rh;
    for (i=l-1; i>=0; i--) {
        rw=int_ceildivpow2(w, i);
        rh=int_ceildivpow2(h, i);
        for (j=0; j<rh; j++)
            dwt_decode_1_real(a+j*w, rw, 1);
        for (j=0; j<rw; j++)
            dwt_decode_1_real(a+j, rh, w);
    }
}

/// <summary>
/// Get gain of 9-7 wavelet transform.
/// </summary>
int dwt_getgain_real(int orient) {
    return 0;
}

/// <summary>
/// Get norm of 9-7 wavelet.
/// </summary>
double dwt_getnorm_real(int level, int orient) {
    return dwt_norms_real[orient][level];
}
