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
// as_bytecode.cpp
//
// A class for constructing the final byte code
//

#include <memory.h> // memcpy()
#include <string.h> // some compilers declare memcpy() here
#include <assert.h>
#include <stdio.h> // fopen(), fprintf(), fclose()

#include "as_config.h"
#include "as_bytecode.h"
#include "as_debug.h" // mkdir()
#include "as_array.h"
#include "as_string.h"
#include "as_module.h"
#include "as_scriptengine.h"

BEGIN_AS_NAMESPACE

asCByteCode::asCByteCode()
{
	first = 0;
	last  = 0;
	largestStackUsed = -1;
}

asCByteCode::~asCByteCode()
{
	ClearAll();
}

void asCByteCode::Finalize()
{
	// verify the bytecode
	PostProcess();

	// Optimize the code (optionally)
	Optimize();

	// Resolve jumps
	ResolveJumpAddresses();

	// Build line numbers buffer
	ExtractLineNumbers();
}

void asCByteCode::ClearAll()
{
	cByteInstruction *del = first;

	while( del ) 
	{
		first = del->next;
		delete del;
		del = first;
	}

	first = 0;
	last = 0;

	lineNumbers.SetLength(0);

	largestStackUsed = -1;

	temporaryVariables.SetLength(0);
}

void asCByteCode::InsertIfNotExists(asCArray<int> &vars, int var)
{
	if( !vars.Find(var) )
		vars.PushLast(var);
}

void asCByteCode::GetVarsUsed(asCArray<int> &vars)
{
	cByteInstruction *curr = first;
	while( curr )
	{
		if( bcTypes[curr->op] == BCTYPE_wW_rW_rW_ARG )
		{
			InsertIfNotExists(vars, curr->wArg[0]);
			InsertIfNotExists(vars, curr->wArg[1]);
			InsertIfNotExists(vars, curr->wArg[2]);
		}
		else if( bcTypes[curr->op] == BCTYPE_rW_ARG    ||
			     bcTypes[curr->op] == BCTYPE_wW_ARG    ||
				 bcTypes[curr->op] == BCTYPE_wW_W_ARG  ||
			     bcTypes[curr->op] == BCTYPE_rW_DW_ARG ||
			     bcTypes[curr->op] == BCTYPE_wW_DW_ARG ||
			     bcTypes[curr->op] == BCTYPE_wW_QW_ARG )
		{
			InsertIfNotExists(vars, curr->wArg[0]);
		}
		else if( bcTypes[curr->op] == BCTYPE_wW_rW_ARG ||
				 bcTypes[curr->op] == BCTYPE_rW_rW_ARG ||
				 bcTypes[curr->op] == BCTYPE_wW_rW_DW_ARG )
		{
			InsertIfNotExists(vars, curr->wArg[0]);
			InsertIfNotExists(vars, curr->wArg[1]);
		}
		else if( bcTypes[curr->op] == BCTYPE_W_rW_ARG )
		{
			InsertIfNotExists(vars, curr->wArg[1]);
		}

		curr = curr->next;
	}
}

bool asCByteCode::IsVarUsed(int offset)
{
	cByteInstruction *curr = first;
	while( curr )
	{
		// Verify all ops that use variables
		if( bcTypes[curr->op] == BCTYPE_wW_rW_rW_ARG )
		{
			if( curr->wArg[0] == offset || curr->wArg[1] == offset || curr->wArg[2] == offset )
				return true;
		}
		else if( bcTypes[curr->op] == BCTYPE_rW_ARG    ||
                 bcTypes[curr->op] == BCTYPE_wW_ARG    ||
				 bcTypes[curr->op] == BCTYPE_wW_W_ARG  ||
				 bcTypes[curr->op] == BCTYPE_rW_DW_ARG ||
				 bcTypes[curr->op] == BCTYPE_wW_DW_ARG ||
				 bcTypes[curr->op] == BCTYPE_wW_QW_ARG )
		{
			if( curr->wArg[0] == offset )
				return true;
		}
		else if( bcTypes[curr->op] == BCTYPE_wW_rW_ARG ||
				 bcTypes[curr->op] == BCTYPE_rW_rW_ARG ||
				 bcTypes[curr->op] == BCTYPE_wW_rW_DW_ARG )
		{
			if( curr->wArg[0] == offset || curr->wArg[1] == offset )
				return true;
		}
		else if( bcTypes[curr->op] == BCTYPE_W_rW_ARG )
		{
			if( curr->wArg[1] == offset )
				return true;
		}

		curr = curr->next;
	}

	return false;
}

void asCByteCode::ExchangeVar(int oldOffset, int newOffset)
{
	cByteInstruction *curr = first;
	while( curr )
	{
		// Verify all ops that use variables
		if( bcTypes[curr->op] == BCTYPE_wW_rW_rW_ARG )
		{
			if( curr->wArg[0] == oldOffset )
				curr->wArg[0] = newOffset;
			if( curr->wArg[1] == oldOffset )
				curr->wArg[1] = newOffset;
			if( curr->wArg[2] == oldOffset )
				curr->wArg[2] = newOffset;
		}
		else if( bcTypes[curr->op] == BCTYPE_rW_ARG    ||
                 bcTypes[curr->op] == BCTYPE_wW_ARG    ||
				 bcTypes[curr->op] == BCTYPE_wW_W_ARG  ||
				 bcTypes[curr->op] == BCTYPE_rW_DW_ARG ||
				 bcTypes[curr->op] == BCTYPE_wW_DW_ARG ||
				 bcTypes[curr->op] == BCTYPE_wW_QW_ARG )
		{
			if( curr->wArg[0] == oldOffset )
				curr->wArg[0] = newOffset;
		}
		else if( bcTypes[curr->op] == BCTYPE_wW_rW_ARG ||
				 bcTypes[curr->op] == BCTYPE_rW_rW_ARG )
		{
			if( curr->wArg[0] == oldOffset )
				curr->wArg[0] = newOffset;
			if( curr->wArg[1] == oldOffset )
				curr->wArg[1] = newOffset;
		}
		else if( bcTypes[curr->op] == BCTYPE_W_rW_ARG )
		{
			if( curr->wArg[1] == oldOffset )
				curr->wArg[1] = newOffset;
		}

		curr = curr->next;
	}
}

void asCByteCode::AddPath(asCArray<cByteInstruction *> &paths, cByteInstruction *instr, int stackSize)
{
	if( instr->marked )
	{
		// Verify the size of the stack
		assert(instr->stackSize == stackSize);
	}
	else
	{
		// Add the destination to the code paths
		instr->marked = true;
		instr->stackSize = stackSize;
		paths.PushLast(instr);
	}
}

bool asCByteCode::IsCombination(cByteInstruction *curr, bcInstr bc1, bcInstr bc2)
{
	if( curr->op == bc1 && curr->next && curr->next->op == bc2 )
		return true;
	
	return false;
}

bool asCByteCode::IsCombination(cByteInstruction *curr, bcInstr bc1, bcInstr bc2, bcInstr bc3)
{
	if( curr->op == bc1 && 
		curr->next && curr->next->op == bc2 &&
		curr->next->next && curr->next->next->op == bc3 )
		return true;
	
	return false;
}

cByteInstruction *asCByteCode::ChangeFirstDeleteNext(cByteInstruction *curr, bcInstr bc)
{
	curr->op = bc;
	
	if( curr->next ) DeleteInstruction(curr->next);
	
	// Continue optimization with the instruction before the altered one
	if( curr->prev ) 
		return curr->prev;
	else
		return curr;
}

cByteInstruction *asCByteCode::DeleteFirstChangeNext(cByteInstruction *curr, bcInstr bc)
{
	assert( curr->next );
	
	cByteInstruction *instr = curr->next;
	instr->op = bc;
	
	DeleteInstruction(curr);
	
	// Continue optimization with the instruction before the altered one
	if( instr->prev ) 
		return instr->prev;
	else
		return instr;
}

void asCByteCode::InsertBefore(cByteInstruction *before, cByteInstruction *instr)
{
	assert(instr->next == 0);
	assert(instr->prev == 0);

	if( before->prev ) before->prev->next = instr;
	instr->prev = before->prev;
	before->prev = instr;
	instr->next = before;

	if( first == before ) first = instr;
}

