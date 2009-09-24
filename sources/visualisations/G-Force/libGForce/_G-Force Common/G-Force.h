
#pragma once


#include "GForcePixPort.h"
#include "..\common\general tools\FileSpecList.h"
#include "..\common\general tools\ArgList.h"
#include "..\common\math\Expression.h"
#include "WaveShape.h"
#include "DeltaField.h"
#include "GF_Palette.h"
#include "..\common\general tools\XLongList.h"

#include "..\common\io\Prefs.h"

enum {
	
	cDispTrackTitle		= 1,
	cGetConfigInfo		= 2,
	cFrameRate			= 3,
	cSpawnNewParticle	= 4,
	cToggleFullsceen	= 5,
	cToggleConfigName	= 6,
	cToggleNormalize	= 7,
	cStartSlideshowAll	= 8,
	cStopSlideshowAll	= 9,
	cPrevDeltaField		= 10,
	cNextDeltaField		= 11,
	cToggleFieldShow	= 12,
	cPrevColorMap		= 13,
	cNextColorMap		= 14,
	cToggleColorShow	= 15,
	cPrevWaveShape		= 16,
	cNextWaveShape		= 17,
	cToggleShapeShow	= 18,
	cDecMagScale		= 19,
	cIncMagScale		= 20,
	cDecNumSSteps		= 21,
	cIncNumSSteps		= 22,
	cToggleParticles	= 23,
	cSetPreset0			= 30,
	cSetPreset1			= 31,
	cSetPreset2			= 32,
	cSetPreset3			= 33,
	cSetPreset4			= 34,
	cSetPreset5			= 35,
	cSetPreset6			= 36,
	cSetPreset7			= 37,
	cSetPreset8			= 38,
	cSetPreset9			= 39,
	cPreset0			= 40,
	cPreset1			= 41,
	cPreset2			= 42,
	cPreset3			= 43,
	cPreset4			= 44,
	cPreset5			= 45,
	cPreset6			= 46,
	cPreset7			= 47,
	cPreset8			= 48,
	cPreset9			= 49
};



class PortPage {

	PixPort*	mPort;
	
};

	

class GForce {

	public:	
								GForce( const char* szColorMaps, const char* szDeltaFields, const char* szParticles, const char* szWaveShapes, void* inRefCon = NULL );
								~GForce();


		void					SetFullscreen( bool inFullScreen );
		inline bool				IsFullscreen()		{ return mAtFullScreen;	}

		// Pre: mSample[] has contains a sample/copy of the freq spectrum
		void					RecordSample( long inCurTime, float* inSound, float inMax, long inNumBins );
		void					RecordZeroSample( long inCurTime );
	
		bool					BorderlessWindow()						{ return mBorderlessWind;						}
	
		// This blackens the entire plugin/window area
		void					Refresh()								{ mNeedsPaneErased = true;						}
		
		bool					HandleKey( long inChar );


		void					StoreWinRect()							{ if ( ! IsFullscreen() ) GetWinRect( mWinRectHolder ); }

		void					SetPort( void* inPort, const Rect& inRect, bool inAtFullsceen );
		
		void					SetWinPort( void* inPort, const Rect* inRect = NULL );
		
		void					GetWinRect( Rect& outRect );
		
		long					DefaultNum_S_Steps()					{ return mNum_S_Steps;							}


		void setDrawParameter(unsigned long *inFrameBuffer, int inPitch) {
			frameBuffer = inFrameBuffer;
			pitch = inPitch;
		}

		Point					GetFullscreenSize() 					{ return mFullscreenSize;	}
		long					GetFullscreenDepth()					{ return mFullscreenDepth;	}
		
		/* NewSong() tells GForce a new track has started and that the following four strings may contain info. */
		UtilStr					mArtist;
		UtilStr					mAlbum;
		UtilStr					mSongTitle;
		void					NewSong();
		
	protected:
		void*					mRefCon;
		bool					mDoingSetPortWin;	// true when a thread is currently inside SetPortWin()
		Rect					mWinRectHolder;		// Win rect holder while we're n FS mode
		Rect					mDispRect;			// Local cords rect that specify where the blt area is
		Rect					mPaneRect;			// Local cords rect within mOSPort we can draw in
		Prefs					mPrefs;
		

		GForcePixPort			mPortA, mPortB;
		GForcePixPort*			mCurPort;
		
		// Console related members
		XStrList				mConsoleLines;
		XLongList				mLineExpireTimes;
		UtilStr					mTemp;
		long					mConsoleDelay;
		long					mConsoleLineDur;
		long					mConsoleExpireTime;
		void					DrawConsole();
		void					Print( char* inStr );
		void					Print( UtilStr* inStr )											{ if ( inStr ) Print( inStr -> getCStr() );		}
		void					Println( char* inStr );
		void					Println( UtilStr* inStr )										{ Println( inStr ? inStr -> getCStr() : NULL ); }
				
