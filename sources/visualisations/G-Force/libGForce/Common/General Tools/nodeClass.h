
#ifndef _NODECLASS_
#define _NODECLASS_

#ifndef NULL
#define NULL 0L
#endif

class nodeClass;
class CEgIStream;
class CEgOStream;

//#define EG_DEBUG

enum {
	nodeClassT				= 1,
	entryClassT				= 2,
	CEgDbMgrT				= 4,
	CEgChapMgrT				= 5,
	CPLDbMgrT				= 6,
	FdeT					= 7,
	FqpT					= 8,
	FAttributeT				= 9,
	FParagraphT				= 10,
	FBitmapT				= 11,
	CDbItemRefNodeT			= 12,
	CStrNodeT				= 13,
	nodeClassReserved		= 14,
	FTabT					= 15,
	FFractionT				= 16,
	FVerticalSubT			= 17,
	CPLItemT				= 90,

	cEgSubEnd				= 0
};


//	Use this macro to register your derived node class with nodeClass
#define	RegisterNode_( ClassName ) TNodeRegistrar<ClassName>::RegisterNode( ClassName##T );

typedef nodeClass*	(*CreatorFuncT)( nodeClass* inParent );

#define isPLItem( nodePtr )				(( nodePtr -> GetType() == 90 ) ? (CPLItem*) nodePtr : NULL)
#define Cast( nodePtr, className )		(( nodePtr -> GetType() == className##T ) ? (className*) nodePtr : NULL)



/*	This is a basic list classn.  Derive from nodeClass and attach you data
*/


class nodeClass {

	friend class XList;
	
	private:
		static long						sClassIDs[ 30 ];
		static CreatorFuncT				sCreatorFunc[ 30 ];
		static int						sNumRegistered;

		void							initSelf();
		
		unsigned short					mFlags;

		nodeClass* 						mNext;
		nodeClass* 						mPrev;	
		nodeClass* 						mParent;		
		
		long							mShallowCount;						
		long							mDeepCount;			//	-1 if Dirty
		
		
		
		virtual void					UpdateCounts( int inShallowChange );
				
	protected:
		nodeClass*						mHead;
		nodeClass*						mTail;
		
		//	Access this enum to typecast this node								
		unsigned char					mType;	
			
		static nodeClass*				CreateNode( long inClassID, nodeClass* inParent );


			
	public:	
		
										nodeClass();
		//	Post:	Constructs a node and places the node at the tail of <inParentPtr>
										nodeClass( nodeClass* inParentPtr );				

		//	Post:	Destroys this node and all sub items inside it
		virtual							~nodeClass();
		
		inline unsigned char			GetType() const									{ return mType;	}
		
		//	Post:	This is how nodeClass "knows" what kind of nodeClass to instantiate when it reads an ID from a stream.
		static void						RegisterNodeClass( long inID, CreatorFuncT inCreatorFunc );


		//	Post:	Returns a random number from <min> to <max>, inclusive.
		static long 					Rnd( long min, long max );


		#ifdef EG_DEBUG
		nodeClass*						mNextDebug;
		static nodeClass*				sFirstDebug;
		static long						sCacheHitsA;
		static long						sCacheHitsB;
		static long						sCacheTrysA;
		static long						sCacheTrysB;
		static long						sCacheHitsC;
		static long						sCacheTrysC;
		static void						DebugBreak();
		#endif

		//	Post:	All sub nodes (at any depth) that are selected are deleted.  If a node is selected, its contents are placed at the same
		//			level of this and this is then deleted.
		virtual void					DeleteSelected();
		
		inline void						Select()										{ mFlags = mFlags | 0x1;		}
		inline void						Unselect()										{ mFlags = mFlags & 0xFFFE;		}
		inline bool						IsSelected() const								{ return mFlags & 0x1; }
		void							SetSelected( bool inIsSelected )				{ if ( inIsSelected ) Select(); else Unselect(); }


		
		//	Post:	Allows client write access to various boolean flags.
		//	Note:	If <inFlagNum> is < 1 or > 9, nothing occurs.			
		void							SetFlag( unsigned int inFlagNum, bool inVal );
		
		//	Post:	Allows client read access to various boolean flags.
		//	Note:	If <inFlagNum> is < 1 or > 9, false is returned.		
		bool							GetFlag( unsigned int inFlagNum ) const;

