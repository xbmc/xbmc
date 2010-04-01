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

#include "ubrowser.h"

#ifdef _WINDOWS
#include <windows.h>
#elif defined(__APPLE__)
    // Grr... glui.h #defines Byte for some reason, which conflicts with the Carbon headers.
    #ifdef Byte
        #undef Byte
    #endif
    #include <Carbon/Carbon.h>
#elif defined(LL_LINUX)
extern "C" {
#include <gtk/gtk.h>
}
#endif

#include <iostream>
#include <sstream>
#include <iomanip>
#include <sstream>
#include <time.h>

// connects GLUI callbacks to the app class
uBrowser* gInstance = 0;

////////////////////////////////////////////////////////////////////////////////
//
uBrowser::uBrowser() :
    mName( "uBrowser" ),
    mCurWindowId( 0 ),
    mCurFace( 0 ),
    mRotX( 180.0f ),
    mRotY( 0.0f ),
    mRotZ( 0.0f ),
    mNumBrowserWindows( MaxBrowsers ),
    mBrowserWindowWidth( 800 ),
    mBrowserWindowHeight( 800 ),
    mTextureWidth( 0 ),
    mTextureHeight( 0 ),
    mTextureScaleX( 0.0f ),
    mTextureScaleY( 0.0f ),
    mViewportAspect( 0.0f ),
    mIdReset( 0x0010 ),
    mIdBookmarks( 0x0011 ),
    mIdExit( 0x0012 ),
    mIdClearCookies( 0x0030 ),
    mIdUrlEdit( 0x0013 ),
    mIdNavBack( 0x0014 ),
    mIdNavStop( 0x0015 ),
    mIdNavHome( 0x0016 ),
    mIdNavForward( 0x0017 ),
    mIdNavReload( 0x001b ),
	mIdNavAddToHistory( 0x001c ),
    mIdBrowserSmall( 0x0018 ),
    mIdBrowserMedium( 0x0019 ),
    mIdBrowserLarge( 0x001a ),
    mIdUpdateTypeRB( 0x0100 ),
    mIdUpdateTypeG( 0x0101 ),
    mIdUpdateTypeApp( 0x0102 ),
    mIdGeomTypeNull( 0x0200 ),
    mIdGeomTypeFlat( 0x0201 ),
    mIdGeomTypeBall( 0x0202 ),
    mIdGeomTypeCube( 0x0203 ),
    mIdGeomTypeFlag( 0x0204 ),
    mIdFace0( 0x0300 ),
    mIdFace1( 0x0301 ),
    mIdFace2( 0x0302 ),
    mIdFace3( 0x0303 ),
    mIdFace4( 0x0304 ),
    mIdFace5( 0x0305 ),
    mStatusText( 0 ),
    mProgressText( 0 ),
    mVersionMajor( 2 ),
    mVersionMinor( 1 ),
    mVersionPatch( 3 )
{
    // use this to connect GLUI callbacks to this class
    gInstance = this;

    // build list of home urls - first used for most things, rest used on a cube
    mHomeUrl[ 0 ] = "http://secondlife.com";
    mHomeUrl[ 1 ] = "http://google.com";
    mHomeUrl[ 2 ] = "http://news.google.com";
    mHomeUrl[ 3 ] = "http://yahoo.com";
    mHomeUrl[ 4 ] = "http://ubrowser.com";
    mHomeUrl[ 5 ] = "http://maps.google.com";

    // list of bookmarks that demonstrate cool stuff!
    mBookmarks.push_back( std::pair< std::string, std::string >( "Select a bookmark", "" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "Second Life Home Page", "http://www.secondlife.com" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "Linden Lab Home Page", "http://www.lindenlab.com" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "Boing Boing", "http://www.boingboing.net" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "Canvas Paint (DHTML version of MS Paint)", "http://www.canvaspaint.org" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "Canvascape", "http://www.abrahamjoffe.com.au/ben/canvascape/" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "DHTML & AJAX tree demo", "http://www.dhtmlgoodies.com/scripts/drag-drop-folder-tree/drag-drop-folder-tree.html" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "DHTML Lemmings!", "http://www.elizium.nu/scripts/lemmings/" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "DHTML graphics demos (astonishing!)", "http://www.dhteumeuleu.com/" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "Digg", "http://www.digg.com" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "eyeOS is an Open Source Web Desktop Environment", "http://eyeos.net/eyeOS" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "Google Home", "http://www.google.com" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "Google Maps", "http://maps.google.com" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "Mah-jongg", "http://www.mah-jongg.ch" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "SVG Opacity & Hover", "http://www.croczilla.com/svg/samples/opacity1/opacity1.xml" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "SVG Tetris (arrow keys and spacebar)", "http://www.croczilla.com/svg/samples/svgtetris/svgtetris.svg" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "SVG interactive Map", "http://www.carto.net/papers/svg/navigationTools/index.svg" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "SVG interactive lines", "http://www.croczilla.com/svg/samples/xbl-shapes2/xbl-shapes2.xml" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "SVG interactive shapes", "http://www.croczilla.com/svg/samples/xbl1/xbl1.xml" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "SVG: Pilat's Draw", "http://pilat.free.fr/dessin_loc/draw.svg" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "Sudoku", "http://www.mah-jongg.ch/sudoku" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "Yahoo! User Interface Library slider", "http://developer.yahoo.com/yui/examples/slider/rgb2.html?mode=dist" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "Canvas Annimation Kit Experiment", "http://glimr.rubyforge.org/cake/canvas.html#DesignSketching" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "Skrbl shared whiteboard (non-flash)", "http://skrbl.com" ) );
    mBookmarks.push_back( std::pair< std::string, std::string >( "404 test page", "http://www.us-japan.org/www.ita.doc.gov" ) );

    std::cout << std::endl << "LLQtWebKit version: " << LLQtWebKit::getInstance()->getVersion() << std::endl;
    std::cout << std::endl << "      GLUT version: " << "3.7.6" << std::endl;    // no way to get real version from GLUT ???
    std::cout << std::endl << "      GLUI version: " << GLUI_Master.get_version() << std::endl;
    std::cout << std::endl << "  uBrowser version: " << mVersionMajor << "." << mVersionMinor << "." << mVersionPatch << std::endl << std::endl;

    mNeedsUpdate[ 0 ] = false;
    mNeedsUpdate[ 1 ] = false;
    mNeedsUpdate[ 2 ] = false;
    mNeedsUpdate[ 3 ] = false;
    mNeedsUpdate[ 4 ] = false;
    mNeedsUpdate[ 5 ] = false;
}

////////////////////////////////////////////////////////////////////////////////
//
uBrowser::~uBrowser()
{
    // clean up - don't generally get here since we quit from a GLUT app with exit..!!
    for( int i = 0; i < mNumBrowserWindows; ++i )
    {
        LLQtWebKit::getInstance()->remObserver( mWindowId[ i ], this );
    };

    LLQtWebKit::getInstance()->reset();
}

