#include <iostream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <assert.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H


#include "Fontdefs.h"
#include "FTFace.h"
#include "FTCharmap.h"


class FTCharmapTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTCharmapTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testSetEncoding);
        CPPUNIT_TEST( testGetGlyphListIndex);
        CPPUNIT_TEST( testGetFontIndex);
        CPPUNIT_TEST( testInsertCharacterIndex);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTCharmapTest() : CppUnit::TestCase( "FTCharmap Test")
        {
            setUpFreetype();
        }
        
        FTCharmapTest( const std::string& name) : CppUnit::TestCase(name) {}
        
        ~FTCharmapTest()
        {
            tearDownFreetype();
        }
        
        
        void testConstructor()
        {
            CPPUNIT_ASSERT( charmap->Error() == 0);
            CPPUNIT_ASSERT( charmap->Encoding() == ft_encoding_unicode);
        }
        
        
        void testSetEncoding()
        {
            CPPUNIT_ASSERT( charmap->CharMap( ft_encoding_unicode));
            
            CPPUNIT_ASSERT( charmap->Error() == 0);
            CPPUNIT_ASSERT( charmap->Encoding() == ft_encoding_unicode);
            
            CPPUNIT_ASSERT( !charmap->CharMap( ft_encoding_johab));
            
            CPPUNIT_ASSERT( charmap->Error() == 0x06); // invalid argument
            CPPUNIT_ASSERT( charmap->Encoding() == ft_encoding_none);
        }
        
        
        void testGetGlyphListIndex()
        {
            charmap->CharMap( ft_encoding_johab);
            
            CPPUNIT_ASSERT( charmap->Error() == 0x06); // invalid argument
            CPPUNIT_ASSERT( charmap->GlyphListIndex( CHARACTER_CODE_A)    == 0);
            CPPUNIT_ASSERT( charmap->GlyphListIndex( BIG_CHARACTER_CODE)  == 0);
            CPPUNIT_ASSERT( charmap->GlyphListIndex( NULL_CHARACTER_CODE) == 0);

            charmap->CharMap( ft_encoding_unicode);
            
            CPPUNIT_ASSERT( charmap->Error() == 0);
            CPPUNIT_ASSERT( charmap->GlyphListIndex( CHARACTER_CODE_A)    == 0);
            CPPUNIT_ASSERT( charmap->GlyphListIndex( BIG_CHARACTER_CODE)  == 0);
            CPPUNIT_ASSERT( charmap->GlyphListIndex( NULL_CHARACTER_CODE) == 0);
            
        }

    
        void testGetFontIndex()
        {
            charmap->CharMap( ft_encoding_johab);
            
            CPPUNIT_ASSERT( charmap->Error() == 0x06); // invalid argument
            CPPUNIT_ASSERT( charmap->FontIndex( CHARACTER_CODE_A)    == FONT_INDEX_OF_A);
            CPPUNIT_ASSERT( charmap->FontIndex( BIG_CHARACTER_CODE)  == BIG_FONT_INDEX);
            CPPUNIT_ASSERT( charmap->FontIndex( NULL_CHARACTER_CODE) == NULL_FONT_INDEX);
            charmap->CharMap( ft_encoding_unicode);

            CPPUNIT_ASSERT( charmap->Error() == 0);

            CPPUNIT_ASSERT( charmap->FontIndex( CHARACTER_CODE_A)    == FONT_INDEX_OF_A);
            CPPUNIT_ASSERT( charmap->FontIndex( BIG_CHARACTER_CODE)  == BIG_FONT_INDEX);
            CPPUNIT_ASSERT( charmap->FontIndex( NULL_CHARACTER_CODE) == NULL_FONT_INDEX);

        }
    
    
        void testInsertCharacterIndex()
        {
            CPPUNIT_ASSERT( charmap->GlyphListIndex( CHARACTER_CODE_A) == 0);
            CPPUNIT_ASSERT( charmap->FontIndex( CHARACTER_CODE_A) == FONT_INDEX_OF_A);

            charmap->InsertIndex( CHARACTER_CODE_A, 69);
            CPPUNIT_ASSERT( charmap->FontIndex( CHARACTER_CODE_A) == FONT_INDEX_OF_A);
            CPPUNIT_ASSERT( charmap->GlyphListIndex( CHARACTER_CODE_A) == 69);

            charmap->InsertIndex( CHARACTER_CODE_G, 999);
            CPPUNIT_ASSERT( charmap->GlyphListIndex( CHARACTER_CODE_G) == 999);
        }
        
        void setUp() 
        {
            charmap = new FTCharmap( face);
        }
        
        
        void tearDown() 
        {
            delete charmap;
        }
        
    private:
        FTFace*      face;
        FTCharmap* charmap;

        void setUpFreetype()
        {
            face = new FTFace( GOOD_FONT_FILE);
            CPPUNIT_ASSERT( !face->Error());
        }
        
        void tearDownFreetype()
        {
            delete face;
        }
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTCharmapTest);

