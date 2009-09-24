#ifndef _FileSpecList_
#define _FileSpecList_


#include "XStrList.h"

class CEgFileSpec;

// Designed to represent a list of file specs.
class FileSpecList {

	public:
								FileSpecList( XStrListOptsT inOption, ListOrderingT inOrdering );
		virtual					~FileSpecList();

		// Removes all the files specs from this list
		void					RemoveAll();
		
		// Makes a private copy of the given spec and inserts it into this list.  The number
		// returned is what element the spec now is in this list (1-based indexing)
		long					AddCopy( const CEgFileSpec& inSpec );

		// Get access to the certain spec
		const CEgFileSpec*		FetchSpec( int inIndex ) const;	
		
		// See what the name of the ith item is (1-based indexing)
		const UtilStr*			FetchSpecName( int inIndex ) const;
		bool					FetchSpecName( int inIndex, UtilStr& outStr ) const;

		//	Returns spec (via index) of closest matching name with inStr
		long					FetchBestMatch( const UtilStr& inStr );

		// Look for a item with the given name.  If nothing is found, 0 is returned
		long					Lookup( UtilStr& inName ) const;
		
		// Returns the number of specs in this list
		long					Count() const										{ return mSpecNames.Count();		}
		
		
	protected:
		XStrList				mSpecNames;
		XPtrList				mSpecs;
		
};


#endif