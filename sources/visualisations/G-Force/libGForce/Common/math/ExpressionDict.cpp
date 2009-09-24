#include "ExpressionDict.h"

#include "..\general tools\UtilStr.h"


ExpressionDict::ExpressionDict() :
	mVarDict( true ) {

}



void ExpressionDict::AddVar( char* inKey, float* inPtr ) {

	mVarDict.Put( new UtilStr( inKey ), inPtr );
}


void ExpressionDict::AddFcn( char* inKey, ExprUserFcn** inFcn ) {
	
	
	mVarDict.Put( new UtilStr( inKey ), inFcn );
}


float* ExpressionDict::LookupVar( const UtilStr& inName ) {
	float* addr;
	
	if ( mVarDict.Get( &inName, (void**)&addr ) )
		return addr;
	else
		return NULL;
}


ExprUserFcn** ExpressionDict::LookupFunc( const UtilStr& inName ) {
	
	return (ExprUserFcn**) LookupVar( inName );
}