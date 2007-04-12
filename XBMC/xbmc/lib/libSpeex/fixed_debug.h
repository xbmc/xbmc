/* Copyright (C) 2003 Jean-Marc Valin */
/**
   @file fixed_debug.h
   @brief Fixed-point operations with debugging
*/
/*
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

#ifndef FIXED_DEBUG_H
#define FIXED_DEBUG_H

#include <stdio.h>

extern long long spx_mips;
#define MIPS_INC spx_mips++,

#define VERIFY_SHORT(x) ((x)<=32767&&(x)>=-32768)
#define VERIFY_INT(x) ((x)<=2147483647LL&&(x)>=-2147483648LL)

#define SHR(a,shift) ((a) >> (shift))
#define SHL(a,shift) ((a) << (shift))

static inline short ADD16(int a, int b) 
{
   int res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "ADD16: inputs are not short: %d %d\n", a, b);
   }
   res = a+b;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "ADD16: output is not short: %d\n", res);
   spx_mips++;
   return res;
}
static inline short SUB16(int a, int b) 
{
   int res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "SUB16: inputs are not short: %d %d\n", a, b);
   }
   res = a-b;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "SUB16: output is not short: %d\n", res);
   spx_mips++;
   return res;
}

static inline int ADD32(long long a, long long b) 
{
   long long res;
   if (!VERIFY_INT(a) || !VERIFY_INT(b))
   {
      fprintf (stderr, "ADD32: inputs are not int: %d %d\n", (int)a, (int)b);
   }
   res = a+b;
   if (!VERIFY_INT(res))
      fprintf (stderr, "ADD32: output is not int: %d\n", (int)res);
   spx_mips++;
   return res;
}

static inline int SUB32(long long a, long long b) 
{
   long long res;
   if (!VERIFY_INT(a) || !VERIFY_INT(b))
   {
      fprintf (stderr, "SUB32: inputs are not int: %d %d\n", (int)a, (int)b);
   }
   res = a-b;
   if (!VERIFY_INT(res))
      fprintf (stderr, "SUB32: output is not int: %d\n", (int)res);
   spx_mips++;
   return res;
}

#define ADD64(a,b) (MIPS_INC(a)+(b))

#define PSHR(a,shift) (SHR((a)+(1<<((shift)-1)),shift))

/* result fits in 16 bits */
static inline short MULT16_16_16(int a, int b) 
{
   int res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_16: inputs are not short: %d %d\n", a, b);
   }
   res = a*b;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_16: output is not short: %d\n", res);
   spx_mips++;
   return res;
}

static inline int MULT16_16(int a, int b) 
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   if (!VERIFY_INT(res))
      fprintf (stderr, "MULT16_16: output is not int: %d\n", (int)res);
   spx_mips++;
   return res;
}
#define MULT16_16B(a,b)     (((short)(a))*((short)(b)))

#define MAC16_16(c,a,b)     (ADD32((c),MULT16_16((a),(b))))
#define MAC16_16_Q11(c,a,b)     (ADD32((c),SHR(MULT16_16((a),(b)),11)))

#define MULT16_32_Q12(a,b) ADD32(MULT16_16((a),SHR((b),12)), SHR(MULT16_16((a),((b)&0x00000fff)),12))
#define MULT16_32_Q13(a,b) ADD32(MULT16_16((a),SHR((b),13)), SHR(MULT16_16((a),((b)&0x00001fff)),13))
#define MULT16_32_Q14(a,b) ADD32(MULT16_16((a),SHR((b),14)), SHR(MULT16_16((a),((b)&0x00003fff)),14))

#define MULT16_32_Q11(a,b) ADD32(MULT16_16((a),SHR((b),11)), SHR(MULT16_16((a),((b)&0x000007ff)),11))
#define MAC16_32_Q11(c,a,b) ADD32(c,ADD32(MULT16_16((a),SHR((b),11)), SHR(MULT16_16((a),((b)&0x000007ff)),11)))

