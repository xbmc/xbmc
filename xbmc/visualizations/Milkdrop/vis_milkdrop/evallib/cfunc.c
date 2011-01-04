/*
  LICENSE
  -------
Copyright 2005 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
    endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include <windows.h>
//#include <xtl.h>
#include <stdio.h>
#include <math.h>
#include "Compiler.h"
#include "eval.h"
#pragma warning(disable: 4312)

static float g_cmpaddtab[2]={0.0,1.0};
static double g_closefact = 0.00001, g_half=0.5;
extern double *nextBlock;


// FUNCTION CALL TEMPLATES - THESE ARE NEVER ACTUALLY CALLED,
// BUT COPIED AND MODIFIED.
//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_simpleValue(void)
{
  __asm 
  {
    mov eax, 0FFFFFFFFh;
    ret
  }
}
__declspec ( naked ) void _asm_simpleValue_end(void){}


//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_function3(void)
{
  __asm 
  {
    mov ecx, 0FFFFFFFFh
    call ecx
    push eax
    mov ecx, 0FFFFFFFFh
    call ecx           
    push eax
    mov ecx, 0FFFFFFFFh
    call ecx
    pop ebx
    pop ecx
    mov edx, 0FFFFFFFFh
    jmp edx                                     
  }
}
__declspec ( naked ) void _asm_function3_end(void){}


//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_function2(void)
{
  __asm 
  {
    mov ecx, 0FFFFFFFFh
    call ecx
    push eax
    mov ecx, 0FFFFFFFFh
    call ecx
    pop ebx
    mov ecx, 0FFFFFFFFh
    jmp ecx
  }                      
}
__declspec ( naked ) void _asm_function2_end(void){}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_function1(void)
{
  __asm 
  {
    mov ecx, 0FFFFFFFFh
    call ecx           
    mov ecx, 0FFFFFFFFh
    jmp ecx            
  }
}
__declspec ( naked ) void _asm_function1_end(void){}

// END FUNCTION CALL TEMPLATES



// our registers
static int a,b;
static double *res;


#define isnonzero(x) (fabs(x) > g_closefact)

//---------------------------------------------------------------------------------------------------------------
static double _rand(double x)
{
  if (x < 1.0) x=1.0;
  return (double)(rand()%(int)max(x,1.0));
}

//---------------------------------------------------------------------------------------------------------------
static double _band(double var, double var2)
{
return isnonzero(var) && isnonzero(var2) ? 1 : 0;
}

//---------------------------------------------------------------------------------------------------------------
static double _bor(double var, double var2)
{
return isnonzero(var) || isnonzero(var2) ? 1 : 0;
}

//---------------------------------------------------------------------------------------------------------------
static double _sig(double x, double constraint)
{
double t = (1+exp(-x*constraint));
return isnonzero(t) ? 1.0/t : 0;
}



//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_int(void)
{
  __asm mov dword ptr a, eax

  res = nextBlock++;

  _asm mov	edx, DWORD PTR a
  _asm fld	QWORD PTR [edx]
   _asm mov	edx, DWORD PTR res
   _asm fistp 	DWORD PTR [edx]
   _asm fild DWORD PTR[edx]
   _asm fstp QWORD PTR [edx]
   _asm mov eax, res
   _asm ret

/*
 MrC - The old version uses _ftol2_sse which stomps over our stack
  *res = (double) ((int)(*((double*)a)));
  __asm 
  {
    mov eax, res
    ret
  }
*/
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_asin(void)
{
  __asm mov dword ptr a, eax

  res = nextBlock++;

  *res = asin(*(double*)a);
  __asm 
  {
    mov eax, res
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_acos(void)
{
  __asm mov dword ptr a, eax

  res = nextBlock++;

  *res = acos(*(double*)a);
  __asm 
  {
    mov eax, res
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_atan(void)
{
  __asm mov dword ptr a, eax

  res = nextBlock++;

  *res = atan(*(double*)a);
  __asm 
  {
    mov eax, res
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_atan2(void)
{
  __asm 
  {
    mov dword ptr a, eax
    mov dword ptr b, ebx
  }

  res = nextBlock++;
  *res = atan2(*(double*)b, *(double*)a);
  __asm 
  {
    mov eax, res
    ret
  }
}


//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_sig(void)
{
  __asm 
  {
    mov dword ptr a, eax
    mov dword ptr b, ebx
  }
  res = nextBlock++;
  *res = _sig(*(double*)b, *(double*)a);
  __asm 
  {
    mov eax, dword ptr res
    ret
  }
}
//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_rand(void)
{
  __asm 
  {
    mov dword ptr a, eax
  }

  res = nextBlock++;
  *res = _rand(*(double*)a);

  __asm 
  {
    mov eax, res
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_band(void)
{
  __asm 
  {
    mov dword ptr a, eax
    mov dword ptr b, ebx
  }

  res = nextBlock++;
  *res = _band(*(double*)b, *(double*)a);
  __asm 
  {
    mov eax, res
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_bor(void)
{
  __asm 
  {
    mov dword ptr a, eax
    mov dword ptr b, ebx
  }

  res = nextBlock++;
  *res = _bor(*(double*)b, *(double*)a);
  __asm 
  {
    mov eax, res
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_pow(void)
{
  __asm 
  {
    mov dword ptr a, eax
    mov dword ptr b, ebx
  }

  res = nextBlock++;
  *res = pow(*(double*)b, *(double*)a);
  __asm 
  {
    mov eax, res
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_exp(void)
{
  __asm mov dword ptr a, eax

  res = nextBlock++;

  *res = exp(*(double*)a);
  __asm 
  {
    mov eax, res
    ret
  }
}










// these below are all asm, no loops, radness






//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_sin(void)
{
  __asm 
  {
    fld qword ptr [eax]
    mov eax, nextBlock
    fsin
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_cos(void)
{
  __asm 
  {
    fld qword ptr [eax]
    mov eax, nextBlock
    fcos
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_tan(void)
{
  __asm 
  {
    fld qword ptr [eax]
    mov eax, nextBlock
    fsincos
    fdiv
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_sqr(void)
{
  __asm 
  {
    fld qword ptr [eax]
    fld st(0)
    mov eax, nextBlock
    fmul
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_sqrt(void)
{
  __asm 
  {
    fld qword ptr [eax]
    mov eax, nextBlock
    fabs
    fsqrt
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}


//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_log(void)
{
  __asm 
  {
    fld1
    fldl2e
    fdiv
    fld qword ptr [eax]
    mov eax, nextBlock
    fyl2x
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_log10(void)
{
  __asm 
  {
    fld1
    fldl2t
    fdiv
    fld qword ptr [eax]
    mov eax, nextBlock
    fyl2x
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_abs(void)
{
  __asm 
  {
    fld qword ptr [eax]
    mov eax, nextBlock
    fabs
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}


//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_assign(void)
{
  __asm 
  {
    mov ecx, [eax]
    mov edx, [eax+4]
    mov [ebx], ecx
    mov [ebx+4], edx
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_add(void)
{
  __asm 
  {
    fld qword ptr [eax]
    mov eax, nextBlock
    fld qword ptr [ebx]
    fadd
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_sub(void)
{
  __asm 
  {
    fld qword ptr [ebx]
    fld qword ptr [eax]
    mov eax, nextBlock
    fsub
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_mul(void)
{
  __asm 
  {
    fld qword ptr [ebx]
    fld qword ptr [eax]
    mov eax, nextBlock
    fmul
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_div(void)
{
  __asm 
  {
    fld qword ptr [ebx]
    fdiv qword ptr [eax]
    mov eax, nextBlock
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_mod(void)
{
  __asm 
  {
    fld qword ptr [ebx]

    fld qword ptr [eax]
    fsub dword ptr [g_cmpaddtab+4]
    fabs
    fadd qword ptr [eax]
    fadd dword ptr [g_cmpaddtab+4]

    fmul qword ptr [g_half]

    mov ebx, nextBlock 

    fistp dword ptr [ebx]
    fistp dword ptr [ebx+4]
    mov eax, [ebx+4]
    xor edx, edx
    div dword ptr [ebx]
    mov [ebx], edx
    fild dword ptr [ebx]
    fstp qword ptr [ebx]
    mov eax, ebx
    add ebx, 8
    mov nextBlock, ebx // eax is still good
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_or(void)
{
  __asm 
  {
    fld qword ptr [ebx]
    fld qword ptr [eax]
    mov eax, nextBlock
    fistp qword ptr [eax]
    fistp qword ptr [eax+8]
    mov ebx, [eax]
    or [eax+4], ebx
    mov ebx, [eax+8]
    or [eax+12], ebx
    fild qword ptr [eax]
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_and(void)
{
  __asm 
  {
    fld qword ptr [ebx]
    fld qword ptr [eax]
    mov eax, nextBlock
    fistp qword ptr [eax]
    fistp qword ptr [eax+8]
    mov ebx, [eax]
    and [eax+4], ebx
    mov ebx, [eax+8]
    and [eax+12], ebx
    fild qword ptr [eax]
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_uplus(void)
{
  __asm 
  {
    mov ebx, nextBlock
    mov ecx, [eax]
    mov [ebx], ecx
    mov ecx, [eax+4]
    mov [ebx+4], ecx
    mov eax, ebx
    add ebx, 8
    mov nextBlock, ebx
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_uminus(void)
{
  __asm 
  {
    mov ebx, nextBlock
    mov ecx, [eax]
    mov [ebx], ecx
    mov ecx, [eax+4]
    xor ecx, 0x80000000
    mov [ebx+4], ecx
    mov eax, ebx
    add ebx, 8
    mov nextBlock, ebx
    ret
  }
}



//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_sign(void)
{
  __asm
  {
    fld qword ptr [eax]
    fld st(0)
    fabs
    mov eax, nextBlock
    fld qword ptr [g_closefact]
    fadd
    fdiv
    fstp qword ptr [eax]
    add eax, 8
    mov nextBlock, eax
    sub eax, 8
    ret
  }
}



//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_bnot(void)
{
  __asm
  {
    mov ebx, nextBlock
    fld qword ptr [g_closefact]
    fld qword ptr [eax]
    fabs
    fcompp
    fstsw ax
    shr eax, 6
    and eax, (1<<2)
    add eax, offset g_cmpaddtab
    fld dword ptr [eax]
    fstp qword ptr [ebx]   
    mov eax, ebx
    add ebx, 8
    mov nextBlock, ebx   
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_if(void)
{
  __asm
  {
    fld qword ptr [eax]
    fld qword ptr [ebx]
    fld qword ptr [g_closefact]
    fld qword ptr [ecx]
    fabs
    fcompp
    fstsw ax
    mov ebx, nextBlock
    fstp qword ptr [ebx]
    fstp qword ptr [ebx+8]
    shr eax, 5
    and eax, (1<<3)
    add eax, ebx
    fld qword ptr [eax]
    fstp qword ptr [ebx]   
    mov eax, ebx
    add ebx, 8
    mov nextBlock, ebx   
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_equal(vpod)
{
  __asm
  {
    fld qword ptr [g_closefact]
    fld qword ptr [eax]
    fld qword ptr [ebx]
    fsub
    fabs
    fcompp
    fstsw ax
    shr eax, 6
    and eax, (1<<2)
    add eax, offset g_cmpaddtab
    fld dword ptr [eax]
    mov eax, nextBlock
    fstp qword ptr [eax]   
    mov ebx, eax
    add ebx, 8
    mov nextBlock, ebx   
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_below(void)
{
  __asm
  {
    fld qword ptr [eax]
    fld qword ptr [ebx]
    mov ebx, nextBlock
    fcompp
    fstsw ax
    shr eax, 6
    and eax, (1<<2)
    add eax, offset g_cmpaddtab
    fld dword ptr [eax]
    fstp qword ptr [ebx]   
    mov eax, ebx
    add ebx, 8
    mov nextBlock, ebx   
    ret
  }
}

//---------------------------------------------------------------------------------------------------------------
__declspec ( naked ) void _asm_above(void)
{
  __asm
  {
    fld qword ptr [ebx]
    fld qword ptr [eax]
    mov ebx, nextBlock
    fcompp
    fstsw ax
    shr eax, 6
    and eax, (1<<2)
    add eax, offset g_cmpaddtab
    fld dword ptr [eax]
    fstp qword ptr [ebx]   
    mov eax, ebx
    add ebx, 8
    mov nextBlock, ebx   
    ret
  }
}


__declspec ( naked ) void _asm_min(void)
{
  __asm 
  {
    fld qword ptr [eax]
    fld qword ptr [ebx]
    fld st(1)
    mov eax, nextBlock
    fld st(1)
    add eax, 8
    fsub
    mov nextBlock, eax
    fabs  // stack contains fabs(1-2),1,2
    fchs
    sub eax, 8
    fadd
    fadd
    fld qword ptr [g_half]
    fmul
    fstp qword ptr [eax]
    ret
  }
}

__declspec ( naked ) void _asm_max(void)
{
  __asm 
  {
    fld qword ptr [eax]
    fld qword ptr [ebx]
    fld st(1)
    mov eax, nextBlock
    fld st(1)
    add eax, 8
    fsub
    mov nextBlock, eax
    fabs  // stack contains fabs(1-2),1,2
    sub eax, 8
    fadd
    fadd
    fld qword ptr [g_half]
    fmul
    fstp qword ptr [eax]
    ret
  }
}
