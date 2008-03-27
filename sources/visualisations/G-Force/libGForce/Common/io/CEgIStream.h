
#ifndef _CEGISTREAM_
#define _CEGISTREAM_




#include "..\general tools\UtilStr.h"
#include "..\io\CEgErr.h"





class CEgIStream : protected UtilStr, public virtual CEgErr {

	protected:
		enum {
			cDefaultBufSize					= 5500
		};
		
	public:

		
										CEgIStream( unsigned short int inBufSize = cDefaultBufSize );
		
		// Client fcns...
		long							GetLong();
		signed short int				GetShort();
		unsigned char					GetByte();
		unsigned char					PeekByte();
		float							GetFloat();
		double							GetDbl();
		inline void						Readln( UtilStr& outStr )				{ Readln( &outStr ); 	}
		void							Readln( UtilStr* outStr );
		void							Readln();
		void							Read( UtilStr& outStr, unsigned long inBytes );
		
		//	Post:	Reads next token (tokens separated by a space or a CR,LF,TAB,SPACE). 
		//	Note:	For Read( UtilStr& ), true is returned if a new line was reached.
		float							ReadFloat();
		long							ReadInt();
		bool							Read( UtilStr& outStr );
		void							Read();
		
		long							GetBlock( void* destPtr, unsigned long inBytes );
		virtual void					skip( long inBytes );

		
		unsigned char					GetByteSW();
		void							ReadNumber( UtilStr& outStr );
		bool							AssertToken( const char* inStr );
		
		//	Post:	Reads <inBytes> worth of data from <inSource> and resets this stream for that data (at pos = 0)
		//	Note:	These fcns are *only* designed as tools for using CEgIStream as a memory (RAM) stream!
		void							Assign( CEgIStream* inSource, long inBytes );
		void							Assign( void* inSource, long inBytes );
		void							Assign( const UtilStr& inSrce );
		//	Post:	Resets this stream to the beginning of its buffer.  This should *only* in conjuction following
		//			a call to Assign().  
		void							ResetBuf();
		
		//	Pre:	<inSrce> *cannot*, for obvious reasons, be destroyed before this is.
		//	Post:	All subsequent 'Get...', skip, and Readln fcns will all read from <inSrce>
		//	Note:	Call this fcn each time you want to reset it.
		// 	Note:	See that the difference between Tie and Assign is that Assign makes an independent copy of the data
		void							Tie( const UtilStr* inSrce );
		void							Tie( const char* inSrce, long inNumBytes = -1 );


	
	protected:
		bool							mIsTied;
		unsigned short					mReadBufSize;
		const char*						mNextPtr;				// Shortcut ptr to the data
		long							mBufPos;				// File pos where buf begins at (0 = begin of file, etc)
		long							mPos;					// Virtual stream/file position

				
		void							fillBuf();
		void							invalidateBuf();
		virtual void					fillBlock( unsigned long inStartPos, void* destPtr, long& ioBytes );
		
				
		static UtilStr					sTemp;
};



#endif