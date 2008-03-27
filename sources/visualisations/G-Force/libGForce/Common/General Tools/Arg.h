#ifndef _ARG_
#define _ARG_

#pragma once

class UtilStr;
class MetaExpr;
class CEgOStream;

class Arg {

	
	public:
								Arg( long inID, long inData, Arg* inNext );	
								Arg( long inID, const char* inStr, Arg* inNext );
								~Arg();
					
		void					Assign( long inData );
		void					Assign( const UtilStr* inStr );
		void					Assign( const char* inStr );
					
		void					ExportTo( CEgOStream* ioStream ) const;
		
		
		inline unsigned long	GetID()	const						{ return mID;						}
		inline bool				IsStr() const						{ return mIsStr;					}
		inline long				GetData() const						{ return mData;						}
		

	protected:
		unsigned long			mID;
		bool					mIsStr;
		long					mData;
		
	public:
		Arg*					mNext;
};


#endif