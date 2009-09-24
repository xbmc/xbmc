#ifndef ExprVirtualMachine_
#define ExprVirtualMachine_

#include "..\general tools\UtilStr.h"


// Valid math fcn calls for MathOp()
enum {
	cSQRT	= 30,
	cATAN	= 31,
	cABS	= 32,
	cSIN	= 33,
	cCOS	= 34,
	cTAN	= 35,
	cLOG	= 36,
	cEXP	= 37,
	cSQR	= 38,
	cSQWV	= 39,
	cPOS	= 40,
	cRND	= 41,
	cSGN	= 42,
	cTRWV	= 43,
	cCLIP	= 44,
	cSEED	= 45,
	cWRAP	= 46,
	cTRNC	= 47,
	cFLOR   = 48
	
};


struct ExprUserFcn {

	long			mNumFcnBins;
	float			mFcn[ 1 ];
};

		
class ExprVirtualMachine {


	public: 
							ExprVirtualMachine();
							
		// Effectively copies inVM into this
		void				Assign( ExprVirtualMachine& inVM );
			
		// Clears the current program
		void				Clear();
		
		//	Call this once after new instructions are added
		void				PrepForExecution();
		
		//	Executes the current program loaded.  FP register zero is returned.
		float				Execute(); //																	{ return Execute_Inline();					}
		//inline float		Execute_Inline();

		// Performs the op: FP[ inReg ] <- FP[ inReg ] <op> FP[ inReg2 ]
		// inReg is from 0 to 3, and inOpCode can be +,-,*,/,^,%
		void				DoOp( int inReg, int inReg2, char inOpCode );
		
		// Moves the value of register zero to some other register
		void 				Move( int inReg, int inDestReg );
		
		// Sets the value of register zero
		void				Loadi( float inVal, int inReg );
		
		// Sets the value of register zero, forever tying this VM to inVal until it's Cleared
		// Note: 	This VM must be Cleared or deleted before the inVal becomes invalid!  Otherwise,
		//			they'll be a dangling pointer in this VM!
		void				Loadi( float* inVal, int inReg );
		
		// Perfroms one of the above fcns on register zero
		void				MathOp( int inReg, char inFcnCode );
		
		//	Allows a piecewise user functions
		void				UserFcnOp( int inReg, ExprUserFcn** inFcn );
		
		// Use these to access/use a register/memory space
		int					AllocReg();
		void				DeallocReg( int inReg );
		
		// Returns a register that's globally free (ie, a reg that's never touched during Execute())
		int					FindGlobalFreeReg();
		
		/* Makes a copy of inVM and each call to Execute() from now on is equivilent to:
		float v1 = this.Execute();
		float v2 = inVM.Execute();
		return ( *inC1 ) * v1 + ( *inC2 ) * v2;
		Note: If inC2 is NULL, *inC2 will be set to ( 1 - *inC1 ) */
		void				Chain( ExprVirtualMachine& inVM, float* inC1, float* inC2 );
		
		static ExprUserFcn	sZeroFcn;
		
				
	protected:
		
		enum {
			OP_LOADIMMED	= 0x02000000,
			OP_LOAD			= 0x03000000,
			OP_OPER			= 0x04000000,
			OP_MATHOP		= 0x05000000,
			OP_USER_FCN		= 0x06000000,
			OP_MOVE			= 0x0A000000,
			OP_WEIGHT		= 0x0B000000,
			OP_WLINEAR		= 0x0C000000,
			
			NUM_REGS		= 32,
			NUM_PHYS_REGS	= 8
		};
		
		UtilStr				mProgram;
		char				mRegColor[ NUM_REGS ];
		
		// Simple shortcut ptrs to save time.
		const char*			mPCStart;
		const char*			mPCEnd;

};


#endif