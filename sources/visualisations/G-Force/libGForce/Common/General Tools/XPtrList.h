#ifndef _XPTRLIST_
#define _XPTRLIST_

// By Andrew O'Meara

#include "UtilStr.h"

// Post:	Returns a num >= 0 iff the first arg is less than or equal to the second arg.
//			Returns a num < 0 iff the first arg is greater than the second arg.
typedef int (*CompFunctionT)(const void*, const void*);


enum ListOrderingT {
	cOrderImportant,
	cOrderNotImportant,
	cSortLowToHigh,
	cSortHighToLow
};

class XPtrList : protected UtilStr {

		friend class XPtrList;
		friend class XStrList;
		friend class XDictionary;
		friend class Hashtable;
		friend class XFloatList;
		friend class XLongList;
	
	public:
								XPtrList( const XPtrList& inList );
								XPtrList( ListOrderingT inOrdering = cOrderNotImportant );
								
		//	Post:	Assigns this from <inList>.
		void					Assign( const XPtrList& inList );
		
		//	Post:	Adds the ptr to this' list
		//	Order:	O( 1 ), O( log n ) if sorting is on.
		//	Note:	Returns what index the new element occupies (1-based indexing).
		long					Add( const void* inPtrToAdd );
		
		//	Post:	Adds the ptr to this' list after (existing) ptr number inN--make inN 0 if you want your ptr to be the 1st element
		//	Order:	O( 1 )
		//	Note:	If inN is invalid, the closest element is used
		void					Add( const void* inPtrToAdd, long inN );
		
		//	Post:	Adds each element of the incoming list to this list.  This is functionally the same as
		//			looping thru the size of inPtrList and calling this.Add() for each element.
		void					Add( const XPtrList& inPtrList );
		
		//	Post:	Searches for the ptr in the list and removes it if found.
		//	Note:	<true> is returned if the ptr was found (and removed)
		//	Order:	O( N ), O( log n + alpha n ) if sorting is on.
		bool					Remove( const void* inPtrToRemove );

		//	Note:	<true> is returned if the element was found (and removed)
		//	Order:	O( N ), O( log n ) if sorting is on.
		//	Note:	When order is set to be preserved (via cOrderImportant or a specificed sort fcn), elements
		//			at the end of the list are removed faster
		bool					RemoveElement( long inIndex );
		bool					RemoveLast();
		
		//	Post:	Empties the ptr list
		void					RemoveAll();
		
		//	Post:	Used to fetch any given ptr. If the index exists, <true> is returned and the ptr is copied to *inPtrDest.
		//	Note:	One based indexing is used
		//	Order:	O( 1 )
		void*					Fetch( long inIndex ) const;
		bool					Fetch( long inIndex, void** ioPtrDest ) const; 
		inline bool				FetchLast( void** ioPtrDest ) const									{ return Fetch( Count(), ioPtrDest );			}

		// 	Allows easy dynamic array usage.  Simple use any index and XPtrList will expand to meet that size.
		//	Impt:	Zero based indexing is used here!! (In contrast to Fetch())
		//	Note:	Elements that are newly accessed are initialized to NULL
		//	Note:	Indexs below 0 lead to sDummy;
		//	Note:	Since caller has access to changes values, any current sorting fcn is not used
		void*&					operator[] ( const long inIndex );			
		
		//	Post:	Moves element at <inIndex> so that it has index 1.
		void					MoveToHead( long inIndex );
		
		//	Post:	Searches for the given ptr in its ptr list and returns the index of the match
		//			If the item cannot be found, 0 is returned
		//	Note:	One based indexing is used
		//	Order:	O( N ), O( log n ) if sorting is on.
		long					FindIndexOf( const void* inMatchPtr ) const;
		
		//	Post:	Returns the number of items in this list.
		//	Order:	O( 1 )
		inline long				Count() const														{ return length() / 4;		}
		
		//	Post:	All ptrs in this list are randomly reordered.
		void					Randomize();
			
		//	Allows the compare fcn to be set.  Causes the current contents to be removed.
		void					SetCompFcn( CompFunctionT inFcn, bool inSortLowToHigh );
		
	protected:

		
		ListOrderingT			mOrdering;
		CompFunctionT			mCompFcn;
		
		//	Pre:	The list of nums must be sorted
		//	Post:	Returns the (one based) index of the predicessor of <inNum>.  If zero is returned,
		//			then <inNum> is <= than all the elements in the list.
		long					FetchPredIndex( const void* inNum ) const;
		
		static void*			sDummy;

};





#endif