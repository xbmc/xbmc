
#include "nodeClass.h"

#include "..\io\CEgIStream.h"
#include "..\io\CEgOStream.h"

#include <stdlib.h>


#ifdef EG_DEBUG
nodeClass*	nodeClass::sFirstDebug	= NULL;
long		nodeClass::sCacheHitsA	= 0;
long		nodeClass::sCacheHitsB	= 0;
long		nodeClass::sCacheHitsC	= 0;
long		nodeClass::sCacheTrysA	= 0;
long		nodeClass::sCacheTrysB	= 0;
long		nodeClass::sCacheTrysC	= 0;
#pragma debug mode on
#endif


long			nodeClass::sClassIDs[ 30 ];
CreatorFuncT	nodeClass::sCreatorFunc[ 30 ];
int				nodeClass::sNumRegistered = 0;

	
nodeClass::nodeClass() {

	initSelf();
}




nodeClass::nodeClass( nodeClass* inParentPtr ) {

	initSelf();
	if ( inParentPtr )
		inParentPtr -> addToTail( this );
}








nodeClass::~nodeClass() {
	deleteContents();											// Dispose of contained nodes
	detach();													// Remove the node from the chain

	#ifdef EG_DEBUG	
	nodeClass*	prevPtr = NULL;
	nodeClass*	nodePtr = sFirstDebug;
	
	while ( nodePtr != this ) {
		prevPtr = nodePtr;
		nodePtr = nodePtr -> mNextDebug;
	}
		
	if ( prevPtr )
		prevPtr -> mNextDebug = mNextDebug;
	else
		sFirstDebug = mNextDebug;
	#endif
}


#ifdef EG_DEBUG	
void nodeClass::DebugBreak() {
	int i = 0;
	nodeClass*	nodePtr = sFirstDebug;
	
	while ( nodePtr ) {
		i++;		// Break here!
		nodePtr = nodePtr -> mNextDebug;
	}

	i = i + 0;
}
#endif




nodeClass* nodeClass::CreateNode( long inClassID, nodeClass* inParent ) {
	int 		i;
	
	for ( i = 0; i < sNumRegistered; i++ ) {
		if ( sClassIDs[ i ] == inClassID )
			return sCreatorFunc[ i ]( inParent );
	}
	
	return NULL;
}

			
			
void nodeClass::RegisterNodeClass( long inID, CreatorFuncT inCreatorFunc ) {
	sClassIDs[ sNumRegistered ]		= inID;
	sCreatorFunc[ sNumRegistered ] 	= inCreatorFunc;
	sNumRegistered++;
}



void nodeClass::initSelf() {
	mNext = NULL;
	mPrev = NULL;
	mParent = NULL;

	mTail = NULL;
	mHead = NULL;
	
	mFlags = 0;
	mDeepCount		= -1;
	mShallowCount	= 0;
	
	mType = nodeClassT;

	#ifdef EG_DEBUG
	mNextDebug = sFirstDebug;
	sFirstDebug = this;
	#endif
}




void nodeClass::SetTreeSelected( bool inSelected ) {
	nodeClass*	nodePtr = mHead;
	
	SetSelected( inSelected );
	
	while ( nodePtr ) {
		nodePtr -> SetTreeSelected( inSelected );
		nodePtr = nodePtr -> GetNext();
	}
}







void nodeClass::DeleteSelected() {
	nodeClass*	nodePtr = mHead;
	nodeClass*	delPtr;
	
	while ( nodePtr ) {
		if ( nodePtr -> IsSelected() ) {
			nodePtr -> absorbAfter( nodePtr );
			delPtr = nodePtr;
			nodePtr = nodePtr -> GetNext();
			delete delPtr; }
		else {
			nodePtr -> DeleteSelected();
			nodePtr = nodePtr -> GetNext();
		}
	}
}






