#ifndef __TempMem__
#define __TempMem__


#include "UtilStr.h"

/* This class allocs a chunk of the system/shared memory */

class TempMem {

	public:	
								TempMem();
								~TempMem();
								
		//	Returns a ptr a block inBytes big (from temporary memory)
		char*					Dim( long inBytes );


	protected:
		long					mDimSize;
		
		#if EG_WIN
		UtilStr					mTemp;
		#elif EG_MAC
		char**					mTempH;
		#endif
};





#endif