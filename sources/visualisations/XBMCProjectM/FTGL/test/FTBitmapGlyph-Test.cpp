#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <assert.h>

#include "Fontdefs.h"
#include "FTBitmapGlyph.h"

class FTBitmapGlyphTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTBitmapGlyphTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testRender);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTBitmapGlyphTest() : CppUnit::TestCase( "FTBitmapGlyph Test")
        {
        }
        
        FTBitmapGlyphTest( const std::string& name) : CppUnit::TestCase(name) {}
        
        ~FTBitmapGlyphTest()
        {
        }
        
        void testConstructor()
        {
            setUpFreetype();
            
            FTBitmapGlyph* bitmapGlyph = new FTBitmapGlyph( face->glyph);            
            CPPUNIT_ASSERT( bitmapGlyph->Error() == 0);
        
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        

            tearDownFreetype();
        }

        void testRender()
        {
            setUpFreetype();
            
            FTBitmapGlyph* bitmapGlyph = new FTBitmapGlyph( face->glyph);            
            CPPUNIT_ASSERT( bitmapGlyph->Error() == 0);
            bitmapGlyph->Render(FTPoint( 0, 0, 0));
            
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

CPPUNIT_TEST_SUITE_REGISTRATION( FTBitmapGlyphTest);