void asCByteCode::RemoveInstruction(cByteInstruction *instr)
{
	if( instr == first ) first = first->next;
	if( instr == last ) last = last->prev;

	if( instr->prev ) instr->prev->next = instr->next;
	if( instr->next ) instr->next->prev = instr->prev;

	instr->next = 0;
	instr->prev = 0;
}

bool asCByteCode::CanBeSwapped(cByteInstruction *curr)
{
	if( !curr || !curr->next || !curr->next->next ) return false;
	if( curr->next->next->op != BC_SWAP4 ) return false;

	cByteInstruction *next = curr->next;

	if( curr->op != BC_PshC4 &&
		curr->op != BC_PshV4 &&
		curr->op != BC_PSF )
		return false;

	if( next->op != BC_PshC4 &&
		next->op != BC_PshV4 &&
		next->op != BC_PSF )
		return false;

	return true;
}

bool asCByteCode::MatchPattern(cByteInstruction *curr)
{
	if( !curr || !curr->next || !curr->next->next ) return false;

	if( curr->op != BC_PshC4 ) return false;

	asDWORD op = curr->next->next->op;
	if( op != BC_ADDi &&
		op != BC_MULi &&
		op != BC_ADDf &&
		op != BC_MULf ) return false;

	asDWORD val = curr->next->op;
	if( val != BC_PshV4 &&
		val != BC_PSF )
		return false;

	return true;
}

cByteInstruction *asCByteCode::OptimizePattern(cByteInstruction *curr)
{
	asDWORD op = curr->next->next->op;

	// Delete the operator instruction
	DeleteInstruction(curr->next->next);

	// Swap the two value instructions
	cByteInstruction *instr = curr->next;
	RemoveInstruction(instr);
	InsertBefore(curr, instr);

	return GoBack(instr);
}

cByteInstruction *asCByteCode::GoBack(cByteInstruction *curr)
{
	// Go back 2 instructions
	if( !curr ) return 0;
	if( curr->prev ) curr = curr->prev;
	if( curr->prev ) curr = curr->prev;
	return curr;
}

bool asCByteCode::PostponeInitOfTemp(cByteInstruction *curr, cByteInstruction **next)
{
	if( curr->op != BC_SetV4 || !IsTemporary(curr->wArg[0]) ) return false;

	// Move the initialization to just before it's use. 
	// Don't move it beyond any labels or jumps.
	cByteInstruction *use = curr->next;
	while( use )
	{
		if( IsTempVarReadByInstr(use, curr->wArg[0]) )
			break;

		if( IsTempVarOverwrittenByInstr(use, curr->wArg[0]) )
			return false;

		if( IsInstrJmpOrLabel(use) )
			return false;

		use = use->next;
	}

	if( use && use->prev != curr )
	{
		*next = curr->next;

		// Move the instruction
		RemoveInstruction(curr);
		InsertBefore(use, curr);

		// Try a RemoveUnusedValue to see if it can be combined with the other 
		cByteInstruction *temp;
		if( RemoveUnusedValue(curr, &temp) )
		{
			*next = GoBack(*next);
			return true;
		}
		
		// Return the instructions to its original position as it wasn't useful
		RemoveInstruction(curr);
		InsertBefore(*next, curr);
	}

	return false;
}

bool asCByteCode::RemoveUnusedValue(cByteInstruction *curr, cByteInstruction **next)
{
	// The value isn't used for anything
	if( (bcTypes[curr->op] == BCTYPE_wW_rW_rW_ARG ||
		 bcTypes[curr->op] == BCTYPE_wW_rW_ARG    ||
		 bcTypes[curr->op] == BCTYPE_wW_rW_DW_ARG ||
		 bcTypes[curr->op] == BCTYPE_wW_ARG       ||
		 bcTypes[curr->op] == BCTYPE_wW_DW_ARG    ||
		 bcTypes[curr->op] == BCTYPE_wW_QW_ARG) &&
		IsTemporary(curr->wArg[0]) &&
		!IsTempVarRead(curr, curr->wArg[0]) )
	{
		if( curr->op == BC_LdGRdR4 && IsTempRegUsed(curr) )
		{
			curr->op = BC_LDG;
			curr->wArg[0] = curr->wArg[1];
			*next = GoBack(curr);
			return true;
		}

		*next = GoBack(DeleteInstruction(curr));
		return true;
	}

	// TODO: There should be one for doubles as well
	// The value is immediately used and then never again
	if( curr->op == BC_SetV4 &&
		curr->next && 
		(curr->next->op == BC_CMPi ||
		 curr->next->op == BC_CMPf ||
		 curr->next->op == BC_CMPu) &&
		curr->wArg[0] == curr->next->wArg[1] &&
		(IsTemporary(curr->wArg[0]) &&                       // The variable is temporary and never used again
		 !IsTempVarRead(curr->next, curr->wArg[0])) )
	{
		if(      curr->next->op == BC_CMPi ) curr->next->op = BC_CMPIi;
		else if( curr->next->op == BC_CMPf ) curr->next->op = BC_CMPIf;
		else if( curr->next->op == BC_CMPu ) curr->next->op = BC_CMPIu;
		curr->next->size = SizeOfType(BCT_CMPIi);
		curr->next->arg = curr->arg;
		*next = GoBack(DeleteInstruction(curr));
		return true;
	}
	
	// The value is immediately used and then never again
	if( curr->op == BC_SetV4 &&
		curr->next && 
		(curr->next->op == BC_ADDi ||
		 curr->next->op == BC_SUBi ||
		 curr->next->op == BC_MULi ||
		 curr->next->op == BC_ADDf ||
		 curr->next->op == BC_SUBf ||
		 curr->next->op == BC_MULf) &&
		curr->wArg[0] == curr->next->wArg[2] &&
		(curr->next->wArg[0] == curr->wArg[0] ||     // The variable is overwritten
		 IsTemporary(curr->wArg[0]) &&                       // The variable is temporary and never used again
		 !IsTempVarRead(curr->next, curr->wArg[0])) )
	{
		if(      curr->next->op == BC_ADDi ) curr->next->op = BC_ADDIi;
		else if( curr->next->op == BC_SUBi ) curr->next->op = BC_SUBIi;
		else if( curr->next->op == BC_MULi ) curr->next->op = BC_MULIi;
		else if( curr->next->op == BC_ADDf ) curr->next->op = BC_ADDIf;
		else if( curr->next->op == BC_SUBf ) curr->next->op = BC_SUBIf;
		else if( curr->next->op == BC_MULf ) curr->next->op = BC_MULIf;
		curr->next->size = SizeOfType(BCT_ADDIi);
		curr->next->arg = curr->arg;
		*next = GoBack(DeleteInstruction(curr));
		return true;
	}

	if( curr->op == BC_SetV4 &&
		curr->next && 
		(curr->next->op == BC_ADDi ||
		 curr->next->op == BC_MULi ||
		 curr->next->op == BC_ADDf ||
		 curr->next->op == BC_MULf) &&
		curr->wArg[0] == curr->next->wArg[1] &&
		(curr->next->wArg[0] == curr->wArg[0] ||     // The variable is overwritten
		 IsTemporary(curr->wArg[0]) &&                       // The variable is temporary and never used again
		 !IsTempVarRead(curr->next, curr->wArg[0])) )
	{
		if(      curr->next->op == BC_ADDi ) curr->next->op = BC_ADDIi;
		else if( curr->next->op == BC_MULi ) curr->next->op = BC_MULIi;
		else if( curr->next->op == BC_ADDf ) curr->next->op = BC_ADDIf;
		else if( curr->next->op == BC_MULf ) curr->next->op = BC_MULIf;
		curr->next->size = SizeOfType(BCT_ADDIi);
		curr->next->arg = curr->arg;

		// The order of the operands are changed
		curr->next->wArg[1] = curr->next->wArg[2];

		*next = GoBack(DeleteInstruction(curr));
		return true;
	}

	// The values is immediately moved to another variable and then not used again
	if( (bcTypes[curr->op] == BCTYPE_wW_rW_rW_ARG || 
		 bcTypes[curr->op] == BCTYPE_wW_rW_DW_ARG) && 
		curr->next && curr->next->op == BC_CpyVtoV4 &&
		curr->wArg[0] == curr->next->wArg[1] && 
		IsTemporary(curr->wArg[0]) &&
		!IsTempVarRead(curr->next, curr->wArg[0]) )
	{
		curr->wArg[0] = curr->next->wArg[0];
		DeleteInstruction(curr->next);
		*next = GoBack(curr);
		return true;
	}

	// The constant value is immediately moved to another variable and then not used again
	if( curr->op == BC_SetV4 && curr->next && curr->next->op == BC_CpyVtoV4 &&
		curr->wArg[0] == curr->next->wArg[1] &&
		IsTemporary(curr->wArg[0]) &&
		!IsTempVarRead(curr->next, curr->wArg[0]) )
	{
		curr->wArg[0] = curr->next->wArg[0];
		DeleteInstruction(curr->next);
		*next = GoBack(curr);
		return true;
	}

	// The register is copied to a temp variable and then back to the register again without being used afterwards
	if( curr->op == BC_CpyRtoV4 && curr->next && curr->next->op == BC_CpyVtoR4 &&
		curr->wArg[0] == curr->next->wArg[0] &&
		IsTemporary(curr->wArg[0]) &&
		!IsTempVarRead(curr->next, curr->wArg[0]) )
	{
		// Delete both instructions
		DeleteInstruction(curr->next);
		*next = GoBack(DeleteInstruction(curr));
		return true;
	}

	// The global value is copied to a temp and then immediately pushed on the stack
	if( curr->op == BC_CpyGtoV4 && curr->next && curr->next->op == BC_PshV4 &&
		curr->wArg[0] == curr->next->wArg[0] &&
		IsTemporary(curr->wArg[0]) &&
		!IsTempVarRead(curr->next, curr->wArg[0]) )
	{
		curr->op = BC_PshG4;
		curr->size = SizeOfType(BCT_PshG4);
		curr->stackInc = bcStackInc[BC_PshG4];
		curr->wArg[0] = curr->wArg[1];
		DeleteInstruction(curr->next);
		*next = GoBack(curr);
		return true;
	}

	// The constant is copied to a temp and then immediately pushed on the stack
	if( curr->op == BC_SetV4 && curr->next && curr->next->op == BC_PshV4 &&
		curr->wArg[0] == curr->next->wArg[0] &&
		IsTemporary(curr->wArg[0]) &&
		!IsTempVarRead(curr->next, curr->wArg[0]) )
	{
		curr->op = BC_PshC4;
		curr->stackInc = bcStackInc[BC_PshC4];
		DeleteInstruction(curr->next);
		*next = GoBack(curr);
		return true;
	}

	// The constant is copied to a global variable and then never used again
	if( curr->op == BC_SetV4 && curr->next && curr->next->op == BC_CpyVtoG4 &&
		curr->wArg[0] == curr->next->wArg[1] &&
		IsTemporary(curr->wArg[0]) &&
		!IsTempVarRead(curr->next, curr->wArg[0]) )
	{
		curr->op = BC_SetG4;
		curr->size = SizeOfType(BCT_SetG4);
		curr->wArg[0] = curr->next->wArg[0];
		DeleteInstruction(curr->next);
		*next = GoBack(curr);
		return true;
	}

	return false;
}