#define MULT16_32_Q15(a,b) ADD32(MULT16_16((a),SHR((b),15)), SHR(MULT16_16((a),((b)&0x00007fff)),15))
#define MAC16_32_Q15(c,a,b) ADD32(c,ADD32(MULT16_16((a),SHR((b),15)), SHR(MULT16_16((a),((b)&0x00007fff)),15)))

static inline int SATURATE(int a, int b)
{
   if (a>b)
      a=b;
   if (a<-b)
      a = -b;
   return a;
}

static inline short MULT16_16_Q11(int a, int b) 
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_Q11: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   res >>= 11;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_Q11: output is not short: %d*%d=%d\n", (int)a, (int)b, (int)res);
   spx_mips++;
   return res;
}
static inline short MULT16_16_Q13(int a, int b) 
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_Q13: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   res >>= 13;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_Q13: output is not short: %d*%d=%d\n", a, b, (int)res);
   spx_mips++;
   return res;
}
static inline short MULT16_16_Q14(int a, int b) 
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_Q14: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   res >>= 14;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_Q14: output is not short: %d\n", (int)res);
   spx_mips++;
   return res;
}
static inline short MULT16_16_Q15(int a, int b) 
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_Q15: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   res >>= 15;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_Q15: output is not short: %d\n", (int)res);
   spx_mips++;
   return res;
}

static inline short MULT16_16_P13(int a, int b) 
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_P13: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   res += 4096;
   if (!VERIFY_INT(res))
      fprintf (stderr, "MULT16_16_P13: overflow: %d*%d=%d\n", a, b, (int)res);
   res >>= 13;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_P13: output is not short: %d*%d=%d\n", a, b, (int)res);
   spx_mips++;
   return res;
}
static inline short MULT16_16_P14(int a, int b) 
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_P14: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   res += 8192;
   if (!VERIFY_INT(res))
      fprintf (stderr, "MULT16_16_P14: overflow: %d*%d=%d\n", a, b, (int)res);
   res >>= 14;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_P14: output is not short: %d*%d=%d\n", a, b, (int)res);
   spx_mips++;
   return res;
}
static inline short MULT16_16_P15(int a, int b) 
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_P15: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   res += 16384;
   if (!VERIFY_INT(res))
      fprintf (stderr, "MULT16_16_P15: overflow: %d*%d=%d\n", a, b, (int)res);
   res >>= 15;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_P15: output is not short: %d*%d=%d\n", a, b, (int)res);
   spx_mips++;
   return res;
}

#define MUL_16_32_R15(a,bh,bl) ADD32(MULT16_16((a),(bh)), SHR(MULT16_16((a),(bl)),15))


static inline int DIV32_16(long long a, long long b) 
{
   long long res;
   if (b==0)
   {
      fprintf(stderr, "DIV32_16: divide by zero: %d/%d\n", (int)a, (int)b);
      return 0;
   }
   if (!VERIFY_INT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "DIV32_16: inputs are not int/short: %d %d\n", (int)a, (int)b);
   }
   res = a/b;
   if (!VERIFY_SHORT(res))
   {
      fprintf (stderr, "DIV32_16: output is not short: %d / %d = %d\n", (int)a,(int)b,(int)res);
      if (res>32767)
         res = 32767;
      if (res<-32768)
         res = -32768;
   }
   spx_mips++;
   return res;
}
static inline int DIV32(long long a, long long b) 
{
   long long res;
   if (b==0)
   {
      fprintf(stderr, "DIV32: divide by zero: %d/%d\n", (int)a, (int)b);
      return 0;
   }

   if (!VERIFY_INT(a) || !VERIFY_INT(b))
   {
      fprintf (stderr, "DIV32: inputs are not int/short: %d %d\n", (int)a, (int)b);
   }
   res = a/b;
   if (!VERIFY_INT(res))
      fprintf (stderr, "DIV32: output is not int: %d\n", (int)res);
   spx_mips++;
   return res;
}



#endif
