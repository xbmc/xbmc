#include "XPtrList.h"
#include "nodeClass.h"


void* XPtrList::sDummy = NULL;


XPtrList::XPtrList( ListOrderingT inOrdering ) {

	mOrdering	= inOrdering;
	mCompFcn	= NULL;
}




void XPtrList::Assign( const XPtrList& inList ) {

	UtilStr::Assign( inList );
}



#define __ptr( idx )	*((void**) (base + idx * 4))

long XPtrList::FetchPredIndex( const void* inPtr ) const {
	long M, L = 0, R = Count()-1;
	char* base = getCStr();
	long order = ( mOrdering == cSortHighToLow ) ? 0x80000000 : 0;

	if ( R < 0 ) 
		return 0;
	else {
		while (L <= R) {
			
			M = (L + R) / 2;
			
			if ( (mCompFcn( inPtr, __ptr( M ) ) ^ order) >= 0 ) // If inPtr <= __ptr( M )...
				R = M - 1;										// Throw away right half
			else
				L = M + 1;										// Throw away left half
		}

		if ( L > R )											// Catch the case where R+1==L
			L = M;												// In this case, M specifies the critical element 
			
		// At this point, we know L is the critical element (case: L==R or L contains M from case above)
		if ( mCompFcn( inPtr, __ptr( L ) ) < 0 )				// If inPtr > __ptr( M )...
			L++;

		return L;
	}
}


void XPtrList::SetCompFcn( CompFunctionT inFcn, bool inSortLowToHigh ) {
	mCompFcn = inFcn;
	
	RemoveAll();
	
	if ( inSortLowToHigh )
		mOrdering = cSortLowToHigh;
	else
		mOrdering = cSortHighToLow;

}



long XPtrList::FindIndexOf( const void* inMatch ) const {	
	long	i = 0;
	char*	curPtr, *endPtr;
	void*	ptr;

	if ( mCompFcn ) {
		i = FetchPredIndex( inMatch );
		curPtr = getCStr() + 4 * i;
		endPtr = getCStr() + length();
		while ( curPtr < endPtr ) {
			i++;
			ptr = *((void**) curPtr);
			if ( ptr == inMatch )
				return i;
				
			// Stop checking when we hit items that aren't equal to inMatch
			else if ( mCompFcn( inMatch, ptr ) != 0 )
				break;
			curPtr += 4;
		} }
	else {
		curPtr = getCStr();
		endPtr = curPtr + length();

		while ( curPtr < endPtr ) {
			i++;
			if ( *((void**) curPtr) == inMatch ) 
				return i;
			else
				curPtr += 4;
		}
	}

	return 0;
}




long XPtrList::Add( const void* inPtrToAdd ) {
	long i;
	
	if ( mCompFcn ) {
		i = FetchPredIndex( inPtrToAdd );
		Insert( i*4, (char*) &inPtrToAdd, 4 ); 
		return i+1; }
	else {
		UtilStr::Append( (char*) &inPtrToAdd, 4 );
		return Count();
	}
}





void XPtrList::Add( const void* inPtrToAdd, long inN ) {
	
	if ( inN < 0 )
		inN = 0;
		
	if ( inN > Count() )
		inN = Count();
	
	Insert( inN * 4, (char*) &inPtrToAdd, 4 );
}




void XPtrList::Add( const XPtrList& inList ) {

	if ( mOrdering == cOrderNotImportant )
		UtilStr::Append( inList );
	else {
		int i, n = inList.Count();
		for ( i = 1; i <= n; i++ ) 
			Add( inList.Fetch( i ) );
	}
}




void*& XPtrList::operator[] ( const long inIndex ) {
	long len;
	
	if ( inIndex >= 0 ) {
		len = mStrLen;
		if ( inIndex >= len >> 2 ) {
			Insert( len, '\0', ( inIndex + 1 ) * 4 - len );
		}
			
		return *( (void**) ( mBuf + inIndex * 4 + 1 ) ); }
	else
		return sDummy;
}
			




bool XPtrList::Remove( const void* inMatchPtr ) {
	long	idx = FindIndexOf( inMatchPtr );
	
	return RemoveElement( idx );
}



bool XPtrList::RemoveElement( long inIndex ) {
	char* s;
	
	if ( inIndex > 0 && inIndex <= Count() ) {
		inIndex--;
		if ( mOrdering == cOrderNotImportant ) {
			s = getCStr();
			*( (void**) (s + inIndex * 4) ) = *( (void**) (s + length() - 4 ) );
			Trunc( 4 ); }
		else 
			UtilStr::Remove( inIndex * 4 + 1, 4 );
		return true; }
	else
		return false;
}




bool XPtrList::RemoveLast() {

	if ( length() > 0 ) {
		Trunc( 4 );
		return true; }
	else
		return false;
}


void XPtrList::RemoveAll() {
	Wipe();
}




void XPtrList::MoveToHead( long inIndex ) {
	void* p;
	char* s;
	
	if ( inIndex > 1 ) {
		if ( Fetch( inIndex, &p ) ) {
			inIndex--;
			s = getCStr();
			if ( mOrdering == cOrderNotImportant )
				*( (void**) (s + inIndex * 4) ) = *( (void**) s);	
			else
				UtilStr::Move( s+4, s, inIndex * 4 );
			*( (void**) s) = p;
		}
	}
}




void* XPtrList::Fetch( long inIndex ) const {
	if ( inIndex >= 1 && inIndex <= length() >> 2 )
		return *( (void**) (getCStr() + ( inIndex - 1 ) * 4) );
	else
		return NULL;
}


bool XPtrList::Fetch( long inIndex, void** ioPtrDest ) const {
	
	if ( ioPtrDest ) {
		if ( inIndex >= 1 && inIndex <= length() / 4 ) {
			*ioPtrDest = *( (void**) (getCStr() + ( inIndex - 1 ) * 4) );
			return true; }
		else
			*ioPtrDest = NULL;
	}
	
	return false;
}







void XPtrList::Randomize() {
	void*	temp, **ptrArray = (void**) getCStr();
	long	i, randIdx, n = Count();
	
	for ( i = 0; i < n; i++ ) {
		randIdx = nodeClass::Rnd( 1, n );
		temp = ptrArray[ i ];
		ptrArray[ i ] = ptrArray[ randIdx-1 ];
		ptrArray[ randIdx-1 ] = temp; 
	}
}