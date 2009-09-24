#include "GForcePixPort.h"


#define __consoleFont		"Courier New"



GForcePixPort::GForcePixPort() {

	mConsoleFontID = CreateFont();
	AssignFont( mConsoleFontID, __consoleFont, 9 );
	
	mTrackTextFontID = CreateFont();
}


void GForcePixPort::SetTrackTextFont( UtilStr& inName, long inSize ) {

	AssignFont( mTrackTextFontID, inName.getCStr(), inSize );
}
