#pragma once

#ifndef _UTILSTR_
#define _UTILSTR_


#ifndef NULL
#define NULL 0L
#endif

#include "Hashable.h"


class CEgIStream;
class CEgOStream;

//	This is versitile string class, optimized for all string sizes
class UtilStr : public Hashable {


	public:						
									UtilStr();
									UtilStr( const char* inCStr );
									UtilStr( const unsigned char* inStrPtr );
									UtilStr( const UtilStr& inStr );
									UtilStr( const UtilStr* inStr );
									UtilStr( long inNum );
									UtilStr( const void* inPtr, long numBytes );
		virtual						~UtilStr();
								
		//	*** Assign ***
		//	Post:	This UtilStr empties itself and appends the argument (ie it copies the argument)
		inline void					Assign( const char* inCStr )								{ mStrLen = 0; Append( inCStr );				}
		void						Assign( const unsigned char* inStrPtr );
		void						Assign( const UtilStr* inStr );
		void						Assign( const UtilStr& inStr );
		void						Assign( long inNum );
		void						Assign( char inChar );
		void						Assign( const void* inPtr, long numBytes );
		void						Assign( CEgIStream& inStream, long numBytes );
		
		//  *** Append ***
		//	Post:	The argument is appended to this string
		void						Append( const char* inCStr );
		inline void					Append( const unsigned char* inStrPtr );
		inline void					Append( char inChar )										{ Append( &inChar, 1 );							}
		void						Append( long inNum );
		inline void					Append( const UtilStr* inStr );
		inline void					Append( const UtilStr& inStr )								{ Append( inStr.getCStr(), inStr.length() );	}
		void						Append( const void* inSrce, long numBytes ); 
		
		//	*** Operators ***
		//	The + signifies Append() an append and the = signifies Assign()
		UtilStr						operator + ( const UtilStr& inStr );
		UtilStr						operator + ( const char* inCStr );
		UtilStr						operator + ( const long inNum );
		UtilStr&					operator = ( const UtilStr& inStr );
		
		
		//	Post:					Makes the length of this <numBytes>.
		//	Note:					The data in this is garbage!
		char*						Dim( unsigned long numBytes )						{ mStrLen = 0; Append( (void*) NULL, numBytes );	 return mBuf;	}
		
		//	Post:	Swaps the internal string ptrs.  This fcn is useful if you need to assign a string value and don't care
		//			if the original string changes.
		void						Swap( UtilStr& ioStr );

		//	Post:	The length of this string is returned
		inline unsigned long		length() const										{ return mStrLen;						}
		
		//	Post:	Returns the <i>th character in this string.  If i < 1 or greater than len, 0 is returned
		char						getChar( unsigned long i ) const;
		
		//	Post:	The <i>th character is replaced with <inChar>.  If i < 1 or greater than len, this
		//			string is unaffected.
		void						setChar( unsigned long i, char inChar );

		//	Post:	<inStr> is appended to the beginning to this string.
		void 						Prepend( const char inChar);
		void						Prepend( const char* inStr );
		void						Prepend( UtilStr& inStr );
		
		//	Post:	<inSrce> (or <inNum>) is inserted in the string after the <inPos>th character
		//	Note:	When <inPos> = 0, this is the same as Prepend(...)
		//	Note:	If <inPos> is greater then or equal to the length of this str, <inSrce> is appended to the end
		//			"HiFred".Insert( 2, "," )  --->  "Hi,Fred"
		void						Insert( unsigned long inPos, const UtilStr& inSrce );
		void						Insert( unsigned long inPos, const char* inSrce, long inSrceLen );
		void						Insert( unsigned long inPos, char inChar, long inNumTimes );
		void						Insert( unsigned long inPos, long inNum );
				
		//	Post:	Returns a ptr to a pascal style string of this string.  Remember that if this string is greater than
		//			255 chars, only the first 255 can be accessed.  *NOTE: This fcn is designed for
		//			instantanious use (ie, if you change the string at all, you *must* re-call this method).  In other words,
		//			treat the ptr returned as *read* only!
		unsigned char*				getPasStr() const;
		
		//	Post:	Returns a ptr to a C style string of this string.  *NOTE: This fcn is designed for
		//			instantanious use (ie, if you change the string at all, you *must* re-call this method). In other words,
		//			treat the ptr returned as *read* only!
		char*						getCStr() const;
		
		//	Post:	This string is emptied (length is zero)
		inline void 				Wipe()														{ mStrLen = 0;		}
		
