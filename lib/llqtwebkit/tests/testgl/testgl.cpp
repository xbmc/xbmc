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

#ifndef _WINDOWS
extern "C" {
#include <unistd.h>
}
#endif

#ifdef _WINDOWS
#include <windows.h>
#include <direct.h>	// for local file access
#endif

#include <iostream>
#include <stdlib.h>

#ifdef LL_OSX
// I'm not sure why STATIC_QT is getting defined, but the Q_IMPORT_PLUGIN thing doesn't seem to be necessary on the mac.
#undef STATIC_QT
#endif

#ifdef STATIC_QT
#include <QtPlugin>
Q_IMPORT_PLUGIN(qgif)
#endif

#ifdef LL_OSX
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#include "GL/glut.h"
#endif
#include "llqtwebkit.h"

////////////////////////////////////////////////////////////////////////////////
// Implementation of the test app - implemented as a class and derrives from
// the observer so we can catch events emitted by LLQtWebKit
//
class testGL :
	public LLEmbeddedBrowserWindowObserver
{
	public:
		testGL() :
			mAppWindowWidth( 800 ),						// dimensions of the app window - can be anything
			mAppWindowHeight( 900 ),
			mBrowserWindowWidth( mAppWindowWidth ),		// dimensions of the embedded browser - can be anything
			mBrowserWindowHeight( mAppWindowHeight ),	// but looks best when it's the same as the app window
			mAppTextureWidth( -1 ),						// dimensions of the texture that the browser is rendered into
			mAppTextureHeight( -1 ),					// calculated at initialization
			mAppTexture( 0 ),
			mBrowserWindowId( 0 ),
			mAppWindowName( "testGL" ),
			mHomeUrl(),
			mNeedsUpdate( true )						// flag to indicate if browser texture needs an update
		{
			char tempPath[255];
            char *cwd = getcwd(tempPath, 255);
            mHomeUrl = cwd;
            mHomeUrl.append("/testpage.html");
            std::cout << "LLQtWebKit version: " << LLQtWebKit::getInstance()->getVersion() << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void init( const std::string argv0, const std::string argv1 )
		{
			// OpenGL initialization
			glClearColor( 0.0f, 0.0f, 0.0f, 0.5f);
			glEnable( GL_COLOR_MATERIAL );
			glColorMaterial( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );
			glEnable( GL_TEXTURE_2D );
			glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
			glEnable( GL_CULL_FACE );

			// calculate texture size required (next power of two above browser window size
			for ( mAppTextureWidth = 1; mAppTextureWidth < mBrowserWindowWidth; mAppTextureWidth <<= 1 )
			{
			};

			for ( mAppTextureHeight = 1; mAppTextureHeight < mBrowserWindowHeight; mAppTextureHeight <<= 1 )
			{
			};

			// create the texture used to display the browser data
			glGenTextures( 1, &mAppTexture );
			glBindTexture( GL_TEXTURE_2D, mAppTexture );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexImage2D( GL_TEXTURE_2D, 0,
				GL_RGB,
					mAppTextureWidth, mAppTextureHeight,
						0, GL_RGB, GL_UNSIGNED_BYTE, 0 );

			// create a single browser window and set things up.
			std::string applicationDir = argv0.substr( 0, argv0.find_last_of("\\/") );

            std::string componentDir = applicationDir;
#ifdef _WINDOWS
			std::string profileDir = applicationDir + "\\" + "testGL_profile";
#else
			std::string profileDir = applicationDir + "/" + "testGL_profile";
#endif
			LLQtWebKit::getInstance()->init( applicationDir, componentDir, profileDir, getNativeWindowHandle() );
			
			// set host language test (in reality, string will be language code passed into client) 
			// IMPORTANT: must be called before createBrowserWindow(...)
			LLQtWebKit::getInstance()->setHostLanguage( "EN-AB-CD-EF" );
			
			// enable Javascript
			LLQtWebKit::getInstance()->enableJavascript( true );

			// enable Plugins
			LLQtWebKit::getInstance()->enablePlugins( true );

			// make a browser window		
			mBrowserWindowId = LLQtWebKit::getInstance()->createBrowserWindow( mBrowserWindowWidth, mBrowserWindowHeight );

			// tell LLQtWebKit about the size of the browser window
			LLQtWebKit::getInstance()->setSize( mBrowserWindowId, mBrowserWindowWidth, mBrowserWindowHeight );

			// observer events that LLQtWebKit emits
			LLQtWebKit::getInstance()->addObserver( mBrowserWindowId, this );

			// append details to agent string
			LLQtWebKit::getInstance()->setBrowserAgentId( mAppWindowName );

			// don't flip bitmap
			LLQtWebKit::getInstance()->flipWindow( mBrowserWindowId, false );

			// target name we open in external browser
			LLQtWebKit::getInstance()->setExternalTargetName( mBrowserWindowId, "Wibblewobbly" );

			// turn on option to catch JavaScript window.open commands and open in same window
			LLQtWebKit::getInstance()->setWindowOpenBehavior( mBrowserWindowId, LLQtWebKit::WOB_REDIRECT_TO_SELF );

			// go to the "home page" or URL passed in via command line
			if ( ! argv1.empty() )
				LLQtWebKit::getInstance()->navigateTo( mBrowserWindowId, argv1 );
			else
				LLQtWebKit::getInstance()->navigateTo( mBrowserWindowId, mHomeUrl );
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void reset( void )
		{
			// unhook observer
			LLQtWebKit::getInstance()->remObserver( mBrowserWindowId, this );

			// clean up
			LLQtWebKit::getInstance()->reset();
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void reshape( int widthIn, int heightIn )
		{
			if ( heightIn == 0 )
				heightIn = 1;

			LLQtWebKit::getInstance()->setSize(mBrowserWindowId, widthIn, heightIn );
			mNeedsUpdate = true;

			glMatrixMode( GL_PROJECTION );
			glLoadIdentity();

			glViewport( 0, 0, widthIn, heightIn );
			glOrtho( 0.0f, widthIn, heightIn, 0.0f, -1.0f, 1.0f );

			// we use these elsewhere so save
			mAppWindowWidth = widthIn;
			mAppWindowHeight = heightIn;
                        mBrowserWindowWidth = widthIn;
                        mBrowserWindowHeight = heightIn;

			glMatrixMode( GL_MODELVIEW );
			glLoadIdentity();

			mNeedsUpdate = true;
			idle();

			glutPostRedisplay();
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void idle()
		{
			LLQtWebKit::getInstance()->pump(100);

			// onPageChanged event sets this
			if ( mNeedsUpdate )
				// grab a page but don't reset 'needs update' flag until we've written it to the texture in display()
				LLQtWebKit::getInstance()->grabBrowserWindow( mBrowserWindowId );

			// lots of updates for smooth motion
			glutPostRedisplay();
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void display()
		{
			// clear screen
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

			glLoadIdentity();

			// use the browser texture
			glBindTexture( GL_TEXTURE_2D, mAppTexture );

			// valid window ?
			if ( mBrowserWindowId )
			{
				// needs to be updated?
				if ( mNeedsUpdate )
				{
					// grab the page
					const unsigned char* pixels = LLQtWebKit::getInstance()->getBrowserWindowPixels( mBrowserWindowId );
					if ( pixels )
					{
						// write them into the texture
						glTexSubImage2D( GL_TEXTURE_2D, 0,
							0, 0,
								// because sometimes the rowspan != width * bytes per pixel (mBrowserWindowWidth)
								LLQtWebKit::getInstance()->getBrowserRowSpan( mBrowserWindowId ) / LLQtWebKit::getInstance()->getBrowserDepth( mBrowserWindowId ),
									mBrowserWindowHeight,
#ifdef _WINDOWS
                                    LLQtWebKit::getInstance()->getBrowserDepth(mBrowserWindowId ) == 3 ? GL_RGBA : GL_RGBA,
#elif defined(__APPLE__)
                                    GL_RGBA,
#elif defined(LL_LINUX)
                                    GL_RGBA,
#endif
											GL_UNSIGNED_BYTE,
												pixels );
					};

					// flag as already updated
					mNeedsUpdate = false;
				};
			};

			// scale the texture so that it fits the screen
			GLfloat textureScaleX = ( GLfloat )mBrowserWindowWidth / ( GLfloat )mAppTextureWidth;
			GLfloat textureScaleY = ( GLfloat )mBrowserWindowHeight / ( GLfloat )mAppTextureHeight;

			// draw the single quad full screen (orthographic)
			glMatrixMode( GL_TEXTURE );
			glPushMatrix();
			glScalef( textureScaleX, textureScaleY, 1.0f );

			glEnable( GL_TEXTURE_2D );
			glColor3f( 1.0f, 1.0f, 1.0f );
			glBegin( GL_QUADS );
				glTexCoord2f( 1.0f, 0.0f );
				glVertex2d( mAppWindowWidth, 0 );

				glTexCoord2f( 0.0f, 0.0f );
				glVertex2d( 0, 0 );

				glTexCoord2f( 0.0f, 1.0f );
				glVertex2d( 0, mAppWindowHeight );

				glTexCoord2f( 1.0f, 1.0f );
				glVertex2d( mAppWindowWidth, mAppWindowHeight );
			glEnd();

			glMatrixMode( GL_TEXTURE );
			glPopMatrix();

			glutSwapBuffers();
		};

		////////////////////////////////////////////////////////////////////////////////
		// convert a GLUT keyboard modifier to an LLQtWebKit one
		// (only valid in mouse and keyboard callbacks
		LLQtWebKit::EKeyboardModifier getLLQtWebKitKeyboardModifierCode()
		{
			int result = LLQtWebKit::KM_MODIFIER_NONE;
			
			int modifiers = glutGetModifiers();

			if ( GLUT_ACTIVE_SHIFT & modifiers )
				result |= LLQtWebKit::KM_MODIFIER_SHIFT;

			if ( GLUT_ACTIVE_CTRL & modifiers )
				result |= LLQtWebKit::KM_MODIFIER_CONTROL;

			if ( GLUT_ACTIVE_ALT & modifiers )
				result |= LLQtWebKit::KM_MODIFIER_ALT;
			
			return (LLQtWebKit::EKeyboardModifier)result;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void mouseButton( int button, int state, int xIn, int yIn )
		{
			// texture is scaled to fit the screen so we scale mouse coords in the same way
			xIn = ( xIn * mBrowserWindowWidth ) / mAppWindowWidth;
			yIn = ( yIn * mBrowserWindowHeight ) / mAppWindowHeight;

			if ( button == GLUT_LEFT_BUTTON )
			{
				if ( state == GLUT_DOWN )
				{
					// send event to LLQtWebKit
					LLQtWebKit::getInstance()->mouseEvent( mBrowserWindowId,
															LLQtWebKit::ME_MOUSE_DOWN,
																LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
																	xIn, yIn,
																		getLLQtWebKitKeyboardModifierCode() );
				}
				else
				if ( state == GLUT_UP )
				{
					// send event to LLQtWebKit
					LLQtWebKit::getInstance()->mouseEvent( mBrowserWindowId,
															LLQtWebKit::ME_MOUSE_UP,
																LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
																	xIn, yIn,
																		getLLQtWebKitKeyboardModifierCode() );


					// this seems better than sending focus on mouse down (still need to improve this)
					LLQtWebKit::getInstance()->focusBrowser( mBrowserWindowId, true );
				};
			};

			// force a GLUT  update
			glutPostRedisplay();
		}

		////////////////////////////////////////////////////////////////////////////////
		//
		void mouseMove( int xIn , int yIn )
		{
			// texture is scaled to fit the screen so we scale mouse coords in the same way
			xIn = ( xIn * mBrowserWindowWidth ) / mAppWindowWidth;
			yIn = ( yIn * mBrowserWindowHeight ) / mAppWindowHeight;

			// send event to LLQtWebKit
			LLQtWebKit::getInstance()->mouseEvent( mBrowserWindowId,
													LLQtWebKit::ME_MOUSE_MOVE,
														LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
															xIn, yIn,
																LLQtWebKit::KM_MODIFIER_NONE );


			// force a GLUT  update
			glutPostRedisplay();
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void keyboard( unsigned char keyIn, bool isDown)
		{
			// ESC key exits
			if ( keyIn == 27 )
			{
				reset();

				exit( 0 );
			};
			
			if(keyIn == 127)
			{
				// Turn delete char into backspace
				keyIn = LLQtWebKit::KEY_BACKSPACE;
			}

			// control-H goes home
			if ( keyIn == 8 )
			{
				LLQtWebKit::getInstance()->navigateTo( mBrowserWindowId, mHomeUrl );
			}
			else
			// control-R reloads
			if ( keyIn == 18 )
			{
				LLQtWebKit::getInstance()->userAction(mBrowserWindowId, LLQtWebKit::UA_NAVIGATE_RELOAD );
			}
			else
			{
				char text[2];
				if(keyIn < 0x80)
				{
					text[0] = (char)keyIn;
				}
				else
				{
					text[0] = 0;
				}

				text[1] = 0;
				
				std::cerr << "key " << (isDown?"down ":"up ") << (int)keyIn << ", modifiers = " << (int)getLLQtWebKitKeyboardModifierCode() << std::endl;
				
				// send event to LLQtWebKit
				LLQtWebKit::getInstance()->keyboardEvent(mBrowserWindowId, isDown?LLQtWebKit::KE_KEY_DOWN:LLQtWebKit::KE_KEY_UP, keyIn, text, getLLQtWebKitKeyboardModifierCode() );
			}
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void keyboardSpecial( int specialIn, bool isDown)
		{
			uint32_t key = LLQtWebKit::KEY_NONE;
			
			switch(specialIn)
			{
				case GLUT_KEY_F1:			key = LLQtWebKit::KEY_F1;		break;
				case GLUT_KEY_F2:			key = LLQtWebKit::KEY_F2;		break;
				case GLUT_KEY_F3:			key = LLQtWebKit::KEY_F3;		break;
				case GLUT_KEY_F4:			key = LLQtWebKit::KEY_F4;		break;
				case GLUT_KEY_F5:			key = LLQtWebKit::KEY_F5;		break;
				case GLUT_KEY_F6:			key = LLQtWebKit::KEY_F6;		break;
				case GLUT_KEY_F7:			key = LLQtWebKit::KEY_F7;		break;
				case GLUT_KEY_F8:			key = LLQtWebKit::KEY_F8;		break;
				case GLUT_KEY_F9:			key = LLQtWebKit::KEY_F9;		break;
				case GLUT_KEY_F10:			key = LLQtWebKit::KEY_F10;		break;
				case GLUT_KEY_F11:			key = LLQtWebKit::KEY_F11;		break;
				case GLUT_KEY_F12:			key = LLQtWebKit::KEY_F12;		break;
				case GLUT_KEY_LEFT:			key = LLQtWebKit::KEY_LEFT;		break;
				case GLUT_KEY_UP:			key = LLQtWebKit::KEY_UP;		break;
				case GLUT_KEY_RIGHT:		key = LLQtWebKit::KEY_RIGHT;	break;
				case GLUT_KEY_DOWN:			key = LLQtWebKit::KEY_DOWN;		break;
				case GLUT_KEY_PAGE_UP:		key = LLQtWebKit::KEY_PAGE_UP;	break;
				case GLUT_KEY_PAGE_DOWN:	key = LLQtWebKit::KEY_PAGE_DOWN;break;
				case GLUT_KEY_HOME:			key = LLQtWebKit::KEY_HOME;		break;
				case GLUT_KEY_END:			key = LLQtWebKit::KEY_END;		break;
				case GLUT_KEY_INSERT:		key = LLQtWebKit::KEY_INSERT;	break;
				
				default:
				break;
			}
			
			if(key != LLQtWebKit::KEY_NONE)
			{
				keyboard(key, isDown);
			}
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onPageChanged( const EventType& /*eventIn*/ )
		{
			// flag that an update is required - page grab happens in idle() so we don't stall
			mNeedsUpdate = true;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onNavigateBegin( const EventType& eventIn )
		{
			std::cout << "Event: begin navigation to " << eventIn.getEventUri() << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onNavigateComplete( const EventType& eventIn )
		{
			std::cout << "Event: end navigation to " << eventIn.getEventUri() << " with response status of " << eventIn.getIntValue() << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onUpdateProgress( const EventType& eventIn )
		{
			std::cout << "Event: progress value updated to " << eventIn.getIntValue() << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onStatusTextChange( const EventType& eventIn )
		{
			std::cout << "Event: status updated to " << eventIn.getStringValue() << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onTitleChange( const EventType& eventIn )
		{
			std::cout << "Event: title changed to  " << eventIn.getStringValue() << std::endl;
			glutSetWindowTitle( eventIn.getStringValue().c_str() );
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onLocationChange( const EventType& eventIn )
		{
			std::cout << "Event: location changed to " << eventIn.getStringValue() << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onClickLinkHref( const EventType& eventIn )
		{
			std::cout << "Event: clicked on link:" << std::endl;
			std::cout << "  URL:" << eventIn.getStringValue() << std::endl;

			if ( LLQtWebKit::LTT_TARGET_NONE == eventIn.getLinkType() )
				std::cout << "  No target attribute - opening in current window" << std::endl;

			if ( LLQtWebKit::LTT_TARGET_BLANK == eventIn.getLinkType() )
				std::cout << "  Blank target attribute (" << eventIn.getStringValue2() << ") - not navigating in this window" << std::endl;

			if ( LLQtWebKit::LTT_TARGET_EXTERNAL == eventIn.getLinkType() )
				std::cout << "  External target attribute (" << eventIn.getStringValue2() << ") - not navigating in this window" << std::endl;

			if ( LLQtWebKit::LTT_TARGET_OTHER == eventIn.getLinkType() )
				std::cout << "  Other target attribute (" << eventIn.getStringValue2() << ") - opening in current window" << std::endl;

			std::cout << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		int getAppWindowWidth()
		{
			return mAppWindowWidth;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		int getAppWindowHeight()
		{
			return mAppWindowHeight;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		std::string getAppWindowName()
		{
			return mAppWindowName;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void* getNativeWindowHandle()
		{
			// My implementation of the embedded browser needs a native window handle
			// Can't get this via GLUT so had to use this hack
	    #ifdef _WINDOWS
			return FindWindow( NULL, (LPCWSTR)mAppWindowName.c_str() );
		#else
                #ifdef LL_OSX
                        // not needed on osx
                        return 0;
                #else
                        //#error "You will need an implementation of this method"
                        return 0;
                #endif
            #endif
		};

	private:
		int mAppWindowWidth;
		int mAppWindowHeight;
		int mBrowserWindowWidth;
		int mBrowserWindowHeight;
		int mAppTextureWidth;
		int mAppTextureHeight;
		GLuint mAppTexture;
		int mBrowserWindowId;
		std::string mAppWindowName;
		std::string mHomeUrl;
		bool mNeedsUpdate;
};

testGL* theApp;

////////////////////////////////////////////////////////////////////////////////
//
void glutReshape( int widthIn, int heightIn )
{
	if ( theApp )
		theApp->reshape( widthIn, heightIn );
};

////////////////////////////////////////////////////////////////////////////////
//
void glutDisplay()
{
	if ( theApp )
		theApp->display();
};

////////////////////////////////////////////////////////////////////////////////
//
void glutIdle()
{
	if ( theApp )
		theApp->idle();
};

////////////////////////////////////////////////////////////////////////////////
//
void glutKeyboard( unsigned char keyIn, int /*xIn*/, int /*yIn*/ )
{
	if ( theApp )
	{
		theApp->keyboard( keyIn, true );
	}
};

////////////////////////////////////////////////////////////////////////////////
//
void glutKeyboardUp( unsigned char keyIn, int /*xIn*/, int /*yIn*/ )
{
	if ( theApp )
	{
		theApp->keyboard( keyIn, false );
	}
};

////////////////////////////////////////////////////////////////////////////////
//
void glutSpecial( int specialIn, int /*xIn*/, int /*yIn*/ )
{
	if ( theApp )
	{
		theApp->keyboardSpecial( specialIn, true );
	}
};

////////////////////////////////////////////////////////////////////////////////
//
void glutSpecialUp( int specialIn, int /*xIn*/, int /*yIn*/ )
{
	if ( theApp )
	{
		theApp->keyboardSpecial( specialIn, false );
	}
};

////////////////////////////////////////////////////////////////////////////////
//
void glutMouseMove( int xIn , int yIn )
{
	if ( theApp )
		theApp->mouseMove( xIn, yIn );
}

////////////////////////////////////////////////////////////////////////////////
//
void glutMouseButton( int buttonIn, int stateIn, int xIn, int yIn )
{
	if ( theApp )
		theApp->mouseButton( buttonIn, stateIn, xIn, yIn );
}

////////////////////////////////////////////////////////////////////////////////
//
int main( int argc, char* argv[] )
{
	// implementation in a class so we can observer events
	// means we need this painful GLUT <--> class shim...
	theApp = new testGL;

	if ( theApp )
	{
		glutInit( &argc, argv );
		glutInitDisplayMode( GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB );

		glutInitWindowPosition( 80, 0 );
		glutInitWindowSize( theApp->getAppWindowWidth(), theApp->getAppWindowHeight() );

		glutCreateWindow( theApp->getAppWindowName().c_str() );

		std::string url = "";
		if ( 2 == argc )
			url = std::string( argv[ 1 ] );

		theApp->init( std::string( argv[ 0 ] ), url );

		glutKeyboardFunc( glutKeyboard );
		glutKeyboardUpFunc( glutKeyboardUp );
		glutSpecialFunc( glutSpecial );
		glutSpecialUpFunc( glutSpecialUp );

		glutMouseFunc( glutMouseButton );
		glutPassiveMotionFunc( glutMouseMove );
		glutMotionFunc( glutMouseMove );

		glutDisplayFunc( glutDisplay );
		glutReshapeFunc( glutReshape );

		glutIdleFunc( glutIdle );

		glutMainLoop();

		delete theApp;
	};

	return 0;
}

