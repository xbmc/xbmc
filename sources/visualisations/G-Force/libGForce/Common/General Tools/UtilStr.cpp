
#include "UtilStr.h"


#include "..\io\CEgIStream.h"
#include "..\io\CEgOStream.h"

#include <math.h>

#include <xtl.h>

UtilStr::UtilStr() {

	init();
}



UtilStr::UtilStr( const char* inCStr ) {
	init();
	Append( inCStr );
}




UtilStr::UtilStr( const unsigned char* inStrPtr ) {
	init();
	Append( inStrPtr );
}



UtilStr::UtilStr( const UtilStr& inStr ) {
	init();
	Append( &inStr );
}



UtilStr::UtilStr( const UtilStr* inStr ) {
	init();

	Append( inStr );
}



UtilStr::UtilStr( long inNum ) {
	init();
	Append( inNum );
}


UtilStr::UtilStr( const void* inPtr, long bytes ) {

	init();
	Append( inPtr, bytes );
}





UtilStr::~UtilStr() {

	if ( mBuf ) 
		delete[] mBuf;
}



void UtilStr::init() {

	mStrLen			= 0;
	mBufSize 		= 0;
	mBuf			= NULL;
}



void UtilStr::Swap( UtilStr& ioStr ) {
	long len = mStrLen, size = mBufSize;
	char* buf = mBuf;
	
	mBuf			= ioStr.mBuf;
	mStrLen			= ioStr.mStrLen;
	mBufSize		= ioStr.mBufSize;
	
	ioStr.mBuf		= buf;
	ioStr.mStrLen	= len;
	ioStr.mBufSize	= size;
}



unsigned char* UtilStr::getPasStr() const {
	
	if ( ! mBuf )
		return (unsigned char*) "\0";
		
	if ( mStrLen < 255 )
		mBuf[0] = mStrLen;
	else
		mBuf[0] = 255;

	return (unsigned char*) &mBuf[0];
}




char* UtilStr::getCStr() const {

	if ( mBuf ) {
		mBuf[ mStrLen + 1 ] = '\0';
		return &mBuf[1]; }
	else
		return "\0";
 
}







void UtilStr::Append( const  char* inCStr ) {
	register unsigned long i = 0;

	if ( inCStr ) {
		while( inCStr[ i ] != '\0' ) 
			i++;
		Append( inCStr, i );
	}
}



void UtilStr::Append( const unsigned char* inStrPtr ) {
	
	if ( inStrPtr ) 
		Append( (char*) &inStrPtr[1], inStrPtr[0] );
	
}
		
		

void UtilStr::Append( const void* inSrce, long numBytes ) {
	unsigned long newLen = numBytes + mStrLen;
	char* oldBuf;
		
	if ( numBytes > 0 ) {
		if ( newLen >= mBufSize ) {
			if ( newLen < 80 )
				mBufSize = newLen + 5;
			else if ( newLen < 500 )
				mBufSize = newLen + 100;
			else mBufSize = newLen + 3000;
				
			oldBuf = mBuf;
			mBuf = new char[ mBufSize + 2 ];	// One for pascal byte, one for c NUL byte
			if ( oldBuf ) {
				if ( mStrLen > 0 )
					Move( &mBuf[1], &oldBuf[1], mStrLen );
			
				delete[] oldBuf;
			}
		}
				
		if ( inSrce && numBytes > 0 )
			Move( &mBuf[mStrLen + 1], inSrce, numBytes );
				
		mStrLen = newLen;
	}
}





void UtilStr::Append( long inNum ) {
	UtilStr temp;
	unsigned long i;

	if ( inNum < 0 ) {
		Append( '-' );
		inNum = - inNum;
	}
		
	if ( inNum == 0 )
		Append( '0' );
		
	while ( inNum > 0 ) {
		temp.Append( (char) ('0' + inNum % 10) );
		inNum = inNum / 10;
	}
	
	for ( i = temp.length(); i > 0; i-- ) 
		Append( temp.getChar( i ) );
}







void UtilStr::Append( const UtilStr* inStr ) { 

	if ( inStr )
		Append( inStr -> getCStr(), inStr -> length() );
}






void UtilStr::Assign( const UtilStr& inStr ) {

	if ( this != &inStr ) {
		Wipe();
		Append( inStr.getCStr(), inStr.length() );
	}
}



void UtilStr::Assign( const void* inPtr, long bytes ) {
	Wipe();
	Append( inPtr, bytes );
}



void UtilStr::Assign( char inChar ) {
	Wipe();
	Append( &inChar, 1 );
}


		
void UtilStr::Assign( long inNum ) {
	
	Wipe();
	Append( inNum );
}



		




void UtilStr::Assign( const unsigned char* inStrPtr ) {
	Wipe();
	Append( inStrPtr );
}





