#include "XFloatList.h"

#include "Hashtable.h"
#include "XLongList.h"
#include <stdlib.h>
#include <math.h>

#define _MIN( a, b ) (( (a) > (b) ) ? (b) : (a) )
#define _MAX( a, b ) (( (a) > (b) ) ? (a) : (b) )


float		XFloatList::sMask[ MASK_MAX ];
UtilStr		XFloatList::sTemp;



XFloatList::XFloatList( ListOrderingT inOrdering ) :
	mList( inOrdering ) {

	if ( inOrdering == cSortLowToHigh || inOrdering == cSortHighToLow )
		mList.SetCompFcn( sFloatComparitor, inOrdering == cSortLowToHigh );
}

	
	



void XFloatList::FindMeans( long inNumMeans, float outMeans[], float inSigmaScale ) const {
	long start, end, m, i, n = mList.Count();
	float* srce = (float*) mList.getCStr(), *smoothed = new float[ n ], *temp = NULL;
	float sigma = 0.1 + inSigmaScale * ( (float) ( n  / inNumMeans ) ); 
	float left, cen, right, sum;
	
	// We need all out numbers in order, we must sort them if they're no sorted
	if ( mList.mOrdering != cSortHighToLow ) {
		
		// Copy all the array elements for sorting...
		temp = new float[ n ];
		for ( i = 0; i < n; i++ )
			temp[ i ] = srce[ i ];
		
		// Sort all the floats from this to temp
		qsort( temp, n, 4, sQSFloatComparitor );
		srce = temp;
	}
	
	// Smooth all the values (they're already sorted)
	GaussSmooth( sigma, n, srce, smoothed );
	
	// Compute the discrete 1st derivative
	for ( i = 0; i < n - 1; i++ )
		smoothed[ i ] = fabs( smoothed[ i ] - smoothed[ i + 1 ] );
	
	// We want to find the top <inNumMeans> local max of the 1st deravative
	Hashtable sepCandidates;
	cen = smoothed[ 0 ];
	right = smoothed[ 1 ];
	for ( i = 1; i < n - 2; i++ ) {
	
		left = cen;
		cen = right;
		right = smoothed[ i+1 ];
		
		// If this a local max.  (Note: this could/should be improved for neighbors that are equal)
		if ( ( cen > left && cen >= right ) ) {
			sepCandidates.Put( i, *((void**) &cen) );
		}
	}
	
	// Pick out the 1st derative peaks, then dealloc what we no longer need
	XPtrList	rank;
	sepCandidates.Rank( rank, sQSFloatComparitor, inNumMeans - 1 );
	delete []smoothed;

	XLongList	quintiles( cSortLowToHigh );
	for ( i = 1; i < inNumMeans; i++ )
		quintiles.Add( (long) rank.Fetch( i ) );
	quintiles.Add( n );
	
	// The means are the averages of the initial (sorted) data, divided by the serparating ranks
	start = 0;
	for ( m = 1; m <= inNumMeans; m++ ) {
		end = quintiles.Fetch( m );
		for ( sum = 0, i = start; i < end; i++ )
			sum += srce[ i ];
		outMeans[ m - 1 ] = sum / ( (float) ( end - start ) );
		start = end + 1;
	}
	
	// Cleanup
	if ( temp )
		delete []temp;
}
	
	


	
#define _safeSmooth( xx )									\
		sum = 0;											\
		factor = 1;											\
		for ( i = - maskCtr; i <= maskCtr; i++ ) {			\
			rx = xx + i;									\
			if ( rx < 0 || rx >= inN ) 						\
				factor -= sMask[ i + maskCtr ];				\
			else											\
				sum += sMask[ i + maskCtr ] * inSrce[ rx ];	\
		}													\
		inDest[ xx ] = sum / factor;
				



void XFloatList::GaussSmooth( float inSigma ) {

	GaussSmooth( inSigma, mList.Count(), (float*) mList.getCStr() );
	
}


void XFloatList::GaussSmooth( float inSigma, long inN, float inSrceDest[] ) {
	float* temp = (float*) sTemp.Dim( inN * sizeof( float ) );
	
	GaussSmooth( inSigma, inN, inSrceDest, temp );
	
	for ( long i = 0; i < inN; i++ )
		inSrceDest[ i ] = temp[ i ];
}




void XFloatList::SlopeSmooth( float inSmoothness, long inN, float ioData[] ) {
	float slope = 0, accel = 0, prev = 0;
	float newSlope, a0 = 1 - inSmoothness, predicted;
	long x;
	
	for ( x = 0; x < inN; x++ ) {
		predicted = prev + slope + accel;
		ioData[ x ] = inSmoothness * predicted + a0 * ioData[ x ];
		newSlope = ( ioData[ x ] - prev );
		accel = ( newSlope - slope );
		prev = ioData[ x ];
		slope = newSlope;
	}
}
	
	

