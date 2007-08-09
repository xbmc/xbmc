#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>


#include "Fontdefs.h"
#include "FTFace.h"
#include "FTGlyph.h"
#include "FTGlyphContainer.h"


class TestGlyph : public FTGlyph
{
    public:
        TestGlyph()
        :   FTGlyph(0)
        {
            advance = FTPoint(50.0f, 0.0f, 0.0f);
        }
        
        virtual const FTPoint& Render( const FTPoint& pen){ return advance;}
};


class FTGlyphContainerTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTGlyphContainerTest);
        CPPUNIT_TEST( testAdd);
        CPPUNIT_TEST( testSetCharMap);
        CPPUNIT_TEST( testGlyphIndex);
        CPPUNIT_TEST( testAdvance);
        CPPUNIT_TEST( testRender);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTGlyphContainerTest() : CppUnit::TestCase( "FTGlyphContainer Test")
        {
            face = new FTFace( GOOD_FONT_FILE);
            face->Size( 72, 72);
        }
        
        FTGlyphContainerTest( const std::string& name) : CppUnit::TestCase(name)
        {
            delete face;
        }

        void testAdd()
        {
            TestGlyph* glyph = new TestGlyph();
            CPPUNIT_ASSERT( glyphContainer->Glyph( CHARACTER_CODE_A) == NULL);

            glyphContainer->Add( glyph, CHARACTER_CODE_A);
            glyphContainer->Add( NULL, 0);

            CPPUNIT_ASSERT( glyphContainer->Glyph( 0) == NULL);
            CPPUNIT_ASSERT( glyphContainer->Glyph( 999) == NULL);
            CPPUNIT_ASSERT( glyphContainer->Glyph( CHARACTER_CODE_A) == glyph);
        }

    
        void testSetCharMap()
        {
            CPPUNIT_ASSERT( glyphContainer->CharMap( ft_encoding_unicode));
            CPPUNIT_ASSERT( glyphContainer->Error() == 0);
    
            CPPUNIT_ASSERT( !glyphContainer->CharMap( ft_encoding_johab));
            CPPUNIT_ASSERT( glyphContainer->Error() == 0x06); // invalid argument
        }


        void testGlyphIndex()
        {
            CPPUNIT_ASSERT( glyphContainer->FontIndex( CHARACTER_CODE_A) == FONT_INDEX_OF_A);
            CPPUNIT_ASSERT( glyphContainer->FontIndex( BIG_CHARACTER_CODE) == BIG_FONT_INDEX);
        }


        void testAdvance()
        {
            TestGlyph* glyph = new TestGlyph();

            glyphContainer->Add( glyph, CHARACTER_CODE_A);
            float advance = glyphContainer->Advance( CHARACTER_CODE_A, 0);
            
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 50, advance, 0.01);
        }
        
        
        void testRender()
        {
            TestGlyph* glyph = new TestGlyph();
            
            glyphContainer->Add( glyph, 'A');
            
            FTPoint pen;
            
            float advance = glyphContainer->Render( 'A', 0, pen).X();
            
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 50, advance, 0.01);
        }
        
        
        void setUp() 
        {
            glyphContainer = new FTGlyphContainer( face);
        }
        
        
        void tearDown() 
        {
            delete glyphContainer;
        }
        
    private:
        FTFace*           face;
        FTGlyphContainer* glyphContainer;

};

CPPUNIT_TEST_SUITE_REGISTRATION( FTGlyphContainerTest);

