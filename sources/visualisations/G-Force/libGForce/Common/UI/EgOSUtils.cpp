#include "EgOSUtils.h"

#include "..\io\CEgFileSpec.h"
#include "..\io\CEgErr.h"
#include "..\general tools\UtilStr.h"

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <xtl.h>
#include <io.h>



int				EgOSUtils::sXdpi				= 72;
int				EgOSUtils::sYdpi				= 72;
long			EgOSUtils::sLastCursor			= -1;
long			EgOSUtils::sLastCursorChange 	= -1;
CEgFileSpec 	EgOSUtils::sAppSpec;
XStrList		EgOSUtils::sFontList( cDuplicatesAllowed );


void EgOSUtils::Initialize( void* inModuleInstance ) {
	
	srand( timeGetTime() );

	sAppSpec.Assign( "q:\\visualisations\\", 0);

//>	char path[ 700 ];
//>	long len = ::GetModuleFileName( (HINSTANCE) inModuleInstance, path, 699 );
//>	if ( len ) {
//>		UtilStr fdir( path, len );
//>		fdir.Keep( fdir.FindLastInstanceOf( '\\' ) );
//>		sAppSpec.Assign( fdir.getCStr(), 0 );
//>	}
}


void EgOSUtils::Shutdown() {
}


void EgOSUtils::SpinCursor() {
}



void EgOSUtils::ShowCursor() {
}


void EgOSUtils::HideCursor() {
}



bool EgOSUtils::GetNextFile( const CEgFileSpec& folderSpec, CEgFileSpec& outSpec, bool inStartOver, bool inFolders ) {
	bool ok = false;

	static intptr_t ipSearch;
  static _finddata_t data;
  
  /*WIN32_FIND_DATA		fileData;
	static HANDLE		hSearch;*/

	UtilStr				name;
	bool				isDir, tryAgain;
	
	do {
		if ( inStartOver ) {
			inStartOver = false;
			name.Assign( (char*) folderSpec.OSSpec() );
			if ( name.getChar( name.length() ) == '/' )
				name.Trunc( 1 );
			name.Trunc(1,false);

//>			ok = SetCurrentDirectory( name.getCStr() );
			ok = true;
			if ( ok ) {
				char path[1024];
				strcpy(path, name.getCStr() );
				strcat(path, "/*.*");
				//hSearch = ::FindFirstFile( path, &fileData ); 
        ipSearch = _findfirst(path,&data);
				ok = (ipSearch != -1);
        //ok = hSearch != INVALID_HANDLE_VALUE; 
			} }
		else
    {
			ok = (_findnext(ipSearch,&data) == 0 );
      //ok = ::FindNextFile( hSearch, &fileData );
    }
		if ( ok ) {
			//name.Assign( fileData.cFileName );
      name.Assign(data.name);
//>			isDir = ::GetFileAttributes( fileData.cFileName ) & FILE_ATTRIBUTE_DIRECTORY;

			isDir = false;			//HACK!HACK!HACK!HACK!

			if ( isDir == inFolders ) {
				tryAgain = name.compareTo( "." ) == 0 || name.compareTo( ".." ) == 0;
				outSpec.Assign( folderSpec );
				if ( isDir ) 
					name.Append( "/" );
				outSpec.Rename( name ); }
			else
				tryAgain = true;
		}
	} while ( ok && tryAgain );

	return ok;
}



void EgOSUtils::Beep() {
}



long EgOSUtils::CurTimeMS() {
	return ::timeGetTime();
}


void EgOSUtils::GetMouse( Point& outPt ) {
//>	POINT p;
//>	::GetCursorPos( &p );

	//wird teilweise von wave benutzt?!?    muss nicht, kann aber von der vmachine benutzt werden

//>	outPt.h = p.x;
//>	outPt.v = p.y;
}

long EgOSUtils::Rnd( long min, long max ) {
	long maxRnd 	= RAND_MAX;
	long retNum 	= rand() * ( max - min + 1 ) / maxRnd + min;
	
	if ( retNum >= max )
		return max;
	else
		return retNum;
}




unsigned long EgOSUtils::RevBytes( unsigned long inNum ) {
	return ( inNum << 24 ) | ( ( inNum & 0xFF00 ) << 8 ) | ( ( inNum & 0xFF0000 ) >> 8 ) | ( inNum >> 24 );
}



#define __SET_RGB( R, G, B ) 	\
	if ( R < 0 )				\
		outRGB.red = 0;			\
	else if ( R <= 0xFFFF )		\
		outRGB.red = R;			\
	else						\
		outRGB.red = 0xFFFF;	\
	if ( G < 0 )				\
		outRGB.green = 0;		\
	else if ( G <= 0xFFFF )		\
		outRGB.green = G;		\
	else						\
		outRGB.green = 0xFFFF;	\
	if ( B < 0 )				\
		outRGB.blue = 0;		\
	else if ( B <= 0xFFFF )		\
		outRGB.blue = B;		\
	else						\
		outRGB.blue = 0xFFFF;	\
	break;
		




void EgOSUtils::HSV2RGB( float H, float S, float V, RGBColor& outRGB ) { 
	// H is given on [0, 1] or WRAPPED. S and V are given on [0, 1]. 
	// RGB are each returned on [0, 1]. 
	long hexQuadrant, m, n, v; 
	H = ( H - floor( H ) ) * 6;  // Wrap the Hue angle around 1.0, then find quadrant
		
	hexQuadrant = H; 
	float f = H - hexQuadrant; 
	
	// Check sat bounds
	if ( S < 0 )
		S = 0;
	if ( S > 1 )
		S = 1;
		
	// Check val bounds
	if ( V < 0 )
		V = 0;
	if ( V > 1 )
		V = 1;
		
	if ( ! ( hexQuadrant & 1 ) ) 
		f = 1 - f; // hexQuadrant i is even 
		
	V *= 65535.0;
	v = V;
	m = V * (1 - S);
	n = V * (1 - S * f); 

	switch ( hexQuadrant ) { 
		case 1: __SET_RGB( n, v, m ); 
		case 2: __SET_RGB( m, v, n ); 
		case 3: __SET_RGB( m, n, v ); 
		case 4: __SET_RGB( n, m, v ); 
		case 5: __SET_RGB( v, m, n ); 
		default: 
			__SET_RGB( v, n, m ); 
	}
} 
