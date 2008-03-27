#pragma once

#ifndef _CEGIFILE_
#define _CEGIFILE_


#include "CEgIStream.h"
class CEgFileSpec;

//	Pre:	<inProcArg> will contain the pre passed in as <inProcArg> in ::Search().  
//			<inFilePos> is the file position were a match was found
//	Note:	Returns the number of bytes that should be skipped until resuming the search.  Returns <0 if search should be stopped.
typedef long (*AddHitFcnT)(void* inProcArg, long inFilePos );


class CEgIFile : public CEgIStream {

	protected:
		unsigned long					mFile;

		void							diskSeek( long inPos );
		virtual void					fillBlock( unsigned long inStartPos, void* destPtr, long& ioBytes );
		
		enum {
			cSearchBufSize 				= 65000
		};
		
	public:
										CEgIFile( unsigned short int inBufSize = cDefaultBufSize );
		virtual 						~CEgIFile();
		
		// Client fcns...
		virtual void					open( const CEgFileSpec* inSpecPtr );	
		void							open( const char* inFileName );
		
		virtual void					close();
		inline bool						is_open()						{ return mFile != NULL;		}
		virtual long					size();
		void							seekEnd();
		virtual long					tell();
		virtual void					seek( long inPos );

		void							Search( UtilStr& inSearchStr, void* inProcArg, bool inCaseSensitive, AddHitFcnT inFcn );
};


#endif