////////////////////////////////////////////////////////////////////////////////
//
bool uBrowser::init( const char* arg0, int appWindowIn )
{
    // save the GLUT window handle since we need to reset it sometimes.
    // (GLUT/GLUI dependency)
    mAppWindow = appWindowIn;

    // build GUI
    makeChrome();

    glClearColor( 0.0f, 0.0f, 0.0f, 0.5f );
    glClearDepth( 1.0f );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );
    glEnable( GL_TEXTURE_2D );

    // create the red/blue texture used in picking
    glGenTextures( 1, &mRedBlueTexture );
    for( int y = 0; y < 256; ++y )
    {
        for( int x = 0; x < 256 * 3 ; x += 3 )
        {
            mRedBlueTexturePixels[ y * 256 * 3 + x + 0 ] = ( x / 3 );
            mRedBlueTexturePixels[ y * 256 * 3 + x + 1 ] = 0;
            mRedBlueTexturePixels[ y * 256 * 3 + x + 2 ] = y;
        };
    }
    glBindTexture( GL_TEXTURE_2D, mRedBlueTexture );
    glTexImage2D( GL_TEXTURE_2D, 0,
        GL_RGB,
            256, 256,
                0, GL_RGB, GL_UNSIGNED_BYTE, mRedBlueTexturePixels );

    // create the green texture used in picking
    glGenTextures( mNumBrowserWindows, &mGreenTexture[ 0 ] );
    for( int i = 0; i < mNumBrowserWindows; ++i )
    {
        for( int j = 0; j < 16 * 16 * 3; j += 3 )
        {
            mGreenTexturePixels[ i ][ j + 0 ] = i;
            mGreenTexturePixels[ i ][ j + 1 ] = ( unsigned char )( j / 3 );
            mGreenTexturePixels[ i ][ j + 2 ] = 0;
        };

        glBindTexture( GL_TEXTURE_2D, mGreenTexture[ i ] );
        glTexImage2D( GL_TEXTURE_2D, 0,
            GL_RGB,
                16, 16,
                    0, GL_RGB, GL_UNSIGNED_BYTE, mGreenTexturePixels[ i ] );
    };

    resetView();

    // start with the flat rectangle so we don't freak people out :)
    mCurObjType = mIdGeomTypeCube;

    // initialize
#ifndef LL_LINUX
    std::string applicationDir = std::string( arg0 ).substr( 0, std::string( arg0 ).find_last_of("\\/") );
    std::string componentDir = applicationDir;
    std::string profileDir = applicationDir + "\\" + "ubrowserprofile";
#else
    // Must use absolute paths on Linux
    std::string applicationDir = getcwd(NULL,0);
    std::string componentDir( applicationDir );
    std::string profileDir( applicationDir + "/" + "ubrowserprofile" );
