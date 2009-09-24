#include "WaveShape.h"

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <xtl.h>

#include "..\common\general tools\ArgList.h"
#include "..\common\general tools\UtilStr.h"
#include "..\common\ui\EgOSUtils.h"
#include "..\common\ui\PixPort.h"


long		WaveShape::sXY[ 2 * MAX_WAVES_PER_SHAPE ];
long		WaveShape::sStartXY[ 2 * MAX_WAVES_PER_SHAPE ];
float		WaveShape::sS;


WaveShape::WaveShape( float* inTPtr ) {
	UtilStr str;
	
	mNumWaves = 0;
	mMouseX = 0;
	mMouseY = 0;
	
	mShapeTrans = 0.0f;

	mDict.AddVar( "S", &sS );
	mDict.AddVar( "T", inTPtr );
	mDict.AddVar( "NUM_SAMPLE_BINS", &mNumSampleBins );
	mDict.AddVar( "MOUSEX", &mMouseX );
	mDict.AddVar( "MOUSEY", &mMouseY );
	
	mTPtr = inTPtr;
	mPI = 3.14159265358979;
	
	mDict.AddVar( "PI", &mPI );	
}



void WaveShape::SetMagFcn( ExprUserFcn** inMagFcn ) {

	mDict.AddFcn( "MAG", inMagFcn );
}

#define _assignOrig( field )  field##Orig = field;



void WaveShape::Load( ArgList& inArgs, long inDefaultNumSteps ) {
	UtilStr str;
	
	// Mix up the rnd seed
	srand( timeGetTime() );

	// Calculate mNumSampleBins -- How many pieces we chop the 0-1 s interval into
	inArgs.GetArg( 'Stps', str );
	mNum_S_Steps.Compile( str, mDict );
	CalcNumS_Steps( NULL, inDefaultNumSteps );
	
	// Compile and link all the temp exprs.  By their spec, A vars can be evaluated now
	mA.Compile( inArgs, 'A', mDict );
	mA.Evaluate();		
	mB.Compile( inArgs, 'B', mDict );
	mC.Compile( inArgs, 'C', mDict );
			
	// The intensity fcn allows drawing of arbitrary intensity
	if ( ! inArgs.GetArg( 'Pen', str ) )
		str.Assign( "1" );
	mIntensity.Compile( str, mDict );

	// If a user doesn't enter a line widh for a wave, assume width 1
	if ( ! inArgs.GetArg( 'LWdt', str ) )
		str.Assign( "1" );
	mLineWidth.Compile( str, mDict );
	
	mPen_Dep_S			= mIntensity.IsDependent( "s" ) || mIntensity.IsDependent( "c" ) || mIntensity.IsDependent( "rnd" );
	mLineWidth_Dep_S	= mLineWidth.IsDependent( "s" ) || mLineWidth.IsDependent( "c" ) || mLineWidth.IsDependent( "rnd" );

	// Compile and link waves
	mWaveX.Compile( inArgs, 'X', mDict );
	mWaveY.Compile( inArgs, 'Y', mDict );

	// Init all the wave shape var counters
	mNumWaves = mWaveX.Count();
		
	mConnectBins		= inArgs.GetArg( 'ConB' );
	mConnectFirstLast	= inArgs.GetArg( 'ConB' ) > 1;
	
	// Make copies/save original values (morph will write over the nonOrg vars)
	_assignOrig( mConnectBins )
	_assignOrig( mConnectFirstLast )
		
	mAspect1to1 = inArgs.GetArg( 'Aspc' );
}



void WaveShape::SetupTransition( WaveShape* inDest ) {
	
	mIntensity.Weight( inDest -> mIntensity, &mShapeTrans, NULL );
	mLineWidth.Weight( inDest -> mLineWidth, &mShapeTrans, NULL );
	
	mPen_Dep_S			= mPen_Dep_S || inDest -> mPen_Dep_S;
	mLineWidth_Dep_S	= mLineWidth_Dep_S|| inDest -> mLineWidth_Dep_S;
}