		//	Post:	This node and all sub nodes (and their sub nodes, etc) are un/selected with <inSelected>.
		void							SetTreeSelected( bool inSelected );
		
		//	Post:	Returns an associated unique or hash for the node.  
		//	Note:	This is how XList knows what order to sort nodes, if that option is enabled.
		virtual long					GetKey()										{ return 0;						}
		
		//	Post:	The node is detached from it's parent.  Obviously, the caller now assumes ownership of the node.
		void 							detach();
		
		//	Post:	This node is inserted in <inPrevNode>'s parent list following <inPrevNode> in the list order
		void							insertAfter( nodeClass* inPrevNode );
		
		//	Post:	If <inAfterNode> <= 0, <inNodeToAdd> is added at this' head.
		//			If 0 < <inAfterNode> <= deepCount(), <inNodeToAdd> is added after the <inAfterNode> item.
		//			If <inAfterNode> > deepCount(), <inNodeToAdd> is added at this' tail.	
		void							insertAfter( long inAfterNode, nodeClass* inNodeToAdd );

		// 	The following are useful for setting up loops, etc
		inline nodeClass* 				GetNext() const									{ return mNext;					}
		inline nodeClass* 				GetPrev() const									{ return mPrev;					}
		inline nodeClass*				GetParent() const								{ return mParent;				}
		inline nodeClass* 				GetTail() const									{ return mTail;					}
		inline nodeClass* 				GetHead() const									{ return mHead;					}

		//	Post:	Returns the node following this heirarchaly.
		//	Post: 	The node returned will have <inCeiling> as a super-parent.
		/*	Sample:			  1				(leftward is the mHead)
							/	\
						  2		6
						/	\
						3	4
							|
							5	
						
			void f( nodeClass* ceiling ) {
				node = ceiling;
				while ( node != NULL ) {
					print( node );
					node = node -> NextInChain( ceiling )
				}
			}
			
			Output of f(1):	2, 3, 4, 5, 6									
			Output of f(2):	3, 4, 5						*/
		nodeClass*						NextInChain( const nodeClass* inCeiling ) const;
		
		//	Post:	The inverse fcn of NextInChain()  (see above)
		nodeClass*						PrevInChain( const nodeClass* inCeiling ) const;
		
		//	Post:	<inPtr> is detached from it's current parent list and is inserted as the last node in this' sub list
		void 							addToTail( nodeClass* inPtr );
		
		//	Post:	<inPtr> is detached from it's current parent list and is inserted at the 1st node in this' sub list
		void 							addToHead( nodeClass* inPtr );
		
		//	Post:	Returns the last deep sub node in the deep sub list.
		//			In other words, this fcn equals: this -> findSubNode( this -> deepCount() )
		nodeClass*						GetDeepTail() const;
		
		//	Post:	Returns the highest parent, say p, of this node where (p -> GetDeepTail() == this) is true. 
		//			In other words, this returns the highest parent that has prev node.
		//	Note:	Returns NULL if no such parent exists.
		nodeClass*						GetParentDeepTail( const nodeClass* inCeiling ) const;
		
		//	Post:	this -> CountDepth( GetParentDeepTail( inCeiling ) );
		//			Or, in easy terms, CountOverhang returns the difference in depth between itself
		//			and the parent node where CountOverhang is zero.
		//			Or, easier yet, 
		/*			1					
					|- 2				2 -> CountOverhang( 1 ) == 0
					|  |-3              3 -> CountOverhang( 1 ) == 0
					|  |-4				4 -> CountOverhang( 1 ) == 1
					|
					|- 5				5 -> CountOverhang( 1 ) == 0
					|  |-6				6 -> CountOverhang( 1 ) == 1
					|    |-7			7 -> CountOverhang( 1 ) == 2
					|- 8				8 -> CountOverhang( 1 ) == 1				*/
		int								CountOverhang( const nodeClass* inCeiling ) const;

		//	Post:	Used by a GUI to refine a given insertion point.
		//	Post:	<ioNodeNum> and <ioDepth> may be adjusted/corrected.
		//	Note:	Depths are such that 0 means root level.
		//	Note:	Fcn succesful iff <true> is returned.
		virtual bool					CheckInsertPt( long& ioNodeNum, long& ioDepth );

