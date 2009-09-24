#ifndef _ExprArray_H
#define _ExprArray_H



#include "..\general tools\UtilStr.h"
#include "Expression.h"

class ArgList;
class ExpressionDict;
class Hashtable;
class XPtrList;
class XLongList;

/* If an arglist has as an array of values (ex., ad1, ad2, ad3, ad4...), this class
helps extract them and evaluate them easily.  */

class ExprArray {


	public:	
							ExprArray();
		virtual				~ExprArray();
		
		// Returns how many exrs are compiled/ready to be evaluated
		inline long			Count()	const								{ return mNumExprs;		}	
		
		// Extracts a sequence of args from an arglist (ie., and array), and compiles them (with 
		// the given link dictionary).  If an arglist was known to contain ad1, ad2, ad3, ad4...,
		// we call: Compile( args, 'ad', theDict );
		// Post: Each identifier/element is added to ioDict.
		virtual void		Compile( const ArgList& inArgs, long inID, ExpressionDict& ioDict );

		// Each loaded expression is evaluated and placed in mVals
		void				Evaluate();
		
		inline float		Evaluate( long inN ) {  return mExprs[ inN ].Evaluate();  }
		
		// See Expression::IsDependent()
		// Returns if any of the elements of this ExprArray are dependent
		bool				IsDependent( char* inStr );

	protected:
		float*				mVals;
		Expression*			mExprs;
		long				mNumExprs, mDimNumExprs;
		UtilStr				mIDStr;	
};


#endif