void UtilStr::Assign( const UtilStr* inStr ) {
	
	
	if ( inStr != this ) {
		Wipe();
		if ( inStr )
			Append( inStr -> getCStr(), inStr -> length() );
	}
}



void UtilStr::Assign( CEgIStream& inStream, long numBytes ) {

	if ( numBytes > 5000000 )						// *** Safe to say that sizes over this are corrupt?
		inStream.throwErr( cCorrupted );
	else if ( numBytes > 0 ) {
		Dim( numBytes );
		if ( length() < numBytes )
			numBytes = length();
		inStream.GetBlock( getCStr(), numBytes );
	}
}

	




void UtilStr::ReadFrom( CEgIStream* inStream ) {
	unsigned long len = inStream -> GetLong();		// Grab the length

	Assign( *inStream, len );
}




void UtilStr::WriteTo( CEgOStream* inStream ) const {

	inStream -> PutLong( length() );
	inStream -> PutBlock( getCStr(), length() );
}






void UtilStr::Prepend( char inChar ) {
	
	Insert( 0, &inChar, 1 );
}


void UtilStr::Prepend( UtilStr& inStr ) {

	Insert( 0, inStr.getCStr(), inStr.length() );
}



void UtilStr::Prepend( const char* inStr ) {
	long len = 0;
	
	while ( *(char*)(inStr + len) != 0 )
		len++;
	
	Insert( 0, inStr, len );
}

	
void UtilStr::Insert( unsigned long inPos, char inChar, long inNumTimes ) {
	unsigned long	oldLen = length();
	unsigned long	numAddable;
	
	if ( inPos > oldLen )
		inPos = oldLen;
	
	Insert( inPos, (char*) NULL, inNumTimes );
	numAddable = length() - oldLen;
	if ( numAddable > 0 && mBuf ) {
		while ( inNumTimes > 0 ) {
			mBuf[ inPos + inNumTimes ] = inChar;
			inNumTimes--;
		}
	}
}


void UtilStr::Insert( unsigned long inPos, long inNum ) {
	UtilStr numStr( inNum );
	
	Insert( inPos, numStr );
	
}

void UtilStr::Insert( unsigned long inPos, const UtilStr& inStr ) {
	Insert( inPos, inStr.getCStr(), inStr.length() );
}

 	
void UtilStr::Insert( unsigned long inPos, const char* inSrce, long inBytes ) {
	unsigned long numToMove, len = length();
	
	if ( inPos >= len )
		Append( inSrce, inBytes );
	else if ( inBytes > 0 ) {
		Append( NULL, inBytes );
		numToMove = len - inPos;
		if ( numToMove > 0 )
			Move( &mBuf[ inPos + inBytes + 1 ], &mBuf[ inPos + 1 ], numToMove );
		if ( inBytes > 0 && inSrce )
			Move( &mBuf[ inPos + 1 ], inSrce, inBytes );
	}
}







void UtilStr::Move( void* inDest, const void* inSrce, unsigned long inNumBytes ) {

	if ( inNumBytes <= 64 ) {
		register unsigned char* dest = (unsigned char*) inDest;
		register unsigned char* srce = (unsigned char*) inSrce;
		if ( srce > dest )
			while ( inNumBytes > 0 ) {
				*dest = *srce;
				dest++;
				srce++;
				inNumBytes--;
			}
		else {
			dest += inNumBytes;
			srce += inNumBytes;
			while ( inNumBytes > 0 ) {
				dest--;
				srce--;
				*dest = *srce;
				inNumBytes--;
			} } }
	else
		#ifdef EG_WIN
		::MoveMemory( inDest, inSrce, inNumBytes );
		#elif EG_MAC
		::BlockMove( inSrce, inDest, inNumBytes );
		#else
		You must have EG_MAC or EG_WIN and EG_WIN32 defined as 1 to compile Common!
		#endif
}


		
void UtilStr::Decapitalize() {
	unsigned long i, len = length();
	unsigned char c, sp;
	
	for( i = 2; i <= len; i++ ) {
		c = 	getChar( i );
		sp = 	getChar( i-1 );
		if ( ( sp >= 'A' && sp <= 'Z' ) || ( sp >= 'a' && sp <= 'z' ) )  {
			if ( getChar( i-1 ) == 'I' && c == 'I' ) { }	// Prevent III from decapitalizing
			else if ( c >= 'A' && c <= 'Z' )
				setChar( i, c + 32 );
		}
	}

}





void UtilStr::Capitalize() {
	unsigned long i, len = length();
	char c;
	
	for( i = 1; i <= len; i++ ) {
		c = getChar( i );
		if ( c >= 'a' && c <= 'z' )  {
			setChar( i, c - 32 );
		}
	}

}




