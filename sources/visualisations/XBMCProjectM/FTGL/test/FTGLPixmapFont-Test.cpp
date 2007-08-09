#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <assert.h>

#include "Fontdefs.h"
#include "FTGLPixmapFont.h"

extern void buildGLContext();

class FTGLPixmapFontTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTGLPixmapFontTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testRender);
        CPPUNIT_TEST( testDisplayList);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTGLPixmapFontTest() : CppUnit::TestCase( "FTGLPixmapFont Test")
        {
        }
        
        FTGLPixmapFontTest( const std::string& name) : CppUnit::TestCase(name) {}
        
        ~FTGLPixmapFontTest()
        {
        }
        
        void testConstructor()
        {
            buildGLContext();
        
            FTGLPixmapFont* pixmapFont = new FTGLPixmapFont( FONT_FILE);            
            CPPUNIT_ASSERT( pixmapFont->Error() == 0);
        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        
        }

        void testRender()
        {
            buildGLContext();
            
            FTGLPixmapFont* pixmapFont = new FTGLPixmapFont( FONT_FILE);            

            pixmapFont->Render(GOOD_ASCII_TEST_STRING);
            CPPUNIT_ASSERT( pixmapFont->Error() == 0x97);   // Invalid pixels per em       
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        

            pixmapFont->FaceSize(18);
            pixmapFont->Render(GOOD_ASCII_TEST_STRING);

            CPPUNIT_ASSERT( pixmapFont->Error() == 0);        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        
        }

        void testDisplayList()
        {
            buildGLContext();
        
            FTGLPixmapFont* pixmapFont = new FTGLPixmapFont( FONT_FILE);            
            pixmapFont->FaceSize(18);
            
            int glList = glGenLists(1);
            glNewList( glList, GL_COMPILE);

                pixmapFont->Render(GOOD_ASCII_TEST_STRING);

            glEndList();

            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);
        }
        
        void setUp() 
        {}
        
        void tearDown() 
        {}
                    
    private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTGLPixmapFontTest);

