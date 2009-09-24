#include "CEgIStream.h"

//#include <string.h>


UtilStr CEgIStream::sTemp;



CEgIStream::CEgIStream( unsigned short int inBufSize ) {
	mReadBufSize	= inBufSize;
	mIsTied			= false;
	mBufPos			= 0;
	mPos			= 0;
	
	Wipe();
}










long CEgIStream::GetLong() {		
	register unsigned long c, n = GetByte();
	
	c = GetByte();
	n = n | ( c << 8 );
	
	c = GetByte();
	n = n | ( c << 16 );
	
	c = GetByte();
	
	return (long) (n | ( c << 24 ));
}


float CEgIStream::GetFloat() {
	long v = GetLong();
	return *( (float*) &v );
}




short int CEgIStream::GetShort() {
	register unsigned long n = GetByte();
	
	return (short int) ((GetByte() << 8) | n);
}





unsigned char CEgIStream::GetByte() {
	register unsigned char c = 0;
	
	if ( mIsTied ) {
		if ( mPos != 0 ) {
			c = *((unsigned char*) mNextPtr);
			mNextPtr++;
			mPos++; }
		else
			throwErr( cTiedEOS ); }
	else if ( mPos < mBufPos + mStrLen && mPos >= mBufPos ) {
		c = *((unsigned char*) mNextPtr);
		mNextPtr++;
		mPos++; }
	else if ( noErr() ) {
		fillBuf();
		c = GetByte();
	}
	
	return c;
}



unsigned char CEgIStream::PeekByte() {
	register unsigned char c = 0;
	
	if ( mIsTied ) {
		if ( mPos != 0 )
			c = *((unsigned char*) mNextPtr); }
	else if ( mPos < mBufPos + mStrLen && mPos >= mBufPos ) 
		c = *((unsigned char*) mNextPtr);
	else if ( noErr() ) {
		fillBuf();
		if ( noErr() )
			c = PeekByte();
		else
			throwErr( cNoErr );
	}
	
	return c;
}







/*
float CEgIStream::GetFloat() {
	//ah!
}



double CEgIStream::GetDbl() {
	ah!
}
*/


void CEgIStream::Readln( UtilStr* outStr ) {
	unsigned char p, c = GetByte();
	
	if ( outStr ) {
		outStr -> Wipe();
				
		while ( noErr() && c != 13 && c != 10 ) {		// Stop on a CR or LF or error
			outStr -> Append( (char) c );
			c = GetByte();
		}
		
		p = PeekByte();
		if ( ( p == 13 && c == 10 ) || ( p == 10 && c == 13 ) ) 
			GetByte();
	}
}




void CEgIStream::Readln() {
	unsigned char p, c = GetByte();
			
	while ( noErr() && c != 13 && c != 10 ) 		// Stop on a CR or LFor error
		c = GetByte();
		
	p = PeekByte();
	if ( ( p == 13 && c == 10 ) || ( p == 10 && c == 13 ) ) 
		GetByte();

}



unsigned char CEgIStream::GetByteSW() {
	unsigned char c = GetByte();

	while ( noErr() && (c == 13 || c == 10 || c == 32 || c == 9) )
		c = GetByte();

	return c;
}


bool CEgIStream::Read( UtilStr& outStr ) {
	outStr.Wipe();
		
	unsigned char c = GetByteSW();
				
	while ( noErr() && c != 13 && c != 9 && c != 32 && c != 10 ) {	// Stop on a CR, LF, TAB, space or error
		outStr.Append( (char) c );
		c = GetByte();
	}
	return c == 13 || c == 10;
}



void CEgIStream::Read() {
	unsigned char c = GetByteSW();
				
	while ( noErr() && c != 13 && c != 9 && c != 32 && c != 10 ) 		// Stop on a CR, LF, space or error
		c = GetByte();
}




float CEgIStream::ReadFloat() {
	Read( sTemp );
	return ((float) sTemp.GetValue( 100000 )) / 100000.0;
}


