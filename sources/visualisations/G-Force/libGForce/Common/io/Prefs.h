#pragma once

#include "CEgFileSpec.h"
#include "..\general tools\ArgList.h"

class Prefs {


	public:
	
		// inSysStored == true means the this pref file stored within the system (mac -> pref folder, pc -> registry),
		//    otherwise, it's stored in the current folder. 
		// ** NOTE: registry store is unimplemented (in win32), so it just stores on the cur folder
						Prefs( const char* inPrefsName, bool inSysStored );
						
		// How a Prefs file accesses the disk.  To use a pref file, Load() it first, and to save
		//    any changes of it to disk, call Store().			
		CEgErr			Load();
		CEgErr			Store();
		
		//	Returns true if the pref ID was found
		bool			GetPref( long inID, UtilStr& outData )				{ return mPrefs.GetArg( inID, outData );	}
		long			GetPref( long inID )								{ return mPrefs.GetArg( inID );				}
		float			GetFloatPref( long inID )							{ return mPrefs.GetFloat( inID );			}
		
		// 	Sets the given pref to the given data.
		void			SetPref( long inID, const UtilStr& inData );
		void			SetPref( long inID, long inData );
		
		//	Overrides what this wants to do with the prefs on a Store().
		//  For example, if you called SetNotDirty() right before a Store() call, nothing would ever get written to the prefs file.
		//	This fcn may be useful if you mess around with the loaded prefs then want to not save the changes on the next Store().
		void			SetNotDirty()										{ mDirty = false;							}
		
		
	protected:
		UtilStr			mPrefName;
		bool			mSysStored;
		bool			mDirty;
		CEgFileSpec		mFileSpec;
		ArgList			mPrefs;
};