bool asCByteCode::IsTemporary(short offset)
{
	for( asUINT n = 0; n < temporaryVariables.GetLength(); n++ )
		if( temporaryVariables[n] == offset )
			return true;

	return false;
}

int asCByteCode::Optimize()
{
	cByteInstruction *instr = first;
	while( instr )
	{
		cByteInstruction *curr = instr;
		instr = instr->next;

		// Remove or combine instructions 
		if( RemoveUnusedValue(curr, &instr) ) continue;

		// Post pone initializations so that they may be combined in the second pass
		if( PostponeInitOfTemp(curr, &instr) ) continue;

		// XXX x, YYY y, SWAP4 -> YYY y, XXX x
		if( CanBeSwapped(curr) )
		{
			// Delete SWAP4
			DeleteInstruction(instr->next);

			// Swap instructions
			RemoveInstruction(instr);
			InsertBefore(curr, instr);

			instr = GoBack(instr);
		}
		// PshC4 x, YYY y, OP -> YYY y, OPI x
		else if( MatchPattern(curr) )
			instr = OptimizePattern(curr);
		// SWAP4, OP -> OP
		else if( IsCombination(curr, BC_SWAP4, BC_ADDi) ||
				 IsCombination(curr, BC_SWAP4, BC_MULi) ||
				 IsCombination(curr, BC_SWAP4, BC_ADDf) ||
				 IsCombination(curr, BC_SWAP4, BC_MULf) )
			instr = GoBack(DeleteInstruction(curr));
		// PSF x, RDS4 -> PshV4 x
		else if( IsCombination(curr, BC_PSF, BC_RDS4) )
			instr = GoBack(ChangeFirstDeleteNext(curr, BC_PshV4));
		// RDS4, POP x -> POP x
		else if( IsCombination(curr, BC_RDS4, BC_POP) && instr->wArg[0] > 0 ) 
			instr = GoBack(DeleteInstruction(curr));
		// LDG x, WRTV4 y -> CpyVtoG4 y, x
		else if( IsCombination(curr, BC_LDG, BC_WRTV4) && !IsTempRegUsed(instr) )
		{
			curr->op = BC_CpyVtoG4;
			curr->size = SizeOfType(BCT_CpyVtoG4);
			curr->wArg[1] = instr->wArg[0];

			DeleteInstruction(instr);
			instr = GoBack(curr);
		}
		// LDG x, RDR4 y -> CpyGtoV4 y, x
		else if( IsCombination(curr, BC_LDG, BC_RDR4) )
		{
			if( !IsTempRegUsed(instr) )
				curr->op = BC_CpyGtoV4;
			else 
				curr->op = BC_LdGRdR4;
			curr->size = SizeOfType(BCT_CpyGtoV4);
			curr->wArg[1] = curr->wArg[0];
			curr->wArg[0] = instr->wArg[0];

			DeleteInstruction(instr);
			instr = GoBack(curr);
		}
		// LDV x, INCi -> IncVi x
		else if( IsCombination(curr, BC_LDV, BC_INCi) && !IsTempRegUsed(instr) )
		{
			curr->op = BC_IncVi;
			
			DeleteInstruction(instr);
			instr = GoBack(curr);
		}
		// LDV x, DECi -> DecVi x
		else if( IsCombination(curr, BC_LDV, BC_DECi) && !IsTempRegUsed(instr) )
		{
			curr->op = BC_DecVi;
			
			DeleteInstruction(instr);
			instr = GoBack(curr);
		}
		// POP a, RET b -> RET b
		else if( IsCombination(curr, BC_POP, BC_RET) )
		{
			// We don't combine the POP+RET because RET first restores
			// the previous stack pointer and then pops the arguments

			// Delete POP
			instr = GoBack(DeleteInstruction(curr));
		}
		// SUSPEND, SUSPEND -> SUSPEND
		// LINE, LINE -> LINE
		else if( IsCombination(curr, BC_SUSPEND, BC_SUSPEND) || 
			     IsCombination(curr, BC_LINE, BC_LINE) )
		{
			// Delete one of the instructions
			instr = GoBack(DeleteInstruction(curr));
		}
		// PUSH a, PUSH b -> PUSH a+b
		else if( IsCombination(curr, BC_PUSH, BC_PUSH) )
		{
			// Combine the two PUSH
			instr->wArg[0] = curr->wArg[0] + instr->wArg[0];
			// Delete current
			DeleteInstruction(curr);
			// Continue with the instruction before the one removed
			instr = GoBack(instr);
		}
		// PshC4 a, GETREF 0 -> PSF a
		else if( IsCombination(curr, BC_PshC4, BC_GETREF) && instr->wArg[0] == 0 )
		{
			// Convert PshC4 a, to PSF a
			curr->wArg[0] = (short)*ARG_DW(curr->arg);
			curr->size = SizeOfType(BCT_PSF);
			curr->op = BC_PSF;
			DeleteInstruction(instr);
			instr = GoBack(curr);
		}
		// YYY y, POP x -> POP x-1
		else if( (IsCombination(curr, BC_PshV4, BC_POP) ||
		          IsCombination(curr, BC_PshC4, BC_POP) ||
				  IsCombination(curr, BC_VAR  , BC_POP)) && instr->wArg[0] > 0 )
		{
			DeleteInstruction(curr);
			instr->wArg[0]--;
			instr = GoBack(instr);
		}
		// TODO: Adjust for pointer size
		// PshRPtr, POP x -> POP x - 1
		else if( (IsCombination(curr, BC_PshRPtr, BC_POP) ||
			      IsCombination(curr, BC_PSF    , BC_POP)) && instr->wArg[0] > 0 )
		{
			DeleteInstruction(curr);
			instr->wArg[0]--;
			instr = GoBack(instr);
		}
		// RDS8, POP 2 -> POP x-1
		else if( IsCombination(curr, BC_RDS8, BC_POP) && instr->wArg[0] > 1 )
		{
			DeleteInstruction(curr);
			instr->wArg[0]--;
			instr = GoBack(instr);
		}
		// YYY y, POP x -> POP x-2
		else if( IsCombination(curr, BC_SET8, BC_POP) && instr->wArg[0] > 1 )
		{
			DeleteInstruction(curr);
			instr->wArg[0] -= 2;
			instr = GoBack(instr);
		}
		// YYY y, POP x -> POP x+1
		else if( (IsCombination(curr, BC_ADDi, BC_POP) ||
		         IsCombination(curr, BC_SUBi, BC_POP)) && instr->wArg[0] > 0 )
		{
			// Delete current
			DeleteInstruction(curr);
			// Increase the POP
			instr->wArg[0]++;
			// Continue with the instruction before the one removed
			instr = GoBack(instr);
		}
		// POP 0 -> remove
		// PUSH 0 -> remove
		else if( (curr->op == BC_POP || curr->op == BC_PUSH ) && curr->wArg[0] == 0 )  
			instr = GoBack(DeleteInstruction(curr));
// Begin PATTERN
		// T**; J** +x -> J** +x
		else if( IsCombination(curr, BC_TZ , BC_JZ ) || 
			     IsCombination(curr, BC_TNZ, BC_JNZ) )
			instr = GoBack(DeleteFirstChangeNext(curr, BC_JNZ));
		else if( IsCombination(curr, BC_TNZ, BC_JZ ) ||
			     IsCombination(curr, BC_TZ , BC_JNZ) )
			instr = GoBack(DeleteFirstChangeNext(curr, BC_JZ));
		else if( IsCombination(curr, BC_TS , BC_JZ ) ||
			     IsCombination(curr, BC_TNS, BC_JNZ) )
			instr = GoBack(DeleteFirstChangeNext(curr, BC_JNS));
		else if( IsCombination(curr, BC_TNS, BC_JZ ) ||
			     IsCombination(curr, BC_TS , BC_JNZ) )
			instr = GoBack(DeleteFirstChangeNext(curr, BC_JS));
		else if( IsCombination(curr, BC_TP , BC_JZ ) ||
			     IsCombination(curr, BC_TNP, BC_JNZ) )
			instr = GoBack(DeleteFirstChangeNext(curr, BC_JNP));
		else if( IsCombination(curr, BC_TNP, BC_JZ ) ||
			     IsCombination(curr, BC_TP , BC_JNZ) )
			instr = GoBack(DeleteFirstChangeNext(curr, BC_JP));
// End PATTERN
		// JMP +0 -> remove
		else if( IsCombination(curr, BC_JMP, BC_LABEL) && int(curr->arg) == instr->wArg[0] )
			instr = GoBack(DeleteInstruction(curr));
		// PSF, ChkRefS, RDS4 -> PshV4, CHKREF
		else if( IsCombination(curr, BC_PSF, BC_ChkRefS) &&
			     IsCombination(instr, BC_ChkRefS, BC_RDS4) )
		{
			// TODO: Pointer size
			curr->op = BC_PshV4;
			instr->op = BC_CHKREF;
			DeleteInstruction(instr->next);
			instr = GoBack(curr);
		}
		// PSF, ChkRefS, POP -> ChkNullV
		// PshV4, CHKREF, POP -> ChkNullV
		else if( (IsCombination(curr, BC_PSF, BC_ChkRefS) &&
			     IsCombination(instr, BC_ChkRefS, BC_POP) &&
				 instr->next->wArg > 0) ||
				 (IsCombination(curr, BC_PshV4, BC_ChkRefS) &&
				 IsCombination(instr, BC_CHKREF, BC_POP) &&
				 instr->next->wArg > 0) )
		{
			// TODO: Pointer size
			curr->op = BC_ChkNullV;
			curr->stackInc = 0;
			DeleteInstruction(instr->next);
			DeleteInstruction(instr);
			instr = GoBack(curr);
		}
	}

	return 0;
}

