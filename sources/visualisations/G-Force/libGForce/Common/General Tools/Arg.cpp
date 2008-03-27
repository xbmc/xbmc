#include "Arg.h"


#include "UtilStr.h"
#include "..\io\CEgOStream.h"



Arg::Arg( long inID, long inData, Arg* inNext ) {

	mID			= inID;
	mIsStr		= false;
	mNext		= inNext;
	
	Assign( inData );
}


	


Arg::Arg( long inID, const char* inStr, Arg* inNext ) {
	mID			= inID;
	mIsStr		= true;
	mNext 		= inNext;
	mData		= NULL; 

	Assign( inStr );
}

	



	
Arg::~Arg() {
	if ( mIsStr && mData )
		delete ((UtilStr*) mData);
		
	if ( mNext )
		delete mNext;
}








void Arg::Assign( long inData ) {
	if ( mIsStr && mData ) 
		delete ((UtilStr*) mData);
		
	mIsStr	= false;
	mData	= inData;
}


void Arg::Assign( const UtilStr* inStr ) {

	if ( inStr )
		Assign( inStr -> getCStr() );
}



void Arg::Assign( const char* inStr ) {
	if ( ! mData || ! mIsStr )
		mData = (long) new UtilStr;
		
	mIsStr = true;
			
	((UtilStr*) mData) -> Assign( inStr );
}



void Arg::ExportTo( CEgOStream* ioStream ) const {
	UtilStr str;
	unsigned char c;
	
	if ( mID > 31 ) {
		for ( int d = 0; d <= 24; d += 8 ) {		// Go thru each byte in ID num
			c = ((mID << d) >> 24 );
			if ( c > 31 && c < 128)
				ioStream -> PutByte( c );
		}
		ioStream -> PutByte( '=' );
		if ( mIsStr )
			str.AppendAsMeta( (UtilStr*) mData );
		else 
			str.Append( (long) mData );
		ioStream -> Write( &str );
	}
}


