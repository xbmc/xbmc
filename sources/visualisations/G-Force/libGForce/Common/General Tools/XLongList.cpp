#include "XLongList.h"

#include <stdlib.h>


#define _MIN( a, b ) ( ( (a) < (b) ) ? (a) : (b) )

long XLongList::sDummy = 0;

XLongList::XLongList( ListOrderingT inOrdering ) :
	mList( inOrdering ) {

	if ( inOrdering == cSortLowToHigh || inOrdering == cSortHighToLow )
		mList.SetCompFcn( sLongComparitor, inOrdering == cSortLowToHigh );
}

	

int XLongList::sLongComparitor( const void* inA, const void* inB ) {
	return ((long) inB - (long) inA);
}



int XLongList::sQSLongComparitor( const void* inA, const void* inB ) {
	return (*((long*) inB) - *((long*) inA));
}








void XLongList::SubtractRange( long inStart, long inEnd ) {
	long x, i = 1;
	
	while ( Fetch( i, &x ) ) {
		if ( x >= inStart && x <= inEnd )
			Remove( x );
		else
			i++;
	}
}


void XLongList::ApplyMask( long inStart, long inEnd ) {
	long x, i = 1;
	
	while ( Fetch( i, &x ) ) {
		if ( x < inStart || x > inEnd )
			Remove( x );
		else
			i++;
	}
}





long& XLongList::operator[] ( const long inIndex ) {
	long len;
	
	if ( inIndex >= 0 ) {
		len = mList.mStrLen;
		if ( inIndex >= len >> 2 ) {
			mList.Insert( len, '\0', ( inIndex + 1 ) * 4 - len );
		}
			
		return *( (long*) ( mList.mBuf + inIndex * 4 + 1 ) ); }
	else
		return sDummy;
}






void XLongList::Rank( XLongList& outRank, long inNumToRank ) const {
	long *p, *temp;
	long* srce;
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
		srce = (long*) mList.getCStr();
		
		// To rank, we must sort, with a tag on each element saying where it came from
		p = temp;
		for ( i = 1; i <= n; i++ ) {
			*p = *srce;
			p++;  srce++;
			*p = i;
			p++;
		}
		
		// Sort the floats
		qsort( temp, n, 8, sQSLongComparitor );
		
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

			