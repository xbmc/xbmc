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

#define LLB_DSIZE (65536-64)
typedef struct _llBlock {
	struct _llBlock *next;
  int sizeused;
	char block[LLB_DSIZE];
} llBlock;

typedef struct _startPtr {
  struct _startPtr *next;
  void *startptr;
} startPtr;

typedef struct {
	llBlock *blocks;
  startPtr *startpts;
	} codeHandleType;

static llBlock *blocks_head = NULL;

double computTable[16384];
double *nextBlock=computTable;

extern CRITICAL_SECTION g_eval_cs;

static void *newBlock(int size);
static void freeBlocks(llBlock *start);


void _asm_sin(void);
void _asm_cos(void);
void _asm_tan(void);
void _asm_int(void);
void _asm_asin(void);
void _asm_acos(void);
void _asm_atan(void);
void _asm_atan2(void);
void _asm_sqr(void);
void _asm_sqrt(void);
void _asm_pow(void);
void _asm_exp(void);
void _asm_log(void);
void _asm_log10(void);
void _asm_abs(void);
void _asm_min(void);
void _asm_max(void);
void _asm_sig(void);
void _asm_sign(void);
void _asm_rand(void);
void _asm_band(void);
void _asm_bor(void);
void _asm_bnot(void);
void _asm_if(void);
void _asm_equal(void);
void _asm_below(void);
void _asm_above(void);
void _asm_assign(void);
void _asm_add(void);
void _asm_sub(void);
void _asm_mul(void);
void _asm_div(void);
void _asm_mod(void);
void _asm_or(void);
void _asm_and(void);
void _asm_uplus(void);
void _asm_uminus(void);
void _asm_function3(void);
void _asm_function3_end(void);
void _asm_function2(void);
void _asm_function2_end(void);
void _asm_function1(void);
void _asm_function1_end(void);
void _asm_simpleValue(void);
void _asm_simpleValue_end(void);

functionType fnTable[27] = {{ "sin",   _asm_sin,   1 },
                           { "cos",    _asm_cos,   1 },
                           { "tan",    _asm_tan,   1 },
                           { "int",    _asm_int,   1 },
                           { "asin",   _asm_asin,  1 },
                           { "acos",   _asm_acos,  1 },
                           { "atan",   _asm_atan,  1 },
                           { "atan2",  _asm_atan2, 2 },
                           { "sqr",    _asm_sqr,   1 },
                           { "sqrt",   _asm_sqrt,  1 },
                           { "pow",    _asm_pow,   2 },
                           { "exp",    _asm_exp,   1 },
                           { "log",    _asm_log,   1 },
                           { "log10",  _asm_log10, 1 },
                           { "abs",    _asm_abs,   1 },
                           { "min",    _asm_min,   2 },
                           { "max",    _asm_max,   2 },
													 { "sigmoid",_asm_sig,   2 } ,
													 { "sign",   _asm_sign,  1 } ,
													 { "rand",   _asm_rand,  1 } ,
													 { "band",   _asm_band,  2 } ,
													 { "bor",    _asm_bor,   2 } ,
													 { "bnot",   _asm_bnot,  1 } ,
													 { "if",     _asm_if,    3 },
													 { "equal",  _asm_equal, 2 },
													 { "below",  _asm_below, 2 },
													 { "above",  _asm_above, 2 },
                           };


//---------------------------------------------------------------------------------------------------------------
void *realAddress(void *fn)
{
#if defined(_DEBUG)
	unsigned char *ptr = (char *)fn;
  if(*ptr == 0xE9)
	  ptr += (*(int *)((ptr+1))+5);
	return ptr;
#else
  // Release Mode
	return fn;
#endif
}

//#define realAddress(a) a

//---------------------------------------------------------------------------------------------------------------
static void freeBlocks(llBlock *start)
{
  while (start)
	{
    llBlock *llB = start->next;
    VirtualFree(start, 0, MEM_RELEASE);
    start=llB;
	}
}

//---------------------------------------------------------------------------------------------------------------
static void *newBlock(int size)
{
  llBlock *llb;
  int alloc_size;
  if (blocks_head && (LLB_DSIZE - blocks_head->sizeused) >= size)
  {
    void *t=blocks_head->block+blocks_head->sizeused;
    blocks_head->sizeused+=size;
    return t;
  }
  alloc_size=sizeof(llBlock);
  if ((int)size > LLB_DSIZE) alloc_size += size - LLB_DSIZE;
  llb = (llBlock *)VirtualAlloc(NULL, alloc_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE); // grab bigger block if absolutely necessary (heh)
  llb->sizeused=size;
  llb->next = blocks_head;  
  blocks_head = llb;
  return llb->block;
}

//---------------------------------------------------------------------------------------------------------------
static int *findFBlock(char *p)
{
  while (*(int *)p != 0xFFFFFFFF) p++;
  return (int*)p;
}


//---------------------------------------------------------------------------------------------------------------
int createCompiledValue(double value, double *addrValue)
{
  int size;
  char *block;
  double *dupValue;
  int i =0;
  char *p;
  char txt[512];

  //size=(int)_asm_simpleValue_end-(int)_asm_simpleValue;
  size = 0x10;
  block=(char *)newBlock(size);

  if (addrValue == NULL)
	  *(dupValue = (double *)newBlock(sizeof(double))) = value;
  else
	  dupValue = addrValue;

  memcpy(block, realAddress(_asm_simpleValue), size);

  p = block;
  while (*(int *)p != 0xFFFFFFFF)
  {
	  txt[i++] = *p;
	  p++;
  };
  txt[i] = 0;

  *findFBlock(block)=(int)dupValue;

  return ((int)(block));

}