		//	Post:	Replaces all instances of <inTarget> with <inReplacement>
		//			Returns how many replaces took place
		long						Replace( char inTarget, char inReplacement );
		long						Replace( char* inTarget, char* inReplacement, bool inCaseSensitive = true );
		
		//	Post:	Truncates <numToChop> chars from this string, either from the right or left.  If <numToChop> is
		//			less than 1, this string is unaffected.  If <numToChop> is greater or equal to the length
		//			if this string, the string is emptied.
		void						Trunc( unsigned long numToChop, bool fromRight = true );
		
		//	Post:	At position <inPos> in the string, <inNum> characters are removed
		//			"Hi,Fred".Remove( 3, 1 )  --->  "HiFred"
		void						Remove( unsigned long inPos, unsigned long inNum );
		
		//	Post:	All instances of a given string are removed from this string
		//	Note:	inLen is the length of inStr.  If inLen < -1, the length of inStr is calculated
		void						Remove( char* inStr, int inLen = -1, bool inCaseSensitive = true );
		
		//	Post:	Truncates after the <inNumToKeep>th character.  If <inNumToKeep> is greater than or equal to
		//			the length, nothing happens.
		void						Keep( unsigned long inNumToKeep );
		
		//	Post:	If this' len is larger than <inMaxLen>, a 'É' replaces the <inPos>th character and chars are removed 
		//			after the 'É' until the this' length is <inMaxLen>
		void						PoliteKeep( unsigned long inMaxLen, unsigned long inPos );

		//	Post:	A simple filter that decapitalized all chars that aren't the first in each word.
		void						Decapitalize();
		
		//	Post:	Capitalizes all the chars in this string.
		void						Capitalize();
		
		//	Post:	Removes any leading spaces in this string. 
		void						ZapLeadingSpaces();
		
		//	Pre:	<pasDestPtr> points to a pascal string of length <inBytesToCopy>
		//	Post:	The contents of this string is copied to <pasDestPtr>, updating its length byte
		void						copyTo( unsigned char* pasDestPtr, unsigned char inBytesToCopy ) const;
		
		//	Pre:	<cDestPtr> points to a c string with an allocated length of <inBytesToCopy>
		//	Post:	The contents of this string is copied to <cDestPtr>, appending the NUL byte
		void						copyTo( char* cDestPtr, unsigned long inBytesToCopy ) const;
		
		//	Post:	This string is read from <inFile> from the current file position
		void						ReadFrom( CEgIStream* inStream );
		
		//	Post:	This string writes itself to <inStream> at the current file position
		void						WriteTo( CEgOStream* inStream ) const;
		
		//	Post:	Returns the rational value of this string times <inMultiplier>.  All chars not '0'..'9' are
		//			ignored, except a leading '-' and a '.', where the digits that follow are decimal digits
		//	Usage:	"12.34".GetValue( 20 ) will return 246
		long						GetValue( long inMultiplier = 1 ) const;
		
		// 	Post:	Returns a floating point value of this string
		double						GetFloatValue() const;
				
		//	Pre:	<inDivisor> != 0
		//			<inNumDecPlaces> < 8
		//	Post:	Assigns the string equivilent of <inVal>/<inDivisor> to this string, truncating the string
		//			after <inNumDecPlaces> decimal places.
		//	Usage:	SetValue( 24686, 20, 2 ) would assign this string to "12.34"
		void						SetValue( long inVal, long inDivisor = 1, int inNumDecPlaces = 5 );
		
		//	Pre:	|inValue| < 2^31
		//	Sets this string to the given float value, using '.' as a decimal point, using at most inDigits behind the decimal point
		void						SetFloatValue( float inValue, int inDigits = 4 );
		
		//	Post:	Assigns this string the roman numeral string for the number <inValue>.
		void						SetRomanValue( long inValue );
		
		//	Post:	Returns the character position within this string of the rightmost occurance of <c>. If
		//			<c> is not found, 0 is returned.
		inline long					FindLastInstanceOf( char c ) const						{ return FindPrevInstanceOf( mStrLen, c );		}

		//	Post:	Returns the character position within this string of the leftmost occurance of <c> after pos <inPos>. 
		//			If <c> is not found, 0 is returned. Setting inPos to 0 starts from the beginning.
		//	"blah,blah!".FindNextInstanceOf( 0, ',' ) == 5
		long						FindNextInstanceOf( long inPos, char c ) const;

		//	Post:	Returns the character position within this string of the rightmost occurance of <c> after pos <inPos>. 
		//			If <c> is not found, 0 is returned.  	
		long						FindPrevInstanceOf( long inPos, char c ) const;
		