bool nodeClass::CheckInsertPt( long& ioNodeNum, long& ioDepth ) {
	nodeClass*	insertPt;
	long		min, max, n = deepCount();
	
	if ( ioNodeNum > n )
		ioNodeNum = n;

	if ( ioDepth < 0 )
		ioDepth = 0;
					
	insertPt = findSubNode( ioNodeNum );
	
	if ( insertPt ) {
		max = insertPt -> CountDepth( this );
			
		if ( ioDepth > max )
			ioDepth = max;
			
		if ( insertPt -> shallowCount() > 0 )
			min = max; 
		else
			min = max - insertPt -> CountOverhang( this ) - 1;
			
		if ( ioDepth < min )
			ioDepth = min;
	/*
		the following moves the cursor around, moving it to valid insert positions like meterowerks'
		project manager... it works great, but it's too advanced for our particular novice computer users. 
			
		for ( n = 0; n <= max - ioDepth - 1; n++ ) {
			ioNodeNum = findSubNode( insertPt ) + insertPt -> deepCount();
			insertPt = insertPt -> GetParent();
		}*/ 
		
		}
	else {
		ioNodeNum = 0;
		ioDepth = 0; 
	}
		
	return true;
}



void nodeClass::MoveSelected( long afterItemNum, long inDepth ) {
	nodeClass*	nextPtr, *nodePtr;
	nodeClass	moveList;
	nodeClass*	insertPt = findSubNode( afterItemNum );
	long 		relDepth = 0;
	
	if ( insertPt ) {
		if ( insertPt -> IsSelected() && insertPt -> PrevInChain( this ) == insertPt -> GetPrev() )
			insertPt = insertPt -> GetPrev();							// Allow special case
			
		if ( insertPt -> IsSelected() ) {								// We can't insert the insert node!
			while ( insertPt ? insertPt -> IsSelected() : false ) {		// Make sure insert pt is not selected 
				insertPt = insertPt -> PrevInChain( this );
			}
		}
		
		if ( insertPt ) {
			relDepth = insertPt -> CountDepth( this ) - inDepth - 1; 	// How many levels we must rise
			
			while ( relDepth > 0 && insertPt ) { 						// Adj the insert node based on <inDepth>
				insertPt = insertPt -> GetParent();
				relDepth--;
			}
		}
	}
	
	if ( insertPt ) {
		nodePtr = insertPt -> GetParent();
		while ( nodePtr && nodePtr != this ) {							// Prevent circular containment by deselecting
			nodePtr -> Unselect();										// parents of the insertion node
			nodePtr = nodePtr -> GetParent();
		} }
	else {																// If no insert pt, we add to this' head.
		insertPt = this;
		relDepth = -1;													// Signal to add to head
	}
	

	nodePtr = mHead;
	while ( nodePtr ) {
		if ( nodePtr -> IsSelected() ) {
			nextPtr = nodePtr -> PrevInChain( this );					// Save where we'll resume
			moveList.addToTail( nodePtr );								// Add the selected item (and its sub tree) to a temp list
			if ( nextPtr )												// If we can resume where we left off...
				nodePtr = nextPtr;
			else 														// If nodePtr was at the head of this 
				nodePtr = mHead;	}									// Start over again
		else
			nodePtr = nodePtr -> NextInChain( this );		
	}
		
	nodePtr = moveList.GetTail();
	while ( nodePtr ) {
		if ( relDepth < 0 )
			insertPt -> addToHead( nodePtr );
		else
			nodePtr -> insertAfter( insertPt );
		VerifyNode( nodePtr );
					
		nodePtr = moveList.GetTail();
	}
}









void nodeClass::VerifyNode( nodeClass* ) {
	

}




