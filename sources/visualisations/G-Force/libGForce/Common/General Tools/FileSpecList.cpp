#include "FileSpecList.h"

#include "..\io\CEgFileSpec.h"




FileSpecList::FileSpecList( XStrListOptsT inOption, ListOrderingT inOrdering ) :
	mSpecs( cOrderImportant ),
	mSpecNames( inOption, inOrdering ) {
	
}



FileSpecList::~FileSpecList() {
	RemoveAll();
}



void FileSpecList::RemoveAll() {
	CEgFileSpec* spec;
	
	while ( mSpecs.FetchLast( (void**)&spec ) ) {
		delete spec;
		mSpecs.RemoveLast();
	}
	
	mSpecNames.RemoveAll();
}



long FileSpecList::AddCopy( const CEgFileSpec& inSpec ) {
	UtilStr name;
	long idx;
	
	inSpec.GetFileName( name );
	idx = mSpecNames.Add( name );
	if ( idx > 0 ) 
		mSpecs.Add( new CEgFileSpec( inSpec ), idx - 1 );
		
	return idx;
}


const CEgFileSpec* FileSpecList::FetchSpec( int inIndex ) const {
	return (CEgFileSpec*) mSpecs.Fetch( inIndex );
}


bool FileSpecList::FetchSpecName( int inIndex, UtilStr& outStr ) const {
	return mSpecNames.Fetch( inIndex, outStr );
}

const UtilStr* FileSpecList::FetchSpecName( int inIndex ) const {
	return mSpecNames.Fetch( inIndex );
}


long FileSpecList::FetchBestMatch( const UtilStr& inStr ) {
	return mSpecNames.FetchBestMatch( inStr );
}



long FileSpecList::Lookup( UtilStr& inName ) const {
	return mSpecNames.FindIndexOf( inName );
}