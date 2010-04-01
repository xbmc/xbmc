/* Copyright (c) 2006-2010, Linden Research, Inc.
 * 
 * LLQtWebKit Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in GPL-license.txt in this distribution, or online at
 * http://secondlifegrid.net/technology-programs/license-virtual-world/viewerlicensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/technology-programs/license-virtual-world/viewerlicensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 */

#ifndef UBROWSER_H
#define UBROWSER_H

#include <string>
#include <vector>
#include <math.h>

#ifdef LL_OSX
#include "GLUT/glut.h"
#include "glui.h"
#else
#include "GL/glut.h"
#include "GL/glui.h"
#endif
#include "llqtwebkit.h"

static void gluiCallbackWrapper( int controlIdIn );

////////////////////////////////////////////////////////////////////////////////
//
class uBrowser :
	public LLEmbeddedBrowserWindowObserver
{
	public:
		uBrowser();
		~uBrowser();

		bool init( const char* arg0, int appWindowIn );
		bool reset();
		const std::string& getName() { return mName; };
		void reshape( int widthIn, int heightIn );
		void makeChrome();
		void windowPosToTexturePos( int winXIn, int winYIn, int& texXOut, int& texYOut, int& faceOut );
		void resetView();
		void drawGeometry( int geomTypeIn, int updateTypeIn );

		void display();
		void idle();
		void keyboard( unsigned char keyIn, int xIn, int yIn );
		void specialKeyboard( int keyIn, int xIn, int yIn );
		void passiveMouse( int xIn, int yIn );
		void mouseButton( int button, int state, int xIn, int yIn );
		void mouseMove( int xIn, int yIn );

		void setSize( int indexIn, int widthIn , int heightIn );

		void gluiCallback( int controlIdIn );

		virtual void onPageChanged( const EventType& eventIn );
		virtual void onNavigateBegin( const EventType& eventIn );
		virtual void onNavigateComplete( const EventType& eventIn );
		virtual void onUpdateProgress( const EventType& eventIn );
		virtual void onStatusTextChange( const EventType& eventIn );
		virtual void onLocationChange( const EventType& eventIn );
		virtual void onClickLinkHref( const EventType& eventIn );
		virtual void onClickLinkNoFollow( const EventType& eventIn );

	private:
		enum Constants { MaxBrowsers = 6 };

		void* getNativeWindowHandle();
		void setFocusNativeWindow();
		GLenum getGLTextureFormat(int size);

		const int mVersionMajor;
		const int mVersionMinor;
		const int mVersionPatch;
		std::string mName;
		int mAppWindow;
		int mWindowWidth;
		int mWindowHeight;
		int mTextureWidth;
		int mTextureHeight;
		int mBrowserWindowWidth;
		int mBrowserWindowHeight;
		float mTextureScaleX;
		float mTextureScaleY;
		float mViewportAspect;
		float mViewPos[ 3 ];
		float mViewRotation[ 16 ];
		unsigned char mPixelColorRB[ 3 ];
		unsigned char mPixelColorG[ 3 ];
		int mCurMouseX;
		int mCurMouseY;
		GLuint mRedBlueTexture;
		unsigned char mRedBlueTexturePixels[ 256 * 256 * 3 ];
		GLuint mGreenTexture[ MaxBrowsers ];
		unsigned char mGreenTexturePixels[ MaxBrowsers ][ 16 * 16 * 3 ];
		GLuint mAppTexture[ MaxBrowsers ];
		unsigned char* mAppTexturePixels;
		int mSelBookmark;
		int mCurObjType;
		GLUI* mTopGLUIWindow;
		GLUI_Button* mNavBackButton;
		GLUI_Button* mNavStopButton;
		GLUI_Button* mNavForwardButton;
		GLUI* mTop2GLUIWindow;
		GLUI_EditText* mUrlEdit;
		GLUI_EditText* mUrlAddToHistoryEdit;
		GLUI_String mNavUrl;
		GLUI* mRightGLUIWindow;
		GLUI_Rotation* mViewRotationCtrl;
		GLUI_Translation* mViewScaleCtrl;
		GLUI_Translation* mViewTranslationCtrl;
		GLUI* mBottomGLUIWindow;
		GLUI_StaticText* mStatusText;
		GLUI_StaticText* mProgressText;
		const int mIdReset;
		const int mIdExit;
		const int mIdClearCookies;
		const int mIdBookmarks;
		const int mIdGeomTypeNull;
		const int mIdGeomTypeFlat;
		const int mIdGeomTypeBall;
		const int mIdGeomTypeCube;
		const int mIdGeomTypeFlag;
		const int mIdUrlEdit;
		const int mIdNavBack;
		const int mIdNavStop;
		const int mIdNavHome;
		const int mIdNavForward;
		const int mIdNavReload;
		const int mIdNavAddToHistory;
		const int mIdBrowserSmall;
		const int mIdBrowserMedium;
		const int mIdBrowserLarge;
		const int mIdFace0;
		const int mIdFace1;
		const int mIdFace2;
		const int mIdFace3;
		const int mIdFace4;
		const int mIdFace5;
		const int mIdUpdateTypeRB;
		const int mIdUpdateTypeG;
		const int mIdUpdateTypeApp;
		std::string mHomeUrl[ MaxBrowsers ];
		std::vector< std::pair< std::string, std::string > > mBookmarks;
		int mNeedsUpdate[ MaxBrowsers ];
		int mWindowId[ MaxBrowsers ];
		int mCurWindowId;
		int mCurFace;
		GLfloat	mRotX;
		GLfloat	mRotY;
		GLfloat	mRotZ;
		const int mNumBrowserWindows;
};

#endif // UBROWSER_H

