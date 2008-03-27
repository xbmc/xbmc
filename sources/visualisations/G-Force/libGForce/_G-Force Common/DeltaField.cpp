// DeltaField.cpp

#include "DeltaField.h"

#include "..\common\general tools\ArgList.h"
#include <math.h>
#include "..\common\ui\EgOSUtils.h"


DeltaField::DeltaField() {

	// Init field map stuff
	mDict.AddVar( "X", &mX_Cord );
	mDict.AddVar( "Y", &mY_Cord );
	mDict.AddVar( "R", &mR_Cord );
	mDict.AddVar( "PI", &mPI );
	mDict.AddVar( "THETA", &mT_Cord );
	mWidth = mHeight = mRowSize = 0;
	mCurrentY = -1;
	mPI = 3.141592653589793;
}



#define DEC_SIZE 8


DeltaFieldData* DeltaField::GetField() {
	bool didCalc = false;
	
	if ( mCurrentY >= 0 ) {

		if ( ! IsCalculated() ) {
		
			EgOSUtils::ShowCursor();
		
			while ( ! IsCalculated() ) {
				EgOSUtils::SpinCursor();
				CalcSome();
			}
		
			EgOSUtils::ShowCursor();
		}
		
		return &mFieldData;
	}
	
	
	return NULL;
}




void DeltaField::Assign( ArgList& inArgs, UtilStr& inName ) {
	UtilStr fx, fy;
	
	mName.Assign( inName );
	
	// Compile and link the temp exprs.  By spec, A-vars are evaluated now
	mAVars.Compile( inArgs, 'A', mDict );
	mAVars.Evaluate();		

	mDVars.Compile( inArgs, 'D', mDict );

	mAspect1to1	= inArgs.GetArg( 'Aspc' );
	mPolar		= inArgs.ArgExists( 'srcR' );

	// Compile the 2D vector field that expresses the source point for a given point
	if ( mPolar ) {
		inArgs.GetArg( 'srcR', fx );
		inArgs.GetArg( 'srcT', fy ); }
	else {
		inArgs.GetArg( 'srcX', fx );
		inArgs.GetArg( 'srcY', fy );
	}
	
	mXField.Compile( fx, mDict );
	mYField.Compile( fy, mDict );
	
	mHasRTerm		= mXField.IsDependent( "R" )		|| mYField.IsDependent( "R" )			|| mDVars.IsDependent( "R" );
	mHasThetaTerm	= mXField.IsDependent( "THETA" )	|| mYField.IsDependent( "THETA" )		|| mDVars.IsDependent( "THETA" );

	// Reset all computation of this delta field...	
	SetSize( mWidth, mHeight, mRowSize, true );
}



void DeltaField::SetSize( long inWidth, long inHeight, long inRowSize, bool inForceRegen ) {
	
	// Only resize if the new size is different...
	if ( inWidth != mWidth || inHeight != mHeight || inForceRegen ) {
		
		mWidth = inWidth;
		mHeight = inHeight;
		mRowSize = inRowSize;
		
		// Each pixel needs 4 bytes of info per pixel (max) plus 4 shorts, 2 bytes per row (max)
		mCurrentRow = mGradBuf.Dim( 4 * mWidth * mHeight + 10 * mHeight + 64 );
		mFieldData.mField = mCurrentRow;
		
		mXScale = 2.0 / ( (float) mWidth );
		mYScale = 2.0 / ( (float) mHeight );
		
		// If we're to keep the xy aspect ratio to 1, change the dim that will get stretched
		if ( mAspect1to1 ) {
			if ( mYScale > mXScale )
				mXScale = mYScale;
			else
				mYScale = mXScale;
		}
		
		// Reset all computation of this delta field
		mCurrentY = 0;
		
		// The current implementation of PixPort for Win32 flips all the Y Cords
		#if EG_WIN
		mYScale *= -1;
		#endif
	}
}





void DeltaField::CalcSome() {
	float xscale2, yscale2, r, fx, fy;
	long px, sx, sy, t;
	unsigned long addrOffset;
	char* g;
	bool outOfBounds;
	
	// If we're still have stuff left to compute...
	if ( mCurrentY >= 0 && mCurrentY < mHeight ) {
	
		// Calc the y we're currently at
		mY_Cord = 0.5 * mYScale * ( mHeight - 2 * mCurrentY );

		// Save some cycles by pre-computing indep stuff
		xscale2 = ( (float) ( 1 << DEC_SIZE ) ) / mXScale;
		yscale2 = ( (float) ( 1 << DEC_SIZE ) ) / mYScale;
	
		// Resume on the pixel we left off at
		g = mCurrentRow;
				
		// Calc the mCurrentY row of the grad field
		for ( px = 0; px < mWidth; px++ ) {
			mX_Cord = 0.5 * mXScale * ( 2 * px - mWidth ); 
							
			// Calculate R and THETA only if the field uses it (don't burn cycles on sqrt() and atan())
			if ( mHasRTerm )
				mR_Cord = sqrt( mX_Cord * mX_Cord + mY_Cord * mY_Cord );
			if( mHasThetaTerm )
				mT_Cord = atan2( mY_Cord, mX_Cord );
				
			// Evaluate any temp variables
			mDVars.Evaluate();	
			
			// Evaluate the source point for (mXCord, mYCord)
			fx = mXField.Evaluate();
			fy = mYField.Evaluate();
			if ( mPolar ) {
				r = fx;
				fx = r * cos( fy );
				fy = r * sin( fy );
			}
			sx = xscale2 * ( fx - mX_Cord );
			sy = yscale2 * ( mY_Cord - fy );
			
			// See if the source cord for the current cord is out of the frame rect
			outOfBounds = false;
			t = px + ( sx >> DEC_SIZE );
			if ( t >= mWidth - 1 || t < 0 )
				outOfBounds = true;
			t = mCurrentY + ( sy >> DEC_SIZE );
			if ( t >= mHeight - 1 || t < 0 )
				outOfBounds = true;
			
			// Get rid of negative numbers
			sx += 0x7F00;
			sy += 0x7F00;

			// Blacken this pixel if the vector is not encodable...
			if ( sx > ( (long) 0xFF00 ) || sx < 0 || sy > ( (long) 0xFF00 ) || sy < 0 )
				outOfBounds = true;
				
			// If this cord is in bounds then encode it, otherwise signal PixPort::Fade()
			if ( outOfBounds )
				*( ( unsigned long* ) g ) = 0xFFFFFFFF; 
			else {
				
				// Precompute the address of the souce quad-pixel fence
				addrOffset = ( sx >> 8 ) + px + ( sy >> 8 ) * mRowSize;
				
				*( ( unsigned long* ) g )	= ( addrOffset << 14 ) |
											  ( ( sx & 0x00FE ) << 6 ) |
											  ( ( sy & 0x00FE ) >> 1 );
			}

			g += 4;
		}
					
		// Store where this row ends
		mCurrentRow = g;
		
		// Signal the compution of the next row
		mCurrentY++;
	}
	
	
	if ( IsCalculated() ) {

		// Give PixPort some needed info and scrap ptrs
		/*mFieldData.mNegYExtents = 1 - ( mNegYExtents >> DEC_SIZE );
		if ( mFieldData.mNegYExtents > mHeight )
			mFieldData.mNegYExtents = mHeight;
			
		// 8-bit pixels, so once byte per pixel
		mFieldData.mYExtentsBuf = mYExtentsBuf.Dim( mFieldData.mNegYExtents * mWidth ); */
	}
}


