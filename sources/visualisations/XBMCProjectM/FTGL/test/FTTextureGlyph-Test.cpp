#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <assert.h>

#include "Fontdefs.h"
#include "FTTextureGlyph.h"

extern void buildGLContext();

class FTTextureGlyphTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTTextureGlyphTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testRender);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTTextureGlyphTest() : CppUnit::TestCase( "FTTextureGlyph Test")
        {
        }
        
        FTTextureGlyphTest( const std::string& name) : CppUnit::TestCase(name) {}
        
        ~FTTextureGlyphTest()
        {
        }
        
        void testConstructor()
        {
            setUpFreetype();
            
            buildGLContext();
            
            GLuint textureID;
            glGenTextures(1, &textureID);
            
            char* texture[64*64];
        
            glBindTexture( GL_TEXTURE_2D, textureID);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, 64, 64, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texture);
    
            FTTextureGlyph* textureGlyph = new FTTextureGlyph( face->glyph, textureID, 0, 0, 64, 64);
         
            CPPUNIT_ASSERT( textureGlyph->Error() == 0);
            CPPUNIT_ASSERT( glGetError() == GL_NO_ERROR);        

            tearDownFreetype();
        }

        void testRender()
        {
            setUpFreetype();
            
            buildGLContext();
            
            GLuint textureID;
            glGenTextures(1, &textureID);
            
            char* texture[64*64];
        
            glBindTexture( GL_TEXTURE_2D, textureID);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, 64, 64, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texture);
    
            FTTextureGlyph* textureGlyph = new FTTextureGlyph( face->glyph, textureID, 0, 0, 64, 64);
            textureGlyph->Render(FTPoint( 0, 0, 0));
            CPPUNIT_ASSERT( textureGlyph->Error() == 0);
        
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

CPPUNIT_TEST_SUITE_REGISTRATION( FTTextureGlyphTest);