int asCByteCode::SizeOfType(int type)
{
	switch(type)
	{
	case BCTYPE_INFO:
		return 0;
	case BCTYPE_NO_ARG:
	case BCTYPE_W_ARG:
	case BCTYPE_rW_ARG:
	case BCTYPE_wW_ARG:
		return 1;
	case BCTYPE_DW_ARG:
	case BCTYPE_wW_rW_ARG:
	case BCTYPE_rW_rW_ARG:
	case BCTYPE_rW_DW_ARG:
	case BCTYPE_wW_DW_ARG:
	case BCTYPE_W_DW_ARG:
	case BCTYPE_wW_rW_rW_ARG:
	case BCTYPE_W_rW_ARG:
	case BCTYPE_wW_W_ARG:
		return 2;
	case BCTYPE_QW_ARG:
	case BCTYPE_DW_DW_ARG:
	case BCTYPE_wW_QW_ARG:
	case BCTYPE_wW_rW_DW_ARG:
		return 3;
	default:
		assert(false);	
		return 0;
	}
}

bool asCByteCode::IsTempVarReadByInstr(cByteInstruction *curr, int offset)
{
	// Which instructions read from variables?
	if( bcTypes[curr->op] == BCTYPE_wW_rW_rW_ARG && 
		(curr->wArg[1] == offset || curr->wArg[2] == offset) )
		return true;
	else if( (bcTypes[curr->op] == BCTYPE_rW_ARG    ||
			  bcTypes[curr->op] == BCTYPE_rW_DW_ARG) &&
			  curr->wArg[0] == offset )
		return true;
	else if( (bcTypes[curr->op] == BCTYPE_wW_rW_ARG ||
			  bcTypes[curr->op] == BCTYPE_wW_rW_DW_ARG ||
			  bcTypes[curr->op] == BCTYPE_W_rW_ARG) &&
			 curr->wArg[1] == offset )
		return true;
	else if( bcTypes[curr->op] == BCTYPE_rW_rW_ARG &&
			 ((signed)curr->wArg[0] == offset || (signed)curr->wArg[1] == offset) )
		return true;

	return false;
}

bool asCByteCode::IsInstrJmpOrLabel(cByteInstruction *curr)
{
	if( curr->op == BC_JS      ||
		curr->op == BC_JNS     ||
		curr->op == BC_JP      ||
		curr->op == BC_JNP     ||
		curr->op == BC_JMPP    ||
		curr->op == BC_JMP     ||
		curr->op == BC_JZ      ||
		curr->op == BC_JNZ     ||
		curr->op == BC_LABEL   )
		return true;

	return false;
}

bool asCByteCode::IsTempVarOverwrittenByInstr(cByteInstruction *curr, int offset)
{
	// Which instructions overwrite the variable or discard it?
	if( curr->op == BC_RET     ||
		curr->op == BC_SUSPEND )
		return true;
	else if( (bcTypes[curr->op] == BCTYPE_wW_rW_rW_ARG || 
			  bcTypes[curr->op] == BCTYPE_wW_rW_ARG    ||
			  bcTypes[curr->op] == BCTYPE_wW_rW_DW_ARG ||
			  bcTypes[curr->op] == BCTYPE_wW_ARG       ||
			  bcTypes[curr->op] == BCTYPE_wW_W_ARG     ||
			  bcTypes[curr->op] == BCTYPE_wW_DW_ARG    ||
			  bcTypes[curr->op] == BCTYPE_wW_QW_ARG) &&
			 curr->wArg[0] == offset )
		return true;

	return false;
}

