#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <assert.h>

#include "Fontdefs.h"
#include "FTGLExtrdFont.h"

extern void buildGLContext();

class FTGLExtrdFontTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTGLExtrdFontTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testRender);
        CPPUNIT_TEST( testBadDisplayList);
        CPPUNIT_TEST( testGoodDisplayList);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTGLExtrdFontTest() : CppUnit::TestCase( "FTGLExtrdFont Test")
        {
        }
        
        FTGLExtrdFontTest( const std::string& name) : CppUnit::TestCase(name) {}
        
        ~FTGLExtrdFontTest()
        {
        }
        
        void testConstructor()
        {
            buildGLContext();
        
            FTGLExtrdFont* extrudedFont = new FTGLExtrdFont( FONT_FILE);            
            CPPUNIT_ASSERT( extrudedFont->Error() == 0);
        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        
        }

        void testRender()
        {
            buildGLContext();
        
            FTGLExtrdFont* extrudedFont = new FTGLExtrdFont( FONT_FILE);            
            extrudedFont->Render(GOOD_ASCII_TEST_STRING);

            CPPUNIT_ASSERT( extrudedFont->Error() == 0x97);   // Invalid pixels per em
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        

            extrudedFont->FaceSize(18);
            extrudedFont->Render(GOOD_ASCII_TEST_STRING);

            CPPUNIT_ASSERT( extrudedFont->Error() == 0);        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        
        }
        
        void testBadDisplayList()
        {
            buildGLContext();
        
            FTGLExtrdFont* extrudedFont = new FTGLExtrdFont( FONT_FILE);
            extrudedFont->FaceSize(18);
            
            int glList = glGenLists(1);
            glNewList( glList, GL_COMPILE);

                extrudedFont->Render(GOOD_ASCII_TEST_STRING);

            glEndList();

            CPPUNIT_ASSERT( glGetError() == GL_INVALID_OPERATION);
        }
        
        void testGoodDisplayList()
        {
            buildGLContext();
        
            FTGLExtrdFont* extrudedFont = new FTGLExtrdFont( FONT_FILE);
            extrudedFont->FaceSize(18);

            extrudedFont->UseDisplayList(false);
            int glList = glGenLists(1);
            glNewList( glList, GL_COMPILE);

                extrudedFont->Render(GOOD_ASCII_TEST_STRING);

            glEndList();

            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);
        }
        
        void setUp() 
        {}
        
        void tearDown() 
        {}
                    
    private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTGLExtrdFontTest);