void nodeClass::absorbMarked( nodeClass* inSourceList ) {
	nodeClass*	nodePtr = NULL;
	nodeClass*	nextPtr;
	
	if ( inSourceList )
		nodePtr = inSourceList -> GetHead();

	while ( nodePtr ) {
		nextPtr = nodePtr -> GetNext();
		if ( nodePtr -> IsSelected() ) 
			addToTail( nodePtr );
		else
			absorbMarked( nodePtr );

		nodePtr = nextPtr;
	}
	
}



bool nodeClass::HasTheParent( const nodeClass* inMaybeParent ) const {
	nodeClass* parPtr = mParent;
	
	if ( inMaybeParent ) {
		while ( parPtr ) {
			if ( parPtr == inMaybeParent )
				return true;
			else
				parPtr = parPtr -> GetParent();
		}
	}

	return false;
}

		
		
void nodeClass::SetFlag( unsigned int inFlagNum, bool inVal ) {
	unsigned short m;
	
	if ( inFlagNum >= 1 && inFlagNum <= 9 ) {
		m = 0x1 << inFlagNum;
		if ( inVal )
			mFlags |= m;
		else
			mFlags &= ~m;
	}
}
		


bool nodeClass::GetFlag( unsigned int inFlagNum ) const {
	unsigned short m;
	
	if ( inFlagNum >= 1 && inFlagNum <= 9 ) {
		m = 0x1 << inFlagNum;
		return mFlags & m; }
	else
		return false;
}




void nodeClass::UpdateCounts( int inShallowChange ) {
	if ( inShallowChange != 0 )
		mShallowCount += inShallowChange;						// Update shallow count
		
	mDeepCount	= -1;											// Invalidate deep count
		
	if ( mParent )
		mParent -> UpdateCounts( 0 );							// Propigate the dirty info
}


void nodeClass::detach() {
	if ( mParent ) {
		mParent -> UpdateCounts( -1 );
		
		if ( mPrev )											// if a link proceeds...
			mPrev -> mNext = mNext; 							// tell prev link where the new next link is
		else													// if this is the 1st item...
			mParent -> mHead = mNext;							// tell header where new 1st link is
			
		if ( mNext )											// if a link follows...
			mNext -> mPrev = mPrev; 							// tell next link where the new prev link is
		else 													// is this is the last item...
			mParent -> mTail = mPrev;							// tell header where new last link is
	}
	
	mNext = NULL;												// if something still points here,
	mPrev = NULL;												// the data remaining will be NULL/bad;
	mParent = NULL;												// be safe

}





void nodeClass::insertAfter( nodeClass* inBefore ) {

	if ( inBefore && inBefore != this ) {
		if ( inBefore -> GetNext() != this ) {
			detach();													// Detach this ob before we go attaching somewhere else
			mParent = inBefore -> GetParent();							// set this object's parent group ptr
			
			if ( mParent ) {
				mParent -> UpdateCounts( 1 );
				
				if ( inBefore == mParent -> GetTail() )					// if inserting after the last ob in the group...
					 mParent -> mTail = this;							// tell group that this is the new end ob in the group
					
				mPrev = inBefore;										// set this ob's prev ob ptr to the ob we're inserting after

				mNext = inBefore -> GetNext();							// obtain the ob this is to be inserted before
				if ( mNext ) 											// if a next ob exists...
					mNext -> mPrev = this;								// then tell it this is its new prev ob ptr

				mPrev -> mNext = this;									// tell prev ob that this is its next ob
			}
		}
	}
}



void nodeClass::insertAfter( long inAfterNode, nodeClass* inNodeToAdd ) {
	nodeClass* insertPt = findSubNode( inAfterNode );
	
	if ( inNodeToAdd ) {
		if ( insertPt )
			inNodeToAdd -> insertAfter( insertPt );
		else if ( inAfterNode <= 0 )
			addToHead( inNodeToAdd );
		else
			addToTail( inNodeToAdd );
	}
}