long CEgIStream::ReadInt() {
	Read( sTemp );
	return sTemp.GetValue();
}

		
		
void CEgIStream::Read( UtilStr& outStr, unsigned long inBytes ) {
	outStr.Assign( NULL, inBytes );
	GetBlock( outStr.getCStr(), inBytes );
}



void CEgIStream::ReadNumber( UtilStr& outStr ) {

	outStr.Wipe();
	char c = GetByteSW();
	
	while ( noErr() && ( c == '.' || (c >= '0' && c <= '9') ) ) {
		outStr.Append( c );
		c = GetByte();
	}
}





bool CEgIStream::AssertToken( const char* inStr ) {
	char c = GetByteSW();
	
	// Check first byte
	if ( *inStr != c || ! noErr() )
		return false;
	inStr++;

	// Make sure following bytes in the stream match inStr 
	while ( *inStr ) {
		c = GetByte();
		if ( *inStr != c || ! noErr() )
			return false;
		inStr++;
	}
	
	return true;
}



long CEgIStream::GetBlock( void* destPtr, unsigned long inBytes ) {
	long bytesRead = inBytes;
		
	if ( mIsTied ) {
		if ( - mPos >= inBytes ) 
			UtilStr::Move( destPtr, mNextPtr, bytesRead );
		else {
			bytesRead = 0;
			throwErr( cTiedEOS ); 
		} }
	else if ( mPos >= mBufPos && mPos + bytesRead <= mBufPos + mStrLen )
		UtilStr::Move( destPtr, mNextPtr, bytesRead );
	else
		fillBlock( mPos, destPtr, bytesRead );
		
	mPos		+= bytesRead;
	mNextPtr	+= bytesRead;
	
	return bytesRead;
}



void CEgIStream::fillBuf() {
	long bytes = mReadBufSize;
	
	Dim( bytes );
	mNextPtr = getCStr();
	mBufPos = mPos;
	if ( length() < bytes )					// Verify that we dimmed ok
		bytes = length();
	fillBlock( mPos, getCStr(), bytes );
	if ( bytes <= 0 )
		throwErr( cEOSErr );
	mStrLen = bytes;						// Our str len could have only gotten shorter
}



void CEgIStream::fillBlock( unsigned long, void*, long& ioBytes ) {
	ioBytes = 0;

}
	


void CEgIStream::invalidateBuf() {

	Wipe();
}




void CEgIStream::skip( long inBytes ) {

	mPos		+= inBytes;
	mNextPtr	+= inBytes;
}




void CEgIStream::ResetBuf() {
	throwErr( cNoErr );
	
	mIsTied		= false;
	mNextPtr	= getCStr();
	mBufPos		= 0;
	mPos		= 0;
}


void CEgIStream::Assign( const UtilStr& inSrce ) {
	Assign( inSrce.getCStr(), inSrce.length() );
}

										
void CEgIStream::Assign( void* inSource, long inBytes ) {
	UtilStr::Assign( inSource, inBytes );
	ResetBuf();
}


void CEgIStream::Assign( CEgIStream* inSource, long inBytes ) {
	

	if ( inSource ) {
		Dim( inBytes );
		if ( length() < inBytes )
			inBytes = length();
		inSource -> GetBlock( getCStr(), inBytes );
	}
	
	ResetBuf();
}



void CEgIStream::Tie( const UtilStr* inSource ) {

	Tie( inSource -> getCStr(), inSource -> length() );
}




void CEgIStream::Tie( const char* inSrce, long inNumBytes ) {
	
	throwErr( cNoErr );
	mIsTied		= true;
	mPos		= - inNumBytes;			// When mPos reaches zero, we're at tied end of stream
	mNextPtr	= inSrce;				// Set up our data ptr
	
	// If -1 was passed in thru inNumBytes, must calc the length of the C str.
	if ( inNumBytes < 0 ) {
		mPos = 0;
		while ( *inSrce ) {
			mPos--;
			inSrce++;
		}
	}
	
	if ( ! mNextPtr )
		mPos = 0;
}