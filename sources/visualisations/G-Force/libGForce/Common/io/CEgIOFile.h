#ifndef _CEGIOFILE_
#define _CEGIOFILE_

#include "CEgOStream.h"
#include "CEgIFile.h"

/* *** Class usage note ***
	Whenever the client switches modes (ie. from writing to reading or vice versa), the client *must*
	call either seek() or seekEnd() (so that internal output buffer is flushed)
*/

class CEgIOFile : public CEgOStream, public CEgIFile {

	private:
		
	protected:
		
		int							mDoTrunc;
		long						mOBufSize;
		
		enum {
			cDefaultOBufSize			= 70000
		};
		
		

	public:
									CEgIOFile( int inDoTrunc = true, long inOBufSize = cDefaultOBufSize );
		virtual 					~CEgIOFile();
		
		virtual void				open( const CEgFileSpec* inSpecPtr );	
		virtual void				close();

		void						flush();
		
		// Overrides from CEgErr
		virtual void				seek( long inPos );
		virtual long				size();
		virtual void				PutBlock( const void* inSrce, long numBytes );

		static long					sCreatorType;
};

#endif