void UtilStr::Trunc( unsigned long numToChop, bool fromRight ) {
	
	if ( fromRight )
		Remove( length() - numToChop + 1, numToChop );
	else
		Remove( 1, numToChop );
}




void UtilStr::Remove( unsigned long inPos, unsigned long inNum ) {
	unsigned long	toMove;
	unsigned long	len		= length();

	if ( inPos < 1 )
		inPos = 1;
	
	if ( inNum > len - inPos + 1 )
		inNum = len - inPos + 1;
	
	if ( inPos <= len && inNum > 0 ) {

		mStrLen = len - inNum;
		toMove = len - inPos - inNum + 1;

		if ( toMove > 0 )
			Move( &mBuf[ inPos ], &mBuf[ inPos + inNum ], toMove );
	}	
}




void UtilStr::Remove( char* inStr, int inLen, bool inCaseSensitive ) {
	long pos;
	char* s;
	
	if ( inLen < 0 ) {
		inLen = 0;
		s = inStr;
		while ( *s ) {
			inLen++;
			s++;
		}
	}
	
	pos = contains( inStr, inLen, 0, inCaseSensitive );
	while ( pos > 0 ) {
		Remove( pos, inLen );
		pos--;
		pos = contains( inStr, inLen, pos, inCaseSensitive );
	} 
}





void UtilStr::Keep( unsigned long inNumToKeep ) {

	if ( inNumToKeep < length() )
		Trunc( length() - inNumToKeep, true );
}




void UtilStr::PoliteKeep( unsigned long inMaxLen, unsigned long inPos ) {
	if ( length() > inMaxLen ) {
		Remove( inPos + 1, length() - inMaxLen + 1 );
		setChar( inPos, 'É' );
	}
}



void UtilStr::ZapLeadingSpaces() {
	unsigned long 	i 		= 1;
	unsigned long	len		= length();

	while ( getChar( i ) == ' ' && i <= len )
		i++;
		
	if ( i > 1 )
		Trunc( i - 1, false );
}





/*


UtilStr UtilStr::MID( unsigned long start, unsigned long inLen ) const {
	UtilStr		newStr;
	unsigned long	len = length();
	
	if ( start < 1 )
		start = 1;
	
	start--;
		
	if ( inLen > len - start ) 
		inLen = len - start;	
		
	if ( start <= len && inLen > 0 ) 
		newStr.Assign( getCStr() + start, inLen );
		
	return newStr;
}




UtilStr UtilStr::RIGHT( unsigned long inLen ) const {
	UtilStr		newStr;
	unsigned long 	start, len = length();
	
	if ( inLen > len )
		inLen = len;
		
	start = len - inLen;

	newStr.Assign( getCStr() + start, inLen );
	return newStr;
}
*/







long UtilStr::FindNextInstanceOf( long inPos, char c ) const {
	long len = length(), i;
	
	if ( inPos < 0 )
		inPos = 0;
		
	for ( i = inPos+1; i <= len; i++ ) {
		if ( mBuf[ i ] == c )
			return i;
	}
	
	return 0;
}


long UtilStr::FindPrevInstanceOf( long inPos, char c ) const {
	long i;
	
	if ( inPos > length() )
		inPos = length();
	
	for ( i = inPos; i > 0; i-- ) {
		if ( mBuf[ i ] == c )
			return i;
	}
	
	return 0;
}


long UtilStr::Replace( char inTarget, char inReplacement ) {
	unsigned long count, i, len = length();
	
	count = 0;
	
	for ( i = 1; i <= len; i++ ) {
		if ( mBuf[ i ] == inTarget ) { 
			mBuf[ i ] = inReplacement;
			count++;
		}	
	}
	
	return count;
}


long UtilStr::Replace( char* inTarget, char* inReplacement, bool inCaseSensitive ) {
	char* srce;
	long count = 0, pos, prevPos = 0;
	
	// Calc the len of the target
	long targLen = 0;
	while ( inTarget[ targLen ] )
		targLen++;
		
	// See if there's at least one instance of the target in this string...		
	pos = contains( inTarget, targLen, 0, inCaseSensitive );
	if ( pos ) {
	
		// Make this the dest str
		UtilStr srceStr( this );
		srce = srceStr.getCStr();
		count = 0;
		Keep( pos - 1 );
		goto _resume;
				
		while ( pos ) {
			Append( srce + prevPos, pos - prevPos - 1 );
_resume:	Append( inReplacement );
			count++;
			prevPos = pos + targLen - 1;
			pos = srceStr.contains( inTarget, targLen, prevPos, inCaseSensitive );
		}
		
		Append( srce + prevPos, srceStr.length() - prevPos );
	}
	
	return count;
}

