#ifndef _HASHTABLE_
#define _HASHTABLE_

#include "Hashable.h"
#include "XPtrList.h"






struct KEntry {
	public:
		unsigned long		mKey;
		const Hashable*		mHashable;
		void*				mValue;
		KEntry*				mNext;			
};




	
	
class Hashtable {


	public:
		// Makes a new Hashtable.  The load factor how efficient memory usage is. The higher
		// the load factor, the more compact the Hashtable it, but at a penalty of performance.
		// In this implementaion, the performance gains out weight the more mem used by 4 or 5 to 1.
		// If inKeysOwned is true, this Hashtable will delete any keys that are of type Hashable when
		// are no longer needed (and on the destruction of this).  For keys added via Put( long, void* ), they
		// are never owned since the keys are actal 4-byte numbers.  For example, a Hashtable used to map
		// ptrs to void*s never owns any keys because the point of using Put( long, void* ) is that the longs used
		// as keys (ie, recast pts) are already unique by nature.
							Hashtable( bool inKeysOwned = false, int inLoadFactor = 50 );
		virtual				~Hashtable();
							
		// Returns the number of entries (ie, key-value pairs) in this hashtable
		inline long 		NumEntries() const											{ return mNumEntries;											}

		//	Associates inValue with inKey.  
		//	Note: If a value is already assigned to that key, that value is replaced with inValue and
		//			the old value is returned, otherwise NULL is returned.
		//	Note: Values are never owned by a Hashtable--it's the client's responsibilty (they couldn't because they don't know what the long represents!).
		//		  Keys are owned by a hashtable depending on what was pass in this' contructor.  
		//	Note: If keys are not owned (see contructor), it's the client's responsibility to (a) delete whatever gets
		//			returned from a Put( Hashable*, void* ) and (b) keep any Hashables currently in this table via Put() 
		//			allocated until this table is deleted/destroyed.
		//	Note: O(1) running time
		inline void*		Put( long				inKey, void* inValue )				{ return put( inKey, NULL, inValue );						}
		inline void*		Put( const Hashable*	inKey, void* inValue )				{ return put( inKey->Hash(), inKey, inValue );				}
				
		//	Fetches the value associated with inKey (from a Put()).   
		//	Note: If a matching key is found, true is returned.  If outValue is NULL, the value is not returned.
		//	Note: O(1) running time
		bool				Get( long				inKey, void** outValue = NULL ) const;
		bool				Get( const Hashable*	inKey, void** outValue = NULL ) const;

		//	Returns a list of all the values in this hashtable in no particular order.
		//	Note: The values are added one by one, so the list can have any sorting/order flag set on it.
		void				GetValues( XPtrList& outValues );
		
		// 	Returns a list of all the keys in this hashtable in no particular order
		//	Note: The keys are added one by one, so the list can have any sorting/order flag set on it.
		void				GetKeys( XPtrList& outKeys );
				
		//	Return's the nth value in this hashtable, with the values in no particular order. If there 
		//	are less than n entries this this hashtable, NULL is returned.
		//	Note: O(N) running time.
		//void*				Get( long inN );
		
		// 	These are 2 other handy ways of invoking Get(long, void**).  The only difference is that if the key isn't
		//	found, it is entered in this dictionary with a value set to zero/NULL.
		void*&				operator[] ( const void* inKey );	
		long&				operator[] ( const long inKey );
		
		//	Removes inKey from this Hashtable.  If inKey is not found, NULL is returned.  
		//	If inKey was found, the key (and its associated value) are removed and the key's value is returned.
		//	Note:  if keys are owned, this fcn deletes the hashable that was used in Put()
		inline void*		Remove( long			inKey )								{ return remove( inKey, NULL );								}
		inline void*		Remove( const Hashable*	inKey )								{ return remove( inKey -> Hash(), inKey );					} 
	
		//	Removes/clears all keys in this Dictionary.
		void				RemoveAll();
		
		//	Pre:	The compare function takes in two values and returns if the first is less, equal, or greater than (See XPtrList.h)
		//	Note:	The compare fcn is arbitrary, but if it's it left out (ie. NULL) then it sorts by value size, low to high.
		//	Note:	The compare fcn has the same prototype as the one in XPtrList.h, but this compare fcn is one qsort uses, which
		//			means the void ptrs comin in point to the array elements (not the 4 bytes in the cur pt in the array)
		//	Post: 	Get( outKeys[ i ] ) is the ith largest value in this list (in reference to the given compariston fcn)
		//	Post: 	outKeys.Count() == inNumToRank  (ie, only inNumToRank values of the ranking are returned)
		//	Note:	If inNumToRank is invalid, the full ranking is returned
		void				Rank( XPtrList& outKeys, CompFunctionT inCompFcn = NULL, long inNumToRank = -1 );
		
	protected:
		void				Rehash();

		KEntry*				fetchEntry( long inKey, const Hashable* inHashable ) const;
		void*				remove( long inKey, const Hashable* inHashable );
		void*				put( long inKey, const Hashable* inHashable, void* inValue );
		
		static int			sLongComparitor( const void* inA, const void* inB );
		enum {
			NUM_SIZES		= 15
		};
		
		
		bool				mKeysOwned;
		static long			sTableSizes[ NUM_SIZES ];

		KEntry**			mTable;
		unsigned long		mTableSize;
		long				mNumEntries;
		long				mLoadFactor;		// Percent from 0 to 100;
		long				mThreshold;			// Always mLoadFactor * mNumKeys / 100
};





#endif