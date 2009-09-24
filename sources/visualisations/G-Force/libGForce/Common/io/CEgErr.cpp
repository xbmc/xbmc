
#include "CEgErr.h"

#include "..\general tools\CStrNode.h"



CEgErr::CEgErr( long inErr ) :
	mOSErr( 0 ),
	mErr( inErr ) {

}

					

bool	 CEgErr::noErr() {
	return getErr() == cNoErr;
}



long CEgErr::getErr() {
	return mErr;
}



void CEgErr::throwErr( long inErr ) {

	if ( noErr() || inErr == cNoErr ) {
		mErr = inErr;
	}
}





void CEgErr::GetErrStr( UtilStr& outStr ) {
	long err;
	bool handled = false;
	
	if ( mOSErr ) {
		err = mOSErr;  
		OSErrMsg( outStr ); }
	else {
		err = getErr();
		switch ( err ) {

			case cNoErr:
				outStr.Append( "No error." );
				break;

			case cBadPLVersion:
			case cBadExamVersion:
				outStr.Append( "This file was made with a different version of Examgen or is damaged and cannot be opened." );
				break;				
				
			case cCorrupted:
				outStr.Append( "This file appears to be corrupt." );
				break;
				
			case cFileNotFound:
				outStr.Append( "File not found." );
				break;
				
			case cEOFErr:
				outStr.Append( "End of file reached." );
				break;
				
			case cEOSErr:
				outStr.Append( "End of file/stream reached." );
				break;
				
			case cBitmapCorrupted:
				outStr.Append( "The bitmap information is corrupt." );
				break;
				
			case cBitmapTooDeep:
				outStr.Append( "The bitmap must be 256 or less colors." );
				break;

			case cBitmapNotMono:
				outStr.Append( "The bitmap must be monochrome." );
				break;
				
			case cBadBitmapType:
				outStr.Append( "The file is not a BMP file." );
				break;			
			
			case cRLENotSupported:
				outStr.Append( "Compressed BMPs are not supported." );
				break;	
				
			default:
				outStr.Append( "Internal error." );
				break;
		}
	}
	
	outStr.Append( " (" );
	outStr.Append( err );
	outStr.Append( ')' );
}




#ifdef EG_MAC
#include <Errors.h>
#endif
	
void CEgErr::OSErrMsg( UtilStr& ioStr ) {


	switch ( mOSErr ) {
		
		#ifdef EG_MAC
		case ioErr:
			ioStr.Append( "I/O Error" );
			break;
			
		case fnfErr:
			ioStr.Append( "File not found" );
			break;
			
		case opWrErr:
			ioStr.Append( "File is in use" );
			break;
				
		case fLckdErr:
		case permErr:
			ioStr.Append( "File is locked" );
			break;
			
		case vLckdErr:
		case wPrErr:
			ioStr.Append( "Disk is write protected" );
			break;
			
		case wrPermErr:
			ioStr.Append( "File does not allow writing" );
			break;
			
		case dskFulErr:
			ioStr.Append( "Disk full" );
			break;
		#endif
		
		#ifdef EG_WIN
		#include "winerror.h"
			
		case ERROR_FILE_NOT_FOUND:
			ioStr.Append( "File not found" );
			break;
			
		case ERROR_PATH_NOT_FOUND:
			ioStr.Append( "Path not found" );
			break;
			
		case ERROR_ACCESS_DENIED:
			ioStr.Append( "Access denied" );
			break;
				
		case ERROR_NOT_ENOUGH_MEMORY:
			ioStr.Append( "Not enough memory" );
			break;
			
		case ERROR_INVALID_BLOCK:
			ioStr.Append( "Invalid block" );
			break;
			
		case ERROR_INVALID_ACCESS:
			ioStr.Append( "File not found" );
			break;
			
		case ERROR_OUTOFMEMORY:
		case ERROR_NO_MORE_FILES:
		case ERROR_HANDLE_DISK_FULL:
			ioStr.Append( "Disk Full" );
			break;
			
		case ERROR_WRITE_PROTECT:
			ioStr.Append( "Disk is write protected" );
			break;
				
		case ERROR_NOT_READY:
			ioStr.Append( "Disk not ready" );
			break;
			
		case ERROR_DUP_NAME:
			ioStr.Append( "Duplicate name" );
			break;
		#endif
		
		default:
			ioStr.Append( "OS Error" );
	
	}

}

