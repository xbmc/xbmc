#include "CEgIOFile.h"
#include "CEgFileSpec.h"

long CEgIOFile::sCreatorType = '    ';

CEgIOFile::CEgIOFile( int inDoTrunc, long inOBufSize ) :
	mDoTrunc( inDoTrunc ),
	CEgIFile(),
	CEgOStream() {
	
	mDoTrunc = inDoTrunc;
	mOBufSize = inOBufSize;
	
	if ( mOBufSize < 100 )
		mOBufSize = 100;
}


CEgIOFile::~CEgIOFile() {
	close();
}


#include <xtl.h>
#define 	__OSWOpen( specPtr )			mFile = (long) ::CreateFile( (char*) (specPtr -> OSSpec()), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0 );	\
											if ( ((void*) mFile) == INVALID_HANDLE_VALUE ) {																		\
												mFile = NULL;																										\
												mOSErr = 0;																							\
											}
#define 	__OSWrite( srcePtr, ioBytes )	DWORD wrote; 															\
											bool ok = ::WriteFile( (void*) mFile, srcePtr, ioBytes, &wrote, 0 );	\
											if ( ! ok || ioBytes != wrote )	{										\
												throwErr( cWriteErr );												\
												mOSErr = 0;											\
											}	


void CEgIOFile::open( const CEgFileSpec* inSpecPtr ) {
	
	close();
	throwErr( cNoErr );
	
	if ( inSpecPtr ) {
		if ( mDoTrunc )
			inSpecPtr -> Delete();
			
		__OSWOpen( inSpecPtr )
	}
		
	if ( mFile == 0 ) {
		#if EG_MAC
		if ( mOSErr == fnfErr )
		#else
		if ( mOSErr == ERROR_FILE_NOT_FOUND )
		#endif
			throwErr( cFileNotFound );
		else
			throwErr( cOpenErr );
	}
}






void CEgIOFile::PutBlock( const void* inSrce, long numBytes ) {

	CEgIFile::skip( numBytes );										// Keep mPos up to date
	
	// If we don't want to exceed our buffer limit...
	if ( numBytes + (long) mOBuf.length() > mOBufSize ) {			// Uh oh, we actually  have to write to disk
		// Get rid of what we have waiting first
		flush();
		
		if ( numBytes > mOBufSize /4  && noErr() ) {
//			__OSWrite( inSrce, numBytes ) 
		}
		else
			CEgOStream::PutBlock( inSrce, numBytes ); }
	else
		CEgOStream::PutBlock( inSrce, numBytes );
}



void CEgIOFile::flush() {
	long ioBytes = mOBuf.length();
	
	if ( ! mFile )
		throwErr( cNotOpen );
	else if ( ioBytes > 0 && noErr() ) {
//		__OSWrite( mOBuf.getCStr(), ioBytes )	
		if ( noErr() ) {
			invalidateBuf();							// Invalidate read buffer
			mOBuf.Wipe();								// We're done with the out buffer
		}
	}
}




long CEgIOFile::size() {
	flush();
	
	return CEgIFile::size();
}



void CEgIOFile::seek( long inPos ) {
	
	if ( noErr() ) {
		flush();										// Write any pending data
		
		if ( noErr() ) {
			CEgIFile::seek( inPos );
			diskSeek( inPos );
		}
	}
}






void CEgIOFile::close() {
	
	if ( is_open() ) {
		flush();
	
		CEgIFile::close();
	}
}


