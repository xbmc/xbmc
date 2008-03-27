#include "ExprVirtualMachine.h"

#include <math.h>
#include <stdlib.h>


#define __addInst( opcode, data16 )		long op = (opcode) | (data16);	\
										mProgram.Append( &op, 4 );
					

#define REG_IN_USE	0x1
#define REG_USED	0x2

ExprUserFcn ExprVirtualMachine::sZeroFcn = { 0, 0 };

#define _fetch( r, val )	switch ( (r) ) {					\
								case 0:		val = FR0;	break;	\
								case 1:		val = FR1;	break;	\
								case 2:		val = FR2;	break;	\
								case 3:		val = FR3;	break;	\
								case 4:		val = FR4;	break;	\
								case 5:		val = FR5;	break;	\
								case 6:		val = FR6;	break;	\
								case 7:		val = FR7;	break;	\
								default:	val = mVirtFR[ r - NUM_PHYS_REGS ];	\
							}
							
#define _store( r, val )	switch ( (r) ) {					\
								case 0:		FR0 = val;	break;	\
								case 1:		FR1 = val;	break;	\
								case 2:		FR2 = val;	break;	\
								case 3:		FR3 = val;	break;	\
								case 4:		FR4 = val;	break;	\
								case 5:		FR5 = val;	break;	\
								case 6:		FR6 = val;	break;	\
								case 7:		FR7 = val;	break;	\
								default:	mVirtFR[ r - NUM_PHYS_REGS ] = val; \
							}


#define _exeFn( r )		switch ( subop ) {							\
							case cSQRT:	r = sqrt( r );	break;		\
							case cABS:	r = fabs( r );	break;		\
							case cSIN:	r = sin( r );	break;		\
							case cCOS:	r = cos( r );	break;		\
							case cSEED: i = *((long*) &r);						\
										size = i % 31;							\
										srand( ( i << size ) | ( i >> ( 32 - size ) )  ); 	break;				\
							case cTAN:	r = tan( r );	break;		\
							case cSGN:	r = ( r >= 0 ) ? 1 : -1;	break;		\
							case cLOG:	r = log( r );	break;		\
							case cEXP:	r = exp( r );	break;		\
							case cSQR:	r = r * r;		break;		\
							case cATAN:	r = atan( r );	break;		\
							case cTRNC:	r = float(int( r ));	break;		\
							case cWRAP:	r = r -  floor( r );  break; 	\
							case cRND:	r = r * ( (float) rand() ) / ( (float) RAND_MAX ); break;	\
							case cSQWV:	r = ( r >= -1 && r <= 1 ) ? 1 : 0;	break;	\
							case cTRWV: r = fabs( .5 * r ); r = 2 * ( r - floor( r ) );  if ( r > 1 ) r = 2 - r;	break;	\
							case cPOS:	if ( r < 0 ) r = 0;			break;		\
							case cCLIP:	if ( r < 0 ) r = 0; else if ( r > 1 ) r = 1; break;	\
							case cFLOR: r = floor( r );	break;		\
						}
				
		
#define _exeOp( r1, r2 ) 	switch ( subop ) {						\
								case '+':	r1 += r2;		break;	\
								case '-':	r1 -= r2;		break;	\
								case '/':	if(r2==0) r1=0; else r1 /= r2;		break;	\
								case '*':	r1 *= r2;		break;	\
								case '^':	r1 = pow( r1, r2 );					break;	\
								case '%':	if(r2==0) r1=0; else r1 = ( (long) r1 ) % ( (long) r2 );	break;	\
							}
		
ExprVirtualMachine::ExprVirtualMachine() {
	mPCStart	= NULL;
	mPCEnd		= NULL;

	sZeroFcn.mNumFcnBins = 1;
	sZeroFcn.mFcn[ 0 ] = 0;
}
				
						
int ExprVirtualMachine::FindGlobalFreeReg() {
	int reg = 1;
	
	// Look for a global free register 
	while ( ( mRegColor[ reg ] & REG_USED ) && reg < NUM_REGS )
		reg++;
	
	
	return reg;
}				
		

int ExprVirtualMachine::AllocReg() {
	int reg = 0;
	
	// Look for a free register (ie, find one not in use right now)...
	while ( ( mRegColor[ reg ] & REG_IN_USE ) && reg < NUM_REGS )
		reg++;
	
	// Color it
	if ( reg < NUM_REGS )
		mRegColor[ reg ] = REG_IN_USE | REG_USED;
	
	return reg;
}



void ExprVirtualMachine::DeallocReg( int inReg ) {

	// Clear the bit that says the reg is allocated
	mRegColor[ inReg ] &= ~REG_IN_USE;
}

			
void ExprVirtualMachine::DoOp( int inReg, int inReg2, char inOpCode ) {

	__addInst( OP_OPER, ( inOpCode << 16 ) | ( inReg2 << 8 ) | inReg )
}


		


void ExprVirtualMachine::Move( int inReg, int inDestReg ) {
	
	if ( inDestReg != inReg ) {
		__addInst( OP_MOVE, ( inDestReg << 8 ) | inReg )	
	}
}
	

	

void ExprVirtualMachine::Loadi( float inVal, int inReg ) {
	
	__addInst( OP_LOADIMMED, inReg )
	mProgram.Append( &inVal, sizeof( float ) );
}