bool asCByteCode::IsTempVarRead(cByteInstruction *curr, int offset)
{
	asCArray<cByteInstruction *> openPaths;
	asCArray<cByteInstruction *> closedPaths;

	// We're not interested in the first instruction, since it is the one that sets the variable
	openPaths.PushLast(curr->next);

	while( openPaths.GetLength() )
	{
		curr = openPaths.PopLast();

		// Add the instruction to the closed paths so that we don't verify it again
		closedPaths.PushLast(curr);

		while( curr )
		{
			if( IsTempVarReadByInstr(curr, offset) ) return true;

			if( IsTempVarOverwrittenByInstr(curr, offset) ) break;

			// In case of jumps, we must follow the each of the paths
			if( curr->op == BC_JMP )
			{
				int r = FindLabel(asDWORD(curr->arg), curr, &curr, 0); assert( r == 0 );

				if( !closedPaths.Find(curr) &&
					!openPaths.Find(curr) )
					openPaths.PushLast(curr);

				break;
			}
			else if( curr->op == BC_JZ || curr->op == BC_JNZ ||
				     curr->op == BC_JS || curr->op == BC_JNS ||
					 curr->op == BC_JP || curr->op == BC_JNP )
			{
				cByteInstruction *dest;
				int r = FindLabel(asDWORD(curr->arg), curr, &dest, 0); assert( r == 0 );

				if( !closedPaths.Find(dest) &&
					!openPaths.Find(dest) )
					openPaths.PushLast(dest);
			}
			// We cannot optimize over BC_JMPP
			else if( curr->op == BC_JMPP ) return true;

			curr = curr->next;
		}
	}

	return false;
}

bool asCByteCode::IsTempRegUsed(cByteInstruction *curr)
{
	// We're not interested in the first instruction, since it is the one that sets the register
	while( curr->next )
	{
		curr = curr->next;

		// Which instructions read from the register?
		if( curr->op == BC_INCi     ||
			curr->op == BC_INCi16   ||
			curr->op == BC_INCi8    ||
			curr->op == BC_INCf     ||
			curr->op == BC_INCd     ||
			curr->op == BC_DECi     ||
			curr->op == BC_DECi16   ||
			curr->op == BC_DECi8    ||
			curr->op == BC_DECf     ||
			curr->op == BC_DECd     ||
			curr->op == BC_WRTV1    ||
			curr->op == BC_WRTV2    ||
			curr->op == BC_WRTV4    ||
			curr->op == BC_WRTV8    ||
			curr->op == BC_RDR1     ||
			curr->op == BC_RDR2     ||
			curr->op == BC_RDR4     ||
			curr->op == BC_RDR8     ||
			curr->op == BC_PshRPtr  ||
			curr->op == BC_CpyRtoV4 ||
			curr->op == BC_CpyRtoV8 ||
			curr->op == BC_TZ       ||
			curr->op == BC_TNZ      ||
			curr->op == BC_TS       ||
			curr->op == BC_TNS      ||
			curr->op == BC_TP       ||
			curr->op == BC_TNP      ||
			curr->op == BC_JZ       ||
			curr->op == BC_JNZ      ||
			curr->op == BC_JS       ||
			curr->op == BC_JNS      ||
			curr->op == BC_JP       ||
			curr->op == BC_JNP      )
			return true;

		// Which instructions overwrite the register or discard the value?
		if( curr->op == BC_CALL      ||
			curr->op == BC_PopRPtr   ||
			curr->op == BC_CALLSYS   ||
			curr->op == BC_CALLBND   ||
			curr->op == BC_SUSPEND   ||
			curr->op == BC_ALLOC     ||
			curr->op == BC_FREE      ||
			curr->op == BC_CpyVtoR4  ||
			curr->op == BC_LdGRdR4   ||
			curr->op == BC_LDG       ||
			curr->op == BC_LDV       ||
			curr->op == BC_TZ        ||
			curr->op == BC_TNZ       ||
			curr->op == BC_TS        ||
			curr->op == BC_TNS       ||
			curr->op == BC_TP        ||
			curr->op == BC_TNP       ||
			curr->op == BC_JS        ||
			curr->op == BC_JNS       ||
			curr->op == BC_JP        ||
			curr->op == BC_JNP       ||
			curr->op == BC_JMPP      ||
			curr->op == BC_JMP       ||
			curr->op == BC_JZ        ||
			curr->op == BC_JNZ       ||
			curr->op == BC_CMPi      ||
			curr->op == BC_CMPu      ||
			curr->op == BC_CMPf      ||
			curr->op == BC_CMPd      ||
			curr->op == BC_CMPIi     ||
			curr->op == BC_CMPIu     ||
			curr->op == BC_CMPIf     ||
			curr->op == BC_LABEL     )
			return false;
	}

	return false;
}

void asCByteCode::ExtractLineNumbers()
{
	int lastLinePos = -1;
	int pos = 0;
	cByteInstruction *instr = first;
	while( instr )
	{
		cByteInstruction *curr = instr;
		instr = instr->next;
		
		if( curr->op == BC_LINE )
		{
			if( lastLinePos == pos )
			{
				lineNumbers.PopLast();
				lineNumbers.PopLast();
			}

			lastLinePos = pos;
			lineNumbers.PushLast(pos);
			lineNumbers.PushLast(*(int*)ARG_DW(curr->arg));

#ifndef BUILD_WITHOUT_LINE_CUES
			// Transform BC_LINE into BC_SUSPEND
			curr->op = BC_SUSPEND;
			curr->size = SizeOfType(BCT_SUSPEND);
			pos += curr->size;
#else
			// Delete the instruction
			DeleteInstruction(curr);
#endif
		}
		else
			pos += curr->size;
	}
}

int asCByteCode::GetSize()
{
	int size = 0;
	cByteInstruction *instr = first;
	while( instr )
	{
		size += instr->GetSize();

		instr = instr->next;
	}

	return size;
}

void asCByteCode::AddCode(asCByteCode *bc)
{
	if( bc->first )
	{
		if( first == 0 )
		{
			first = bc->first;
			last = bc->last;
			bc->first = 0;
			bc->last = 0;
		}
		else
		{
			last->next = bc->first;
			bc->first->prev = last;
			last = bc->last;
			bc->first = 0;
			bc->last = 0;
		}
	}
}

int asCByteCode::AddInstruction()
{
	cByteInstruction *instr = new cByteInstruction();
	if( first == 0 )
	{
		first = last = instr;
	}
	else
	{
		last->AddAfter(instr);
		last = instr;
	}

	return 0;
}

int asCByteCode::AddInstructionFirst()
{
	cByteInstruction *instr = new cByteInstruction();
	if( first == 0 )
	{
		first = last = instr;
	}
	else
	{
		first->AddBefore(instr);
		first = instr;
	}

	return 0;
}

void asCByteCode::Call(bcInstr instr, int funcID, int pop)
{
	if( AddInstruction() < 0 )
		return;

	assert(bcTypes[instr] == BCTYPE_DW_ARG);

	last->op = instr;
	last->size = SizeOfType(bcTypes[instr]);
	last->stackInc = -pop; // BC_CALL and BC_CALLBND doesn't pop the argument but when the callee returns the arguments are already popped
	*((int*)ARG_DW(last->arg)) = funcID;
}

void asCByteCode::Alloc(bcInstr instr, int objID, int funcID, int pop)
{
	if( AddInstruction() < 0 )
		return;

	assert(bcTypes[instr] == BCTYPE_DW_DW_ARG);

	last->op = instr;
	last->size = SizeOfType(bcTypes[instr]);
	last->stackInc = -pop; // BC_ALLOC
	*((int*)ARG_DW(last->arg)) = objID;
	*((int*)(ARG_DW(last->arg)+1)) = funcID;
}

void asCByteCode::Ret(int pop)
{
	if( AddInstruction() < 0 )
		return;

	assert(BCT_RET == BCTYPE_W_ARG);

	last->op = BC_RET;
	last->size = SizeOfType(BCT_RET);
	last->stackInc = 0; // The instruction pops the argument, but it doesn't affect current function
	last->wArg[0] = pop;
}

void asCByteCode::JmpP(int var, asDWORD max)
{
	if( AddInstruction() < 0 )
		return;
	
	assert(bcTypes[BC_JMPP] == BCTYPE_rW_ARG);

	last->op       = BC_JMPP;
	last->size     = SizeOfType(BCT_JMPP);
	last->stackInc = bcStackInc[BC_JMPP];
	last->wArg[0]  = var;

	// Store the largest jump that is made for PostProcess()
	*ARG_DW(last->arg) = max;
}