long nodeClass::findInstance() const {
	long 			nodeCount		= 0;
	nodeClass*		nodePtr;
	int				foundMatch 		= false;
	
	if ( mParent ) {
		nodePtr = mParent -> GetHead();
		
		while ( nodePtr && ! foundMatch ) {
			nodeCount++;
			if ( this == nodePtr ) 
				foundMatch = true;
			nodePtr = nodePtr -> GetNext();
		}
	}
	
	if ( foundMatch )
		return nodeCount;
	else
		return 0;
}
	





void nodeClass::absorbContents( nodeClass* inSourceList, int inPutAtHead ) {
	nodeClass*	nodePtr;
	
	if ( inSourceList ) {
		do {
			if ( inPutAtHead ) {
				nodePtr = inSourceList -> mTail;
				addToHead( nodePtr ); }
			else {
				nodePtr = inSourceList -> mHead;
				addToTail( nodePtr );
			}
		} while ( nodePtr );
	}

}






void nodeClass::absorbAfter( nodeClass* inSourceList ) {
	nodeClass*	nodePtr;
	nodeClass*	lastPtr = this;
	
	if ( inSourceList && mParent ) {
		do {
			nodePtr = inSourceList -> mHead;
			if ( nodePtr ) {
				nodePtr -> insertAfter( lastPtr );
				lastPtr = nodePtr;
			}
		} while ( nodePtr );
	}
}






void nodeClass::deleteContents() {
	nodeClass* 		nodePtr 		= mHead;
	nodeClass* 		nextNodePtr;
	
	while ( nodePtr ) {
		nextNodePtr = nodePtr -> GetNext();
		delete nodePtr;
		nodePtr = nextNodePtr;
	}
}







	
	
	










void nodeClass::addToHead( nodeClass* nodeToAdd ) {
	if ( nodeToAdd ) {
		nodeToAdd -> detach();								// Detach it before we add it here
		nodeToAdd -> mParent = this;						// let ob know where parent group is
		UpdateCounts( 1 );
		if ( mTail == NULL) {								// if this is the 1st item in the list to be...
			nodeToAdd -> mPrev = NULL;						// there is no prev link
			nodeToAdd -> mNext = NULL;	 					// there is no next link
			mHead = mTail = nodeToAdd;	}					// let link header know where the 1st and last link is
		else {												// if there is already items in the list...
			mHead -> mPrev = nodeToAdd;						// let old first link know where new last link is
			nodeToAdd -> mPrev = NULL;						// tell new last link there is no prev link 
			nodeToAdd -> mNext = mHead; 					// let new first link know where old last link is
			mHead = nodeToAdd; 								// let link header know where new last link is
		}
	}
}






void nodeClass::addToTail( nodeClass* nodeToAdd ) {
	if ( nodeToAdd ) {										// if i have a valid ptr...
		nodeToAdd -> detach();								// Detach it before we add it here
		nodeToAdd -> mParent = this;						// let ob know where parent group is
		UpdateCounts( 1 );
		if ( mHead ) {										// if there is already items in the list...
			mTail -> mNext = nodeToAdd;						// let old last link know where new last link is
			nodeToAdd -> mPrev = mTail;						// let new last link know where old last link is
			nodeToAdd -> mNext = NULL; 						// tell new last link there is no next link
			mTail = nodeToAdd; 	}							// let link header know where new last link is
		else {												// if this is the 1st item in the list to be...
			nodeToAdd -> mPrev = NULL;						// there is no prev link
			nodeToAdd -> mNext = NULL; 						// there is no next link
			mHead = nodeToAdd;								// let link header know where the 1st link is
			mTail = nodeToAdd; 								// let link header know where the last link is
		}
	}															
}













