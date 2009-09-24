
#include "Hashtable.h"

#include <stdlib.h>



long Hashtable::sTableSizes[ NUM_SIZES ] = { 23, 97, 397, 797, 3203, 6421, 12853, 51437, 205759, 411527, 1646237, 3292489, 6584983, 13169977, 52679969 };

#define _MIN( a, b )	( (a) > (b) ? (b) : (a) )
	

Hashtable::Hashtable( bool inKeysOwned, int inLoadFactor ) {
	mKeysOwned		= inKeysOwned;
	mTableSize		= 0;
	mTable			= NULL;
	mNumEntries		= 0;
	mThreshold		= 0;
	mLoadFactor		= inLoadFactor;
	
	// Don't let the client kill himself with a bad load factor
	if ( mLoadFactor > 100 )
		mLoadFactor = 100;
	else if ( mLoadFactor < 10 )
		mLoadFactor = 10;
		
	Rehash();
}


Hashtable::~Hashtable() {
	RemoveAll();
	
	if ( mTable )
		delete []mTable;
}

	
void Hashtable::Rehash() {
	long		i, index, oldSize = mTableSize;
	KEntry*		temp, *entry;
	KEntry**	oldTable = mTable;
	
	// Find the next bigger table size
	for ( i = 0; mTableSize <= oldSize; i++ )
		mTableSize = sTableSizes[ i ];

	
	// Alloc the new table and make it empty	
	mTable = new KEntry*[ mTableSize ];
	for ( i = 0; i < mTableSize; i++ )
		mTable[ i ] = NULL;
	
	// Rehash all the old values into the new table
	for ( i = 0; i < oldSize; i++ ) {
		for ( entry = oldTable[ i ]; entry; ) {
			index = entry -> mKey % mTableSize;
			temp = entry -> mNext;
			entry -> mNext = mTable[ index ];
			mTable[ index ]	= entry;
			entry = temp;
		}	
	}

	// Set the new size thet we'll rehash at
	mThreshold = mLoadFactor * mTableSize / 100;
	
	// We don't need the old table anymore
	if ( oldTable )
		delete []oldTable;
}





void* Hashtable::put( long inKey, const Hashable* inHKey, void* inValue ) {
	long		index;
	KEntry*		entry;
	void*		oldVal;

	// See if we need to make the hash table bigger
	if ( mNumEntries >= mThreshold )
		Rehash();
	
	// If we already have the key, replace the value and pass the old one back
	entry = fetchEntry( inKey, inHKey );
	if ( entry ) {
		oldVal			= entry -> mValue;
		if ( mKeysOwned && inHKey )
			delete inHKey; }
	else { 
		oldVal				= NULL;
		index				= ((unsigned long) inKey) % mTableSize;
		entry				= new KEntry;
		entry -> mHashable	= inHKey;
		entry -> mKey		= ((unsigned long) inKey);
		entry -> mNext		= mTable[ index ];
		mTable[ index ]		= entry;
		mNumEntries++;
	}
	
	entry -> mValue		= inValue;
	
	return oldVal;
}



bool Hashtable::Get( long inKey, void** outValue ) const {
	KEntry*	entry = fetchEntry( inKey, NULL );

	if ( entry && outValue ) 
		*outValue = entry -> mValue;

	return entry != NULL;
}


bool Hashtable::Get( const Hashable* inKey, void** outValue ) const {
	KEntry*	entry = fetchEntry( inKey -> Hash(), inKey );

	if ( entry && outValue ) 
		*outValue = entry -> mValue;

	return entry != NULL;
}



void Hashtable::GetValues( XPtrList& outValues ) {
	KEntry** entryP = mTable, *entry;
	int i;
	
	outValues.RemoveAll();
	outValues.Dim( 4 );
	
	for ( i = 0; i < mTableSize; i++ ) {
		entry = *entryP;
		while ( entry ) {
			outValues.Add( entry -> mValue );
			entry = entry -> mNext;
		}

		entryP++;
	}
}


void Hashtable::GetKeys( XPtrList& outKeys ) {
	KEntry** entryP = mTable, *entry;
	int i;
	
	outKeys.RemoveAll();
	outKeys.Dim( 4 * NumEntries() );
	
	for ( i = 0; i < mTableSize; i++ ) {
		entry = *entryP;
		while ( entry ) {
			outKeys.Add( ( entry -> mHashable ) ? entry -> mHashable : (void*) entry -> mKey );
			entry = entry -> mNext;
		}

		entryP++;
	}
}