void UtilStr::copyTo( unsigned char* pasDestPtr, unsigned char inBytesToCopy ) const {
	unsigned long 	bytes = length() + 1;

	if ( bytes > inBytesToCopy ) 
		bytes = inBytesToCopy;
		
	getPasStr();			// refreshes len byte for pas str 
	
	if ( bytes > 255 )
		bytes = 255;

    Move( pasDestPtr, &mBuf[0], bytes );
}




void UtilStr::copyTo( char* inDestPtr, unsigned long inBytesToCopy ) const {
	unsigned long bytes = length() + 1;

	if ( bytes > inBytesToCopy ) 
		bytes = inBytesToCopy;
		
	getCStr();			// refreshes NUL byte for c strs


	Move( inDestPtr, &mBuf[1], bytes );
}







char UtilStr::getChar( unsigned long i ) const {

	if ( i <= mStrLen && i > 0 )
		return mBuf[ i ];
	else
		return '\0'; 
}



void UtilStr::setChar( unsigned long i, char inChar ) {


	if ( i <= mStrLen && i > 0 ) 
		mBuf[ i ] = inChar; 
}





int UtilStr::StrCmp( const char* s1, const char* s2, long inN, bool inCaseSensitive ) {
	char c1, c2;
	
	if ( inN < 0 ) {
		inN = 0;
		const char* s = (*s1 != 0) ? s1 : s2;
		while ( *s ) {
			s++;
			inN++;
		}
	}
		
	while ( inN > 0 ) {
		inN--;
		c1 = *s1;
		s1++;
		c2 = *s2;
		s2++;
		if ( ! inCaseSensitive ) {
			if ( c1 >= 'a' && c1 <= 'z' )
				c1 -= 32;
			if ( c2 >= 'a' && c2 <= 'z' )
				c2 -= 32;	
		}
		if ( c1 != c2 )
			return c1 - c2;
	}
	
	return 0;	
}




int UtilStr::compareTo( const unsigned char* inPStr, bool inCaseSensitive ) const {
	
	if ( inPStr ) {
		if ( length() == inPStr[0] ) {
			return StrCmp( getCStr(), (char*) (inPStr + 1), length(), inCaseSensitive );
		}
	}

	return -1;
}


int UtilStr::compareTo( const UtilStr* inStr, bool inCaseSensitive ) const {

	if ( inStr )
		return StrCmp( inStr -> getCStr(), getCStr(), length() + 1, inCaseSensitive );
	else
		return -1;
}


int UtilStr::compareTo( const char* inStr, bool inCaseSensitive ) const {
	if ( inStr ) 
		return StrCmp( inStr, getCStr(), length() + 1, inCaseSensitive );
	else
		return -1;
}



long UtilStr::contains( const char* inSrchStr, int inLen, int inStartingPos, bool inCaseSensitive ) const {
	char	srchChar, srchCharLC, c;
	char*	endPtr, *curPtr = getCStr();

	if ( inLen < 0 ) {
		inLen = 0;
		while ( *(inSrchStr+inLen) )
			inLen++;
	}
	
	endPtr = curPtr + length() - inLen;
	
	srchChar = *inSrchStr;
	if ( srchChar >= 'a' && srchChar <= 'z' )
		srchChar -= 32;
	srchCharLC	= srchChar + 32;
	if ( inStartingPos > 0 )
		curPtr += inStartingPos;
	
	while ( curPtr <= endPtr ) {
		c = *curPtr;
		if ( c == srchChar || c == srchCharLC ) {
			if ( StrCmp( curPtr, inSrchStr, inLen, inCaseSensitive ) == 0 )
				return curPtr - getCStr() + 1;
		}
		curPtr++;
	}
	
	return 0;
}



