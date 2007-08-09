#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <assert.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "Fontdefs.h"
#include "FTBBox.h"


class FTBBoxTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTBBoxTest);
        CPPUNIT_TEST( testDefaultConstructor);
        CPPUNIT_TEST( testGlyphConstructor);
        CPPUNIT_TEST( testMoveBBox);
        CPPUNIT_TEST( testPlusEquals);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTBBoxTest() : CppUnit::TestCase( "FTBBox Test")
        {}
        
        FTBBoxTest( const std::string& name) : CppUnit::TestCase(name) {}

        void testDefaultConstructor()
        {
            FTBBox boundingBox;

            CPPUNIT_ASSERT( boundingBox.lowerX == 0.0f);
            CPPUNIT_ASSERT( boundingBox.lowerY == 0.0f);
            CPPUNIT_ASSERT( boundingBox.lowerZ == 0.0f);
            CPPUNIT_ASSERT( boundingBox.upperX == 0.0f);
            CPPUNIT_ASSERT( boundingBox.upperY == 0.0f);
            CPPUNIT_ASSERT( boundingBox.upperZ == 0.0f);
        }
        
        
        void testGlyphConstructor()
        {    
            setUpFreetype();

            FTBBox boundingBox( face->glyph);

            CPPUNIT_ASSERT_DOUBLES_EQUAL(   2, boundingBox.lowerX, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( -15, boundingBox.lowerY, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(   0, boundingBox.lowerZ, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  35, boundingBox.upperX, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  38, boundingBox.upperY, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(   0, boundingBox.upperZ, 0.01);

            tearDownFreetype();
        }     

        void testMoveBBox()
        {
            FTBBox  boundingBox;
            FTPoint firstMove( 3.5f, 1.0f, -2.5f);
            FTPoint secondMove( -3.5f, -1.0f, 2.5f);
            
            boundingBox.Move( firstMove);
        
            CPPUNIT_ASSERT( boundingBox.lowerX ==  3.5f);
            CPPUNIT_ASSERT( boundingBox.lowerY ==  1.0f);
            CPPUNIT_ASSERT( boundingBox.lowerZ == -2.5f);
            CPPUNIT_ASSERT( boundingBox.upperX ==  3.5f);
            CPPUNIT_ASSERT( boundingBox.upperY ==  1.0f);
            CPPUNIT_ASSERT( boundingBox.upperZ == -2.5f);

            boundingBox.Move( secondMove);
        
            CPPUNIT_ASSERT( boundingBox.lowerX == 0.0f);
            CPPUNIT_ASSERT( boundingBox.lowerY == 0.0f);
            CPPUNIT_ASSERT( boundingBox.lowerZ == 0.0f);
            CPPUNIT_ASSERT( boundingBox.upperX == 0.0f);
            CPPUNIT_ASSERT( boundingBox.upperY == 0.0f);
            CPPUNIT_ASSERT( boundingBox.upperZ == 0.0f);
        }
        
        void testPlusEquals()
        {
            setUpFreetype();

            FTBBox boundingBox1;
            FTBBox boundingBox2( face->glyph);
            
            boundingBox1 += boundingBox2;
        
            CPPUNIT_ASSERT_DOUBLES_EQUAL(   2, boundingBox2.lowerX, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( -15, boundingBox2.lowerY, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(   0, boundingBox2.lowerZ, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  35, boundingBox2.upperX, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  38, boundingBox2.upperY, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(   0, boundingBox2.upperZ, 0.01);
            
            float advance  = 40;
            
            boundingBox2.Move( FTPoint( advance, 0, 0));
            boundingBox1 += boundingBox2;
            
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  42, boundingBox2.lowerX, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( -15, boundingBox2.lowerY, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(   0, boundingBox2.lowerZ, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  75, boundingBox2.upperX, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  38, boundingBox2.upperY, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(   0, boundingBox2.upperZ, 0.01);

            tearDownFreetype();
        }
        
        void setUp() 
        {}
        
        
        void tearDown() 
        {}
        
    private:
        FT_Library   library;
        FT_Face      face;
        FT_Glyph     glyph;

        void setUpFreetype()
        {
            FT_Error error = FT_Init_FreeType( &library);
            CPPUNIT_ASSERT(!error);
            error = FT_New_Face( library, GOOD_FONT_FILE, 0, &face);
            CPPUNIT_ASSERT(!error);
            
            long glyphIndex = FT_Get_Char_Index( face, CHARACTER_CODE_G);
            
            FT_Set_Char_Size( face, 0L, FONT_POINT_SIZE * 64, RESOLUTION, RESOLUTION);
            
            error = FT_Load_Glyph( face, glyphIndex, FT_LOAD_DEFAULT);
            CPPUNIT_ASSERT(!error);
            error = FT_Get_Glyph( face->glyph, &glyph);
            CPPUNIT_ASSERT(!error);
        }
        
        void tearDownFreetype()
        {
            FT_Done_Glyph( glyph);
            FT_Done_Face( face);
            FT_Done_FreeType( library);
        }
        
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTBBoxTest);

