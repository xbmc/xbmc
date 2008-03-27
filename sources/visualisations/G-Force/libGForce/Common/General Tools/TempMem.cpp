// TempMem.cpp

#include "TempMem.h"

#if EG_MAC
#include <Memory.h>
#endif

TempMem::TempMem() {
	
	mDimSize = 0;
	
	#if EG_MAC
	mTempH = NULL;
	#endif
}


TempMem::~TempMem() {

	#if EG_MAC
	OSErr err;
	if ( mTempH ) {
		::TempHUnlock( mTempH, &err );
		::TempDisposeHandle( mTempH, &err );
	}
	#endif
}


char* TempMem::Dim( long inBytes ) {

	if ( inBytes > mDimSize ) {
		mDimSize = 0;
		
		#if EG_MAC
		OSErr err;
		if ( mTempH ) {
			::TempHUnlock( mTempH, &err );
			::TempDisposeHandle( mTempH, &err );
		}
	
		mTempH = ::TempNewHandle( inBytes, &err );
		if ( ! err ) {
			mDimSize = inBytes;
			::TempHLock( mTempH, &err );
		}
		#else
		mDimSize = inBytes;
		#endif
	}
	
	
	#if EG_MAC
	if ( mTempH )
		return *mTempH;
	else 
		return NULL;
	#else
	return mTemp.Dim( inBytes );
	#endif
}