/* 
*** The following is some string silmilarity/matching theory...  man, i miss cornell cs...

Dynamic Programming Algorithm for Sequence Alignment.
Dynamic Programming Algorithms are used for finding shortest paths in graphs, and in many other optimization problems, but in the comparison or alignment of strings (as in Biological DNA, RNA and protein sequence analysis, speech recognition and shape comparison) the following, or similar, is often called "the" dynamic programming algorithm (DPA). 

Generic Dynamic Programming Algorithm for Comparing Two Strings:
Given two strings or sequences A[1..|A|] and B[1..|B|] 

M[0, 0] = z                                    -- usually z=0
M[i, 0] = f( M[i-1, 0  ], c(A[i], "_" ) )      -- Boundary conditions - delete A[i]
M[0, j] = f( M[0,   j-1], c("_",  B[j]) )      -- Boundary conditions - delete B[j]

M[i, j] = g( f( M[i-1, j-1], c(A[i], B[j]) ),  -- match/mismatch
             f( M[i-1, j  ], c(A[i], "_" ) ),  -- delete A[i]
             f( M[i,   j-1], c("_",  B[j]) ) ) -- insert B[j]

Note that "_" represents the null (pseudo-)character. 

M[i,j] represents the cost (or score) of the partial sequences A[1..i] and B[1..j], and the algorithm requires that M[i,j] can be calculated from the three neighbours of M[i,j] - to the north (M[i,j-1]), west (M[i-1,j]), and north west (M[i-1,j-1]). 

c( x, y ):  Cost of replacing character x with character y
f( ):       String reconcatination function
g( ):       Substring score chooser function

O( AB ) running time (Assming const running const running time for g, f and c)
O( A ) or O( B ) storage

So what are z, f(), g(), and c()?  By varing them, the returned score comparison will follow different match criteria/behavior.  Here's 5 sets of assignments:

1)  === Longest Common Subsequence (LCS or LCSS)
z = 0 
g( ) = min( ) 
f( ) = + 
c(x,x) = 0, c(x,y) = c(x,"_") = c("_",x) = 1 
A big LCS score means the sequences are similar - lots of matches c(x,x). 
Note that an optimal LCS sequence alignment can be recovered either by retracing the `max' choices that were made from M[|A|,|B|] to M[0,0], or by using Hirschberg's (1975) divide and conquer technique. 

2)  === Levenshtein Metric or Sellers' Edit Distance
A big edit distance value means the sequences are dissimilar - lots of changes c(x,y), and indels c(x,"_") and c("_",x). 

z = 0 
g( ) = min( ) 
f( ) = + 
c(x,x) = 0, c(x,y), c(x,"_"), c("_",x) > 0 
The above assumes "simple" gap costs; linear but some other more complex gap costs can be incorporated with modifications. 

3)  === Probability of Alignments
For given probabilities, P(match), P(mismatch), P(insert) and P(delete), the following dynamic programming algorithm finds a most probable alignment of two given sequences: 

z = 1       -- NB. 
g( ) = max( ) 
f( ) = * 
c(x,x) = P(match) * P(x)
c(x,y) = P(mismatch) * P(x,y | x!=y)
c(x,"_") = P(delete) * P(x)
c("_",x) = P(insert) * P(x) 
Unfortunately the quantities calculated become very small for strings of realistic lengths and underflow is the consequence. It is more convenient to deal with the -log's of probabilities and this also corresponds to a coding or information theory interpretation - see below. 

4)   === Minimum Message Length MML Optimal Alignment
aka Minimum Description Length MDL

z = 0 
g( ) = min( ) 
f( ) = + 
c(x,x) = -log2(P(match)) - log2(P(x))
c(x,y) = -log2(P(mismatch)) - log2(P(x,y | x!=y))
c(x,"_") = -log2(P(delete)) - log2(P(x))
c("_",x) = -log2(P(insert) -log2(P(x)) 
- assuming "simple" gap costs; modify if not. Base-two logs are taken if you wish to interpret the quantities as bits; some prefer natural logarithms, giving nits. 
An alignment can be thought of as a hypothesis of how strings A and B are related, and it can be compared with the null-theory that they are not related (Allison et al 1990(a), 1990(b), 1992). Thus, if an alignment gives some real data compression for the pair of strings, it is an acceptable hypothesis. 

5)   === Minimum Message Length R-Theory, Sum Over All Alignments
The r-theory (r for related) is the hypothesis that two strings are related in some unspecified way. Its (-log2) probability is calculated as above except that 

g(,) = logplus(,)
where logplus(-log2(P1), -log2(P2)) = -log2(P1+P2) 
One alignment is just one hypothesis of how one string changed into the other. There may be many optimal alignments and many more suboptimal alignments. Two alignments are two different or exclusive hypotheses so their probabilities can be added. 

Thus the sum over all alignments gives the -log2 probability of the two strings being created in a related but unspecified way. The complement of this r-theory is that the strings are not related. Comparing the message lengths of the r-theory and the null-theory gives the posterior -log odds-ratio that the two strings are related (for a given model of string relation of course): 

r-theory:
  P(A & B & related) = P(A & B).P(related | A & B)
                     = P(related).P(A & B | related)
  where P(A & B | related) = Sum[all L] P(A & B | alignment L)

null-theory:
  P(A & B & not related) = P(not related).P(A & B | not related)
                         = P(not related).P(A).P(B)
                         = P(A & B).P(not related | A & B)


We can put a 50:50 prior on being related or unrelated. We do not know the prior probability of the data, P(A&B), but we can cancel it out to get the posterior -log-odds ratio of the r-theory and the null-theory. 

It is fortunate that the sum of P(A&B|L) over all alignments L can be calculated in O(|A|*|B|) time for "finite-state" models of relation (based on finite-state machines) using the dynamic programming algorithm above and variations upon it (Allison et al 1990(a), 1990(b), 1992). 

References.
L. Allison, C. S. Wallace and C. N. Yee. When is a String like a String? AI & Maths 1990(a) 
L. Allison, C. S. Wallace and C. N. Yee. Inductive inference over macro-molecules. TR 90/148 Department of Computer Science, Monash University, November 1990(b) 
L. Allison, C. S. Wallace and C. N. Yee. Finite-State Models in the Alignment of Macromolecules. Jrnl. Molec. Evol. 35 77-89 1992 
D. S. Hirschberg. A Linear Space Algorithm for Computing Maximal Common Subsequences. Comm. Assoc. Comp. Mach. 18(6) 341-343 1975 
V. I. Levenshtein. Binary Codes Capable of Correcting Deletions, Insertions and Reversals. Soviet Physics Doklady 10(8) 707-710 1966 and Doklady Akademii Nauk SSSR 163(4) 845-848 1965 
P. Sellers. On the Theory and Computation of Evolutionary Distances. SIAM J. Appl. Math 26(4) 787-793 1974 
Lloyd Allison, Department of Computer Science, Monash University, Australia 3168 
*/			