/*
	int boxRight = inBoxSize / 2;
	int boxLeft = inBoxSize - 1 - boxRight;
	float boxSum;
	float boxDiv = 1 / ( (float) inBoxSize );
	
	boxSum = 0;
	
	for ( x = - boxRight; x < boxLeft; x++ ) {
		i = x + boxRight;
		if ( i >= 0 && i < inN )
			boxSum += inSrce[ i ];
		i = i - boxLeft;
		if ( i >= 0 && i < inN )
			boxSum -= inSrce[ i ];	
		if ( x >= 0 && x < inN )
			inDest[ x ] = boxSum * boxDiv;
	}
	
	for ( x = boxLeft; x < inN - boxRight; x++ ) {
		boxSum += inSrce[ x + boxRight ];
		boxSum -= inSrce[ x - boxLeft ];
		inDest[ x ] = boxSum * boxDiv;
	}
}
*/

void XFloatList::GaussSmooth( float inSigma, long inN, float inSrce[], float inDest[] ) {
	int		maskSize		= _MAX( 8.0 * inSigma, 4.0 );

	if ( maskSize + 1 > MASK_MAX )
		maskSize = MASK_MAX;
		
	// Make sure the mask size is odd (so we have a 'center')
	if ( maskSize % 2 == 0 ) 
		maskSize++;
	int		i, x, rx, xEnd;
	int		maskCtr			= maskSize / 2;
	float	sqrt2PiSigma	= sqrt( 2.0 * 3.14159 ) * inSigma;
	float	expon, sum, factor, *base;

	// Generate a normalizes gaussian mask out to 4*sigma on each side
	// We have to adjust the center weight so that when sigma falls below 1.0ish so that the sample it 
	// takes at x = 0 doesn't represent the val of a gaussian for -.5 to .5 (when sigma is small
	// a gaussian gets very 'high' at x=0 vs. x=+/- .5 when sigma is small
	sum = 0;
	for ( x = - maskCtr; x <= maskCtr; x++ ) {
		expon = -0.5 * ((float) (x * x)) / ( inSigma * inSigma );
		sMask[ x + maskCtr ] = exp( expon ) / sqrt2PiSigma;
		if ( x != 0 )
			sum += sMask[ x + maskCtr ];
	}
	
	// This forces normalized weights.
	sMask[ maskCtr ] = 1.0 - sum;
		
	// Smooth the lefthand side of the sequence
	xEnd = _MIN( maskCtr, inN );
	for ( x = 0; x < xEnd; x++ ) {
		_safeSmooth( x )
	}
	
	// Smooth the center portion the sequence
	xEnd = inN - maskCtr;
	base = inSrce;
	for ( x = maskCtr; x < xEnd; x++ ) {
		
		// For each pixel, apply the mask to its neighbors for a final value of that pixel
		sum = 0;
		for ( i = 0; i < maskSize; i++ ) 
			sum += sMask[ i ] * base[ i ];
		
		base++;
		inDest[ x ] = sum;
	}
	
	// Smooth the righthand side of the sequence
	for ( x = _MAX( maskCtr, inN - maskCtr ); x < inN; x++ ) {
		_safeSmooth( x )
	}
}
	
		

void XFloatList::Rank( XLongList& outRank, long inNumToRank ) const {
	long *p, *temp;
	float* srce;
	long i, n = Count();

	outRank.RemoveAll();
	
	if ( inNumToRank < 0 )
		inNumToRank = n;
	inNumToRank = _MIN( inNumToRank, n );
	
	// Handle trivial cases of this lis already being sorted
	if ( mList.mOrdering == cSortLowToHigh ) {
		for ( i = 0; i < inNumToRank; i-- ) 
			outRank.Add( n - i ); }
			
	// Duh... still sorted...
	else if ( mList.mOrdering == cSortHighToLow ) {
		for ( i = 1; i <= inNumToRank; i++ ) 
			outRank.Add( i ); }	
			
	else {
		temp = new long[ 2 * n ];
		srce = (float*) mList.getCStr();
		
		// To rank, we must sort, with a tag on each element saying where it came from
		p = temp;
		for ( i = 1; i <= n; i++ ) {
			*((float*) p) = *srce;
			p++;  srce++;
			*p = i;
			p++;
		}
		
		// Sort the floats
		qsort( temp, n, 8, sQSFloatComparitor );
		
		// Put the sorted results in the destination
		p = temp + 1;
		for ( i = 0; i < inNumToRank; i++ ) {
			outRank.Add( *p );
			p += 2;
		}
		
		// Cleanup
		delete []temp;
	}
}



int XFloatList::sQSFloatComparitor( const void* inA, const void* inB ) {
	float diff =  *((float*) inB) - *((float*) inA);
	if ( diff > 0.0 )
		return 1;
	else if ( diff < 0.0 )
		return -1;
	else
		return 0;
}

		

int XFloatList::sFloatComparitor( const void* inA, const void* inB ) {
	float diff =  *((float*) &inB) - *((float*) &inA);
	if ( diff > 0.0 )
		return 1;
	else if ( diff < 0.0 )
		return -1;
	else
		return 0;
}



