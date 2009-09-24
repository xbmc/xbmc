#ifndef GF_Palette_h
#define GF_Palette_h

#include "..\Common\math\ExpressionDict.h"
#include "..\common\math\ExprArray.h"
#include "..\common\ui\PixPort.h"

class ArgList;

/* Takes an arg list, looks for 'H', 'S', 'V' in terms of T, and I.  T represents the system time
index (in seconds) and I is the intensity param value, ranging from 0 to 1, where all line
drawing drawing draws an intensity of 1.  */

class GF_Palette {

	public: 
							GF_Palette( float* inT, float* inIntensity );
						
		// Compile the 'H', 'S', and 'V' expressions.	
		void				Assign( const ArgList& inArgs );
		
				
		// Evaluates the palette based on the current time
		void				Evaluate( PixPalEntry outPalette[ 256 ] );
		
		void				SetupTransition( GF_Palette* inDest, float* inC );

	
	protected:
		float*				mIntensity, mPI;
		Expression			mH, mS, mV;
		ExpressionDict		mDict;
		bool				mH_I_Dep, mS_I_Dep, mV_I_Dep;
		ExprArray			mAVars;

};


#endif