// An implementaion of the Longest Common Subsequece function set for the general Sequence Alignment alg described above
#define __MIN( a, b ) ( ( (a) < (b) ) ? (a) : (b) )
#define STACK_TEMP_SIZE  30
#define COST_CONTIGUOUS			1
#define COST_OF_DEL				1 /* Cost of having to delete chars from this to match inStr when preceding char matches */
#define COST_OF_INS				16 /* Cost of having to insert chars to this to match inStr.  higher cost than DEL to penalize chars contained in inStr but not in this */
#define COST_OF_CHANGE 			17 /* Cose of changing a char in this to match inStr */
#define COST_OF_CASE			1 /* Cose of having to change the case of a char in this to match a char in inStr */

long UtilStr::LCSMatchScore( const char* inStr, long ALen ) const {
	const char *A, *B;
	long*	M;
	long	temp[ STACK_TEMP_SIZE + 1 ];
	long	BLen, a, b, M_bm1, ins, del, mat_mis, c_a, c_b, c_bUC, prev_b_UC, cost;
	
	// Calc the length if it wasn't given to us
	if ( ALen < 0 ) {
		ALen = 0;
		while ( *(inStr+ALen) )
			ALen++;
	}

	/* Subtract 1 from the str address to account for the fact that all accesses
	are 1 based indexing */
	A = inStr - 1;
	B = getCStr() - 1;
	BLen = length();

	
	/* let A and B be the two strings to be compared.
	We need to make a 2D table, M[][], that has corner M[ |A| ][ |B| ].  If we evaluate M, row by row, we never
	need to access a row more further than one row.  This permits us to evaluate [ |A| ][ |B| ] with only having
	to allocate a single row's worth of elements (vs. allocating all |A| rows).  */
	
	// Don't use stack-allocated temp mem if our string is too big...
	// Top most significant byte in M[][] contains info!
	if ( ALen < STACK_TEMP_SIZE )
		M = temp;
	else
		M = new long[ ALen + 1 ];  // +1 for the nul str column/representation

	// Initialize the topmost row of M[][] ...
	M[ 0 ] = 0;
	for ( a = 1; a <= ALen; a++ )
		M[ a ] = M[ a - 1 ] + COST_OF_INS;
		
	// Evaluate from row 2 to row BLen
	c_bUC = 0;
	for ( b = 1; b <= BLen; b++ ) {
		prev_b_UC = c_bUC;
		c_bUC = c_b = B[ b ];
		if ( c_bUC >= 'a' && c_bUC <= 'z' )
			c_bUC -= 32;

		// Evaluate M[ 0, b ] via boundry conditions
		M_bm1 = M[ 0 ];
		M[ 0 ] += COST_OF_DEL;
		
		// Evaluate from column 2 to column ALen
		for ( a = 1; a <= ALen; a++ ) {
			
			// === Calc NW term...
			// Calc the cost of changing B[b] to A[a]...
			c_a = A[ a ];
			cost = 0;
			if ( c_a != c_b ) {
				if ( c_a >= 'a' && c_a <= 'z' )
					c_a -= 32;
					
				if ( c_a != c_bUC )
					cost = COST_OF_CHANGE;
				else
					cost = COST_OF_CASE;  
			}
			// NW Term:  M_bm1 is M[ a - 1, b - 1 ] (replace A[a] with B[b])	
			mat_mis = M_bm1 + cost;
			
			// === Calc N term...
			// Calc the cost of changing B[b] to nul.  Favor contiguity.			
			// Favor contiguity (case insensitive--any case match is 'contiguous')
			cost = COST_OF_DEL;
			if ( c_a == prev_b_UC )		
				cost += COST_CONTIGUOUS;
				
			// N Term: M[ a ] is M[ a, b - 1 ] ( delete B[b] )
			del = M[ a ] + cost;
			
			// === Calc W term...
			// W Term: M[ a - 1 ] is M[ a - 1, b ]  ( insert A[a] )
			ins = M[ a - 1 ] + COST_OF_INS;

			// Maintain M_bm1 for next iteration of a
			M_bm1 =	M[ a ];					

			// Choose the lost cost direction:  M[ a, b ] = max( mat_mis, delAa, insBb )
			M[ a ] = __MIN( del, ins );
			M[ a ] = __MIN( M[ a ], mat_mis );
		}
	}
		
	if ( ALen >= STACK_TEMP_SIZE )
		delete []M;

	return 100000 - M[ ALen ]; 
}


