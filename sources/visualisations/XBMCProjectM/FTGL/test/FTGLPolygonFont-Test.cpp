#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <assert.h>

#include "Fontdefs.h"
#include "FTGLPolygonFont.h"

extern void buildGLContext();

class FTGLPolygonFontTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTGLPolygonFontTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testRender);
        CPPUNIT_TEST( testBadDisplayList);
        CPPUNIT_TEST( testGoodDisplayList);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTGLPolygonFontTest() : CppUnit::TestCase( "FTGLPolygonFont Test")
        {
        }
        
        FTGLPolygonFontTest( const std::string& name) : CppUnit::TestCase(name) {}
        
        ~FTGLPolygonFontTest()
        {
        }
        
        void testConstructor()
        {
            buildGLContext();
        
            FTGLPolygonFont* polygonFont = new FTGLPolygonFont( FONT_FILE);            
            CPPUNIT_ASSERT( polygonFont->Error() == 0);
        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        
        }

        void testRender()
        {
            buildGLContext();
        
            FTGLPolygonFont* polygonFont = new FTGLPolygonFont( FONT_FILE);            

            polygonFont->Render(GOOD_ASCII_TEST_STRING);

            CPPUNIT_ASSERT( polygonFont->Error() == 0x97);   // Invalid pixels per em       
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        

            polygonFont->FaceSize(18);
            polygonFont->Render(GOOD_ASCII_TEST_STRING);

            CPPUNIT_ASSERT( polygonFont->Error() == 0);        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        
        }

        void testBadDisplayList()
        {
            buildGLContext();
        
            FTGLPolygonFont* polygonFont = new FTGLPolygonFont( FONT_FILE);            
            polygonFont->FaceSize(18);
            
            int glList = glGenLists(1);
            glNewList( glList, GL_COMPILE);

                polygonFont->Render(GOOD_ASCII_TEST_STRING);

            glEndList();

            CPPUNIT_ASSERT( glGetError() == GL_INVALID_OPERATION);
        }
        
        void testGoodDisplayList()
        {
            buildGLContext();
        
            FTGLPolygonFont* polygonFont = new FTGLPolygonFont( FONT_FILE);            
            polygonFont->FaceSize(18);

            polygonFont->UseDisplayList(false);
            int glList = glGenLists(1);
            glNewList( glList, GL_COMPILE);

                polygonFont->Render(GOOD_ASCII_TEST_STRING);

            glEndList();

            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);
        }
        
        void setUp() 
        {}
        
        void tearDown() 
        {}
                    
    private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTGLPolygonFontTest);

