#ifndef ExpressionDict_H
#define ExpressionDict_H

#include "..\general tools\Hashtable.h"
#include "..\general tools\nodeClass.h"
#include "ExprVirtualMachine.h"


class FcnEntry : public nodeClass {

	public:
		UtilStr					mName;
		long					mSize;
		float**					mFcn;

};



class ExpressionDict {


	public:
								ExpressionDict();
								
								
		void					AddVar( UtilStr& inKey, float* inPtr )				{ AddVar( inKey.getCStr(), inPtr );	}
		void					AddVar( char* inKey, float* inPtr );
		
		void					AddFcn( char* inKey, ExprUserFcn** inFcn );


		float*					LookupVar( const UtilStr& inName );
		ExprUserFcn**			LookupFunc( const UtilStr& inName );
		
	protected:
		Hashtable				mVarDict;
		nodeClass				mFcnEntries;

};


#endif



