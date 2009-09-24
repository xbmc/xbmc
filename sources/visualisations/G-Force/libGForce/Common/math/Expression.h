#ifndef __Equation__
#define __Equation__


#include "..\general tools\UtilStr.h"
#include "ExprVirtualMachine.h"

class ExpressionDict;


class Expression : protected ExprVirtualMachine {
	
	public:
	
		/* Makes a copy of inExpr and each subsequent call to Evaluate() becomes equivilent to:
		float v1 = this.Evaluate();
		float v2 = inExpr.Evaluate();
		return ( *inC1 ) * v1 + ( *inC2 ) * v2;  */	
		// Note: Weight() does *not* update this expression such that calls to IsDependent() also check inExpr
		bool				Weight( Expression& inExpr, float* inC1, float* inC2 );
		
		bool				Compile( const UtilStr& inStr, ExpressionDict& inDict );
		
		inline float		Evaluate()	{ return Execute();	}
		
		bool				IsDependent( char* inStr );
		
		bool				GetNextToken( UtilStr& outStr, long& ioPos );

		void				Assign( Expression& inExpr );
		
		
	protected:
		UtilStr				mEquation;
		bool				mIsCompiled;
		
		static int			Compile( char* inStr, long inLen, ExpressionDict& inDict, ExprVirtualMachine& inVM );
};






#endif