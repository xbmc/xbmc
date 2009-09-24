
#ifndef __GFPixPort__
#define __GFPixPort__

#include "..\Common\ui\PixPort.h"

class GForcePixPort : public PixPort {

	public:
								GForcePixPort();
					
		void					SetTrackTextFont( UtilStr& inName, long inSize );
					
		void					SetConsoleFont()				{  SelectFont( mConsoleFontID );	}
		void					SetTrackTextFont()				{  SelectFont( mTrackTextFontID );	}
				
		
	protected:
		long					mTrackTextFontID;
		long					mConsoleFontID;


};



#endif