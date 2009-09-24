#ifndef _XFloatList_
#define _XFloatList_

// by Andrew O'Meara

#include "XPtrList.h"

class XLongList;


class XFloatList {
	
	friend class XFloatList;

	public:
								XFloatList( ListOrderingT inOrdering = cOrderNotImportant );		// See XPtrList.h for ListOrderingT choices
								
		// See XPtrList.h for description of functions.
		virtual long			Add( float inNum )								{ return mList.Add( *((void**) &inNum) );			}
		virtual void			Add( const XFloatList& inList )					{ mList.Add( inList.mList );						}
		virtual bool			RemoveElement( long inIndex )					{ return mList.RemoveElement( inIndex );			}
		virtual void			RemoveAll()										{ mList.RemoveAll(); 								}
		virtual float			Fetch( long inIndex )							{ long t = (long) mList.Fetch( inIndex ); return *((float*) &t);}
		virtual bool			Fetch( long inIndex, float* ioPtrDest ) const	{ return mList.Fetch( inIndex, (void**)ioPtrDest );	}
		virtual long			Count()	const									{ return mList.Count();								}

		//	Post: Ranks all the values in this list.
		//	Post: Fetch( outRank[ i ] ) is the ith largest value in this list.
		//	Post: outRank.Count() == inNumToRank  (ie, only inNumToRank values of the ranking are returned)
		//	Note: If inNumToRank is invalid, the full ranking is returned
		//	Note: O( N log N ) running time
		void					Rank( XLongList& outRank, long inNumToRank = -1 ) const;
		
		// Computes a specified number of values that represent center-values for that current list of floats
		// Note: if this float list isn't already sorted from LowToHigh, GetMeans() will have to perform a full sort!!
		void					FindMeans( long inNumMeans, float outMeans[], float inSigmaScale = 0.05 ) const;

		// Smoothes all the floats in this list
		void					GaussSmooth( float inSigma );

		float					operator[] ( const long inIndex )				{ long t = (long) mList.Fetch( inIndex ); return *((float*) &t); } 
		
		// Generic utility fcn to gauss-smooth a 1D curve.
		static void				GaussSmooth( float inSigma, long inN, float inSrceDest[] );
		static void				GaussSmooth( float inSigma, long inN, float inSrce[], float inDest[] );


		static void				SlopeSmooth( float inSmoothness, long inN, float ioData[] );
		
	protected:
		static int				sFloatComparitor( const void* inA, const void* inB );
		static int				sQSFloatComparitor( const void* inA, const void* inB );
		
		#define MASK_MAX		40
		
		static float			sMask[ MASK_MAX ];
		static UtilStr			sTemp;
		
		XPtrList				mList;
		
		
};




#endif