KEntry*	Hashtable::fetchEntry( long inKey, const Hashable* inHKey ) const {
	long		index = ( (unsigned long) inKey ) % mTableSize;
	KEntry*		entry = mTable[ index ];

	// Look thru the chain for the key and the entry holding that key-value pair
	while ( entry ) {
		if ( entry -> mKey == inKey ) {
			if ( ! entry -> mHashable || ! inHKey )
				return entry;
			else if ( inHKey -> Equals( entry -> mHashable ) )
				return entry;
		}
		entry = entry -> mNext;
	}
	
	return NULL;
}






void Hashtable::RemoveAll() {
	long i;
	KEntry*	entry, *temp;
	
	// Step theu the dict table and delete all the KEntries
	for ( i = 0; i < mTableSize; i++ ) {
		for ( entry = mTable[ i ]; entry; ) {
			if ( mKeysOwned && entry -> mHashable )
				delete entry -> mHashable;
			temp = entry -> mNext;
			delete entry;
			entry = temp;
		}
		mTable[ i ] = NULL;
	}
	mNumEntries = 0;
}



void* Hashtable::remove( long inKey, const Hashable* inHKey ) {
	long		index = ( (unsigned long) inKey ) % mTableSize;
	KEntry*		entry = mTable[ index ], *prev = NULL;
	bool		isEqual;
	void*		retVal;

	// Look thru the chain for a matching key and delete it
	while ( entry ) {
		if ( entry -> mKey == inKey ) {
			if ( inHKey && entry -> mHashable )
				isEqual = inHKey -> Equals( entry -> mHashable );
			else
				isEqual = true;
				
			if ( isEqual ) {
				if ( mKeysOwned && entry -> mHashable )
					delete entry -> mHashable;

				if ( prev == NULL ) 
					mTable[ index ] = NULL;
				else 
					prev -> mNext = entry -> mNext;
				retVal = entry -> mValue;
				delete entry;
				mNumEntries--;
				return retVal;
			}
		}
		prev = entry;
		entry = entry -> mNext;
	}
	
	return NULL;
}



long& Hashtable::operator[] ( const long inKey ) {
	KEntry*	entry = fetchEntry( inKey, NULL );
	
	if ( ! entry ) {
		Put( inKey, 0 );
		entry = fetchEntry( inKey, NULL );
	}
	
	return (long&) entry -> mValue;
}



void*& Hashtable::operator[] ( const void* inKey ) {
	KEntry*	entry = fetchEntry( (long) inKey, NULL );
	
	if ( ! entry ) {
		Put( (long) inKey, 0 );
		entry = fetchEntry( (long) inKey, NULL );
	}
	
	return entry -> mValue;
}






void Hashtable::Rank( XPtrList& outKeys, CompFunctionT inCompFcn, long inNumToRank ) {
	long i, n = NumEntries();
	KEntry** entryP = mTable, *entry;
	void **p; 
	void **temp = new void*[ 2 * n ];
	
	if ( inNumToRank < 0 )
		inNumToRank = n;
	inNumToRank = _MIN( inNumToRank, n );

	// To rank, we must sort by value, with a tag on each element of its key
	p = temp;
	for ( i = 0; i < mTableSize; i++ ) {
		entry = *entryP;
		while ( entry ) {
			*p = entry -> mValue;  
			p++; 
			*p = ( entry -> mHashable ) ? (void*)entry -> mHashable : (void*) entry -> mKey;
			p++;
			entry = entry -> mNext;
		}

		entryP++;
	}

	// Default to long-long ranking
	if ( ! inCompFcn )
		inCompFcn = sLongComparitor;
	
	// Sort the floats
	qsort( temp, n, 8, inCompFcn );
	
	// Put the sorted results in the destination
	outKeys.RemoveAll();
	p = temp + 1;
	for ( i = 0; i < n; i++ ) {
		outKeys.Add( *p );
		p += 2;
	}
	
	// Cleanup
	delete []temp;
}





int Hashtable::sLongComparitor( const void* inA, const void* inB ) {
	return ((long) inB - (long) inA);
}