void ExprVirtualMachine::Loadi( float* inVal, int inReg ) {
	
	__addInst( OP_LOAD, inReg )
	mProgram.Append( &inVal, 4 );
}	



void ExprVirtualMachine::UserFcnOp( int inReg, ExprUserFcn** inFcn ) {
	
	if ( inFcn ) {
		__addInst( OP_USER_FCN, inReg )
		mProgram.Append( &inFcn, 4 );  }
	else
		Loadi( 0.0, inReg );
}

	
	
void ExprVirtualMachine::MathOp( int inReg, char inFcnCode ) {
	
	if ( inFcnCode ) {
		__addInst( OP_MATHOP, ( inFcnCode << 16 ) | inReg )
	}
}		







void ExprVirtualMachine::Clear() {

	// Init register coloring
	for ( int i = 0; i < NUM_REGS; i++ )
		mRegColor[ i ] = 0;
		
	mProgram.Wipe();
}



void ExprVirtualMachine::PrepForExecution() {
	mPCStart	= mProgram.getCStr();
	mPCEnd		= mPCStart + mProgram.length();
}

						
// An inst looks like:
// 0-7: 	Inst opcode
// 8-15:	Sub opcode
// 16-23:	Source Reg
// 24-31:	Dest Register number
								
float ExprVirtualMachine::Execute/*_Inline*/() {
	register float	FR0, FR1, FR2, FR3, FR4, FR5, FR6, FR7;
	register float	v1, v2;
	const char*	PC	= mPCStart;
	const char*	end	= mPCEnd;
	unsigned long	inst, opcode, subop, size, i, r2, r1;
	float			mVirtFR[ NUM_REGS - NUM_PHYS_REGS ];
	
	FR0 = FR1 = FR2 = FR3 = FR4 = FR5 = FR6 = FR7 = 0;

	while ( PC < end ) {
		inst = *((long*) PC);	
		PC += 4;

		opcode = inst & 0xFF000000;	
		r1 = inst & 0xFF;
		r2 = ( inst >> 8 ) & 0xFF;
		
		if ( opcode == OP_LOADIMMED ) {
			v1 = *((float*) PC);
			PC += 4; }
		else if ( opcode == OP_LOAD ) {				
			v1 = **((float**) PC);
			PC += 4; }
		else {

			_fetch( r1, v1 )

			switch ( opcode ) {
			
				case OP_OPER:
					subop = ( inst >> 16 ) & 0xFF;
					_fetch( r2, v2 )
					_exeOp( v1, v2 )
					break;	

				case OP_MATHOP:		
					subop = ( inst >> 16 ) & 0xFF;
					_exeFn( v1 )
					break;
					
				case OP_MOVE:	
					r1 = r2;
					break;
					
				case OP_USER_FCN:
					{
						ExprUserFcn* fcn = **((ExprUserFcn***) PC);
						size = fcn -> mNumFcnBins;
						i = v1 * size;
						if ( i >= 0 && i < size )
							v1 = fcn -> mFcn[ i ];
						else if ( i < 0 )
							v1 = fcn -> mFcn[ 0 ];
						else
							v1 = fcn -> mFcn[ size - 1 ];
						PC += 4;
					}
					break;
						
				case OP_WLINEAR:
				case OP_WEIGHT:
					_fetch( r2, v2 )
					float temp = **((float**) PC);
					if ( opcode == OP_WEIGHT ) {
						v1 = temp * v2 + ( 1.0 - temp ) * v1;
						PC += 4; }
					else {
						v1 = **((float**) PC) * v1 + **((float**) PC+4) * v2;
						PC += 8;
					}
					break;
		
			}
		}
		_store( r1, v1 )
	}
	
	return FR0;
}





void ExprVirtualMachine::Chain( ExprVirtualMachine& inVM, float* inC1, float* inC2 ) {
	int tempReg = inVM.FindGlobalFreeReg();
	
	// Move the output of this VM to a reg that won't get overwritten by inVM
	Move( 0, tempReg );
	
	// Now execute inVM (we know it won't touch tempReg)
	mProgram.Append( inVM.mProgram );
	
	// Use the special weight op that combines the two outputs via an embedded addr to a 0 to 1 value
	// Note that the output is moved to register 0
	if ( inC2 ) {
		__addInst( OP_WLINEAR, ( tempReg << 8 ) | 0 )
		mProgram.Append( &inC1, 4 );
		mProgram.Append( &inC2, 4 ); }
	else {
		__addInst( OP_WEIGHT, ( tempReg << 8 ) | 0 )
		mProgram.Append( &inC1, 4 );	
	}
	
	// The reg coloring for this VM is the OR of the two's coloring
	for ( int i = 0; i < NUM_REGS; i++ )
		mRegColor[ i ] |= inVM.mRegColor[ i ];	
		
	PrepForExecution();
}



void ExprVirtualMachine::Assign( ExprVirtualMachine& inExpr ) {

	mProgram.Assign( inExpr.mProgram );
	
	for ( int i = 0; i < NUM_REGS; i++ )
		mRegColor[ i ] = inExpr.mRegColor[ i ];
		
	PrepForExecution();
}
