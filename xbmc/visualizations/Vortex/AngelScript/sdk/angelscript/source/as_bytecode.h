/*
   AngelCode Scripting Library
   Copyright (c) 2003-2006 Andreas Jönsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jönsson
   andreas@angelcode.com
*/


//
// as_bytecode.h
//
// A class for constructing the final byte code
//



#ifndef AS_BYTECODE_H
#define AS_BYTECODE_H

#include "as_config.h"
#include "as_bytecodedef.h"
#include "as_array.h"

BEGIN_AS_NAMESPACE

#define BYTECODE_SIZE  4
#define MAX_DATA_SIZE  8
#define MAX_INSTR_SIZE (BYTECODE_SIZE+MAX_DATA_SIZE)

class asCModule;
class asCScriptEngine;
class cByteInstruction;

class asCByteCode
{
public:
	asCByteCode();
	~asCByteCode();

	void ClearAll();

	int GetSize();

	void Finalize();

	int  Optimize();
	void ExtractLineNumbers();
	int  ResolveJumpAddresses();
	int  FindLabel(int label, cByteInstruction *from, cByteInstruction **dest, int *positionDelta);

	void AddPath(asCArray<cByteInstruction *> &paths, cByteInstruction *instr, int stackSize);

	void Output(asDWORD *array);
	void AddCode(asCByteCode *bc);

	void PostProcess();
	void DebugOutput(const char *name, asCModule *module, asCScriptEngine *engine);

	int  GetLastInstr();
	int  RemoveLastInstr();
	asDWORD GetLastInstrValueDW();

	void InsertIfNotExists(asCArray<int> &vars, int var);
	void GetVarsUsed(asCArray<int> &vars);
	bool IsVarUsed(int offset);
	void ExchangeVar(int oldOffset, int newOffset);

	void Label(short label);
	void Line(int line, int column);
	void Call(bcInstr bc, int funcID, int pop);
	void Alloc(bcInstr bc, int objID, int funcID, int pop);
	void Ret(int pop);
	void JmpP(int var, asDWORD max);

	int InsertFirstInstrDWORD(bcInstr bc, asDWORD param);
	int InsertFirstInstrQWORD(bcInstr bc, asQWORD param);
	int Instr(bcInstr bc);
	int InstrQWORD(bcInstr bc, asQWORD param);
	int InstrDOUBLE(bcInstr bc, double param);
	int InstrDWORD(bcInstr bc, asDWORD param);
	int InstrWORD(bcInstr bc, asWORD param);
	int InstrSHORT(bcInstr bc, short param);
	int InstrFLOAT(bcInstr bc, float param);
	int InstrINT(bcInstr bc, int param);
	int InstrW_W_W(bcInstr bc, int a, int b, int c);
	int InstrW_DW(bcInstr bc, asWORD a, asDWORD b);
	int InstrSHORT_DW(bcInstr bc, short a, asDWORD b);
	int InstrW_QW(bcInstr bc, asWORD a, asQWORD b);
	int InstrSHORT_QW(bcInstr bc, short a, asQWORD b);
	int InstrW_FLOAT(bcInstr bc, asWORD a, float b);
	int InstrW_W(bcInstr bc, int w, int b);

	int Pop (int numDwords);
	int Push(int numDwords);

	asCArray<int> lineNumbers;
	int largestStackUsed;

	static int SizeOfType(int type);

	void DefineTemporaryVariable(int varOffset);

protected:
	// Helpers for Optimize
	bool MatchPattern(cByteInstruction *curr);
	cByteInstruction *OptimizePattern(cByteInstruction *curr);
	bool CanBeSwapped(cByteInstruction *curr);
	bool IsCombination(cByteInstruction *curr, bcInstr bc1, bcInstr bc2);
	bool IsCombination(cByteInstruction *curr, bcInstr bc1, bcInstr bc2, bcInstr bc3);
	cByteInstruction *ChangeFirstDeleteNext(cByteInstruction *curr, bcInstr bc);
	cByteInstruction *DeleteFirstChangeNext(cByteInstruction *curr, bcInstr bc);
	cByteInstruction *DeleteInstruction(cByteInstruction *instr);
	void RemoveInstruction(cByteInstruction *instr);
	cByteInstruction *GoBack(cByteInstruction *curr);
	void InsertBefore(cByteInstruction *before, cByteInstruction *instr);
	bool RemoveUnusedValue(cByteInstruction *curr, cByteInstruction **next);
	bool IsTemporary(short offset);
	bool IsTempRegUsed(cByteInstruction *curr);
	bool IsTempVarRead(cByteInstruction *curr, int offset);
	bool PostponeInitOfTemp(cByteInstruction *curr, cByteInstruction **next);
	bool IsTempVarReadByInstr(cByteInstruction *curr, int var);
	bool IsTempVarOverwrittenByInstr(cByteInstruction *curr, int var);
	bool IsInstrJmpOrLabel(cByteInstruction *curr);

	int AddInstruction();
	int AddInstructionFirst();

	cByteInstruction *first;
	cByteInstruction *last;

	asCArray<int> temporaryVariables;
};

class cByteInstruction
{
public:
	cByteInstruction();

	void AddAfter(cByteInstruction *nextCode);
	void AddBefore(cByteInstruction *nextCode);
	void Remove();

	int  GetSize();
	int  GetStackIncrease();

	cByteInstruction *next;
	cByteInstruction *prev;

	bcInstr op;
	asQWORD arg;
	short wArg[3];
	int size;
	int stackInc;

	// Testing
	bool marked;
	int stackSize;
};

END_AS_NAMESPACE

#endif