void asCByteCode::Label(short label)
{
	if( AddInstruction() < 0 )
		return;

	last->op       = BC_LABEL;
	last->size     = 0;
	last->stackInc = 0;
	last->wArg[0]  = label;
}

void asCByteCode::Line(int line, int column)
{
	if( AddInstruction() < 0 )
		return;

	last->op       = BC_LINE;
	last->size     = SizeOfType(BCT_LINE);
	last->stackInc = 0;
	*((int*)ARG_DW(last->arg)) = (line & 0xFFFFF)|((column & 0xFFF)<<20);
}

int asCByteCode::FindLabel(int label, cByteInstruction *from, cByteInstruction **dest, int *positionDelta)
{
	// Search forward
	int labelPos = -from->GetSize();

	cByteInstruction *labelInstr = from;
	while( labelInstr )
	{
		labelPos += labelInstr->GetSize();
		labelInstr = labelInstr->next;

		if( labelInstr && labelInstr->op == BC_LABEL )
		{
			if( labelInstr->wArg[0] == label )
				break;
		}
	}

	if( labelInstr == 0 )
	{
		// Search backwards
		labelPos = -from->GetSize();

		labelInstr = from;
		while( labelInstr )
		{
			labelInstr = labelInstr->prev;
			if( labelInstr )
			{
				labelPos -= labelInstr->GetSize();

				if( labelInstr->op == BC_LABEL )
				{
					if( labelInstr->wArg[0] == label )
						break;
				}
			}
		}
	}

	if( labelInstr != 0 )
	{
		if( dest ) *dest = labelInstr;
		if( positionDelta ) *positionDelta = labelPos;
		return 0;
	}

	return -1;
}

int asCByteCode::ResolveJumpAddresses()
{
	int pos = 0;
	cByteInstruction *instr = first;
	while( instr )
	{
		// The program pointer is updated as the instruction is read
		pos += instr->GetSize();

		if( instr->op == BC_JMP || 
			instr->op == BC_JZ || instr->op == BC_JNZ ||
			instr->op == BC_JS || instr->op == BC_JNS || 
			instr->op == BC_JP || instr->op == BC_JNP )
		{
			int label = *((int*) ARG_DW(instr->arg));
			int labelPosOffset;			
			int r = FindLabel(label, instr, 0, &labelPosOffset);
			if( r == 0 )
				*((int*) ARG_DW(instr->arg)) = labelPosOffset;
			else
				return -1;
		}

		instr = instr->next;
	}

	return 0;
}


cByteInstruction *asCByteCode::DeleteInstruction(cByteInstruction *instr)
{
	if( instr == 0 ) return 0;

	cByteInstruction *ret = instr->prev ? instr->prev : instr->next;
	
	RemoveInstruction(instr);

	delete instr;
	
	return ret;
}

void asCByteCode::Output(asDWORD *array)
{
	// TODO: Receive a script function pointer
	// TODO: When arguments are too large put them in the constant memory instead
	//       4 byte arguments may remain in the instruction code for now. But move 
	//       the 8 byte arguments to the constant memory
	//       Pointers will also be moved to the pointer array

	asDWORD *ap = array;

	cByteInstruction *instr = first;
	while( instr )
	{
		if( instr->GetSize() > 0 )
		{
			if( bcTypes[instr->op] == BCTYPE_NO_ARG )
			{
				*ap = instr->op;
			}
			else if( bcTypes[instr->op] == BCTYPE_wW_rW_rW_ARG )
			{
				*ap = instr->op | (instr->wArg[0] << 16);
				*(ap+1) = asWORD(instr->wArg[1]) | (instr->wArg[2] << 16);
			}
			else if( bcTypes[instr->op] == BCTYPE_wW_DW_ARG ||
				     bcTypes[instr->op] == BCTYPE_rW_DW_ARG ||
					 bcTypes[instr->op] == BCTYPE_W_DW_ARG  )
			{
				*ap = (instr->op) | (instr->wArg[0] << 16);
				*(ap+1) = asDWORD(instr->arg);
			}
			else if( bcTypes[instr->op] == BCTYPE_wW_rW_DW_ARG )
			{
				*ap = (instr->op) | (instr->wArg[0]<<16);
				*(ap+1) = instr->wArg[1];
				*(ap+2) = asDWORD(instr->arg);
			}
			else if( bcTypes[instr->op] == BCTYPE_wW_QW_ARG )
			{
				*ap = (instr->op) | (instr->wArg[0] << 16);
				memcpy(ap+1, &instr->arg, instr->GetSize()*4-4);
			}
			else if( bcTypes[instr->op] == BCTYPE_W_ARG  ||
				     bcTypes[instr->op] == BCTYPE_rW_ARG || 
					 bcTypes[instr->op] == BCTYPE_wW_ARG )
			{
				*ap = (instr->op) | (instr->wArg[0]<<16);
			}
			else if( bcTypes[instr->op] == BCTYPE_wW_rW_ARG ||
					 bcTypes[instr->op] == BCTYPE_rW_rW_ARG ||
					 bcTypes[instr->op] == BCTYPE_W_rW_ARG  ||
					 bcTypes[instr->op] == BCTYPE_wW_W_ARG  )
			{
				*ap = (instr->op) | (instr->wArg[0]<<16);
				*(ap+1) = instr->wArg[1];
			}
			else
			{
				memcpy(ap, &instr->op, 4);
				memcpy(ap+1, &instr->arg, instr->GetSize()*4-4);
			}
		}

		ap += instr->GetSize();
		instr = instr->next;
	}
}

void asCByteCode::PostProcess()
{
	if( first == 0 ) return;

	// This function will do the following
	// - Verify if there is any code that never gets executed and remove it
	// - Calculate the stack size at the position of each byte code 
	// - Calculate the largest stack needed

	largestStackUsed = 0;

	cByteInstruction *instr = first;
	while( instr )
	{
		instr->marked = false;
		instr->stackSize = -1;
		instr = instr->next;
	}

	// Add the first instruction to the list of unchecked code paths
	asCArray<cByteInstruction *> paths;
	AddPath(paths, first, 0);

	// Go through each of the code paths
	for( asUINT p = 0; p < paths.GetLength(); ++p )
	{
		instr = paths[p];
		int stackSize = instr->stackSize;
		
		while( instr )
		{
			instr->marked = true;
			instr->stackSize = stackSize;
			stackSize += instr->stackInc;
			if( stackSize > largestStackUsed ) 
				largestStackUsed = stackSize;

			// PSP -> PSF
			if( instr->op == BC_PSP )
			{
				instr->op = BC_PSF;
				instr->wArg[0] = instr->wArg[0] + instr->stackSize;
			}

			if( instr->op == BC_JMP )
			{
				// Find the label that we should jump to
				int label = *((int*) ARG_DW(instr->arg));
				cByteInstruction *dest;
				int r = FindLabel(label, instr, &dest, 0);
				assert( r == 0 );
				
				AddPath(paths, dest, stackSize);
				break;
			}
			else if( instr->op == BC_JZ || instr->op == BC_JNZ ||
					 instr->op == BC_JS || instr->op == BC_JNS ||
					 instr->op == BC_JP || instr->op == BC_JNP )
			{
				// Find the label that is being jumped to
				int label = *((int*) ARG_DW(instr->arg));
				cByteInstruction *dest;
				int r = FindLabel(label, instr, &dest, 0);
				assert( r == 0 );
				
				AddPath(paths, dest, stackSize);
				
				// Add both paths to the code paths
				AddPath(paths, instr->next, stackSize);
				
				break;
			}
			else if( instr->op == BC_JMPP )
			{
				// I need to know the largest value possible
				asDWORD max = *ARG_DW(instr->arg);
								
				// Add all destinations to the code paths
				cByteInstruction *dest = instr->next;
				for( asDWORD n = 0; n <= max && dest != 0; ++n )
				{
					AddPath(paths, dest, stackSize);
					dest = dest->next;
				}				
				
				break;				
			}
			else
			{
				instr = instr->next;
				if( instr == 0 || instr->marked )
					break;
			}
		}
	}
	
	// Are there any instructions that didn't get visited?
	instr = first;
	while( instr )
	{
		if( instr->marked == false )
		{
			// TODO:
			// Give warning of unvisited code

			// Remove it
			cByteInstruction *curr = instr;
			instr = instr->next;

			// TODO: Add instruction again
			DeleteInstruction(curr);
		}
		else
			instr = instr->next;
	}	
}

