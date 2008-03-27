#ifndef _XLongList_
#define _XLongList_

// by Andrew O'Meara

#include "XPtrList.h"


class XLongList {
	
	friend class XLongList;

	public:
								XLongList( ListOrderingT inOrdering = cOrderNotImportant );		// See XPtrList.h for ListOrderingT choices
								
		// See XPtrList.h for description of functions.
		inline long				Add( long inNum )								{ return mList.Add( (void*) inNum );				}
		inline void				Add( const XLongList& inList )					{ mList.Add( inList.mList );						}
		inline bool				Remove( long inNum )							{ return mList.Remove( (void*) inNum );				}
		inline bool				RemoveElement( long inIndex )					{ return mList.RemoveElement( inIndex );			}
		inline void				RemoveAll()										{ mList.RemoveAll(); 								}
		inline long				Fetch( long inIndex )							{ return (long) mList.Fetch( inIndex );				}
		inline bool				Fetch( long inIndex, long* ioPtrDest ) const	{ return mList.Fetch( inIndex, (void**)ioPtrDest );	}
		inline long				FindIndexOf( const long inMatch ) const			{ return mList.FindIndexOf( (void*) inMatch );		}
		inline long				Count()	const									{ return mList.Count();								}

		void					Randomize()										{ mList.Randomize();								}
	
		//	Post:	Any nums in this list that are in the interval [ inStart, inEnd ] are removed.
		//	Note:	Note how the interval is inclusive.
		void					SubtractRange( long inStart, long inEnd );
		
		//	Post:	Any nums in this list that are not in the interval [ inStart, inEnd ] are removed.
		//	Note:	Note how the interval is inclusive.
		void					ApplyMask( long inStart, long inEnd );
	
		// 	Allows easy dynamic array usage.  Simply use any index and XLongList will expand to meet that size.
		//	Impt:	Zero based indexing is used here!! (In contrast to Fetch())
		//	Note:	Elements that are newly accessed are initialized to 0
		//	Note:	Indexs below 0 lead to sDummy;
		//	Note:	Since caller has access to changes values, any current sorting fcn is not used
		long&					operator[] ( const long inIndex );			


		//	Post: Ranks all the values in this list.
		//	Post: Fetch( outRank[ i ] ) is the ith largest value in this list.
		//	Post: outRank.Count() == inNumToRank  (ie, only inNumToRank values of the ranking are returned)
		//	Note: If inNumToRank is invalid, the full ranking is returned
		//	Note: O( N log N ) running time
		void					Rank( XLongList& outRank, long inNumToRank = -1 ) const;

		
	protected:
		static int				sLongComparitor( const void* inA, const void* inB );
		static int				sQSLongComparitor( const void* inA, const void* inB );

		XPtrList				mList;
		
		static long				sDummy;
		
};




#endif