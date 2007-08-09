#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <assert.h>

#include "Fontdefs.h"
#include "FTGLOutlineFont.h"

extern void buildGLContext();

class FTGLOutlineFontTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTGLOutlineFontTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testRender);
        CPPUNIT_TEST( testBadDisplayList);
        CPPUNIT_TEST( testGoodDisplayList);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTGLOutlineFontTest() : CppUnit::TestCase( "FTGLOutlineFont Test")
        {
        }
        
        FTGLOutlineFontTest( const std::string& name) : CppUnit::TestCase(name) {}
        
        ~FTGLOutlineFontTest()
        {
        }
        
        void testConstructor()
        {
            buildGLContext();
        
            FTGLOutlineFont* outlineFont = new FTGLOutlineFont( FONT_FILE);            
            CPPUNIT_ASSERT( outlineFont->Error() == 0);
        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        
        }
        
        void testRender()
        {
            buildGLContext();
        
            FTGLOutlineFont* outlineFont = new FTGLOutlineFont( FONT_FILE);            
            outlineFont->Render(GOOD_ASCII_TEST_STRING);

            CPPUNIT_ASSERT( outlineFont->Error() == 0x97);   // Invalid pixels per em
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        

            outlineFont->FaceSize(18);
            outlineFont->Render(GOOD_ASCII_TEST_STRING);

            CPPUNIT_ASSERT( outlineFont->Error() == 0);        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        
        }

        void testBadDisplayList()
        {
            buildGLContext();
        
            FTGLOutlineFont* outlineFont = new FTGLOutlineFont( FONT_FILE);            
            outlineFont->FaceSize(18);
            
            int glList = glGenLists(1);
            glNewList( glList, GL_COMPILE);

                outlineFont->Render(GOOD_ASCII_TEST_STRING);

            glEndList();

            CPPUNIT_ASSERT( glGetError() == GL_INVALID_OPERATION);
        }
        
        void testGoodDisplayList()
        {
            buildGLContext();
        
            FTGLOutlineFont* outlineFont = new FTGLOutlineFont( FONT_FILE);            
            outlineFont->FaceSize(18);

            outlineFont->UseDisplayList(false);
            int glList = glGenLists(1);
            glNewList( glList, GL_COMPILE);

                outlineFont->Render(GOOD_ASCII_TEST_STRING);

            glEndList();

            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);
        }
        
        void setUp() 
        {}
        
        void tearDown() 
        {}
                    
    private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTGLOutlineFontTest);

