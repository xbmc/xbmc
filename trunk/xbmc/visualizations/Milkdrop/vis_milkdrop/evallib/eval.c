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
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include "cal_tab.h"
#include "compiler.h"
#include "eval.h"

#define INTCONST 1
#define DBLCONST 2
#define HEXCONST 3
#define VARIABLE 4
#define OTHER    5

#define strnicmp _strnicmp
#define strcmpi  _strcmpi

int yyparse(char *exp);
void llinit(void);
int gettoken(char *lltb, int lltbsiz);
char yytext[256]="";
char expression[4096]="";
char lastVar[256]="";
int *errPtr;
int result;
int colCount=0;

varType *varTable;


//------------------------------------------------------------------------------
void *compileExpression(char *exp)
{
  int errv=0;
  errPtr=&errv;
  llinit();
  if (!yyparse(exp) && !*errPtr)
  {
    return (void*)result;
  }
  return 0;
}

// NOTE: be sure to initialize & clean these up from your app!	(InitializeCriticalSection(&g_eval_cs);)
CRITICAL_SECTION	g_eval_cs;
BOOL				g_eval_cs_valid;

//------------------------------------------------------------------------------
void resetVars(varType *vars)
{
	if (!g_eval_cs_valid)
	{
		g_eval_cs_valid = TRUE;
//		InitializeCriticalSection(&g_eval_cs);
	}

//  if (vars) EnterCriticalSection(&g_eval_cs);
  varTable=vars;
//  if (!vars) LeaveCriticalSection(&g_eval_cs);
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void setLastVar(void)
{
  gettoken(lastVar, sizeof lastVar);
}

//------------------------------------------------------------------------------
int setVar(int varNum, double value)
{
  int i=varNum;
  if (varNum < 0)
    for (i=0;i<EVAL_MAX_VARS;i++)
    {
      if (!varTable[i].name[0] || !strnicmp(varTable[i].name,lastVar,sizeof(varTable[i].name)))
          break;
    }
  if (i==EVAL_MAX_VARS) return -1;
  if (!varTable[i].name[0]) 
  {
    strncpy(varTable[i].name,lastVar,sizeof(varTable[i].name));
    varTable[i].value = value;
  }
  return i;
}

//------------------------------------------------------------------------------
int getVar(int varNum)
{
  if (varNum < 0)
	  return createCompiledValue(0, NULL);

  return createCompiledValue(0, &(varTable[varNum].value));
}

//------------------------------------------------------------------------------
double *getVarPtr(char *var)
{
  int i;
  for (i=0;i<EVAL_MAX_VARS;i++)
          if (!strnicmp(varTable[i].name, yytext,sizeof(varTable[i].name)))
                  return &(varTable[i].value);
  return NULL;
}

//------------------------------------------------------------------------------
double *registerVar(char *var)
{
  int i;
  for (i=0;i<EVAL_MAX_VARS;i++)
      if (!varTable[i].name[0] || 
        !strnicmp(varTable[i].name,var,sizeof(varTable[i].name)))
          break;
  if (i==EVAL_MAX_VARS) return NULL;
  if (!varTable[i].name[0])
  {
    strncpy(varTable[i].name,var,sizeof(varTable[i].name));
    varTable[i].value = 0.0;
  }
  return &(varTable[i].value);
}

//------------------------------------------------------------------------------
int translate(int type)
{
  int v;
  int n;
  *yytext = 0;
  gettoken(yytext, sizeof yytext);

  switch (type)
  {
    case INTCONST: return createCompiledValue((double)atoi(yytext), NULL);
    case DBLCONST: return createCompiledValue((double)atof(yytext), NULL);
    case HEXCONST:
      v=0;
      n=0;
      while (1)
      {
        int a=yytext[n++];
        if (a >= '0' && a <= '9') v+=a-'0';
        else if (a >= 'A' && a <= 'F') v+=10+a-'A';
        else if (a >= 'a' && a <= 'f') v+=10+a-'a';
        else break;
        v<<=4;
      }
		return createCompiledValue((double)v, NULL);
  }
  return 0;
}

//------------------------------------------------------------------------------
int objectId(int nParams)
{
  switch (nParams)
  {
    case 1: return FUNCTION1;
    case 2: return FUNCTION2;
    case 3: return FUNCTION3;
  }
  return IDENTIFIER;
}

//------------------------------------------------------------------------------
int lookup(int *typeOfObject)
{
  int i;
  gettoken(yytext, sizeof yytext);
  for (i=0;i<EVAL_MAX_VARS;i++)
    if (!strcmpi(varTable[i].name, yytext))
    {
      *typeOfObject = IDENTIFIER;
      return i;
    }
  for (i=0;i<sizeof(fnTable)/sizeof(functionType);i++)
  {
    if (!strcmpi(fnTable[i].name, yytext))
    {
      *typeOfObject = objectId(fnTable[i].nParams);
      return i;
    }
  }
  *typeOfObject = IDENTIFIER;
  setLastVar();
  i = setVar(-1, 0);
  //(*errPtr)++;
  return i;
}



//---------------------------------------------------------------------------
void count(void)
{
  gettoken(yytext, sizeof yytext);
  colCount+=strlen(yytext);
}

//---------------------------------------------------------------------------
int yyerror(char *txt)
{
  *errPtr = colCount;
  return 0;
}