#define __evalIntensity( var )		clr = 65535.0 * mIntensity.Evaluate() * inFader;	\
									var = clr;											\
									if ( clr < 0 )				var = 0;				\
									else if ( clr > 0xFFFF )	var = 0xFFFF;



void WaveShape::CalcNumS_Steps( WaveShape* inWave2, long inDefaultNumBins ) {
	int n;
	
	// See if this shape has an overriding number of s steps
	mNumSampleBins = inDefaultNumBins;

	mNumSampleBins = mNum_S_Steps.Evaluate();
	if(mNumSampleBins == 9)
		char blub=2;

	if ( mNumSampleBins <= 0 )
		mNumSampleBins = inDefaultNumBins;
		
		
	if ( inWave2 ) {
		n = inWave2 -> mNum_S_Steps.Evaluate();
		if ( n <= 0 )
			n = inDefaultNumBins;

		mNumSampleBins = ( 1 - mShapeTrans ) * n + mShapeTrans * mNumSampleBins;
	}
}


void WaveShape::Draw( long inNumSteps, PixPort& inDest, float inFader, WaveShape* inWave2, float inMorphPct ) {
	long i, x, y;
	long xoff = inDest.GetX() >> 1;
	long yoff = inDest.GetY() >> 1;
	long maxWaves, w2Waves, clr;
	float dialate, tx, ty, stepSize;
	float xscale, yscale, xscaleW2, yscaleW2 ;
	RGBColor	rgb, rgbPrev, rgbStart;

	rgb.blue = 0;
	rgb.green = 0;
	rgb.red = 0;

	// Calc the x and y scale factors
	xscale = xoff;
	yscale = yoff;
	if ( mAspect1to1 ) {
		if ( yscale < xscale )
			xscale = yscale;
		else
			yscale = xscale;
	}
				
	// See if this shape has an overriding number of s steps
	CalcNumS_Steps( inWave2, inNumSteps );
	
	// Setup default step size--inv of how many bins are available
	if ( mNumSampleBins > 1 )
		stepSize = 1.0 / ( mNumSampleBins - 1.0 );
	else
		stepSize = 1;

	// If we're not in a transition/morph
	if ( ! inWave2 ) {
		dialate = 1;
		maxWaves = mNumWaves;
		w2Waves = 0; 
	}
			
	// If we're transitioning from one waveshape to another
	else {
		w2Waves = inWave2 -> mNumWaves;
		dialate = inMorphPct;
		mShapeTrans = pow( dialate, SHAPE_MORPH_ALPHA );
		SetupFrame( inWave2, mShapeTrans );
		
		if ( mNumWaves > w2Waves ) {
			maxWaves = mNumWaves;
			dialate = 1.0 - dialate; }
		else
			maxWaves = w2Waves;
		
		// Set the wave scale factor to the wave leaving/arriving
		dialate = 20.0 * pow( dialate, 4.0f ) + 1.0;

		// Calc the x and y scale factors for wave 2
		xscaleW2 = xoff;
		yscaleW2 = yoff;
		if ( inWave2 -> mAspect1to1 ) {
			if ( yscaleW2 < xscaleW2 )
				xscaleW2 = yscaleW2;
			else
				yscaleW2 = xscaleW2;
		}
	}

	// Setup/store the mouse position for possible virtual machine access
	Point mousePt;
	EgOSUtils::GetMouse( mousePt );
	mMouseX = ( (float) mousePt.h ) / xscale;
	mMouseY = ( (float) mousePt.v ) / yscale;

	// Evaluate the expressions dependent on 't'/the current frame
	mB.Evaluate();	
	if ( inWave2 ) 
		inWave2 -> mB.Evaluate();	

	// Calc linewidth, add a little to make .999 into 1.  If it's not dep on s, we can evaluate it now
	if ( ! mLineWidth_Dep_S )
		inDest.SetLineWidth( mLineWidth.Evaluate() + 0.001 );

	// Calc pen intensity.  If it's not dep on s, we can evaluate it now.
	if ( ! mPen_Dep_S ) {
		__evalIntensity( rgb.red );	
		rgbPrev = rgb;
	}
					
	// Step thru s (the xy exprs will give us the cords)
	for ( sS = 0; sS <= 1.0; sS += stepSize ) {
	
		// Evaluate the expressions dependent on 's'
		mC.Evaluate();	
		if ( inWave2 )	
			inWave2 -> mC.Evaluate();	

		// Calc linewidth, add a little to make .999 into 1.
		if ( mLineWidth_Dep_S )
			inDest.SetLineWidth( mLineWidth.Evaluate() + 0.001 );
		
		// Calc pen intensity
		if ( mPen_Dep_S ) {
			rgbPrev = rgb;
			__evalIntensity( rgb.red );	
		}
		
		// Draw all the waves
		for ( i = 0; i < maxWaves; i++ ) {
		
			if ( i < mNumWaves ) {
				
				// Find the cords for waveshape1, wave number i
				tx = xscale * mWaveX.Evaluate( i );
				ty = yscale * mWaveY.Evaluate( i );
				
				// If we have two waves to mix...
				if ( i < w2Waves ) {
					tx = mShapeTrans * tx + ( 1.0 - mShapeTrans ) * xscaleW2 * inWave2 -> mWaveX.Evaluate( i );
					ty = mShapeTrans * ty + ( 1.0 - mShapeTrans ) * yscaleW2 * inWave2 -> mWaveY.Evaluate( i ); }
				else {
					tx *= dialate;
					ty *= dialate;
				} }
			else {
				
				// Find the cords for waveshape2, wave number i
				tx = dialate * xscaleW2 * inWave2 -> mWaveX.Evaluate( i );
				ty = dialate * yscaleW2 * inWave2 -> mWaveY.Evaluate( i );
			}
			
			// Switch to screen cords, baby, and draw the line segment
			x = xoff + tx;
			y = yoff - ty;
				
			if ( mConnectBins ) {
				if ( sS > 0 )
					inDest.Line( sXY[ 2 * i ], sXY[ 2 * i + 1 ], x, y, rgbPrev, rgb );
				else {
					sStartXY[ 2 * i ]		= x;
					sStartXY[ 2 * i + 1 ]	= y;
					rgbStart = rgb;
				}
				sXY[ 2 * i ] = x;
				sXY[ 2 * i + 1 ] = y;  }
			else 
				inDest.Line( x, y, x, y, rgb, rgb );				
		}
	}

	// Draw all the first-last segments for each wave
	if ( mConnectFirstLast ) {
		for ( i = 0; i < maxWaves; i++ )
			inDest.Line( sXY[ 2 * i ], sXY[ 2 * i + 1 ], sStartXY[ 2 * i ], sStartXY[ 2 * i + 1 ], rgb, rgbStart );
	}	
	
	// Make sure we restore a random seed (one of the virtual machines could be seeding to the same value)
	srand( *((long*) mTPtr) );

}




#define __weightFLT( field ) field = ( inW * ( (float) field##Orig ) + w1 * ( (float) inDest -> field ) );
#define __weightINT( field ) field = ( 0.5 + inW * ( (float) field##Orig ) + w1 * ( (float) inDest -> field ) );
#define __weightBOL( field ) field = .5 < ( inW * ( field##Orig ? 1.0 : 0.0 ) + w1 * ( inDest -> field ? 1.0 : 0.0 ) );


void WaveShape::SetupFrame( WaveShape* inDest, float inW ) {
	float w1 = 1.0 - inW;
		
	__weightBOL( mConnectBins )
	__weightBOL( mConnectFirstLast )
}

