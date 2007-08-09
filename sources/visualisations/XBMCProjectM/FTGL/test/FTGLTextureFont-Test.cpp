#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <assert.h>

#include "Fontdefs.h"
#include "FTGLTextureFont.h"

extern void buildGLContext();

class FTGLTextureFontTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTGLTextureFontTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testResizeBug);
        CPPUNIT_TEST( testRender);
        CPPUNIT_TEST( testDisplayList);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTGLTextureFontTest() : CppUnit::TestCase( "FTGLTextureFontTest Test")
        {
        }
        
        FTGLTextureFontTest( const std::string& name) : CppUnit::TestCase(name) {}
        
        ~FTGLTextureFontTest()
        {
        }
        
        void testConstructor()
        {
            buildGLContext();
        
            FTGLTextureFont* textureFont = new FTGLTextureFont( FONT_FILE);            
            CPPUNIT_ASSERT( textureFont->Error() == 0);            
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        
        }

        void testResizeBug()
        {
            buildGLContext();
        
            FTGLTextureFont* textureFont = new FTGLTextureFont( FONT_FILE);            
            CPPUNIT_ASSERT( textureFont->Error() == 0);
            
            textureFont->FaceSize(18);
            textureFont->Render("first");

            textureFont->FaceSize(38);
            textureFont->Render("second");
            
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        
        }

        void testRender()
        {
            buildGLContext();
        
            FTGLTextureFont* textureFont = new FTGLTextureFont( FONT_FILE);            

            textureFont->Render(GOOD_ASCII_TEST_STRING);
            CPPUNIT_ASSERT( textureFont->Error() == 0x97);   // Invalid pixels per em       
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        

            textureFont->FaceSize(18);
            textureFont->Render(GOOD_ASCII_TEST_STRING);

            CPPUNIT_ASSERT( textureFont->Error() == 0);        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        
        }

        void testDisplayList()
        {
            buildGLContext();
        
            FTGLTextureFont* textureFont = new FTGLTextureFont( FONT_FILE);            
            textureFont->FaceSize(18);
            
            int glList = glGenLists(1);
            glNewList( glList, GL_COMPILE);

                textureFont->Render(GOOD_ASCII_TEST_STRING);

            glEndList();

            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);
        }
        
        void setUp() 
        {}
        
        void tearDown() 
        {}
                    
    private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTGLTextureFontTest);