		//	Post:	Mirrors a GUI drag of the selected items dropped before cell <inAboveCell> at depth <inDepth>
		//	Note:	If <inDepth> is invalid or <inAboveCell> < 0 or > N, the closest boundry is used.
		//	Calls:	VerifyNode()
		virtual void					MoveSelected( long afterItemNum, long inDepth );
		
		//	Post:	Examines <nodeAdded> to see if it's in a valid location.  If not, it may be moved or deleted.
		virtual void					VerifyNode( nodeClass* nodeAdded );
		
		//	Post:	Delete's all sub nodes in this node
		virtual void 					deleteContents();
		
		//	Post:	Travels upward in its hierarchy until it finds a parent without its own parent
		nodeClass*						getHighestParent();
				
		//	Post:	Returns <true> if <inMaybeParent> is any parent of this.
		//	Note:	If inMaybeParent == NULL, false is returned.
		bool							HasTheParent( const nodeClass* inMaybeParent ) const;
		
		//	Post:	Returns <true> if this node contains other nodes  (optimized for speed)
		inline bool						IsEmpty() const 								{ return mHead == NULL;		}
		
		//	Post:	Returns the number of nodes in this node's sub list (in contrast to deepCount() )
		inline long						shallowCount() const							{ return mShallowCount;		}
		
		//	Post:	Returns the total number of nodes in this node's sub list PLUS the number of items in *their* sub lists.
		long							deepCount();
		
		//	Post:	Returns the number of super lists above this node. Ex, if this node didn't have a parent, countDepth()
		//			would return 0.  If we then put it inside a newly constructed node, countDepth() would return 1.
		long							CountDepth( const nodeClass* inCeiling ) const;
		
		//	Post:	Return what node number this node is in its parent's shallow list.  Ex, if you added this node to the head of
		//			another node, findInstance() would return a 1.  If this node is not in a sub list (ie, it has no parent), 
		//			a 0 is returned.
		long							findInstance() const;
		
		//	Post:	Returns the <inNodeNum>th node in this node's (shallow) sub list. A NULL is returned if <inNodeNum>
		//			is less than or equal to zero or if greater than the number of items in this node's sub list.
		nodeClass*						findNodeNum( long inNodeNum );

		//	Post:	Similar to findNodeNum( long ) except that the node returned is the <inNodeNum>th node in the _entire_
		//			_sub_ _tree_ (as opposed to the base sub level).  If we had 2 sub nodes per node and we had 3 levels of
		//			these, there would be a total of 2^3 leaves and a total of 2^4-1 nodes.  findSubNode( 8 ) would return the
		//			second node at the root level.  The inverse of this function is findSubNode( nodeClass* ) and findNum(). 
		//	Note:	If <inNodeNum> does not exist (ie, it's <= 0 or > deepCount()) then NULL is returned.
		virtual nodeClass*				findSubNode( long inNodeNum );
					
		//	Post:	This is the exact inverse function of findSubNode( long ). This fcn returns the deep instance number of <inNodePtr>
		//			in this node deep sub tree.  If the node was not found in this node's sub tree, 0 is returned.
		//	Note:	If <inNodePtr> is NULL, 0 is returned.
		virtual long					findSubNode( nodeClass* inNodePtr );

		//	Post:	All sub nodes in <sourceList> are appended in order inside this node.  If <inPutAtHead> is true, the nodes
		//			in <sourceList> are put at the head of this, and if it is false, the nodes are put at the tail of this.
		void							absorbContents( nodeClass* sourceList, int inPutAtHead = true );

		//	Post:	All sub nodes in <sourceList> are moved to the tail of this if they are marked
		void							absorbMarked( nodeClass* sourceList );
		
		//	Post:	All sub nodes in <sourceList> are moved after this node (thus placing them in this node's parent's sub list)	
		void							absorbAfter( nodeClass* sourceList );

		//	Post:	Randomizes all sub nodes within this node.		
		void							RandomizeSubs();
		
		virtual void					WriteTo( CEgOStream* inStream );
		virtual void					ReadFrom( CEgIStream* inFile );
		virtual void					StartRead( CEgIStream* inFile );

};



template <class T> class TNodeRegistrar {

	public:
		static T*						CreateFromStream( nodeClass* inParent ) {
                      						return new T( inParent );
                   						}


		static void						RegisterNode( long inClassID ) {
                     						nodeClass::RegisterNodeClass( inClassID, (CreatorFuncT) CreateFromStream );
                   						}
};


#endif

