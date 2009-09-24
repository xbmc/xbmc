#include "XStrList.h"


XStrList::XStrList( XStrListOptsT inOption, ListOrderingT inOrdering ) :
	mStrings( inOrdering ) {
	
	mStrListOption = inOption;
	
	bool lowToHigh = inOrdering == cSortLowToHigh;
	
	if ( inOrdering == cSortLowToHigh || inOrdering == cSortHighToLow ) {
		if ( mStrListOption == cNoDuplicates_CaseInsensitive )
			mStrings.SetCompFcn( sStrComparitorCI, lowToHigh );
		else
			mStrings.SetCompFcn( sStrComparitor, lowToHigh );
	}
}



XStrList::~XStrList() {

	RemoveAll();
}



void XStrList::RemoveAll() {
	int			i = 1;
	UtilStr*	str;
	
	while ( mStrings.Fetch( i, (void**) &str ) ) {
		delete str;
		i++;
	}
	mStrings.RemoveAll();
}


void XStrList::Remove( long inIndex ) {
	UtilStr*	str;
	
	if ( mStrings.Fetch( inIndex, (void**) &str ) )
		delete str;
	
	mStrings.RemoveElement( inIndex );
}
 

long XStrList::Add( const void* inData, long inLen ) {
	UtilStr* s		= new UtilStr( inData, inLen );
	bool 	doAdd	= true;
	
	if ( mStrListOption != cDuplicatesAllowed ) 
		doAdd = FindIndexOf( *s ) == 0;
	
	if ( doAdd )
		return mStrings.Add( s );
	else {
		delete s;	
		return 0;
	}
}



long XStrList::Add( const char* inStr ) {
	UtilStr* s		= new UtilStr( inStr );
	bool 	doAdd	= true;
	
	if ( mStrListOption != cDuplicatesAllowed ) 
		doAdd = FindIndexOf( *s ) == 0;
	
	if ( doAdd ) 
		return mStrings.Add( s );
	else {
		delete s;
		return 0;
	}
}



long XStrList::Add( const UtilStr& inStr ) {
	bool doAdd = true;
	
	if ( mStrListOption != cDuplicatesAllowed ) 
		doAdd = FindIndexOf( inStr ) == 0;
	
	if ( doAdd )
		return mStrings.Add( new UtilStr( inStr ) );
	else
		return 0;
}
		
		
long XStrList::FetchBestMatch( const UtilStr& inStr ) {
	long			best, bestScore, score, i;
	UtilStr*		str;
	
	best = 0;
		
	for ( i = 1; mStrings.Fetch( i, (void**) &str ); i++ ) {
		score = str -> LCSMatchScore( inStr );
		if ( score > bestScore || i == 1 ) {
			best = i;
			bestScore = score;
		}
	}
		
	return best;
}


/*
long XStrList::FindIndexOf( const char* inStr ) const {
	bool			caseSens	= mStrListOption != cNoDuplicates_CaseInsensitive;
	static UtilStr	sTemp;
	int				i = 1;
	UtilStr*		str;
	
	if ( mStrings.mCompFcn ) {
		sTemp.Assign( inStr );
		return FindIndexOf( sTemp ); }
	else {
		while ( mStrings.Fetch( i, (void**) &str ) ) {
			if ( str -> compareTo( inStr, caseSens ) == 0 )
				return i;
			i++;
		}
	}
	
	return 0;
}
*/

long XStrList::FindIndexOf( const UtilStr& inStr ) const {
	int			i = 1;
	UtilStr*	str;
	bool		caseSens	= mStrListOption != cNoDuplicates_CaseInsensitive;
	
	if ( mStrings.mCompFcn ) {
		i = mStrings.FetchPredIndex( &inStr ) + 1;
		if ( mStrings.Fetch( i, (void**) &str ) ) {
			if ( str -> compareTo( &inStr, caseSens ) == 0 )
				return i;
		} }
	else
		return FindIndexOf( inStr.getCStr() );
	
	return 0;
}



bool XStrList::Fetch( long inIndex, UtilStr& outStr ) const {
	UtilStr*	str;
	
	if ( mStrings.Fetch( inIndex, (void**) &str ) ) {
		outStr.Assign( str );
		return true; }
	else
		return false;
}


UtilStr* XStrList::Fetch( long inIndex ) const {
	return (UtilStr*) mStrings.Fetch( inIndex );
}





int XStrList::sStrComparitor( const void* inA, const void* inB ) {
	return ( (UtilStr*) inA ) -> compareTo( (UtilStr*) inB, true );
}

int XStrList::sStrComparitorCI( const void* inA, const void* inB ) {
	return ( (UtilStr*) inA ) -> compareTo( (UtilStr*) inB, false );
}
