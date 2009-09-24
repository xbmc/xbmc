#include "ArgList.h"

#include "Arg.h"
#include "UtilStr.h"
#include "..\io\CEgOStream.h"
#include "..\io\CEgIStream.h"






ArgList::ArgList() {
	
	mHeadArg = NULL;
}



ArgList::~ArgList() {

	Clear();
}




void ArgList::Clear() {

	if ( mHeadArg ) {
		delete mHeadArg;
		mHeadArg = NULL;
	}
}



void ArgList::WriteTo( CEgOStream* ioStream ) {
	Arg* arg = mHeadArg;
	
	ioStream -> PutLong( NumArgs() );
	
	while ( arg && ioStream -> noErr() ) {
		ioStream -> PutLong( arg -> GetID() );
		if ( arg -> IsStr() ) {
			ioStream -> PutByte( '$' );
			((UtilStr*) arg -> GetData()) -> WriteTo( ioStream ); }
		else {
			ioStream -> PutByte( '#' );
			ioStream -> PutLong( arg -> GetData() );
		}
		arg = arg -> mNext;
	}	
}


void ArgList::ReadFrom( CEgIStream* ioStream ) {
	long		ID, n;
	UtilStr	str;
	
	for ( n = ioStream -> GetLong(); n > 0 && ioStream -> noErr() ; n-- ) {
		ID = ioStream -> GetLong();
		if ( ioStream -> GetByte() == '#' ) 
			SetArg( ID, ioStream -> GetLong() );
		else {
			str.ReadFrom( ioStream );
			SetArg( ID, str );
		}
	}
}



long ArgList::NumArgs() const {
	long n = 0;
	Arg* arg = mHeadArg;
	
	while ( arg ) {
		arg = arg -> mNext;
		n++;
	}
	
	return n;
}



Arg* ArgList::FetchArg( long inID ) const {
	Arg* arg = mHeadArg;
	
	while ( arg ) {
		if ( arg -> GetID() == inID )
			return arg;
		arg = arg -> mNext;
	}
	
	return NULL;
}






void ArgList::DeleteArg( long inArgID ) {
	Arg* prev = NULL, *arg = mHeadArg;
	
	while ( arg ) {
		if ( arg -> GetID() == inArgID ) {
			if ( prev )
				prev -> mNext = arg -> mNext;
			else
				mHeadArg = arg -> mNext;
			arg -> mNext = NULL;
			delete arg;
			arg = NULL; }
		else {
			prev = arg;
			arg = arg -> mNext;
		}
	}
}



void ArgList::SetArg( long inArgID, long inArg ) {
	Arg* arg = FetchArg( inArgID );
	
	if ( arg )
		arg -> Assign( inArg );
	else
		mHeadArg = new Arg( inArgID, inArg, mHeadArg ); 
}



void ArgList::SetArg( long inArgID, const UtilStr& inArg ) {
	SetArg( inArgID, inArg.getCStr() );
}


void ArgList::SetArg( long inArgID, const char* inArgStr ) {
	Arg* arg = FetchArg( inArgID );
	
	if ( arg )
		arg -> Assign( inArgStr );
	else
		mHeadArg = new Arg( inArgID, inArgStr, mHeadArg ); 
}





bool ArgList::GetArg( long inArgID, UtilStr& outStr ) const {
	Arg* 	arg		= FetchArg( inArgID );
	
	outStr.Wipe();
	
	if ( arg ) {
		 if ( arg -> IsStr() )
		 	outStr.Assign( (UtilStr*) arg -> GetData() );
		 else		// Allows calling for strings if you're not sure the Arg is a long of not
		 	outStr.Assign( (long) arg -> GetData() );
		
		return true;
	}
	
	return false;
}


long ArgList::GetArraySize( long inID ) const {
	long i = 0;
	
	while ( FetchArg( IndexedID2ID( inID, i ) ) ) {
		i++;
	}
	
	return i; 
}


long ArgList::IndexedID2ID( long inBaseID, long inIndex ) {
	long id = inBaseID;
	

	if ( inIndex >= 100 ) {
		id = ( id << 8 ) | ( ( inIndex / 100 ) + '0' );
		inIndex = inIndex % 100;
	}

	if ( inIndex >= 10 ) {
		id = ( id << 8 ) | ( ( inIndex / 10 ) + '0' );
		inIndex = inIndex % 10;
	}
		
	id = ( id << 8 ) | ( inIndex + '0' );

	return id;
}






const UtilStr* ArgList::GetStr( long inArgID ) const {
	Arg* 	arg		= FetchArg( inArgID );
	
	if ( arg ) {
		 if ( arg -> IsStr() )
			return (UtilStr*) arg -> GetData();
	}
	
	return NULL;
}




