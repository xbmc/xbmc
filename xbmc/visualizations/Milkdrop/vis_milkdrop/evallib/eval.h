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
#ifndef __EVAL_H
#define __EVAL_H

#ifdef __cplusplus
extern "C" {
#endif

  // stuff that apps will want to use
#define EVAL_MAX_VARS 103//90
        // note: in order for presets not to break, this 
        // value must be equal to the number of built-in
        // per-frame variables (see CState::var_pf_*), 
        // PLUS 30.  otherwise, the presets might try to
        // create too many of their own new custom per-
        // frame variables, and there wouldn't be room.
        // 
        // A good preset to test with is Krash & Telek - Real Noughts and Crosses (random ending),
        // as it uses exactly 30 custom per-frame vars.
        // 
        // History:         # pf vars  EVAL_MAX_VARS  # left for user (should never decrease!)
        // ---------------  ---------  -------------  ----------------------------------------
        //  Milkdrop 1.03   60         90             30
        //  Milkdrop 1.04   73         103            30

typedef struct 
{
  char name[8];
  double value;
} varType;

void resetVars(varType *vars);
double *getVarPtr(char *varName);
double *registerVar(char *varName);


// other shat

extern varType *varTable;
extern int *errPtr;
extern int colCount;
extern int result;

int setVar(int varNum, double value);
int getVar(int varNum);
void *compileExpression(char *txt);



#ifdef __cplusplus
}
#endif

#endif