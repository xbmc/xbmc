#include "ExprArray.h"

#include "ExpressionDict.h"
#include "..\general tools\ArgList.h"
#include "Expression.h"

// guide me, Lord, guide me...

#include "..\general tools\Hashtable.h"

ExprArray::ExprArray() {
	mExprs	= NULL;
	mVals	= NULL;
	mNumExprs			= 0;
	mDimNumExprs		= 0;
}




ExprArray::~ExprArray() {

	if ( mVals )
		delete []mVals;
		
	if ( mExprs )
		delete []mExprs;
}






void ExprArray::Compile( const ArgList& inArgs, long inID, ExpressionDict& ioDict ) {
	UtilStr str;
	unsigned long i;
	
	// Determine the name of this expression array
	i = inID;
	mIDStr.Wipe();
	while ( i > 0 ) {
		mIDStr.Prepend( (char) ( i & 0xFF ) );
		i = i >> 8;
	}

	// Maintain memory heap for arbitrary array size...
	mNumExprs = inArgs.GetArraySize( inID );
	if ( mNumExprs > mDimNumExprs ) {
	
		if ( mVals )
			delete []mVals;
				
		if ( mExprs )
			delete []mExprs;
			
		mVals	= new float[ mNumExprs + 1 ];
		mExprs	= new Expression[ mNumExprs + 1 ];
		mDimNumExprs = mNumExprs;
	}

	// Add/Insert the vars to the dict
	for ( i = 0; i < mNumExprs; i++ ) {
		str.Assign( mIDStr );
		str.Append( (long) i );
		mVals[ i ] = 0;
		ioDict.AddVar( str, &mVals[ i ] );
	}
		
	// Compile each expression array element
	for ( i = 0; i < mNumExprs; i++ ) {
		inArgs.GetArg( inID, str, i );
		mExprs[ i ].Compile( str, ioDict );
	}
}


void ExprArray::Evaluate() {
	int i;
	
	for ( i = 0; i < mNumExprs; i++ )
		mVals[ i ] = mExprs[ i ].Evaluate();
}


bool ExprArray::IsDependent( char* inStr ) {
	int i;
		
	for ( i = 0; i < mNumExprs; i++ ) {
		if ( mExprs[ i ].IsDependent( inStr ) )
			return true;
			
	}
	
	return false;
}
