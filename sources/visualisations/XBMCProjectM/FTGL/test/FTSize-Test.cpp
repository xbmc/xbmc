#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <assert.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "Fontdefs.h"
#include "FTSize.h"


class FTSizeTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTSizeTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testSetCharSize);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTSizeTest() : CppUnit::TestCase( "FTSize Test")
        {}
        
        FTSizeTest( const std::string& name) : CppUnit::TestCase(name) {}

        void testConstructor()
        {
            FTSize size;
            
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, size.CharSize(), 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, size.Ascender(), 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, size.Descender(), 0.01);

            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, size.Height(), 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, size.Width(), 0.01);

            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, size.Underline(), 0.01);

        }
        
        
        void testSetCharSize()
        {
            setUpFreetype();
            
            FTSize size;

            CPPUNIT_ASSERT( size.CharSize( &face, FONT_POINT_SIZE, RESOLUTION, RESOLUTION));
            CPPUNIT_ASSERT( size.Error() == 0);
            
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  72, size.CharSize(), 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  52, size.Ascender(), 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( -15, size.Descender(), 0.01);

            CPPUNIT_ASSERT_DOUBLES_EQUAL( 81.86, size.Height(), 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 76.32, size.Width(), 0.01);

            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, size.Underline(), 0.01);
            
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
            error = FT_New_Face( library, GOOD_FONT_FILE, 0, &face);
            assert(!error);
        }
        
        void tearDownFreetype()
        {
            FT_Done_Face( face);
            FT_Done_FreeType( library);
        }
        
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTSizeTest);

