#include "CEgOStream.h"
#include "CEgIStream.h"



CEgOStream::CEgOStream() {

}








void CEgOStream::PutByte( unsigned char inChar ) {
		
	PutBlock( &inChar, 1 );
}




void CEgOStream::PutLong( long inNum ) {
	unsigned long int u = inNum;

	PutByte( (unsigned char) (u & 0xFF) );
	u /= 0x100;
	PutByte( (unsigned char) (u & 0xFF) );
	u /= 0x100;
	PutByte(  (unsigned char) (u & 0xFF) );
	PutByte( (unsigned char) (u / 0x100) );
}



void CEgOStream::PutShort( signed short int inNum ) {
	unsigned short int u = inNum;
	
	PutByte( (unsigned char) (u & 0xFF) );
	PutByte( (unsigned char) (u / 0x100) );
}



void CEgOStream::Writeln( const char* inStr ) {
	
	if ( noErr() ) 	{
		Write( inStr );
		PutByte( 13 );
		
		// Our readln() is set up so it doesn't matter if LFs are present or not.  Since 99% of the time,
		// files will be edited on the platform they were made on, put LFs in the windows files.
		#if EG_WIN
		PutByte( 10 );
		#endif
	}											
	

		
}
void CEgOStream::Writeln( const UtilStr& inStr ) {
	
	if ( noErr() ) 	
		PutBlock( inStr.getCStr(), inStr.length() );	// Put string
		
	Writeln();
	
}


void CEgOStream::Write( const char* inStr ) {
	const char*	s = inStr;
	
	if ( inStr ) {
		while ( *s )
			s++;

		PutBlock( inStr, s - inStr ); 
	}	
}



void CEgOStream::Write( const UtilStr* inData ) {
	
	if ( inData )
		PutBlock( inData -> getCStr(), inData -> length() );
}


void CEgOStream::PutBlock( const void* inSrce, long numBytes ) {
	
	mOBuf.Append( (char*) inSrce, numBytes );
}


void CEgOStream::PutBlock( CEgIStream& inStream, long inBytes ) {
	static UtilStr buf;
	
	buf.Assign( inStream, inBytes );
	if ( inStream.noErr() ) 
		PutBlock( buf.getCStr(), inBytes );
	else
		throwErr( cOStreamEOfIS );
}





void CEgOStream::skip( long inBytes ) {
	
	if ( inBytes > 0 )
		PutBlock( NULL, inBytes );
}



void CEgOStream::Reset() {
	mOBuf.Wipe();
	throwErr( ::cNoErr );
}





