#include "CEgIFile.h"

#include "CEgFileSpec.h"


#include "..\ui\EgOSUtils.h"






#ifdef EG_MAC

#include <Files.h>
#include <Errors.h>

#define __OSROpen( specPtr )			short int refNum;																\
										mOSErr = ::FSpOpenDF( (FSSpec*) specPtr -> OSSpec(), fsRdPerm, &refNum );		\
										if ( mOSErr == ::noErr )														\
											mFile = refNum;

#define	__OSClose						mOSErr = ::FSClose( mFile );													\
										if ( mOSErr != ::noErr )														\
											throwErr( cCloseErr );																					


#define __OSRead( destPtr, ioBytes )	OSErr err = ::FSRead( mFile, &ioBytes, destPtr );								\
										if ( err != ::noErr && err != eofErr ) {										\
											throwErr( cReadErr );														\
											mOSErr = err;																\
										}
#endif





#ifdef EG_WIN16

#include <stdio.h>

#define __OSROpen( specPtr )			mFile = (long) fopen( (char*) (specPtr -> OSSpec()), "rb" );			
																							

#define	__OSClose						if ( fclose( (FILE*) mFile ) != 0 ) {									\
											throwErr( cCloseErr );												\											\
										}


#define __OSRead( destPtr, ioBytes )	DWORD outRead = fread( destPtr, 1, ioBytes, (FILE*) mFile );			\
										if ( outRead > 0 || ioBytes == 0 ) 										\
											ioBytes = outRead;													\
										else {																	\
											throwErr( cReadErr );												\
										}	

#endif




#ifdef EG_WIN32


#include <xtl.h>
#include <stdio.h>

#define __OSROpen( specPtr )			mFile = (long) fopen(((char*) (specPtr -> OSSpec()))+1,"rb"); \
                    if (mFile < 0) {														\
											mFile = NULL;																						\
											mOSErr = 0;																			\
										}

#define	__OSClose			if (!fclose((FILE*)(mFile)))                          \
                      {                                                 \
                        throwErr( cCloseErr );													\
											  mOSErr = 0;																			\
                      }                                                  

#define __OSRead( destPtr, ioBytes )	DWORD outRead;												\
                    outRead = fread(destPtr,1,ioBytes,(FILE*)(mFile));                 \
                    if ( outRead > 0 || ioBytes == 0)                                         \
                      ioBytes = outRead;                  \
                    else                                      \
									  {																									\
                      throwErr( cReadErr );																				\
                      mOSErr = 0; \
                      ioBytes = 0; \
										}
#endif




CEgIFile::CEgIFile( unsigned short int inBufSize ) :
	mFile( 0 ),
	CEgIStream( inBufSize ) {

}






CEgIFile::~CEgIFile() {

	close();
}











void CEgIFile::close() {

	if ( is_open() ) {
		__OSClose
		
		mFile = 0;
		invalidateBuf();
	}
}





void CEgIFile::open( const CEgFileSpec* inSpec ) {

	close();
	throwErr( cNoErr );
	mPos = 0;
	
	if ( inSpec ) {
		__OSROpen( inSpec )	
	}
	
	if ( mFile == 0 ) {
		#if EG_MAC
		if ( mOSErr == fnfErr )
		#elif EG_WIN
		if ( mOSErr == ERROR_FILE_NOT_FOUND || mOSErr == ERROR_PATH_NOT_FOUND )
		#endif
			throwErr( cFileNotFound );
		else
			throwErr( cOpenErr ); 
	}

}



void CEgIFile::open( const char* inFileName ) {
	CEgFileSpec fileSpec( inFileName );
	
	open( &fileSpec );
}



long CEgIFile::size() {
	long retSize = 0;
	long curPos = tell();
	
	if ( mFile ) {
	
		#ifdef EG_MAC
		::GetEOF( mFile, &retSize );
		#endif
		
		#ifdef EG_WIN16
		if ( fseek( (FILE*) mFile, 0, SEEK_CUR ) == 0 )
			retSize = ftell( (FILE*) mFile );
		#endif
		
		#ifdef EG_WIN32
    if (fseek((FILE*)mFile,0,SEEK_END))
		  retSize = ftell((FILE*)mFile);
    #endif
	}
	
	if ( curPos >= 0 && curPos <= retSize )	
		seek( curPos );
		
	return retSize;
}



void CEgIFile::seek( long inPos ) {

	mNextPtr = getCStr() + inPos - mBufPos;
	mPos = inPos;
}




long CEgIFile::tell() {

	return mPos;
}



void CEgIFile::seekEnd() {

	seek( size() );
}



void CEgIFile::diskSeek( long inPos ) {
	
	if ( noErr() && mFile ) {
	
		#ifdef EG_MAC
		mOSErr = ::SetFPos( mFile, fsFromStart, inPos );
		if ( mOSErr != ::noErr ) 
			throwErr( cSeekErr );
		#endif
		
		#ifdef EG_WIN16
		fseek( (FILE*) mFile, inPos, SEEK_SET );
		if ( ferror( (FILE*) mFile ) )
			throwErr( cSeekErr );
		#endif
		
		#ifdef EG_WIN32
    fseek( (FILE*) mFile, inPos, SEEK_SET );
    if (ftell((FILE*)mFile) != inPos)
      throwErr( cSeekErr );
    // HACK HACK! should really check for ferror. ohwell
		#endif
	}	
}



void CEgIFile::fillBlock( unsigned long inStartPos, void* destPtr, long& ioBytes ) {
	
	if ( ! mFile )
		throwErr( cNotOpen );
		
	diskSeek( inStartPos );
	
	if ( noErr() && ioBytes > 0 ) {
	
		__OSRead( destPtr, ioBytes )
		
		
		if ( noErr() && ioBytes <= 0 )
			throwErr( cEOFErr );
	}

}



void CEgIFile::Search( UtilStr& inSearchStr, void* inProcArg, bool inCaseSensitive, AddHitFcnT inAddHitFcn ) {
	unsigned char*	buf = new unsigned char[ cSearchBufSize ];
	unsigned char*	curPtr, *endPtr;
	unsigned char 	srchChar, srchCharLC, c;
	unsigned long	strLen = inSearchStr.length();
	unsigned long	bufLen, bufPos = 0, fileSize = size();
	
	srchChar	= inSearchStr.getChar( 1 );
	if ( srchChar >= 'a' && srchChar <= 'z' )
		srchChar -= 32;
	srchCharLC	= srchChar + 32;

	while ( noErr() && bufPos + strLen < fileSize ) {
		EgOSUtils::SpinCursor();
		seek( bufPos );
		bufLen = GetBlock( buf, cSearchBufSize );
		if ( bufLen >= strLen ) {
			curPtr 		= buf;
			endPtr 		= buf + bufLen - strLen;
			while ( curPtr <= endPtr ) {
				c = *curPtr;
				if ( (c == srchChar) || (c == srchCharLC) ) {
					if ( UtilStr::StrCmp( inSearchStr.getCStr(), (char*) curPtr, strLen, inCaseSensitive ) == 0 )  {
						long reqSkip = inAddHitFcn( inProcArg, bufPos + curPtr - buf );			// Add hit and If client told us to abort search...
						if ( reqSkip < 0 ) {
							curPtr = endPtr;													// Exit inner loop
							bufPos = fileSize; }												// Exit block reading loop
						else
							curPtr += reqSkip;
					}		
				}
				curPtr++;
			}	
			bufPos += curPtr - buf + 1;
		}
	}
	
	delete buf;
}