long nodeClass::CountDepth( const nodeClass* inCeiling ) const {
	nodeClass*	nodePtr = mParent;
	int			count = 1;
	

	while ( nodePtr && nodePtr != inCeiling ) {
		nodePtr = nodePtr -> GetParent();
		count++;
	}
	
	if ( ! nodePtr )
		count--;

	return count;
}




	
nodeClass* nodeClass::findNodeNum( long inNum ) {
	nodeClass*		nodePtr 	= mHead;
	int				nodeCount	= 0;
	
	while ( nodePtr ) {
		nodeCount++;
		if ( nodeCount == inNum )
			return nodePtr;
		nodePtr = nodePtr -> GetNext();
	}
	
	return NULL;

}


/*

nodeClass* nodeClass::findPrevNonMarkedSubNode( long inNum ) {
	nodeStep	i( this );
	nodeClass*	start 		= findSubNode( inNum );	
	nodeClass*	nodePtr 	= i.GetNext();
	nodeClass*	prevPtr 	= NULL;	
	
	if ( start ) {
		if ( start -> isMarked() )  {
			while ( nodePtr != start ) {
				if ( ! nodePtr -> isMarked() )
					prevPtr = nodePtr;
						
				nodePtr = i.GetNext();
			}
			start = prevPtr;
		} 	
	}
	
	return start;
}
*/



nodeClass* nodeClass::GetDeepTail() const {
	nodeClass* retPtr = mTail;
	
	if ( retPtr ) {
		while ( retPtr -> mTail )
			retPtr = retPtr -> mTail;
	}
	
	return retPtr;
}


/*
Not Yet Tested:
nodeClass* nodeClass::GetParentDeepTail( const nodeClass* inCeiling ) const {
	nodeClass* retPtr = this;
	int n = CountOverhang( inCeiling );
	
	while ( n > 0 && retPtr )
		retPtr = retPtr -> GetParent();
}
*/

int nodeClass::CountOverhang( const nodeClass* inCeiling ) const {
	const nodeClass*	nodePtr	= this;
	int 				n		= 0;
	
	while ( nodePtr && inCeiling != nodePtr ) {
		if ( nodePtr -> GetNext() )
			return n;
		nodePtr = nodePtr -> GetParent();
		n++;
	}
	
	return n;
}


nodeClass* nodeClass::findSubNode( long inNum ) {
	nodeClass*		nodePtr = mHead;
	long			d, i = 0;
	
	if ( inNum > 0 ) {
		while ( nodePtr ) {											// Loop while there's nodes and we haven't found desired
			i++;
			if ( inNum == i )										// If we found the desired node
				return nodePtr;
			else {
				d = nodePtr -> deepCount();							// See how big this sub tree is.
				if ( inNum - i <= d )								// If stepping over it will miss our desired node
					return nodePtr -> findSubNode( inNum - i );		// Then the node we want is inside this sub tree
				else {								
					i += d;											// We can step over this sub tree, baby.
					nodePtr = nodePtr -> GetNext();	
				}
			}
		}
																	// i contains the deep count if above loop terminated
		mDeepCount = i;												// If deepCount is invalid, we might as well use what we have
	}
	
	return NULL;
}



long nodeClass::findSubNode( nodeClass* inNodePtr ) {
	nodeClass*		nodePtr		= mHead;
	long			d, n = 0;
	bool			done = false;
	
	while ( nodePtr && ! done ) {					// Loop till end of shallow list or until we found desired node
		n++;										// Count the shallow node we're on right now
		if ( nodePtr == inNodePtr )					// Is the current (shallow) node our man?
			done = true;
		else {
			d = nodePtr -> findSubNode( inNodePtr );
			if ( d > 0 ) {							// If desired node was in the shallow node's deep tree
				done = true;
				n += d; }							// Adjust n to be the correct num for <inNodePtr>
			else {
				n += nodePtr -> deepCount();		// Adjust n to reflect the current # of nodes checked
				nodePtr = nodePtr -> GetNext();		// Move to the next shallow node
			}
		}
	}
	
	if ( ! done ) {									// If a match wasn't found...
		if ( mDeepCount < 0 )						// If deep count # was invalid		
			mDeepCount = n;							// We might as well use what we info we have
		n = 0;
	}
		
	return n;
}