		//	Post:	Compares this str with <inStr> and returns:
		//			"hi".compareTo( "hi", 3, true ) == 0
		//			"a".compareTo( "z", 2, true ) < 0
		//			"Z".compareTo( "a", 2, false ) > 0
		int							compareTo( const UtilStr* inStr,			bool inCaseSensitive = true ) const;
		int							compareTo( const unsigned char* inPStr, 	bool inCaseSensitive = true ) const;
		int							compareTo( const char* inCStr, 				bool inCaseSensitive = true ) const;


		//	Post:	Compares the first <inN> chars of data at <s1> and <s2> and returns:
		//			StrCmp( "hi", "hi", 3, true ) == 0
		//			StrCmp( "a", "z", 2, true ) < 0
		//			StrCmp( "z", "a", 2, true ) > 0
		//	Note:	If <inN> < 0, the length of s1 (as a C string) is used.
		//	Note:	StrCmp( *, *, 0, * ) always returns false.
		static int					StrCmp( const char* s1, const char* s2, long inN, bool inCaseSensitive );
				
		//	Post:	Returns the 1st next instance of <inStr> in this str, skipping the first inStartingPos chars.   If an instance cannot be found, 0 is returned.
		//	Note:	inLen is the length of inStr.  If inLen < -1, the length of inStr is calculated
		//			"Hi,Fred".contains( "fred", false ) == 4
		inline long 				contains( const UtilStr&	inStr,					int inStartingPos = 0,	bool inCaseSensitive = true ) const			{ return contains( inStr.getCStr(), inStr.length(), inStartingPos, inCaseSensitive );	}		
		long 						contains( const char*		inStr, int inLen = -1,	int inStartingPos = 0, 	bool inCaseSensitive = true ) const;		
		
		//	Post:	Blockmoves memory
		static void					Move( void* inDest, const void* inSrce, unsigned long inNumBytes ); 

		//	Post:	Appends <inStr>, exporting/translating <inData> to "meta" format (ie, all translated bytes >= 32 or <= 127)
		void						AppendAsMeta( const UtilStr* inData );
		void						AppendAsMeta( const void* inData, long inLen );
		
		//	Post:	Appends <inStr>, importing/translating <inStr> from "meta" format.
		//	Note:	See AppendAsMeta()
		//	Ex:		s.Wipe(); t.Wipe();								// Empty our strings s and t
		//			s.AppendAsMeta( dataPtr, 5 ); 					// Encode some data and put it in s
		//			t.AppendFromMeta( s.getCStr(), s.length() );	// t now contains exactly what was in dataPtr				
		void						AppendFromMeta( const UtilStr* inStr );
		void						AppendFromMeta( const void* inStr, long inLen );
		
		// Post:	Returns a psudorandom long for this string
		long						Hash() const;
		
		// Must be defined as a Hashable
		bool						Equals( const Hashable* inComp ) const;
		
		
		
		// Pre:		inB1 and inB2 are '0' to '9', 'A' to 'F', or 'a' to 'f'
		// Post:	The corresponding byte value is appended to this string
		void						AppendHex( char inB1, char inB2 );

		// Post:	Returns a floating point number for the given ASCII chars
		static double				GetFloatVal( char* inNumStr, long inLen );
		
		//	Post:	Returns the truncated signed long value of this string.  All characters not '0'..'9' are ignored
		//			except a leading '-'
		//			<*outPlacePtr> is set to 10^( 1 + trunc( log10( <strVal> ) ) )
		//	Usage:	"12.9".GetIntValue( &i ) returns 12 and assigns i to 100
		static long					GetIntValue( char* inStr, long inLen, long* outPlacePtr = NULL );

		//	Longest Common Substring Seach Score
		// 	Post:	Returns a 'score' -- the greater the score, the more similar inStr is this string.
		//	Note:	If inLen < 0, inStr (as a C string) is used.
		//	Note:	This fcn is *not* communitive ( "cat" is more similar to "bigcat" then "bigcat" is to "cat" )
		// 	Note:	Running time O( this.length * inStr.length )
		long						LCSMatchScore( const char* inStr, long inStrLen = -1 ) const;
		long						LCSMatchScore( const UtilStr& inStr	) const			{ return LCSMatchScore( inStr.getCStr(), inStr.length() );	}

	protected:
		unsigned long				mBufSize; 				// Physical size of buf
		unsigned long				mStrLen;				// NOTE: ALWAYS holds string size
		char*						mBuf;					// Holds ptr to str data
		
		void						init();

		friend class UtilStr;
};




#endif