/*
bool UtilStr::equalTo( const char* inStr, int inFlags ) const {
	bool		stillOk 			= inStr != NULL;
	char*		thisStr 			= getCStr();
	char		c1 = 1, c2			= 2;
	bool		caseInsensitive 	= inFlags & cCaseInsensitive != 0;
		
	while ( stillOk && c2 != 0 ) {
		c1 = *inStr;
		c2 = *thisStr;
		if ( caseInsensitive ) {
			if ( c1 >= 'a' && c1 <= 'z' )
				c1 -= 32;
			if ( c2 >= 'a' && c2 <= 'z' )
				c2 -= 32;	
		}
		stillOk = c1 == c2;
		thisStr	+= 1;
		inStr	+= 1;
	}
	
	if ( ( inFlags & cLefthand ) && c2 == 0 )
		stillOk = true;
	
	return stillOk;
}
*/











long UtilStr::GetIntValue( char* inStr, long inLen, long* outPlacePtr ) {
	bool seenNum = false;
	char c;
	long i, ret = 0, place = 1;
	
	for ( i = inLen - 1; i >= 0; i-- ) {
		c = inStr[ i ];
		if ( c >= '0' && c <= '9' ) {
			seenNum = true;
			ret += place * ( c - '0' );
			place *= 10; }
		else if ( seenNum )
			i = 0;		// Stop loop
	}
	
	if ( outPlacePtr )
		*outPlacePtr = place;
		
	return ret;
}





double UtilStr::GetFloatVal( char* inStr, long inLen ) {
	unsigned long i, decLoc = 0, foundLet = false;
	char c;
	double n = 0, place = 1.0;
	bool isNeg = false;
	

	for ( i = 0; i < inLen; i++ ) {
		c = inStr[ i ];
		
		if ( c == '-' && ! foundLet ) 
			isNeg = true;
			
		if ( c >= '0' && c <= '9' ) {
			n = 10.0 * n + ( c - '0' );
			if ( decLoc )
				place *= 10.0;
		}
		
		if ( c != ' ' )
			foundLet = true;
			
		if ( c == '.' )
			decLoc = i+1;
	}
	
	if ( isNeg )
		n = - n;

	return n / place;
}




double UtilStr::GetFloatValue() const {
	return GetFloatVal( mBuf + 1, mStrLen );
}




long UtilStr::GetValue( long inMultiplier ) const {
	unsigned long i, len = length(), decLoc = 0, foundLet = false;
	char c;
	long left, right, place;
	
	for ( i = 1; i <= len; i++ ) {
		c = mBuf[ i ];
		
		if ( c == '-' && ! foundLet ) 
			inMultiplier *= -1;
		
		if ( c != ' ' )
			foundLet = true;
			
		if ( c == '.' )
			decLoc = i;
	}
	
	if ( ! decLoc )
		decLoc = len + 1;
		
	left 	= GetIntValue( mBuf + 1, decLoc - 1 );
	right 	= GetIntValue( mBuf + decLoc + 1, len - decLoc, &place );

	return ( inMultiplier * right + place / 2 ) / place + inMultiplier * left;
}




void UtilStr::SetValue( long inVal, long inDivisor, int inNumDecPlaces ) {	
	long			i, part 	= inVal % inDivisor;
	UtilStr		partStr;
	
	for ( i = 0; i < inNumDecPlaces; i++ )
		part *= 10;
		
	part /= inDivisor;
	
	i = inVal / inDivisor;
	if ( i != 0 || part <= 0 )
		Assign( i );
	else
		Wipe();
	
	if ( part > 0 ) {
		Append( '.' );
		partStr.Append( part );
		
		for ( i = inNumDecPlaces - partStr.length(); i > 0; i-- )
			Append( '0' );
			
		Append( &partStr );
		while ( getChar( length() ) == '0' )
			Trunc( 1 ); 
	}
		
}
	


