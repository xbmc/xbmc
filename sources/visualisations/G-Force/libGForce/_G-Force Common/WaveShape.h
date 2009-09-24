#ifndef WaveShape_H
#define WaveShape_H

#include "..\Common\math\ExprArray.h"
#include "..\Common\math\ExpressionDict.h"
#include "G-Force_Proj.h"

class ArgList;
class PixPort;

class WaveShape {
	
	public:
		
		
								WaveShape( float* inTPtr );

		void					SetMagFcn( ExprUserFcn** inMagFcn );
		
		void					Load( ArgList& inArgs, long inDefaultNumSteps );

	
		void					SetupTransition( WaveShape* inDest );

		#define					SHAPE_MORPH_ALPHA	1.7f
		

		void					Draw( long inNumSteps, PixPort& inDest, float inFader, WaveShape* inWave2, float inMorphPct );
		
	protected:

		// Holds a copy of the ptr to the external time index
		float*					mTPtr;


		// Dict vars
		float					mPI, mNumSampleBins;
		float					mMouseX, mMouseY;
		
		ExpressionDict			mDict;
		float					mShapeTrans;
		long					mNumWaves;
		bool					mAspect1to1;
		bool					mConnectBins, mConnectBinsOrig;
		bool					mConnectFirstLast, mConnectFirstLastOrig;
		ExprArray				mA, mB, mC;
		ExprArray				mWaveY;
		ExprArray				mWaveX;
		Expression				mLineWidth;	
		Expression				mNum_S_Steps;
		Expression				mIntensity;
		bool					mPen_Dep_S;
		bool					mLineWidth_Dep_S;
		

		void					SetupFrame( WaveShape* inDest, float inW );
		
		static float			sS;
		static long				sXY[ 2 * MAX_WAVES_PER_SHAPE ];
		static long				sStartXY[ 2 * MAX_WAVES_PER_SHAPE ];
	
		void					CalcNumS_Steps( WaveShape* inWave2, long inDefaultNumBins );
};

#endif