double ArgList::GetFloat( long inArgID ) const {
	Arg* 	arg		= FetchArg( inArgID );
		
	if ( arg ) {
		if ( arg -> IsStr() )
			return ( (UtilStr*) arg -> GetData() ) -> GetFloatValue();
	}
	
	return 0;
}





	
bool ArgList::GetArg( long inArgID, bool& outArg ) const {
	Arg* arg = FetchArg( inArgID );
	bool		found = false;
	
	outArg = false;
	
	if ( arg ) {
		found = ! arg -> IsStr();
		if ( found )
			outArg = arg -> GetData() != 0;
	}
	
	return found;
}



bool ArgList::GetArg( long inArgID, long& outArg ) const {
	Arg* 		arg 	= FetchArg( inArgID );
	bool		found 	= false;
		
	if ( arg ) {
		found = ! arg -> IsStr();
		if ( found )
			outArg = arg -> GetData();
	}
	
	if ( ! found )
		outArg = 0;
		
	return found;	
}
	
	

long ArgList::GetArg( long inArgID ) const {
	Arg* 		arg 	= FetchArg( inArgID );
		
	if ( arg ) {
		if ( ! arg -> IsStr() )
			return arg -> GetData();
		else
			return ( (UtilStr*) arg -> GetData() ) -> GetValue();
	}
		
	return 0;	
}
	
	
	

void ArgList::SetArgs( const ArgList& inArgs ) {
	Arg* arg = inArgs.mHeadArg;
	long data;
	long id;
	
	while ( arg ) {
		id =  arg -> GetID();
		data = arg -> GetData();
		if ( arg -> IsStr() ) 
			SetArg( id, (UtilStr*) data );
		else 
			SetArg( id, data );
		arg = arg -> mNext;
	}
}


	
void ArgList::SetArgs( const char* curPtr, long inLen ) {
	const char* lastPtr;
	const char* endPtr = curPtr;
	long ID;
	bool terminated, isStr;
	UtilStr	s;
	
	if ( inLen > 0 ) 
		endPtr = curPtr + inLen;
	else {
		endPtr = curPtr;
		while ( *endPtr )
			endPtr++;
	}
	
	do {										// Loop thru each arg in the str
		terminated	= true;
		
		// When we're not inside a string, igrore oddball/whitespace chars (chars <= 32)
		while ( curPtr < endPtr && *curPtr <= ' ' )
			curPtr++;
		lastPtr = curPtr;

			
		while ( curPtr < endPtr && ( *curPtr != cArgSeparator || ! terminated ) ) {
			if ( *curPtr == '"' )
				terminated = ! terminated;		// Toggle string acceptance
			curPtr++;
		}
		
		// Extract the 4 byte ID...
		ID = 0;
		while ( *lastPtr != '=' && *lastPtr != '-' && curPtr > lastPtr ) {
			ID = (ID << 8) | ((unsigned long) *lastPtr);
			lastPtr++;
		}
		lastPtr++;					// Skip the '='
		isStr = *lastPtr == '\"';	// This arg is a string if we see a "
		if ( curPtr > lastPtr ) {
			if ( isStr ) {
				s.Wipe();
				s.AppendFromMeta( lastPtr, curPtr - lastPtr );
				SetArg( ID, s ); }
			else {
				s.Assign( lastPtr, curPtr - lastPtr );
				SetArg( ID, s.GetValue() );
			}
		}
		curPtr++;
	} while ( curPtr < endPtr );
}



void ArgList::SetArgs( CEgIStream* inStream ) {
	UtilStr str, configText;
	long numQuotes, pos, i, end;
	
	
	if ( inStream -> noErr() ) { 
	
		// Read and chuck any comments
		while ( inStream -> noErr() ) {
			inStream -> Readln( str );
			pos = 1;
			numQuotes = 0;
			do {
				i = str.contains( "//", 2, pos - 1 );
				for ( ; pos <= i; pos++ ) {
					if ( str.getChar( pos ) == '\"' )
						numQuotes++;
				}
			} while ( numQuotes % 2 == 1 && i > 0 );
			
			if ( i > 0 )
				str.Keep( i - 1 );
			configText.Append( str );
		} 
		inStream -> throwErr( cNoErr );
		
		// Remove block comments
		do {
			i = configText.contains( "/*" );
			if ( i > 0 ) {
				end = configText.contains( "*/" );
				if ( end > 0 )
					configText.Remove( i, end - i + 2 );
			}
		} while ( i > 0 && end > 0 );
		
		// Parse the args/dict...
		SetArgs( configText );
	}
}



 

void ArgList::ExportTo( UtilStr& ioStr, bool inLineBreaks ) const {
	CEgOStream ostream;
	
	ExportTo( &ostream, inLineBreaks );
	ostream.mOBuf.Swap( ioStr );
}	



void ArgList::ExportTo( CEgOStream* ioStream, bool inLineBreaks ) const {
	Arg* arg = mHeadArg;
	
	while (arg) {
		arg -> ExportTo( ioStream );
		arg = arg -> mNext;
		if ( arg ) {
			ioStream -> PutByte( cArgSeparator );
			if ( inLineBreaks )
				ioStream -> Writeln();
		}
	}
}	

