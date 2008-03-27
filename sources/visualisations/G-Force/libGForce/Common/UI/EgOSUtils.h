
#ifndef _EGOSUTILS_
#define _EGOSUTILS_

#include "../io/CEgFileSpec.h"
#include "..\general tools\XStrList.h"
#include "../EG Common.h"

#define INITAL_DIR_STRLEN	502

class UtilStr;
class CEgFileSpec;
class CEgErr;

class EgOSUtils {
			
						
	public:
		// 			Call this once at program startup/end!!
		//  If you're compiling under windows and making a DLL, pass the DLL instance otherwise NULL is fine.
		static void					Initialize( void* inModuleInstance = NULL );
		static void					Shutdown();
	
		//	Post:	Assigns spec info for the next file in the dir specified by <folderSpec>.  If the directory doesn't exist or there
		//			aren't any more files, <false> is returned.  If a next file is found, <true> is returned and <outSpec> contains a spec to that file.
		//			If inFolders is true, only folders are retuned
		static bool					GetNextFile( const CEgFileSpec& folderSpec, CEgFileSpec& outSpec, bool inStartOver, bool inFolders );
	
		//			After Initialize, these contain the screen pixels per inch
		static int					sXdpi;
		static int					sYdpi;
							
		//	Post:	Makes the computer beep	
		static void					Beep();
		

		//	Pre:	Designed to be called continously during a long process.
		//	Post:	Displays a spinning curor after a certain period of time.
		static void					SpinCursor();
		
		//	Post:	Call this to restore the cursor if it's been altered (ie, after a SpinCursor).
		//	Note:	If This proc is installed in the main event loop, you can call SpinCursor() whenever things are intensive
		static void					ResetCursor()												{ if ( sLastCursor >= 0 ) EgOSUtils::ShowCursor();  }
		
		// 	Post:	Hides/Shows the mouse cursor.
		static void					ShowCursor();
		static void					HideCursor();
		
		// 	Returns the current time in milliseconds since the system start.
		static long					CurTimeMS();
		
		//	Returns the global cords of the mouse
		static void					GetMouse( Point& outPt );
		
		//	Post:	Returns a random number from <min> to <max>, inclusive.
		static long 				Rnd( long min, long max );
		
		//	Post:	Reverses the byte order of the given long
		static unsigned long		RevBytes( unsigned long inNum );
		
		// Is the directory/folder of this app/lib
		static CEgFileSpec			sAppSpec;
		
		// Converts a float HSV (each component is 0 to 1) into a 3x16 bit RGB value
		// H = .2 == 1.2 == 7.2 == -.8 == -3.8  (ie, H values are 'wrapped')
		// S,V = 1.0 == 1.1 == 6.9
		// S,V = 0 == -.1 == -4
		static void					HSV2RGB( float H, float S, float V, RGBColor& outRGB );
		
	protected:
		static XStrList				sFontList;
		static long					sLastCursor;
		static long					sLastCursorChange;
};

#endif