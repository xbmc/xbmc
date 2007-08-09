#include <iostream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <assert.h>

#include "Fontdefs.h"
#include "FTPixmapGlyph.h"

#define GL_ASSERT() {GLenum sci_err; while ((sci_err = glGetError()) != GL_NO_ERROR) \
        std::cerr << "OpenGL error: " << (char *)gluErrorString(sci_err) << " at " << __FILE__ <<":" << __LINE__ << std::endl;}


extern void buildGLContext();

class FTPixmapGlyphTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTPixmapGlyphTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testRender);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTPixmapGlyphTest() : CppUnit::TestCase( "FTPixmapGlyph Test")
        {
        }
        
        FTPixmapGlyphTest( const std::string& name) : CppUnit::TestCase(name) {}
        
        ~FTPixmapGlyphTest()
        {
        }
        
        void testConstructor()
        {
            setUpFreetype();
            
            buildGLContext();
        
            FTPixmapGlyph* PixmapGlyph = new FTPixmapGlyph( face->glyph);            
            CPPUNIT_ASSERT( PixmapGlyph->Error() == 0);
        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        

            tearDownFreetype();
        }

        void testRender()
        {
            setUpFreetype();
            
            buildGLContext();
        
            FTPixmapGlyph* pixmapGlyph = new FTPixmapGlyph( face->glyph);            
            CPPUNIT_ASSERT( pixmapGlyph->Error() == 0);
            pixmapGlyph->Render(FTPoint( 0, 0, 0));
            
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        

            tearDownFreetype();
        }

        void setUp() 
        {}
        
        void tearDown() 
        {}
                    
    private:
        FT_Library   library;
        FT_Face      face;
            
        void setUpFreetype()
        {
            FT_Error error = FT_Init_FreeType( &library);
            assert(!error);
            error = FT_New_Face( library, FONT_FILE, 0, &face);
            assert(!error);
            
            FT_Set_Char_Size( face, 0L, FONT_POINT_SIZE * 64, RESOLUTION, RESOLUTION);
            
            error = FT_Load_Char( face, CHARACTER_CODE_A, FT_LOAD_DEFAULT);
            assert( !error);        
        }
        
        void tearDownFreetype()
        {
            FT_Done_Face( face);
            FT_Done_FreeType( library);
        }

};

CPPUNIT_TEST_SUITE_REGISTRATION( FTPixmapGlyphTest);

