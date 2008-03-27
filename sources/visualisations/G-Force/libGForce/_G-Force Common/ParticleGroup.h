#ifndef ParticleGroup_H
#define ParticleGroup_H

#include "WaveShape.h"


class ParticleGroup : public WaveShape, public nodeClass {
	
	public:
		
		
								ParticleGroup( float* inTPtr, ExprUserFcn** inMagFcn );
		
		void					Load( ArgList& inArgs );

		void					SetDuration( float inSecs )							{ mEndTime = *mTPtr + inSecs;	}
		inline bool				IsExpired()											{ return *mTPtr > mEndTime;	}
		
		void					DrawGroup( PixPort& inDest );

		UtilStr					mTitle;

	protected:
		const float*			mTPtr;
		
		float					mID, mNumInstances;
		float					mEndTime, mStartTime;
		float					mFadeTime;
};

#endif
