#include "Prefs.h"

#include "CEgIOFile.h"
#include "..\ui\EgOSUtils.h"

#ifdef EG_MAC
#include <Files.h>
#include <Folders.h>
#endif


Prefs::Prefs( const char* inPrefsName, bool inSysStored ) {
	mSysStored	= inSysStored;
	mDirty		= true;
	
	mPrefName.Assign( inPrefsName );
	
	#ifdef EG_MAC
  	short int	theVRef;
	long		theDirID;
	short		theErr;
	FSSpec		prefSpec;
	
	if ( inSysStored ) {
		theErr = ::FindFolder( kOnSystemDisk, kPreferencesFolderType, kCreateFolder, &theVRef, &theDirID );			
		if ( theErr != noErr ) {
			theVRef = 0;
			theDirID = 0;
		} }
	else {
		theVRef		= ( (FSSpec*) EgOSUtils::sAppSpec.OSSpec() ) -> vRefNum;
		theDirID	= ( (FSSpec*) EgOSUtils::sAppSpec.OSSpec() ) -> parID;
	}	
	::FSMakeFSSpec( theVRef, theDirID, mPrefName.getPasStr(), &prefSpec );
	mFileSpec.Assign( &prefSpec, 'TEXT' );
	#endif
	
	#ifdef EG_WIN
	// Note: mSysStored == true is unimplmented--just continue as mSysStored == false
	// (yah, right--like i'm gonna even *think* about touching the registry!)
	UtilStr prefPath;
	prefPath.Assign( (char*) EgOSUtils::sAppSpec.OSSpec() );
	prefPath.Append( mPrefName );
	mFileSpec.Assign( prefPath.getCStr(), 0 );
	#endif
}



CEgErr Prefs::Load() {
	CEgIFile iFile;
	
	mPrefs.Clear();
	iFile.open( &mFileSpec );
	mPrefs.SetArgs( &iFile );
	
	
	if ( iFile.noErr() )
		mDirty = false;
	
	return iFile;
}


CEgErr Prefs::Store() {
	CEgIOFile oFile;
	
	if ( mDirty ) {
		long origType = CEgIOFile::sCreatorType;
		CEgIOFile::sCreatorType = '    ';

		oFile.open( &mFileSpec );
		
		if ( oFile.noErr() ) {

			mPrefs.ExportTo( &oFile, true );
			oFile.Writeln();
		}
		mDirty = false;
		CEgIOFile::sCreatorType = origType;
	}
	
	return oFile;
}


void Prefs::SetPref( long inID, const UtilStr& inData ) { 
	
	if ( ! mDirty ) {
		const UtilStr* str;
		
		str = mPrefs.GetStr( inID );
		if ( str ) {
			if ( str -> compareTo( &inData ) )
				mDirty = true; }
		else
			mDirty = true;
	}
	
	mPrefs.SetArg( inID, inData );
}


void Prefs::SetPref( long inID, long inData ) {
	bool exists;
	long num;
	
	if ( ! mDirty ) {
	
		exists = mPrefs.GetArg( inID, num );
		if ( ! exists || num != inData )
			mDirty = true;
	}
	
	mPrefs.SetArg( inID, inData );
}