//---------------------------------------------------------------------------------------------------------------
int getFunctionAddress(int fntype, int fn)
{
  switch (fntype)
	{
  	case MATH_SIMPLE:
	  	switch (fn)
			{
			  case FN_ASSIGN:
				  return (int)realAddress(_asm_assign);
			  case FN_ADD:
				  return (int)realAddress(_asm_add);
			  case FN_SUB:
				  return (int)realAddress(_asm_sub);
			  case FN_MULTIPLY:
				  return (int)realAddress(_asm_mul);
			  case FN_DIVIDE:
				  return (int)realAddress(_asm_div);
			  case FN_MODULO:
				  return (int)realAddress(_asm_mod);
			  case FN_AND:
				  return (int)realAddress(_asm_and);
			  case FN_OR:
				  return (int)realAddress(_asm_or);
			  case FN_UPLUS:
				  return (int)realAddress(_asm_uplus);
			  case FN_UMINUS:
				  return (int)realAddress(_asm_uminus);
			}
	  case MATH_FN:
		  return (int)realAddress(fnTable[fn].afunc);
	}		
  return 0;
}


//---------------------------------------------------------------------------------------------------------------
int createCompiledFunction3(int fntype, int fn, int code1, int code2, int code3)
{
  int *p;
  int size;
  char *block;

//  size=(int)_asm_function3_end-(int)_asm_function3;
  size = 0x30;
  block=(char *)newBlock(size);

  memcpy(block, realAddress(_asm_function3), size);

  p=findFBlock(block); *p++=code1;
  p=findFBlock((char*)p); *p++=code2;
  p=findFBlock((char*)p); *p++=code3;
  p=findFBlock((char*)p); *p++=getFunctionAddress(fntype, fn);

  return ((int)(block));
}

//---------------------------------------------------------------------------------------------------------------
int createCompiledFunction2(int fntype, int fn, int code1, int code2)
{
  int *p;
  int size;
  char *block;

//  size=(int)_asm_function2_end-(int)_asm_function2;
	size = 0x20;
  block=(char *)newBlock(size);

  memcpy(block, realAddress(_asm_function2), size);

  p=findFBlock(block); *p++=code1;
  p=findFBlock((char*)p); *p++=code2;
  p=findFBlock((char*)p); *p++=getFunctionAddress(fntype, fn);

  return ((int)(block));
}


//---------------------------------------------------------------------------------------------------------------
int createCompiledFunction1(int fntype, int fn, int code)
{
  int size;
  int *p;
  char *block;

//  size=(int)_asm_function1_end-(int)_asm_function1;
  size = 0x20;
  block=(char *)newBlock(size);

  memcpy(block, realAddress(_asm_function1), size);

  p=findFBlock(block); *p++=code;
  p=findFBlock((char*)p); *p++=getFunctionAddress(fntype, fn);

  return ((int)(block));
}

//------------------------------------------------------------------------------
int compileCode(char *expression)
{
  char expr[4096];
  codeHandleType *handle;
  startPtr *scode=NULL;
  blocks_head=0;

  if (!expression || !*expression) return 0;
  if (!varTable) return 0;

  handle = (codeHandleType*)newBlock(sizeof(codeHandleType));

  if (!handle) return 0;
  
  memset(handle,0,sizeof(codeHandleType));

  while (*expression)
	{
    startPtr *tmp;
    char *expr_ptr;
    int l=4095;
    colCount=0;

    // single out segment
    while (*expression == ';' || *expression == ' ' || *expression == '\n' || 
           *expression == '\r' || *expression == '\t') expression++;
    if (!*expression) break;
    expr_ptr = expr;
	  while (l-->0) 
    {
      int a=*expression;
      if (a) expression++;
      if (!a || a == ';') break;
      if (a == '\n' || a=='\r' || a=='\t') a=' ';
      *expr_ptr++ = a;
    }
	  *expr_ptr = 0;

    // parse
    tmp=(startPtr*) newBlock(sizeof(startPtr));
    if (!tmp) break;
    tmp->startptr=compileExpression(expr);
    if (!tmp->startptr) { scode=NULL; break; }
    tmp->next=NULL;
    if (!scode) scode=handle->startpts=tmp;
    else
    {
      scode->next=tmp;
      scode=tmp;
    }
  }

  // check to see if failed on the first startingCode
  if (!scode)
  {
    freeBlocks(blocks_head);  // free blocks
    handle=NULL;              // return NULL (after resetting blocks_head)
  }
  else handle->blocks = blocks_head;

  blocks_head=0;

  return (int)handle;
}

//------------------------------------------------------------------------------
void executeCode(int handle)
{
  codeHandleType *h = (codeHandleType *)handle;
  startPtr *p;
  if (!h) return;
//  EnterCriticalSection(&g_eval_cs);
  p=h->startpts;
  while (p)
  {
    void *startPoint=p->startptr;
    if (!startPoint) break;
    p=p->next;
 	  nextBlock=computTable;
    __asm pusha // Lets cover our ass
    __asm call startPoint
    __asm popa
  }
//  LeaveCriticalSection(&g_eval_cs);
}

//------------------------------------------------------------------------------
void freeCode(int handle)
{
  codeHandleType *h = (codeHandleType *)handle;
  if (h != NULL)
  {
    freeBlocks(h->blocks);
  }
}

