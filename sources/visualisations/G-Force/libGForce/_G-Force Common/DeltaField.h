#ifndef __DeltaField__
#define __DeltaField__


#include "..\common\math\ExpressionDict.h"
#include "..\common\math\ExprArray.h"
#include "..\common\general tools\TempMem.h"
#include "..\common\ui\PixPort.h"



class ArgList;

class DeltaField {


	public:
								DeltaField();
	
		// Suck in a new grad field.  Note: Resize must be called after Assign()
		void					Assign( ArgList& inArgs, UtilStr& inName );
		
		// Reinitiate/reset the computation of this grad field.
		void					SetSize( long inWidth, long inHeight, long inRowSize, bool inForceRegen = false );
		
		// Compute a small portion of the grad field.  Call GetField() to see if the field finished.
		void					CalcSome();
		
		// See if this delta field is 100% calculated
		bool					IsCalculated()								{ return mCurrentY == mHeight;	}
		
		//  Returns a ptr to the buf of this grad field.
		//	Note:  If the field is not 100% calculated, it will finish calculating and may take a couple seconds.
		DeltaFieldData*			GetField();
		
		char*					GetName()									{ return mName.getCStr();		}
		
	protected:
	
		
		long					mCurrentY;
		ExpressionDict			mDict;
		float					mX_Cord, mY_Cord, mR_Cord, mT_Cord;
		float					mXScale, mYScale;
		float					mPI;
		Expression				mXField, mYField;
		bool					mPolar, mHasRTerm, mHasThetaTerm;
		long					mWidth, mHeight, mRowSize;	
		long					mAspect1to1;
		ExprArray				mAVars, mDVars;
		UtilStr					mName;
		TempMem					mGradBuf;
		//TempMem				mYExtentsBuf;
		//long					mNegYExtents;
		DeltaFieldData			mFieldData;

		char*					mCurrentRow;
};


#endif