void UtilStr::SetFloatValue( float inValue, int inPercision ) {
	int deci_digits, left_digits = log10( fabs( inValue ) ) + 1.00001;
	float scale;

	if ( left_digits < 9 ) {
		deci_digits = 10 - left_digits;
		if ( deci_digits > inPercision )
			deci_digits = inPercision;
		scale =  pow( 10, deci_digits );
		SetValue( inValue * scale, scale, deci_digits ); }
	else
		Assign( "Overflow" );
}














void UtilStr::AppendAsMeta( const UtilStr* inStr ) {

	if ( inStr )
		AppendAsMeta( inStr -> getCStr(), inStr -> length() );
}




void UtilStr::AppendAsMeta( const void* inPtr, long inLen ) {
	const unsigned char* ptr = (unsigned char*) inPtr;
	unsigned char c;
	
	Append( '"' );
	
	if ( ptr ) {
		while ( inLen > 0 ) {
			c = *ptr;
			
			if ( c == '"' ) 						// Our flag was detected...
				Append( (char) '"' );				// Two flags in a row signal an actual "
			
			if ( c < 32 || c > 127 ) {
				Append( (char) '"' );
				Append( (long) c );
				Append( (char) '"' ); }
			else 
				Append( &c, 1 );
			inLen--;
			ptr++;
		}
	}
	Append( '"' );
}




void UtilStr::AppendFromMeta( const void* inPtr, long inLen ) {
	const unsigned char* ptr = (unsigned char*) inPtr;
	unsigned char c;
	static UtilStr ascNum;
	
	if ( ptr ) {
		if ( *ptr != '"' )							// First char should be a "
			return;
		inLen--;
		ptr++;
		
		while ( inLen > 1 ) {						// We stop 1 char early cuz last char should be a "
			c = *ptr;
			
			if ( c == '"' ) {						// If the flag is detected...
				inLen--;
				ptr++;
				c = *ptr;
				if ( inLen > 1 && c != '"' ) {		// Ignore double flags (they signify the flag char)
					ascNum.Wipe();
					while ( c >= '0' && c <= '9' ) {
						ascNum.Append( (char) c );			
						inLen--;
						ptr++;
						c = *ptr;
					}
					c = ascNum.GetValue();						
				} 
			}
			
			Append( (char) c );

			inLen--;
			ptr++;
		}
	}
}







UtilStr UtilStr::operator + ( const UtilStr& inStr ) {
	UtilStr	newStr( this );
	
	newStr.Append( &inStr );
	return newStr;
}



UtilStr UtilStr::operator + ( const char*  inCStr ) {
	UtilStr	newStr( this );
	
	newStr.Append( inCStr );
	return newStr;
}



UtilStr UtilStr::operator + ( long inNum ) {
	UtilStr	newStr( this );
	
	newStr.Append( inNum );
	return newStr;
}




UtilStr& UtilStr::operator = ( const UtilStr& inStr ) {
	Wipe();
	
	Append( inStr.getCStr() );
	return *this;
}







long UtilStr::Hash() const {
	long hash = 0;
	const char* stop	= getCStr();
	const char* curPos	= stop + mStrLen - 1;

	if ( mStrLen < 16 ) {
		// Sample all the characters
 	    while ( curPos >= stop ) {
 			hash = ( hash * 37 ) + *curPos; 
 			curPos--;
 		} }
 	else {
 	    // only sample some characters
 	    int skip = mStrLen / 7;
 	    while ( curPos >= stop ) {
 			hash = ( hash * 39 ) + *curPos;
 			curPos -= skip;
 		}
 	}
 	
 	return hash;
}



bool UtilStr::Equals( const Hashable* inComp ) const {
	
	return compareTo( (UtilStr*) inComp ) == 0;
}





void UtilStr::AppendHex( char inB1, char inB2 ) {
	unsigned char c;
	
	if ( inB1 >= '0' && inB1 <= '9' )
		inB1 -= '0';
	else
		inB1 = 9 + inB1 & 0xF;
	c = inB1 << 4;
	
	if ( inB2 >= '0' && inB2 <= '9' )
		c += (inB2 - '0');
	else
		c += 9 + inB2 & 0xF;
	

	Append( (char) c );	
}




/*



long UtilStr::Hash( const char* inStr, long inStrLen ) {
	long hash = 0;
	const char* curPos = inStr + inStrLen - 1;
	
	if ( inStrLen < 0 )
		inStrLen = inStr's len
		
	if ( inStrLen < 16 ) {
		// Sample all the characters
 	    while ( curPos >= inStr ) {
 			hash = ( hash * 37 ) + *curPos; 
 			curPos--;
 		} }
 	else {
 	    // only sample some characters
 	    int skip = inStrLen / 7;
 	    while ( curPos >= inStr ) {
 			hash = ( hash * 39 ) + *curPos;
 			curPos -= skip;
 		}
 	}
 	
 	return hash;
}(*/




