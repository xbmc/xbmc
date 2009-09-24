#ifndef _XSTRLIST_
#define _XSTRLIST_

#include "XPtrList.h"


enum XStrListOptsT {
	cDuplicatesAllowed,
	cNoDuplicates_CaseSensitive,
	cNoDuplicates_CaseInsensitive
};



class XStrList {

	public:
								XStrList( XStrListOptsT inOption, ListOrderingT inOrdering = cOrderNotImportant );
		virtual					~XStrList();
			
		// Returns what index the copy of the given string now occupies (1-based indexing).  If 0 is returned,
		// a copy of the string was not added because it was a duplicate (if that flag is on)
		long					Add( const void* inData, long inLen );
		long					Add( const char* inStr );					
		long					Add( const UtilStr& inStr );
		
		void					RemoveAll();
		void					Remove( long inIndex ); 
		
		// Look for an item with a matching name.  If 0 is returned, no item was found.
		//virtual long			FindIndexOf( const char* inStr ) const;
		long					FindIndexOf( const UtilStr& inStr ) const;
		
		// Returns a copy of the string's contents
		bool					Fetch( long inIndex, UtilStr& outStr ) const;	
		
		// Returns direct access to the string.  You're free to modify the string,
		//   but be reminded that it's owned by this (so don't delete it)
		UtilStr*				Fetch( long inIndex ) const;

		// Returns an index of the closest matching string to inStr
		long					FetchBestMatch( const UtilStr& inStr );


		inline long				Count()	const									{ return mStrings.Count();	}

	protected:
		static int				sStrComparitor( const void* inA, const void* inB );
		static int				sStrComparitorCI( const void* inA, const void* inB );
		XStrListOptsT			mStrListOption;
		XPtrList				mStrings;
		
};


#endif