long nodeClass::deepCount() {
	nodeClass*	nodePtr;
	
	if ( mDeepCount < 0 ) {								// If the cached counter was/is invalid
		nodePtr = mHead;
		mDeepCount = mShallowCount;						// Prepare to recompute it
		while ( nodePtr ) {
			mDeepCount += nodePtr -> deepCount();
			nodePtr = nodePtr -> GetNext();
		}
	}
		
	return mDeepCount;	
}




nodeClass* nodeClass::NextInChain( const nodeClass* inCeiling ) const {
	nodeClass* nodePtr, *retPtr;
	
	if ( mHead )
		return mHead;
	else if ( this == inCeiling )
		return NULL;
	else if ( mNext )
		return mNext;
	else { 
		nodePtr = mParent;
		retPtr 	= NULL;
		while ( nodePtr && ! retPtr && inCeiling != nodePtr ) {
			retPtr	= nodePtr -> GetNext();
			nodePtr = nodePtr -> GetParent();
		}
		return retPtr;
	}
}





nodeClass* nodeClass::PrevInChain( const nodeClass* inCeiling ) const {
	nodeClass* retPtr;
	
	if ( mPrev ) {
		retPtr = mPrev;
		while ( retPtr -> mTail )
			retPtr = retPtr -> mTail;
		return retPtr; }
	else if ( mParent != inCeiling )
		return mParent;
	else
		return NULL;
}


/*
Is code-opposite of NextInChain(), but does not *do* the opposite:
nodeClass* nodeClass::PrevInChain( const nodeClass* inCeiling ) const {
	nodeClass* nodePtr, *retPtr;
	
	if ( mTail )
		return mTail;
	else if ( mPrev )
		return mPrev;
	else { 
		nodePtr = mParent;
		retPtr 	= NULL;
		while ( nodePtr && ! retPtr && inCeiling != nodePtr ) {
			retPtr	= nodePtr -> GetPrev();
			nodePtr = nodePtr -> GetParent();
		}
		return retPtr;
	}

}
*/










void nodeClass::StartRead( CEgIStream* inStream ) {

	if ( inStream ) {
		if ( inStream -> noErr() ) {
			inStream -> GetByte();				// Throw away type flag for head node to be read
			ReadFrom( inStream );
		}
	}
}




void nodeClass::ReadFrom( CEgIStream* inStream ) {
	int			kind;
	nodeClass*	nodePtr;	

	do {
		kind = inStream -> GetByte();
		
		if ( kind != cEgSubEnd ) {
			nodePtr = CreateNode( kind, this );
			if ( nodePtr )
				nodePtr -> ReadFrom( inStream );
			else
				inStream -> throwErr( cCorrupted );
		}
			
	} while ( inStream -> noErr() && kind != cEgSubEnd );

}




void nodeClass::WriteTo( CEgOStream* inStream ) {
	nodeClass* 	nodePtr = mHead;
		
	inStream -> PutByte( mType );
	
	while ( nodePtr && inStream -> noErr() ) {
		nodePtr -> WriteTo( inStream );
		nodePtr = nodePtr -> GetNext();
	}
	inStream -> PutByte( cEgSubEnd );
}





void nodeClass::RandomizeSubs() {
	long		rnd, i;
	nodeClass*	nodePtr;
	nodeClass	holder;
		
	for ( i = shallowCount(); i > 0; i-- ) {
		rnd = Rnd( 1, i );
		nodePtr = findNodeNum( rnd );
		holder.addToTail( nodePtr );
	}
	
	absorbContents( &holder );
}







long nodeClass::Rnd( long min, long max ) {
	long maxRnd 	= RAND_MAX;
	long retNum 	= rand() * ( max - min + 1 ) / maxRnd + min;
	
	if ( retNum >= max )
		return max;
	else
		return retNum;
}