void asCByteCode::DebugOutput(const char *name, asCModule *module, asCScriptEngine *engine)
{
#ifdef AS_DEBUG
	mkdir("AS_DEBUG");

	asCString str = "AS_DEBUG/";
	str += name;

	FILE *file = fopen(str.AddressOf(), "w");

	fprintf(file, "Temps: ");
	for( asUINT n = 0; n < temporaryVariables.GetLength(); n++ )
	{
		fprintf(file, "%d", temporaryVariables[n]);
		if( n < temporaryVariables.GetLength()-1 )
			fprintf(file, ", ");
	}
	fprintf(file, "\n\n");

	int pos = 0;
	asUINT lineIndex = 0;
	cByteInstruction *instr = first;
	while( instr )
	{
		if( lineIndex < lineNumbers.GetLength() && lineNumbers[lineIndex] == pos )
		{
			asDWORD line = lineNumbers[lineIndex+1];
			fprintf(file, "- %d,%d -\n", line&0xFFFFF, line>>20);
			lineIndex += 2;
		}

		fprintf(file, "%5d ", pos);
		pos += instr->GetSize();

		fprintf(file, "%3d %c ", instr->stackSize, instr->marked ? '*' : ' ');

		switch( bcTypes[instr->op] )
		{
		case BCTYPE_W_ARG:
			if( instr->op == BC_STR )
			{
				int id = instr->wArg[0];
				const asCString &str = module->GetConstantString(id);
				fprintf(file, "   %-8s %d         (l:%d s:\"%.10s\")\n", bcName[instr->op].name, instr->wArg[0], str.GetLength(), str.AddressOf());
			}
			else
				fprintf(file, "   %-8s %d\n", bcName[instr->op].name, instr->wArg[0]);
			break;

		case BCTYPE_wW_ARG:
		case BCTYPE_rW_ARG:
			fprintf(file, "   %-8s v%d\n", bcName[instr->op].name, instr->wArg[0]);
			break;

		case BCTYPE_wW_rW_ARG:
		case BCTYPE_rW_rW_ARG:
			fprintf(file, "   %-8s v%d, v%d\n", bcName[instr->op].name, instr->wArg[0], instr->wArg[1]);
			break;

		case BCTYPE_W_rW_ARG:
			fprintf(file, "   %-8s %d, v%d\n", bcName[instr->op].name, instr->wArg[0], instr->wArg[1]);
			break;

		case BCTYPE_wW_W_ARG:
			fprintf(file, "   %-8s v%d, %d\n", bcName[instr->op].name, instr->wArg[0], instr->wArg[1]);
			break;

		case BCTYPE_wW_rW_DW_ARG:
			switch( instr->op )
			{
			case BC_ADDIf:
			case BC_SUBIf:
			case BC_MULIf:
				fprintf(file, "   %-8s v%d, v%d, %f\n", bcName[instr->op].name, instr->wArg[0], instr->wArg[1], *((float*) ARG_DW(instr->arg)));
				break;
			default:
				fprintf(file, "   %-8s v%d, v%d, %d\n", bcName[instr->op].name, instr->wArg[0], instr->wArg[1], *((int*) ARG_DW(instr->arg)));
				break;
			}
			break;

		case BCTYPE_DW_ARG:
			switch( instr->op )
			{
			case BC_PshC4:
			case BC_OBJTYPE:
			case BC_TYPEID:
				fprintf(file, "   %-8s 0x%lx          (i:%d, f:%g)\n", bcName[instr->op].name, *ARG_DW(instr->arg), *((int*) ARG_DW(instr->arg)), *((float*) ARG_DW(instr->arg)));
				break;

			case BC_CALL:
			case BC_CALLSYS:
			case BC_CALLBND:
				{
					int funcID = *(int*)ARG_DW(instr->arg) | module->moduleID;
					asCString decl = engine->GetFunctionDeclaration(funcID);

					fprintf(file, "   %-8s %d           (%s)\n", bcName[instr->op].name, *((int*) ARG_DW(instr->arg)), decl.AddressOf());
				}
				break;	

			case BC_FREE:
			case BC_REFCPY:
				fprintf(file, "   %-8s %d\n", bcName[instr->op].name, *((int*) ARG_DW(instr->arg)));
				break;

			case BC_JMP:
			case BC_JZ:
			case BC_JS:
			case BC_JP:
			case BC_JNZ:
			case BC_JNS:
			case BC_JNP:
				fprintf(file, "   %-8s %+d              (d:%d)\n", bcName[instr->op].name, *((int*) ARG_DW(instr->arg)), pos+*((int*) ARG_DW(instr->arg)));
				break;

			default:
				fprintf(file, "   %-8s %d\n", bcName[instr->op].name, *((int*) ARG_DW(instr->arg)));
				break;
			}
			break;

		case BCTYPE_QW_ARG:
#ifdef __GNUC__
			fprintf(file, "   %-8s 0x%llx           (i:%lld, f:%g)\n", bcName[instr->op].name, *ARG_QW(instr->arg), *((__int64*) ARG_QW(instr->arg)), *((double*) ARG_QW(instr->arg)));
#else
			fprintf(file, "   %-8s 0x%I64x          (i:%I64d, f:%g)\n", bcName[instr->op].name, *ARG_QW(instr->arg), *((__int64*) ARG_QW(instr->arg)), *((double*) ARG_QW(instr->arg)));
#endif
			break;

		case BCTYPE_wW_QW_ARG:
#ifdef __GNUC__
			fprintf(file, "   %-8s v%d, 0x%llx           (i:%lld, f:%g)\n", bcName[instr->op].name, instr->wArg[0], *ARG_QW(instr->arg), *((__int64*) ARG_QW(instr->arg)), *((double*) ARG_QW(instr->arg)));
#else
			fprintf(file, "   %-8s v%d, 0x%I64x          (i:%I64d, f:%g)\n", bcName[instr->op].name, instr->wArg[0], *ARG_QW(instr->arg), *((__int64*) ARG_QW(instr->arg)), *((double*) ARG_QW(instr->arg)));
#endif
			break;

		case BCTYPE_DW_DW_ARG:
			if( instr->op == BC_ALLOC )
				fprintf(file, "   %-8s 0x%lx, %d\n", bcName[instr->op].name, *(int*)ARG_DW(instr->arg), *(int*)(ARG_DW(instr->arg)+1));
			else
				fprintf(file, "   %-8s %u, %d\n", bcName[instr->op].name, *(int*)ARG_DW(instr->arg), *(int*)(ARG_DW(instr->arg)+1));
			break;

		case BCTYPE_INFO:
			if( instr->op == BC_LABEL )
				fprintf(file, "%d:\n", instr->wArg[0]);
			else
				fprintf(file, "   %s\n", bcName[instr->op].name);
			break;

		case BCTYPE_rW_DW_ARG:
		case BCTYPE_wW_DW_ARG:
			if( instr->op == BC_SetV4 )
				fprintf(file, "   %-8s v%d, 0x%lx          (i:%d, f:%g)\n", bcName[instr->op].name, instr->wArg[0], *ARG_DW(instr->arg), *((int*) ARG_DW(instr->arg)), *((float*) ARG_DW(instr->arg)));
			else if( instr->op == BC_CMPIf )
				fprintf(file, "   %-8s v%d, %f\n", bcName[instr->op].name, instr->wArg[0], *(float*)ARG_DW(instr->arg));
			else
				fprintf(file, "   %-8s v%d, %d\n", bcName[instr->op].name, instr->wArg[0], *ARG_DW(instr->arg));
			break;

		case BCTYPE_W_DW_ARG:
			if( instr->op == BC_SetG4 )
				fprintf(file, "   %-8s %d, 0x%lx          (i:%d, f:%g)\n", bcName[instr->op].name, instr->wArg[0], *ARG_DW(instr->arg), *((int*) ARG_DW(instr->arg)), *((float*) ARG_DW(instr->arg)));
			break;

		case BCTYPE_wW_rW_rW_ARG:
			fprintf(file, "   %-8s v%d, v%d, v%d\n", bcName[instr->op].name, instr->wArg[0], instr->wArg[1], instr->wArg[2]);
			break;

		case BCTYPE_NO_ARG:
			fprintf(file, "   %s\n", bcName[instr->op].name);
			break;

		default:
			assert(false);
		}

		instr = instr->next;
	}

	fclose(file);
#endif
}

