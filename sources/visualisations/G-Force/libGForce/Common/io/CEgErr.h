

#ifndef _CEGERR_
#define _CEGERR_

enum {
	cNoErr				= 0,
	cBadClassID			= -554,
	cCorrupted			= -555,
	cBadExamVersion		= -556,
	cBadPLVersion		= -557,
	

	cEOFErr				= -558,
	cReadErr			= -559,
	cNotOpen			= -560,
	cOpenErr			= -561,
	cFileNotFound		= -625,
	cCloseErr			= -562,
	cCreateErr			= -563,
	cWriteErr			= -564,
	cSeekErr			= -565,
	
	
	cCantFetchPLFile	= -566,
	cPLItemNotFound		= -567,
	
	cEOSErr				= -568,
	cTiedEOS			= -569,
	cOStreamEOfIS		= -570,
	
	cResourceNotFound	= -590,
	cResHeaderCorrupt	= -591,
	cResourceInvalid	= -592,
	
	cBitmapCorrupted	= -595,
	cBitmapNotMono		= -596,
	cBitmapTooDeep		= -597,
	cBadBitmapType	 	= -598,
	cPICTNotSupported	= -599,
	cRLENotSupported	= -600,
	cNoImageOnClip		= -601,
	cUnsupportedMetafileMM = -602,
	
	cBadPDFVersion		= -650,
	cNoStartXRefFound	= -651,
	cBadXRefLoad		= -652,
	cBadObjRefDef		= -653,
	cPStringExpected	= -654,
		
	cBadParseDictValue	= -660,
	cNoEndDictFound		= -661,
	cNoStartDictFound	= -662,

	cEndOfArrayExpected	= -670,
	
	cStreamLoadErr		= -680
};


#include "..\general tools\UtilStr.h"

class CEgErr {

	protected:
		short int			mErr;
		short				mOSErr;


		void				OSErrMsg( UtilStr& ioStr );	
		
	public:
							CEgErr( long inErr = cNoErr );
						
		virtual bool		noErr();
		virtual void		throwErr( long inErr );
		virtual long		getErr();
		virtual void		GetErrStr( UtilStr& outStr );

};

#endif