		// Palette stuff
		PixPalEntry				mPalette[ 256 ];
		GF_Palette				mPal1, mPal2, *mGF_Palette, *mNextPal;
		float					mNextPaletteUpdate;
		float					mIntensityParam;
		
	
		// Plugin prefs
		float					mScrnSaverDelay;
		long					mTransitionLo;
		long					mTransitionHi;
		long					mHandleKeys;
		float					mMagScale;
		long					mBorderlessWind;
		long					mNum_S_Steps;
		bool					mNewConfigNotify;
		bool					mNormalizeInput;
		bool					mParticlesOn;
		Point					mMaxSize;
		Point					mFullscreenSize;
		long					mFullscreenDepth;
		long					mFullscreenDevice;
		UtilStr					mKeyMap;
		UtilStr					mParticleDuration;			// Num secs a particle will stay around
		UtilStr					mParticleProbability;		// Probability a new sausage will start
		UtilStr					mTrackMetaText;
		UtilStr					mTrackFont;
		UtilStr					mTrackTextStartStr;
		UtilStr					mTrackTextDurationStr;
		long					mTrackTextPosMode;
		long					mTrackTextSize;
	
		
		// Particle stuff
		long					mNextParticleCheck;
		float					mLastParticleStart;			// LAST_PARTICLE_START
		float					mNumRunningParticles;		// NUM_PARTICLES
		Expression				mParticleProbabilityFcn;	// mParticleProbability compiled
		Expression				mParticleDurationFcn;		// mParticleDuration compiles
		ExpressionDict			mDict;
		nodeClass				mStoppedParticlePool;
		nodeClass				mRunningParticlePool;

		FileSpecList			mDeltaFields,		mColorMaps,			mWaveShapes,		mParticles;
		long					mCurFieldNum,		mCurColorMapNum,	mCurShapeNum,		mCurParticleNum;
		float					mNextFieldChange, 	mNextColorChange,	mNextShapeChange;
		XLongList				mFieldPlayList,		mColorPlayList,		mShapePlayList,		mParticlePlayList;
		UtilStr					mFieldIntervalStr,	mColorIntervalStr,	mShapeIntervalStr;
		long										mColorTransEnd,		mShapeTransEnd;	
		long										mColorTransTime,	mShapeTransTime;	// When > 0, transition is in progress
		float										mColorTrans;
		bool					mFieldSlideShow,	mColorSlideShow,	mShapeSlideShow;
		UtilStr										mColorMapName,		mWaveShapeName;
		Expression				mFieldInterval,		mColorInterval,		mShapeInterval;
		#define			TRANSITION_ALPHA	1.45f


		// Linked dict vars/addressed data spaces
		float					mT;
		
		// Field stuff
		DeltaField*				mField, *mNextField;
		DeltaField				mField1, mField2;
		
		// WaveShape stuff
		float					mWaveXScale;
		float					mWaveYScale;
		WaveShape				mWave1, mWave2, *mWave, *mNextWave;
		long					mT_MS, mT_MS_Base;
		UtilStr					mSamplesBuf;
		UtilStr					mSineBuf;
		float*					mSine;
		
		ExprUserFcn*			mSampleFcn;
				
		void					loadDeltaField( long inFieldNum );
		void					loadWaveShape( long inShapeNum, bool inAllowMorph );
		void					loadColorMap( long inColorMapNum, bool inAllowMorph );
		void					loadParticle( long inParticleNum );


		void					RecordSample( long inCurTime );
		void          BuildConfigLists(const char* szColorMaps, const char* szDeltaFields, const char* szParticles, const char* szWaveShapes);
		void					DrawWave( PixPort& inDest );
		
		// Frame rate/calc members
		long					mCurFrameRate;
		float					mFrameCountStart;
		long					mFrameCount;

		// Stuff dealing with full screen & screensaver mode
		bool					mAtFullScreen;
		bool					mMouseWillAwaken;
		Point					mLastMousePt;
		float					mLastActiveTime, mLastKeyPollTime;
		KeyMap					mCurKeys, mPastKeys;
		void					IdleMonitor();
	
		unsigned long *frameBuffer; 
		int pitch;
		
		// GUI related stuff
		long					mLastCursorUpdate;
		long					mLastGetKeys;
		bool					mNeedsPaneErased;
		void					ErasePane();
		void					DrawFrame();
		
		void					StoreConfigState( long inParamName );
		bool					RestoreConfigState( long inParamName );
		void					ShowHelp();
		
		// Track text related
		float					mLastSongStart;
		float					mTrackTextStartTime;
		float					mTrackTextDur;
		Expression				mTrackTextStartFcn, mTrackTextDurFcn;
		Point					mTrackTextPos;
		UtilStr					mTrackText;
		void					CalcTrackTextPos();
		void					StartTrackText();
		
		void					DrawParticles( PixPort& inPort );
		
		void					SetNumSampleBins( long inNumBins );
		
		void					ManageColorChanges();
		void					ManageShapeChanges();
		void					ManageFieldChanges();
		void					ManageParticleChanges();
		
		void					SpawnNewParticle();

};