#endif // LL_LINUX

    LLQtWebKit::getInstance()->init( applicationDir, componentDir, profileDir, getNativeWindowHandle() );

    // append details to agent string
    std::ostringstream codec;
    codec << "(uBrowser " << mVersionMajor << "." << mVersionMinor << "." << mVersionPatch << ")";
    LLQtWebKit::getInstance()->setBrowserAgentId( codec.str() );

    // enable cookies
    LLQtWebKit::getInstance()->enableCookies( true );

    // turn proxy off
    LLQtWebKit::getInstance()->enableProxy( false, "", 0 );

    // create and set up browser windows
    for( int i = 0; i < mNumBrowserWindows; ++i )
    {
        mWindowId[ i ] = LLQtWebKit::getInstance()->createBrowserWindow( mBrowserWindowWidth, mBrowserWindowHeight );

        if ( i == 0 )
            mCurWindowId = mWindowId[ i ];

        setSize( i, mBrowserWindowWidth, mBrowserWindowHeight );

        // this is the default color - just here to show that it can be done
        LLQtWebKit::getInstance()->setBackgroundColor( mWindowId[ i ], 0xff, 0xff, 0xff );

        // this is the default color - just here to show that it can be done
        LLQtWebKit::getInstance()->setCaretColor( mWindowId[ i ], 0x00, 0x00, 0x00 );

        // don't flip bitmap
        LLQtWebKit::getInstance()->flipWindow( mWindowId[ i ], false );

        // test the 404 page
        LLQtWebKit::getInstance()->set404RedirectUrl( mWindowId[ i ], "http://google.com/search?q=404" );
    };

    for( int i = 0; i < mNumBrowserWindows; ++i )
    {
        LLQtWebKit::getInstance()->addObserver( mWindowId[ i ], this );

        LLQtWebKit::getInstance()->navigateTo( mWindowId[ i ], mHomeUrl[ i ] );
    };

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
bool uBrowser::reset()
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::reshape( int widthIn, int heightIn )
{
    // save these as we'll need them later
    mWindowWidth = widthIn;
    mWindowHeight = heightIn;

    // just a rough calculation
    mUrlEdit->set_w( mWindowWidth - 400 );
	mUrlAddToHistoryEdit->set_w( mWindowWidth - 500 );
    mStatusText->set_w( mWindowWidth - 100 );

    // update viewport (the active window inside the chrome stuff)
    int viewportX, viewportY;
    int viewportHeight, viewportWidth;
    GLUI_Master.get_viewport_area( &viewportX, &viewportY, &viewportWidth, &viewportHeight );
    glViewport( viewportX, viewportY, viewportWidth, viewportHeight );

    // need this when we come to display
    mViewportAspect = ( float )( viewportWidth ) / ( float)( viewportHeight );

    // GLUI requires this
    if ( glutGetWindow() != mAppWindow )
        glutSetWindow( mAppWindow );

    // trigger re-display
    glutPostRedisplay();
};

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::drawGeometry( int geomTypeIn, int updateTypeIn )
{
    // plain old flat rectangle
    if ( geomTypeIn == mIdGeomTypeFlat )
    {
        glEnable( GL_TEXTURE_2D );
        glColor3f( 1.0f, 1.0f, 1.0f );

        if ( updateTypeIn == mIdUpdateTypeG )
            glBindTexture( GL_TEXTURE_2D, mGreenTexture[ 0 ] );
        else
        if ( updateTypeIn == mIdUpdateTypeApp )
            glBindTexture( GL_TEXTURE_2D, mAppTexture[ 0 ] );

        glBegin( GL_QUADS );
            glTexCoord2f( 0.0f, 0.0f ); glVertex3f( -1.0f, -1.0f,  0.0f );
            glTexCoord2f( 1.0f, 0.0f ); glVertex3f(  1.0f, -1.0f,  0.0f );
            glTexCoord2f( 1.0f, 1.0f ); glVertex3f(  1.0f,  1.0f,  0.0f );
            glTexCoord2f( 0.0f, 1.0f ); glVertex3f( -1.0f,  1.0f,  0.0f );
        glEnd();
    }
    else
    // a ball - just because you can :) draw my own ball since the glu/glut ones don't get me
    // enough control over the texture coordinate details
    if ( geomTypeIn == mIdGeomTypeBall )
    {
        glEnable( GL_TEXTURE_2D );
        glColor3f( 1.0f, 1.0f, 1.0f );

        if ( updateTypeIn == mIdUpdateTypeG )
            glBindTexture( GL_TEXTURE_2D, mGreenTexture[ 0 ] );
        else
        if ( updateTypeIn == mIdUpdateTypeApp )
            glBindTexture( GL_TEXTURE_2D, mAppTexture[ 0 ] );

        // this is based on some code from here: http://astronomy.swin.edu.au/~pbourke/opengl/sphere/
        const int numElems = 40;
        const GLfloat  piDiv2 = 1.57079632679489661923f;
        const GLfloat piMul2 = 6.28318530717958647692f;

        for( int j = 0; j < numElems / 2; ++j )
        {
            GLfloat theta1 = j * ( piMul2 ) / numElems - piDiv2;
            GLfloat theta2 = ( j + 1 ) * ( piMul2 ) / numElems - piDiv2;

            glBegin( GL_TRIANGLE_STRIP );

            for( int i = 0; i <= numElems; i++ )
            {
                GLfloat theta3 = i * piMul2 / numElems;

                GLfloat pointX = cos( theta2 ) * cos( theta3 );
                GLfloat pointY = sin( theta2 );
                GLfloat pointZ = cos( theta2 ) * sin( theta3 );

                glNormal3f( pointX, pointY, pointZ );
                glTexCoord2f( i / ( GLfloat )numElems, 2 * ( j + 1 ) / ( GLfloat )numElems );
                glVertex3f( pointX, pointY, pointZ );

                pointX = cos( theta1 ) * cos( theta3 );
                pointY = sin( theta1 );
                pointZ = cos( theta1 ) * sin( theta3 );

                glNormal3f( pointX, pointY, pointZ );
                glTexCoord2f( i / ( GLfloat )numElems, 2 * j / ( GLfloat )numElems);
                glVertex3f( pointX, pointY, pointZ );
            };
            glEnd();
        };
    }
    else
    // cube with a different browser on each face
    if ( geomTypeIn == mIdGeomTypeCube )
    {
        static GLfloat textureCoords[ MaxBrowsers * 8 ] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
                                                            1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                                                            0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                                                            0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
                                                            1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                                            0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };

        // vertex positions for each side
        static GLfloat vertexPos[ MaxBrowsers * 12 ] = { -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
                                                         -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
                                                         -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f,
                                                         -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f,
                                                         -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
                                                         1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f };

        // size multiplier for the cube
        const GLfloat delta = 0.75f;

        if ( updateTypeIn == mIdUpdateTypeRB )
        {
            glEnable( GL_TEXTURE_2D );
            glColor3f( 1.0f, 1.0f, 1.0f );

            glBegin( GL_QUADS );
            for( int each = 0, currentTC = 0, currentVP = 0; each < 6; ++each )
            {
                for( int corner = 0; corner < 4; ++corner )
                {
                    glTexCoord2f( textureCoords[ currentTC ], textureCoords[ currentTC + 1 ] );
                    currentTC += 2;

                    glVertex3f( vertexPos[ currentVP ] * delta, vertexPos[ currentVP + 1 ] * delta, vertexPos[ currentVP + 2 ] * delta );
                    currentVP += 3;
                };
            };
            glEnd();
        }
        else
        if ( updateTypeIn == mIdUpdateTypeG )
        {
            glEnable( GL_TEXTURE_2D );
            glColor3f( 1.0f, 1.0f, 1.0f );

            for( int each = 0, currentTC = 0, currentVP = 0; each < 6; ++each )
            {
                glBindTexture( GL_TEXTURE_2D, mGreenTexture[ each ] );
                glBegin( GL_QUADS );
                for( int corner = 0; corner < 4; ++corner )
                {
                    glTexCoord2f( textureCoords[ currentTC ], textureCoords[ currentTC + 1 ] );
                    currentTC += 2;

                    glVertex3f( vertexPos[ currentVP ] * delta, vertexPos[ currentVP + 1 ] * delta, vertexPos[ currentVP + 2 ] * delta );
                    currentVP += 3;
                };
                glEnd();
            };
        }
        if ( updateTypeIn == mIdUpdateTypeApp )
        {
            for( int each = 0, currentTC = 0, currentVP = 0, hilite = 0; each < 6; ++each )
            {
                glEnable( GL_TEXTURE_2D );
                glColor3f( 1.0f, 1.0f, 1.0f );
                glBindTexture( GL_TEXTURE_2D, mAppTexture[ each ] );
                glBegin( GL_QUADS );

                hilite = currentVP;

                for( int corner = 0; corner < 4; ++corner )
                {
                    glTexCoord2f( textureCoords[ currentTC ], textureCoords[ currentTC + 1 ] );
                    currentTC += 2;

                    glVertex3f( vertexPos[ currentVP ] * delta, vertexPos[ currentVP + 1 ] * delta, vertexPos[ currentVP + 2 ] * delta );
                    currentVP += 3;
                };
                glEnd();

                // draw highlight around selected face
                if ( mCurFace == each )
                {
                    glDisable( GL_TEXTURE_2D );
                    glColor3f( 0.0f, 0.0f, 0.6f );
                    glLineWidth( 2.0f );
                    glBegin( GL_LINE_LOOP );
                    for( int vert = 0; vert < 4; ++vert )
                    {
                        glVertex3f( vertexPos[ hilite + vert * 3 ] * delta, vertexPos[ hilite + vert * 3 + 1 ] * delta, vertexPos[ hilite + vert * 3 + 2 ] * delta );
                    };
                    glEnd();
                    glColor3f( 0.0f, 0.0f, 1.0f );
                    glLineWidth( 1.0f );
                    glBegin( GL_LINE_LOOP );
                    for( int vert = 0; vert < 4; ++vert )
                    {
                        glVertex3f( vertexPos[ hilite + vert * 3 ] * delta, vertexPos[ hilite + vert * 3 + 1 ] * delta, vertexPos[ hilite + vert * 3 + 2 ] * delta );
                    };
                    glEnd();
                };
            };
        };
    }
    else
    // waving flag - again - not useful but it looks great..
    // time is frame based which is bad but avoids the need for a cross platform timer.
    if ( geomTypeIn == mIdGeomTypeFlag )
    {
        // this is based on some code from here: http://nehe.gamedev.net
        const int resolution = 45;
        static GLfloat points[ resolution ][ resolution ][ 3 ];
        static int wiggle = 0;
        static bool init = false;

        if ( ! init )
        {
            for( int x = 0; x < resolution; x++ )
            {
                for( int y = 0; y < resolution; y++ )
                {
                    points[ x ][ y ][ 0 ] = ( GLfloat )( ( x / ( ( GLfloat )( resolution ) / 2.0f ) ) - 1.0f );
                    points[ x ][ y ][ 1 ] = ( GLfloat )( ( y / ( ( GLfloat )( resolution ) / 2.0f ) ) - 1.0f );
                    points[ x ][ y ][ 2 ] = ( GLfloat )( cos ( ( ( ( ( x + y ) / 5.0f ) * 40.0f ) / 360.0f ) * 3.141592654 * 2.0f ) * 0.1f );
                };
            };
            init = true;
        };

        glEnable( GL_TEXTURE_2D );
        glColor3f( 1.0f, 1.0f, 1.0f );

        if ( updateTypeIn == mIdUpdateTypeG )
            glBindTexture( GL_TEXTURE_2D, mGreenTexture[ 0 ] );
        else
        if ( updateTypeIn == mIdUpdateTypeApp )
            glBindTexture( GL_TEXTURE_2D, mAppTexture[ 0 ] );

        glBegin( GL_QUADS );
        for( int x = 0; x < resolution - 1; x++ )
        {
            for( int y = resolution - 2; y > -1; --y )
            {
                int xCoord1 = x;
                int yCoord1 = y;

                int xCoord2 = x;
                int yCoord2 = y + 1;

                int xCoord3 = x + 1;
                int yCoord3 = y + 1;

                int xCoord4 = x + 1;
                int yCoord4 = y;

                glTexCoord2f( ( GLfloat )( xCoord1 ) / ( GLfloat )( resolution - 1 ),
                    ( GLfloat )( yCoord1 ) / ( GLfloat )( resolution - 1 ) );
                glVertex3f( points[ xCoord1 ][ yCoord1 ][ 0 ], points[ xCoord1 ][ yCoord1 ][ 1 ], points[ xCoord1 ][ yCoord1 ][ 2 ] );

                glTexCoord2f( ( GLfloat )( xCoord2 ) / ( GLfloat )( resolution - 1 ),
                    ( GLfloat )( yCoord2 ) / ( GLfloat )( resolution - 1 ) );
                glVertex3f( points[ xCoord2 ][ yCoord2 ][ 0 ], points[ xCoord2 ][ yCoord2 ][ 1 ], points[ xCoord2 ][ yCoord2 ][ 2 ] );

                glTexCoord2f( ( GLfloat )( xCoord3 ) / ( GLfloat )( resolution - 1 ),
                    ( GLfloat )( yCoord3 ) / ( GLfloat )( resolution - 1 ) );
                glVertex3f( points[ xCoord3 ][ yCoord3 ][ 0 ], points[ xCoord3 ][ yCoord3 ][ 1 ], points[ xCoord3 ][ yCoord3 ][ 2 ] );

                glTexCoord2f( ( GLfloat )( xCoord4 ) / ( GLfloat )( resolution - 1 ),
                    ( GLfloat )( yCoord4 ) / ( GLfloat )( resolution - 1 ) );
                glVertex3f( points[ xCoord4 ][ yCoord4 ][ 0 ], points[ xCoord4 ][ yCoord4 ][ 1 ], points[ xCoord4 ][ yCoord4 ][ 2 ] );
            };
        };
        glEnd();

        // only update wiggle once at the end
        if ( updateTypeIn == mIdUpdateTypeApp )
        {
            if ( ++wiggle == 3 )
            {
                for( int y = 0; y < resolution; ++y )
                {
                    GLfloat swap = points[ 0 ][ y ][ 2 ];
                    for( int x = 0; x < ( resolution - 1 ); ++x )
                    {
                        points[ x ][ y ][ 2 ] = points[ x + 1 ][ y ][ 2 ];
                    };
                    points[ resolution - 1 ][ y ][ 2 ] =swap;
                };
                wiggle = 0;
            };
        };
    };
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::display()
{
    // let's start with a clean slate
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // scale all texture so that they fit the geometry exactly
    glMatrixMode( GL_TEXTURE );
    glPushMatrix();
    glScalef( mTextureScaleX, mTextureScaleY, 1.0f );

    // set up OpenGL view
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glFrustum( -mViewportAspect * 0.04f, mViewportAspect * 0.04f, -0.04f, 0.04f, 0.1f, 50.0f );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glTranslatef( 0.0, 0.0, -3.0f );
    glTranslatef( mViewPos[ 0 ], mViewPos[ 1 ], -mViewPos[ 2 ] );
    glMultMatrixf( mViewRotation );

    static GLfloat localRotX = 180.f;
    static GLfloat localRotY = 0.f;
    static GLfloat localRotZ = 0.f;

    if ( fabs( localRotX - mRotX ) > 0.1f  )
        localRotX += ( mRotX - localRotX ) / 25.0f;

    if ( fabs( localRotY - mRotY ) > 0.1f  )
        localRotY += ( mRotY - localRotY ) / 25.0f;

    if ( fabs( localRotZ - mRotZ ) > 0.1f  )
        localRotZ += ( mRotZ - localRotZ ) / 25.0f;

    glRotatef( localRotX, 1.0f, 0.0f, 0.0f );
    glRotatef( localRotY, 0.0f, 1.0f, 0.0f );
    glRotatef( localRotZ, 0.0f, 0.0f, 1.0f );
    glScalef( 1.0f, 1.0f, 1.0f );

    // red blue pattern
    glBindTexture( GL_TEXTURE_2D, mRedBlueTexture );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    drawGeometry( mCurObjType, mIdUpdateTypeRB );

    // read colors and get red/blue value
    glReadPixels( mCurMouseX, mCurMouseY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, mPixelColorRB );

    // green texture mask
    for( int i = 0; i < mNumBrowserWindows; ++i )
    {
        glBindTexture( GL_TEXTURE_2D, mGreenTexture[ i ] );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
        glMatrixMode( GL_TEXTURE );
        glPushMatrix();
        glScalef( mTextureScaleX * 256.0f, mTextureScaleX * 256.0f, 1.0f );   // scale the scale by the scale :)
        drawGeometry( mCurObjType, mIdUpdateTypeG );
        glPopMatrix();
        glMatrixMode( GL_MODELVIEW );
    };

    // read colors and get green value
    glReadPixels( mCurMouseX, mCurMouseY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, mPixelColorG );

    for( int i = 0; i < mNumBrowserWindows; ++i )
    {
        if ( mWindowId[ i ] )
        {
            if ( mNeedsUpdate[ i ] == 1)
            {
                const unsigned char* pixels = LLQtWebKit::getInstance()->getBrowserWindowPixels( mWindowId[ i ] );
                if ( pixels )
                {
                    glBindTexture( GL_TEXTURE_2D, mAppTexture[ i ] );
                    int rowSpan = LLQtWebKit::getInstance()->getBrowserRowSpan( mWindowId[ i ] );
                    int depth = LLQtWebKit::getInstance()->getBrowserDepth( mWindowId[ i ] );
                    int mBrowserWindowWidth = LLQtWebKit::getInstance()->getBrowserWidth( mWindowId[ i ] );
                    glTexSubImage2D( GL_TEXTURE_2D, 0,
                        0, 0,
                            // because sometimes the rowspan != width * bytes per pixel (mBrowserWindowWidth)
                            //rowSpan / depth,
                            mBrowserWindowWidth,
                                mBrowserWindowHeight,
                                    getGLTextureFormat(depth),
                                        GL_UNSIGNED_BYTE,
                                            pixels );
                }

                mNeedsUpdate[ i ] = 0;
            };
        };
    };

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

    // draw the browser texture
    drawGeometry( mCurObjType, mIdUpdateTypeApp );

    // restore matrix after texture scale
    glMatrixMode( GL_TEXTURE );
    glPopMatrix();

    glutSwapBuffers();
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::idle()
{
    LLQtWebKit::getInstance()->pump(100);
    // we need to grab the contents of the rendered page
    for( int i = 0; i < mNumBrowserWindows; ++i )
    {
        if ( mNeedsUpdate[ i ] == 2)
            LLQtWebKit::getInstance()->grabBrowserWindow( mWindowId[ i ] );
	    mNeedsUpdate[ i ] = 1;
        }

    // enable/disable back button depending on whether we can go back or not
	if ( LLQtWebKit::getInstance()->userActionIsEnabled( mCurWindowId, LLQtWebKit::UA_NAVIGATE_BACK ) )
        mNavBackButton->enable();
    else
        mNavBackButton->disable();

    // enable/disable back button depending on whether we can go back or not
    if ( LLQtWebKit::getInstance()->userActionIsEnabled( mCurWindowId, LLQtWebKit::UA_NAVIGATE_FORWARD ) )
        mNavForwardButton->enable();
    else
        mNavForwardButton->disable();

    // GLUI needs this
    if ( glutGetWindow() != mAppWindow )
        glutSetWindow( mAppWindow );

    // lots of updates for smooth motion
    glutPostRedisplay();
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::resetView()
{
    mViewRotationCtrl->reset();

    mRotX = 180.0f;
    mRotY = 0.0f;
    mRotZ = 0.0f;

    mViewScaleCtrl->set_x( 0.0f );
    mViewScaleCtrl->set_y( 0.0f );
    mViewScaleCtrl->set_z( 0.0f );

    mViewTranslationCtrl->set_x( 0.0f );
    mViewTranslationCtrl->set_y( 0.0f );
    mViewTranslationCtrl->set_z( 0.0f );
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::makeChrome()
{
    // top UI bar
    mTopGLUIWindow = GLUI_Master.create_glui_subwindow( mAppWindow, GLUI_SUBWINDOW_TOP );

    mNavBackButton = mTopGLUIWindow->add_button( "<<<", mIdNavBack, gluiCallbackWrapper );
    mNavBackButton->set_w( 56 );

    mTopGLUIWindow->add_column( false );
    GLUI_Button* button = mTopGLUIWindow->add_button( "STOP", mIdNavStop, gluiCallbackWrapper );
    button->set_w( 56 );

    mTopGLUIWindow->add_column( false );
    button = mTopGLUIWindow->add_button( "HOME", mIdNavHome, gluiCallbackWrapper );
    button->set_w( 56 );

    mTopGLUIWindow->add_column( false );
    mNavForwardButton = mTopGLUIWindow->add_button( ">>>", mIdNavForward, gluiCallbackWrapper );
    mNavForwardButton->set_w( 56 );

    mTopGLUIWindow->add_column( false );
    GLUI_Button* mNavReloadButton = mTopGLUIWindow->add_button( "Reload", mIdNavReload, gluiCallbackWrapper );
    mNavReloadButton->set_w( 56 );

    mTopGLUIWindow->add_column( false );
#ifdef LL_NEWER_GLUI
    mUrlEdit = mTopGLUIWindow->add_edittext( "Url:", GLUI_EDITTEXT_TEXT, mNavUrl, mIdUrlEdit, gluiCallbackWrapper );
#else // LL_NEWER_GLUI
    // Friendly to older GLUI versions.
    mUrlEdit = mTopGLUIWindow->add_edittext( "Url:", mNavUrl, mIdUrlEdit, gluiCallbackWrapper );
#endif // LL_NEWER_GLUI

    mTopGLUIWindow->set_main_gfx_window( mAppWindow );

    // top UI bar (second part)
    mTop2GLUIWindow = GLUI_Master.create_glui_subwindow( mAppWindow, GLUI_SUBWINDOW_TOP );

    mTop2GLUIWindow->add_column( false );
    GLUI_Listbox* bookmarkList = mTop2GLUIWindow->add_listbox( "Bookmarks", &mSelBookmark, mIdBookmarks, gluiCallbackWrapper );
    for( unsigned int each = 0; each < mBookmarks.size(); ++each )
    {
        bookmarkList->add_item( each, const_cast< char* >( mBookmarks[ each ].first.c_str() ) );
    };

	mTop2GLUIWindow->add_column( false );
#ifdef LL_NEWER_GLUI
	mUrlAddToHistoryEdit = mTop2GLUIWindow->add_edittext( "Add Url History item:", GLUI_EDITTEXT_TEXT, mNavUrl, mIdNavAddToHistory, gluiCallbackWrapper );
#else // LL_NEWER_GLUI
	// Friendly to older GLUI versions.
	mUrlAddToHistoryEdit = mTop2GLUIWindow->add_edittext( "Add Url History item:", mNavUrl, mIdNavAddToHistory, gluiCallbackWrapper );
#endif // LL_NEWER_GLU
    mTop2GLUIWindow->set_main_gfx_window( mAppWindow );

    // bottom UI bar
    mBottomGLUIWindow = GLUI_Master.create_glui_subwindow( mAppWindow, GLUI_SUBWINDOW_BOTTOM );

    mStatusText = mBottomGLUIWindow->add_statictext( "" );

    mBottomGLUIWindow->add_column( false );
    mProgressText = mBottomGLUIWindow->add_statictext( "" );
    mProgressText->set_alignment( GLUI_ALIGN_RIGHT );

    mBottomGLUIWindow->set_main_gfx_window( mAppWindow );

#if __APPLE__
	// the Apple GLUT implementation doesn't seem to set the graphic offset of subwindows correctly when they overlap in certain ways.
	// Use a separate controls window in this case.
	// GLUI window at right containing manipulation controls and other buttons
	int x = glutGet(GLUT_WINDOW_X) + glutGet(GLUT_WINDOW_WIDTH) + 4;
	int y = glutGet(GLUT_WINDOW_Y);
	mRightGLUIWindow = GLUI_Master.create_glui( "", 0, x, y );
#else
    // right side UI bar
    mRightGLUIWindow = GLUI_Master.create_glui_subwindow( mAppWindow, GLUI_SUBWINDOW_RIGHT );
#endif

    // object selector
    GLUI_Listbox* objTypelist = mRightGLUIWindow->add_listbox( "Type ", &mCurObjType );
    objTypelist->add_item( mIdGeomTypeCube, "Cube" );
    objTypelist->add_item( mIdGeomTypeFlat, "Flat" );
    objTypelist->add_item( mIdGeomTypeBall, "Ball" );
    objTypelist->add_item( mIdGeomTypeFlag, "Flag" );
    mRightGLUIWindow->add_statictext( "" );

    mViewRotationCtrl = mRightGLUIWindow->add_rotation( "Rotation", mViewRotation );

    mViewTranslationCtrl = mRightGLUIWindow->add_translation( "Translate", GLUI_TRANSLATION_XY, mViewPos );
    mViewTranslationCtrl->set_speed( 0.01f );

    mViewScaleCtrl = mRightGLUIWindow->add_translation( "Scale", GLUI_TRANSLATION_Z, &mViewPos[ 2 ] );
    mViewScaleCtrl->set_speed( 0.05f );

    mRightGLUIWindow->add_statictext( "" );

    mRightGLUIWindow->add_statictext( "" );
    button = mRightGLUIWindow->add_button( "Small", mIdBrowserSmall, gluiCallbackWrapper );
    button->set_w( 64 );

    button = mRightGLUIWindow->add_button( "Medium", mIdBrowserMedium, gluiCallbackWrapper );
    button->set_w( 64 );

    button = mRightGLUIWindow->add_button( "Large", mIdBrowserLarge, gluiCallbackWrapper );
    button->set_w( 64 );

    GLUI_StaticText* text = mRightGLUIWindow->add_statictext( "Browser resolution" );
    text->set_alignment( GLUI_ALIGN_CENTER );
    mRightGLUIWindow->add_statictext( "" );

    mRightGLUIWindow->add_statictext( "" );
    button = mRightGLUIWindow->add_button( "Front", mIdFace0, gluiCallbackWrapper );
    button->set_w( 64 );
    button = mRightGLUIWindow->add_button( "Back", mIdFace1, gluiCallbackWrapper );
    button->set_w( 64 );
    button = mRightGLUIWindow->add_button( "Left", mIdFace4, gluiCallbackWrapper );
    button->set_w( 64 );
    button = mRightGLUIWindow->add_button( "Right", mIdFace5, gluiCallbackWrapper );
    button->set_w( 64 );
    button = mRightGLUIWindow->add_button( "Top", mIdFace2, gluiCallbackWrapper );
    button->set_w( 64 );
    button = mRightGLUIWindow->add_button( "Bottom", mIdFace3, gluiCallbackWrapper );
    button->set_w( 64 );

    mRightGLUIWindow->add_statictext( "" );
    button = mRightGLUIWindow->add_button( "Clear Cookies", mIdClearCookies, gluiCallbackWrapper );
    button->set_w( 64 );

    mRightGLUIWindow->add_statictext( "" );
    button = mRightGLUIWindow->add_button( "Reset", mIdReset, gluiCallbackWrapper );
    button->set_w( 64 );

    mRightGLUIWindow->add_statictext( "" );
    button = mRightGLUIWindow->add_button( "Exit", mIdExit, gluiCallbackWrapper );
    button->set_w( 64 );

    mRightGLUIWindow->set_main_gfx_window( mAppWindow );
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::windowPosToTexturePos( int winXIn, int winYIn, int& texXOut, int& texYOut, int& faceOut )
{
    // this is how we convert from the color in the textures draw in the first 2 passes to an
    // XY location in the coordinate space of the texture
    mCurMouseX = winXIn;
    mCurMouseY = mWindowHeight - winYIn;    // opposite Ycoordinate systems..

    // red gives 0..255 on X and blue value gives 0.255 on Y
    // green divides each discrete values by 16 giving 256 * 16 (4096) resolution in each of X & Y
    texXOut = ( mPixelColorRB[ 0 ] * 16 + ( mPixelColorG[ 1 ] & 0x0f ) ) / ( 4096 / mTextureWidth );
    texYOut = ( mPixelColorRB[ 2 ] * 16 + ( mPixelColorG[ 1 ] >> 4 ) ) / ( 4096 / mTextureHeight  );

    //std::cout << "texture cursor at " << texXOut << " x " << texYOut << std::endl;

    // component 0 of mPixelColorG gives the face number 0..n
    faceOut = mPixelColorG[ 0 ];
}

// convert a GLUT keyboard modifier to an LLQtWebKit one
// (only valid in mouse and keyboard callbacks
static LLQtWebKit::EKeyboardModifier getLLQtWebKitKeyboardModifierCode()
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
void uBrowser::keyboard( unsigned char keyIn, int xIn, int yIn )
{
	if(keyIn == 127)
	{
		// Turn delete char into backspace
		keyIn = LLQtWebKit::KEY_BACKSPACE;
	}

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
		
	// send event to LLQtWebKit
	LLQtWebKit::getInstance()->keyboardEvent(mCurWindowId, LLQtWebKit::KE_KEY_DOWN, keyIn, text, getLLQtWebKitKeyboardModifierCode() );
	LLQtWebKit::getInstance()->keyboardEvent(mCurWindowId, LLQtWebKit::KE_KEY_UP, keyIn, text, getLLQtWebKitKeyboardModifierCode() );
};

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::specialKeyboard( int keyIn, int xIn, int yIn )
{
	uint32_t key = LLQtWebKit::KEY_NONE;
	
	switch(keyIn)
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
		keyboard(key, xIn, yIn);
	}
};

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::passiveMouse( int xIn, int yIn )
{
    // called when the mouse is moving and button isn't pressed
    int x, y, face;
    windowPosToTexturePos( xIn, yIn, x, y, face );

	// send event to LLQtWebKit
	LLQtWebKit::getInstance()->mouseEvent( mCurWindowId,
											LLQtWebKit::ME_MOUSE_MOVE,
												LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
													x, y,
														LLQtWebKit::KM_MODIFIER_NONE );
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::mouseButton( int button, int state, int xIn, int yIn )
{
    int x, y, face;
    windowPosToTexturePos( xIn, yIn, x, y, face );

    if ( button == GLUT_LEFT_BUTTON )
    {
        if ( state == GLUT_DOWN )
        {
			// send event to LLQtWebKit
			LLQtWebKit::getInstance()->mouseEvent( mCurWindowId,
													LLQtWebKit::ME_MOUSE_DOWN,
														LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
															x, y,
																getLLQtWebKitKeyboardModifierCode() );
        }
        else
        if ( state == GLUT_UP )
        {
            mCurWindowId = mWindowId[ face ];

            // record which face was clicked on
            mCurFace = face;

			// send event to LLQtWebKit
			LLQtWebKit::getInstance()->mouseEvent( mCurWindowId,
													LLQtWebKit::ME_MOUSE_UP,
														LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
															x, y,
																getLLQtWebKitKeyboardModifierCode() );

            // this seems better than sending focus on mouse down (still need to improve this)
            LLQtWebKit::getInstance()->focusBrowser( mCurWindowId, true );
        };
    }
    else
    if ( button == GLUT_RIGHT_BUTTON )
    {
        if ( state == GLUT_DOWN )
        {
			// send event to LLQtWebKit
			LLQtWebKit::getInstance()->mouseEvent( mCurWindowId,
													LLQtWebKit::ME_MOUSE_DOWN,
														LLQtWebKit::MB_MOUSE_BUTTON_RIGHT,
															x, y,
																getLLQtWebKitKeyboardModifierCode() );
        }
        else
        if ( state == GLUT_UP )
        {
			// send event to LLQtWebKit
			LLQtWebKit::getInstance()->mouseEvent( mCurWindowId,
													LLQtWebKit::ME_MOUSE_UP,
														LLQtWebKit::MB_MOUSE_BUTTON_RIGHT,
															x, y,
																getLLQtWebKitKeyboardModifierCode() );
        };

    };
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::mouseMove( int xIn , int yIn )
{
    // called when mouse moves and button is down
    int x, y, face;
    windowPosToTexturePos( xIn, yIn, x, y, face );

    mCurWindowId = mWindowId[ face ];

	// send event to LLQtWebKit
	LLQtWebKit::getInstance()->mouseEvent( mCurWindowId,
											LLQtWebKit::ME_MOUSE_MOVE,
												LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
													x, y,
														LLQtWebKit::KM_MODIFIER_NONE );
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::setSize( int indexIn, int widthIn , int heightIn )
{
    // tell the embedded browser that things changed
    LLQtWebKit::getInstance()->setSize( mWindowId[ indexIn ], widthIn, heightIn );

    // calculate the next power of 2 bigger than reqquested size for width and height
    for ( mTextureWidth = 1; mTextureWidth < widthIn; mTextureWidth <<= 1 )
    {
    };

    for ( mTextureHeight = 1; mTextureHeight < heightIn; mTextureHeight <<= 1 )
    {
    };

    // save the height and width
    mBrowserWindowWidth = widthIn;
    mBrowserWindowHeight = heightIn;

    // we scale all textures by this much so that they fit the geometry
    mTextureScaleX = ( GLfloat )mBrowserWindowWidth / ( GLfloat )mTextureWidth;
    mTextureScaleY = ( GLfloat )mBrowserWindowHeight / ( GLfloat )mTextureHeight;

    // delete the old texture handle and create a new one
    if ( mAppTexture[ indexIn ] )
        glDeleteTextures( 1, &mAppTexture[ indexIn ] );
    glGenTextures( 1, &mAppTexture[ indexIn ] );
    glBindTexture( GL_TEXTURE_2D, mAppTexture[ indexIn ] );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, mTextureWidth, mTextureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::gluiCallback( int controlIdIn )
{
    if ( controlIdIn == mIdExit )
    {
        // clean up code - since GLUT doesn't let us exit the event loop, we'll do it here
        // to make sure it works (deleting the browser instance was a major problem before)
        for( int i = 0; i < mNumBrowserWindows; ++i )
        {
            LLQtWebKit::getInstance()->remObserver( mWindowId[ i ], this );
        };
        LLQtWebKit::getInstance()->reset();

        // write something out to the GLUT console to indicate we're all done
        std::cout << "Application finished. Buh-bye!" << std::endl;

        // only way out with GLUT
        exit( 0 );
    }
    else
    if ( controlIdIn == mIdReset )
    {
        if ( glutGetWindow() != mAppWindow )
            glutSetWindow( mAppWindow );

        resetView();

        glutPostRedisplay();
    }
    else
    if ( controlIdIn == mIdClearCookies )
    {
        LLQtWebKit::getInstance()->clearAllCookies();

        glutPostRedisplay();
    }
    else
    if ( controlIdIn == mIdBrowserSmall )
    {
        for( int i = 0; i < mNumBrowserWindows; ++i )
        {
            setSize( i, 400, 400 );
        };

        if ( glutGetWindow() != mAppWindow )
            glutSetWindow( mAppWindow );

        glutPostRedisplay();
    }
    else
    if ( controlIdIn == mIdBrowserMedium )
    {
        for( int i = 0; i < mNumBrowserWindows; ++i )
        {
            setSize( i, 800, 800 );
        };

        if ( glutGetWindow() != mAppWindow )
            glutSetWindow( mAppWindow );

        glutPostRedisplay();
    }
    else
    if ( controlIdIn == mIdBrowserLarge )
    {
        for( int i = 0; i < mNumBrowserWindows; ++i )
        {
            setSize( i, 1024, 1024 );
        };

        if ( glutGetWindow() != mAppWindow )
            glutSetWindow( mAppWindow );

        glutPostRedisplay();
    }
    else
    if ( controlIdIn == mIdNavBack )
    {
		LLQtWebKit::getInstance()->userAction( mCurWindowId, LLQtWebKit::UA_NAVIGATE_BACK );
    }
    else
    if ( controlIdIn == mIdNavStop )
    {
        LLQtWebKit::getInstance()->userAction( mCurWindowId, LLQtWebKit::UA_NAVIGATE_STOP );
    }
    else
    if ( controlIdIn == mIdNavHome )
    {
        LLQtWebKit::getInstance()->navigateTo( mCurWindowId, mHomeUrl[ mCurFace ].c_str() );
    }
    else
    if ( controlIdIn == mIdNavForward )
    {
        LLQtWebKit::getInstance()->userAction( mCurWindowId, LLQtWebKit::UA_NAVIGATE_FORWARD );
    }
    else
    if ( controlIdIn == mIdNavReload )
    {
        LLQtWebKit::getInstance()->userAction( mCurWindowId, LLQtWebKit::UA_NAVIGATE_RELOAD );
    }
	else
	if ( controlIdIn == mIdNavAddToHistory )
	{
		LLQtWebKit::getInstance()->prependHistoryUrl( mCurWindowId, mUrlAddToHistoryEdit->get_text() );
		mUrlAddToHistoryEdit->set_text("");
		const std::string &foo = LLQtWebKit::getInstance()->dumpHistory(mCurWindowId);
		std::cout << foo << "\n";
    }
    else
    if ( controlIdIn == mIdUrlEdit )
    {
        LLQtWebKit::getInstance()->navigateTo( mCurWindowId, mUrlEdit->get_text() );
    }
    else
    if ( controlIdIn == mIdBookmarks )
    {
        LLQtWebKit::getInstance()->navigateTo( mCurWindowId, mBookmarks[ mSelBookmark ].second.c_str() );
    }
    else
    // implies the ids are sequential.... ;)
    if ( controlIdIn >= mIdFace0  && controlIdIn <= mIdFace5 )
    {
        static GLfloat rots[] = { 180.0f,0.0f,0.0f, 180.0f,180.0f,0.0f, 270.0f,00.0f,0.0f, 90.0f,0.0f,0.0f, 180.0f,-90.0f,0.0f, 180.0f,90.0f,0.0f };

        mViewRotationCtrl->reset();
        mCurWindowId = mWindowId[ controlIdIn - mIdFace0 ];
        mCurFace = controlIdIn - mIdFace0;

        mRotX = rots[ ( controlIdIn - mIdFace0 ) * 3 + 0 ];
        mRotY = rots[ ( controlIdIn - mIdFace0 ) * 3 + 1 ];
        mRotZ = rots[ ( controlIdIn - mIdFace0 ) * 3 + 2 ];
    };
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::onNavigateBegin( const EventType& eventIn )
{
    // could do something here like start a throbber :)
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::onClickLinkHref( const EventType& eventIn )
{
    // this is fired when you click on a link in the browser
    std::cout << "onClickLinkHref fired with a link of " << eventIn.getStringValue() << " and a tartget of " << eventIn.getStringValue2() << std::endl;
};

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::onClickLinkNoFollow( const EventType& eventIn )
{
    std::cout << "onClickLinkNoFollow fired with value of " << eventIn.getStringValue() << std::endl;
};

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::onNavigateComplete( const EventType& eventIn )
{
    // this test calls a Javascript function in the page contents
    //std::string myScript = "hello();";
    //std::string elementContent = LLQtWebKit::getInstance()->evaluateJavascript( mCurWindowId, myScript );
    //std::cout << "navigate complete: elemnent content = " << elementContent << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::onUpdateProgress( const EventType& eventIn )
{
    // observed event - page progress changes
    glutSetWindow( mBottomGLUIWindow->get_glut_window_id() );

    std::ostringstream conv;
    conv << "[" << eventIn.getIntValue() << "% loaded]";
    mProgressText->set_text( const_cast< char* >( conv.str().c_str() ) );
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::onStatusTextChange( const EventType& eventIn )
{
    // observed event - status text changes, either during load or when you mouse over a link or a script does it
    glutSetWindow( mBottomGLUIWindow->get_glut_window_id() );

    // NOTE: only display the first 100 chars since anything longer breaks the display of percent loaded
    // on the right hand side - normally, you'd want to use the whole thing.
    mStatusText->set_text( const_cast< char*>( eventIn.getStringValue().substr( 0, 100 ).c_str() ) );
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::onLocationChange( const EventType& eventIn )
{
    // observed event - URL location changes - e.g. when a site redirects somewhere else
    // (ought to check that this is the top frame or this will be wrong)
    glutSetWindow( mTopGLUIWindow->get_glut_window_id() );

    // only set the text for the window that is currently selected
    if ( mCurWindowId == eventIn.getEventWindowId() )
    {
        mUrlEdit->set_text( const_cast< char*>( eventIn.getEventUri().c_str() ) );
    };
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::onPageChanged( const EventType& eventIn )
{
    // example of how to grab the location of the dirty rectangle even though it's not used yet
    int x, y, width, height;
    eventIn.getRectValue( x, y, width, height );

    // flag that an update is required - page grab happens in idle() so we don't stall
    for( int i = 0; i < mNumBrowserWindows; ++i )
    {
        if ( mWindowId[ i ] == eventIn.getEventWindowId() )
        {
            mNeedsUpdate[ i ] = 2;
        };
    };
}

////////////////////////////////////////////////////////////////////////////////
//
void* uBrowser::getNativeWindowHandle()
{
    // My implementation of the embedded browser needs a native window handle
    // Can't get this via GLUT so had to use this hack
#ifdef _WINDOWS

    return FindWindow( NULL, (LPCWSTR)mName.c_str() );

#elif defined(__APPLE__)
    // The libraries need a "native window ref" for some reason.  They don't care which window it is, though.
    // The GLUT window doesn't have a Carbon WindowRef, because it's a Cocoa window.  This code will create a window just to pacify
    // the libraries.
    static WindowRef dummyWindow = NULL;
    if(dummyWindow == NULL)
    {
        // Create a window just for this purpose.
        Rect window_rect = {100, 100, 200, 200};

        dummyWindow = NewCWindow(
            NULL,
            &window_rect,
            (const unsigned char*)"\p",
            false,                // Create the window invisible.
            zoomDocProc,        // Window with a grow box and a zoom box
            kLastWindowOfClass,        // create it behind other windows
            false,                    // no close box
            0);
    }
    return (void*)(dummyWindow);

#elif defined(LL_LINUX)
    gtk_disable_setlocale();
    gtk_init(NULL, NULL);

    GtkWidget *win = gtk_window_new(GTK_WINDOW_POPUP);
    // Why a layout widget?  A MozContainer would be ideal, but
    // it involves exposing headers to mozlib-using apps.
    // A layout widget with a GtkWindow parent has the desired
    // properties of being plain GTK, having a window, and being
    // derived from a GtkContainer.
    GtkWidget *rtnw = gtk_layout_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(win), rtnw);
    gtk_widget_realize(rtnw);
    GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(rtnw), GTK_NO_WINDOW);

    return rtnw;
#else
#error uBrowser::getNativeWindowHandle() needs an implementation for this platform!
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
void uBrowser::setFocusNativeWindow()
{
    // need to set focus to the browser window so that keyboard events work
#ifdef _WINDOWS
SetFocus( FindWindow( NULL, (LPCWSTR)mName.c_str() ) );
#elif defined(__APPLE__)
// Don't think this is necessary on the Mac...
#elif defined(LL_LINUX)
// It doesn't really works like this on Linux...
#else
    #error uBrowser::setFocusNativeWindow() needs an implementation for this platform!
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
GLenum uBrowser::getGLTextureFormat(int size)
{
    return GL_RGBA;
}

////////////////////////////////////////////////////////////////////////////////
//
static void gluiCallbackWrapper( int controlIdIn )
{
    // dispatch GLUI callbacks into my class cos I like it that way :)
    if ( gInstance )
        gInstance->gluiCallback( controlIdIn );
}

