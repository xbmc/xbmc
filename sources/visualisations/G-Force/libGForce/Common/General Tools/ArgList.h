#ifndef _ARGLIST_
#define _ARGLIST_

#pragma once


#include "UtilStr.h"

class MetaExpr;
class Arg;
class CEgOStream;
class CEgIStream;


class ArgList  {

	friend class ArgList;
	
	public:	
								ArgList();
		virtual					~ArgList();
	
		// 	Post:	Adds any args in the given string meta line to this arg list.  Note that if arg 'blah' is in this list
		// 			and someone calls this.SetArgs( "hi=123,bye=345" ), then 'blah' is still in this list and has the original
		// 			value it had before the call to SetArgs().
		//	Note:	An example of a meta arg list:  hi=123,name="sally",andy="really cool"
		//	Note:	This call is functionally the same as:    ArgList temp; temp.SetArgs( str ); this.SetArgs( temp );
		//	Note:	In the above example, if you wanted to get rid of blah (and all other args before the Set, call Clear())
		//	Note:	For SetArgs( char*, long ), len == -1 means that the length will be computed. 
		void					SetArgs( const UtilStr& inArgList )								{ SetArgs( inArgList.getCStr(), inArgList.length() );		}
		void					SetArgs( const char* inArgList, long inLen = -1 );
		
		//	A more flexible version of SetArgs() than above.  This ignores line breaks, "//" comments, and "/*"-"*/" block comments.
		void					SetArgs( CEgIStream* inStream );
		
		//	Post:	Similar to above except that the arg list is already in the form of an arg list. 
		void					SetArgs( const ArgList& inArgs );
		
		// Appends/Exports the args in this list to a meta string format.  This means all chars in the string
		//    will be above or equal to 0x20 and less than 0x80.
		//	Post:  If inLineBreaks is true, a CR/LF is put after each arg
		void					ExportTo( UtilStr& ioStr, bool inLineBreaks = false ) const;
		void					ExportTo( CEgOStream* ioStream, bool inLineBreaks = false ) const;
		
		// Writes this arg list to a stream, with no regard to byte range (in contrast to ExportTo, where all the
		//    output bytes are >= 0x20 and < 0x80
		void					WriteTo( CEgOStream* ioStream );
		void					ReadFrom( CEgIStream* ioStream );
		
		// Removes all args from this argList
		void					Clear();

		
		void					DeleteArg( long inArgID );
		
		//	Pre:	No byte in <inArgID> can be a -, =, or "
		void					SetArg( long inArgID, long inArg );
		void					SetArg( long inArgID, const UtilStr& inArg );
		void					SetArg( long inArgID, const char* inArgStr );
		

		
		//	Post:	The specified argument <inArgID> is fetched and transferred to <outArg>.
		//	Note:	If the arg is found, true is returned, else false is returned and <outArg> is zeroed.
		//  Note: 	If the arg is found and is a string, GetArg( long, UtilStr& ) puts a copy of the arg's string in the argument. 
		//          If the arg is found and is a long, GetArg( long, UtilStr& ) stores a string of the long (in base 10)
		inline bool				ArgExists( long inArgID ) const								{ return FetchArg( inArgID ) != NULL;			}
		bool					GetArg( long inArgID, bool& outArg ) const;
		bool					GetArg( long inArgID, char& outArg ) const;
		bool					GetArg( long inArgID, long& outArg ) const;
		bool					GetArg( long inArgID, UtilStr& outStr ) const;
		bool					GetArg( long inID, UtilStr& outStr, long inIndex ) const	{ return GetArg( IndexedID2ID( inID, inIndex ), outStr );	}
		long					GetArg( long inArgID ) const;
		double					GetFloat( long inArgID ) const;
		const UtilStr*			GetStr( long inArgID ) const;
		inline bool				GetArg( long inArgID, unsigned char& outArg ) const			{ return GetArg( inArgID, (char&) outArg );		}
		inline bool				GetArg( long inArgID, unsigned long& outArg ) const			{ return GetArg( inArgID, (long&) outArg );		}

		long					GetArraySize( long inID ) const;
		long					NumArgs() const;
		
		enum {
			cArgSeparator		= ','
		};
	
		static long				IndexedID2ID( long inBaseID, long inIndex );

	protected:		
		Arg*					mHeadArg;
		
		Arg*					FetchArg( long inID ) const;
};



#endif