//=============================================================================

// Decrease stack with "numDwords"
int asCByteCode::Pop(int numDwords)
{
	assert(BCT_POP == BCTYPE_W_ARG);

	if( AddInstruction() < 0 )
		return 0;

	last->op = BC_POP;
	last->wArg[0] = (short)numDwords;
	last->size = SizeOfType(BCT_POP);
	last->stackInc = -numDwords;

	return last->stackInc;
}

// Increase stack with "numDwords"
int asCByteCode::Push(int numDwords)
{
	assert(BCT_PUSH == BCTYPE_W_ARG);

	if( AddInstruction() < 0 )
		return 0;

	last->op = BC_PUSH;
	last->wArg[0] = (short)numDwords;
	last->size = SizeOfType(BCT_PUSH);
	last->stackInc = numDwords;

	return last->stackInc;
}


int asCByteCode::InsertFirstInstrDWORD(bcInstr bc, asDWORD param)
{
	assert(bcTypes[bc] == BCTYPE_DW_ARG);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstructionFirst() < 0 )
		return 0;

	first->op = bc;
	*ARG_DW(first->arg) = param;
	first->size = SizeOfType(bcTypes[bc]);
	first->stackInc = bcStackInc[bc];

	return first->stackInc;
}

int asCByteCode::InsertFirstInstrQWORD(bcInstr bc, asQWORD param)
{
	assert(bcTypes[bc] == BCTYPE_QW_ARG);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstructionFirst() < 0 )
		return 0;

	first->op = bc;
	*ARG_QW(first->arg) = param;
	first->size = SizeOfType(bcTypes[bc]);
	first->stackInc = bcStackInc[bc];

	return first->stackInc;
}

int asCByteCode::Instr(bcInstr bc)
{
	assert(bcTypes[bc] == BCTYPE_NO_ARG);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op       = bc;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrW_W_W(bcInstr bc, int a, int b, int c)
{
	assert(bcTypes[bc] == BCTYPE_wW_rW_rW_ARG);
	assert(bcStackInc[bc] == 0);

	if( AddInstruction() < 0 )
		return 0;

	last->op       = bc;
	last->wArg[0]  = a;
	last->wArg[1]  = b;
	last->wArg[2]  = c;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrW_W(bcInstr bc, int a, int b)
{
	assert(bcTypes[bc] == BCTYPE_wW_rW_ARG ||
		   bcTypes[bc] == BCTYPE_rW_rW_ARG);
	assert(bcStackInc[bc] == 0);

	if( AddInstruction() < 0 )
		return 0;

	last->op       = bc;
	last->wArg[0]  = a;
	last->wArg[1]  = b;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrW_DW(bcInstr bc, asWORD a, asDWORD b)
{
	assert(bcTypes[bc] == BCTYPE_wW_DW_ARG ||
           bcTypes[bc] == BCTYPE_rW_DW_ARG);
	assert(bcStackInc[bc] == 0);

	if( AddInstruction() < 0 )
		return 0;

	last->op       = bc;
	last->wArg[0]  = a;
	*((int*) ARG_DW(last->arg)) = b;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrSHORT_DW(bcInstr bc, short a, asDWORD b)
{
	assert(bcTypes[bc] == BCTYPE_wW_DW_ARG || 
	       bcTypes[bc] == BCTYPE_rW_DW_ARG);
	assert(bcStackInc[bc] == 0);

	if( AddInstruction() < 0 )
		return 0;

	last->op       = bc;
	last->wArg[0]  = a;
	*((int*) ARG_DW(last->arg)) = b;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrW_QW(bcInstr bc, asWORD a, asQWORD b)
{
	assert(bcTypes[bc] == BCTYPE_wW_QW_ARG);
	assert(bcStackInc[bc] == 0);

	if( AddInstruction() < 0 )
		return 0;

	last->op       = bc;
	last->wArg[0]  = a;
	*ARG_QW(last->arg) = b;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrSHORT_QW(bcInstr bc, short a, asQWORD b)
{
	assert(bcTypes[bc] == BCTYPE_wW_QW_ARG);
	assert(bcStackInc[bc] == 0);

	if( AddInstruction() < 0 )
		return 0;

	last->op       = bc;
	last->wArg[0]  = a;
	*ARG_QW(last->arg) = b;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrW_FLOAT(bcInstr bc, asWORD a, float b)
{
	assert(bcTypes[bc] == BCTYPE_wW_DW_ARG);
	assert(bcStackInc[bc] == 0);

	if( AddInstruction() < 0 )
		return 0;

	last->op       = bc;
	last->wArg[0]  = a;
	*((float*) ARG_DW(last->arg)) = b;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrSHORT(bcInstr bc, short param)
{
	assert(bcTypes[bc] == BCTYPE_rW_ARG || 
		   bcTypes[bc] == BCTYPE_wW_ARG || 
		   bcTypes[bc] == BCTYPE_W_ARG);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op       = bc;
	last->wArg[0]  = param;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrINT(bcInstr bc, int param)
{
	assert(bcTypes[bc] == BCTYPE_DW_ARG);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op = bc;
	*((int*) ARG_DW(last->arg)) = param;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrDWORD(bcInstr bc, asDWORD param)
{
	assert(bcTypes[bc] == BCTYPE_DW_ARG);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op = bc;
	*ARG_DW(last->arg) = param;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrQWORD(bcInstr bc, asQWORD param)
{
	assert(bcTypes[bc] == BCTYPE_QW_ARG);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op = bc;
	*ARG_QW(last->arg) = param;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrWORD(bcInstr bc, asWORD param)
{
	assert(bcTypes[bc] == BCTYPE_W_ARG  || 
		   bcTypes[bc] == BCTYPE_rW_ARG || 
		   bcTypes[bc] == BCTYPE_wW_ARG);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op       = bc;
	last->wArg[0]  = param;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrFLOAT(bcInstr bc, float param)
{
	assert(bcTypes[bc] == BCTYPE_DW_ARG);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op = bc;
	*((float*) ARG_DW(last->arg)) = param;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrDOUBLE(bcInstr bc, double param)
{
	assert(bcTypes[bc] == BCTYPE_QW_ARG);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op = bc;
	*((double*) ARG_QW(last->arg)) = param;
	last->size     = SizeOfType(bcTypes[bc]);
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::GetLastInstr()
{
	if( last == 0 ) return -1;

	return last->op;
}

int asCByteCode::RemoveLastInstr()
{
	if( last == 0 ) return -1;

	if( first == last )
	{
		delete last;
		first = 0;
		last = 0;
	}
	else
	{
		cByteInstruction *bc = last;
		last = bc->prev;

		bc->Remove();
		delete bc;
	}

	return 0;
}

asDWORD asCByteCode::GetLastInstrValueDW()
{
	if( last == 0 ) return 0;

	return *ARG_DW(last->arg);
}

void asCByteCode::DefineTemporaryVariable(int varOffset)
{
	temporaryVariables.PushLast(varOffset);
}

//===================================================================

cByteInstruction::cByteInstruction()
{
	next = 0;
	prev = 0;

	op = BC_LABEL;

	size = 0;
	stackInc = 0;
}

void cByteInstruction::AddAfter(cByteInstruction *nextCode)
{
	if( next )
		next->prev = nextCode;

	nextCode->next = next;
	nextCode->prev = this;
	next = nextCode;
}

void cByteInstruction::AddBefore(cByteInstruction *prevCode)
{
	if( prev )
		prev->next = prevCode;

	prevCode->prev = prev;
	prevCode->next = this;
	prev = prevCode;
}

int cByteInstruction::GetSize()
{
	return size;
}

int cByteInstruction::GetStackIncrease()
{
	return stackInc;
}

void cByteInstruction::Remove()
{
	if( prev ) prev->next = next;
	if( next ) next->prev = prev;
	prev = 0;
	next = 0;
}

END_AS_NAMESPACE
