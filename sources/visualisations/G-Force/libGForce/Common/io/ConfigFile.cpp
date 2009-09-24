#include "ConfigFile.h"

#include "CEgFileSpec.h"
#include "CEgIFile.h"
#include "..\general tools\ArgList.h"


bool ConfigFile::Load( const CEgFileSpec* inSpec, ArgList& outArgs ) {
	UtilStr str, configText, num;
	CEgIFile file;
	int i, end;
	
	file.open( inSpec );
	
	if ( file.noErr() ) { 
	
		// Read the config and chuck any comments
		while ( file.noErr() ) {
			file.Readln( str );
			i = str.contains( "//" );
			if ( i > 0 )
				str.Keep( i - 1 );
			configText.Append( str );
		} 
		file.throwErr( cNoErr );
		
		// Remove block comments
		do {
			i = configText.contains( "/*" );
			if ( i > 0 ) {
				end = configText.contains( "*/" );
				if ( end > 0 )
					configText.Remove( i, end - i + 2 );
			}
		} while ( i > 0 && end > 0 );
		
		// Parse the args/dict...
		outArgs.SetArgs( configText );
		
    file.close();
    return true; }
	else {
		return false;
	}	
}