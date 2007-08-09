#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <assert.h>

#include "Fontdefs.h"
#include "FTGLBitmapFont.h"

extern void buildGLContext();

class FTGLBitmapFontTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTGLBitmapFontTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testRender);
        CPPUNIT_TEST( testPenPosition);
        CPPUNIT_TEST( testDisplayList);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTGLBitmapFontTest() : CppUnit::TestCase( "FTGLBitmapFont Test")
        {
        }
        
        FTGLBitmapFontTest( const std::string& name) : CppUnit::TestCase(name) {}
        
        ~FTGLBitmapFontTest()
        {
        }
        
        void testConstructor()
        {
            buildGLContext();
        
            FTGLBitmapFont* bitmapFont = new FTGLBitmapFont( FONT_FILE);            
            CPPUNIT_ASSERT( bitmapFont->Error() == 0);
        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        
        }

        void testRender()
        {
            buildGLContext();

            FTGLBitmapFont* bitmapFont = new FTGLBitmapFont( FONT_FILE);            
            bitmapFont->Render(GOOD_ASCII_TEST_STRING);

            CPPUNIT_ASSERT( bitmapFont->Error() == 0);        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        

            bitmapFont->FaceSize(18);
            bitmapFont->Render(GOOD_ASCII_TEST_STRING);

            CPPUNIT_ASSERT( bitmapFont->Error() == 0);        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        
        }
        
        
        void testPenPosition()
        {
            buildGLContext();
            float rasterPosition[4];
            
            glRasterPos2f(0.0f,0.0f);
            
            glGetFloatv(GL_CURRENT_RASTER_POSITION, rasterPosition);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, rasterPosition[0], 0.01);
            
            FTGLBitmapFont* bitmapFont = new FTGLBitmapFont( FONT_FILE);            
            bitmapFont->FaceSize(18);

            bitmapFont->Render(GOOD_ASCII_TEST_STRING);
            bitmapFont->Render(GOOD_ASCII_TEST_STRING);

            glGetFloatv(GL_CURRENT_RASTER_POSITION, rasterPosition);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 122, rasterPosition[0], 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, rasterPosition[1], 0.01);
        }


        void testDisplayList()
        {
            buildGLContext();
        
            FTGLBitmapFont* bitmapFont = new FTGLBitmapFont( FONT_FILE);            
            bitmapFont->FaceSize(18);
            
            int glList = glGenLists(1);
            glNewList( glList, GL_COMPILE);

                bitmapFont->Render(GOOD_ASCII_TEST_STRING);

            glEndList();

            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);
        }
        
        void setUp() 
        {}
        
        void tearDown() 
        {}
                    
    private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTGLBitmapFontTest);

