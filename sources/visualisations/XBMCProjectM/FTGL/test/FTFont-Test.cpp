#include "cppunit/extensions/HelperMacros.h"
#include "cppunit/TestCaller.h"
#include "cppunit/TestCase.h"
#include "cppunit/TestSuite.h"

#include "Fontdefs.h"
#include "FTGlyph.h"
#include "FTFont.h"


class TestGlyph : public FTGlyph
{
    public:
        TestGlyph( FT_GlyphSlot glyph)
        :   FTGlyph( glyph)
        {}
        
        const FTPoint& Render( const FTPoint& pen){ return advance;}
};


class TestFont : public FTFont
{
    public:
        TestFont( const char* fontFilePath)
        :   FTFont( fontFilePath)
        {}
        
        TestFont( const unsigned char *pBufferBytes, size_t bufferSizeInBytes)
        :   FTFont( pBufferBytes, bufferSizeInBytes)
        {}

        FTGlyph* MakeGlyph( unsigned int g)
        {
            FT_GlyphSlot ftGlyph = face.Glyph( g, FT_LOAD_NO_HINTING);
        
            if( ftGlyph)
            {
                TestGlyph* tempGlyph = new TestGlyph( ftGlyph);
                return tempGlyph;
            }
        
            err = face.Error();
            return NULL;
        }
};


class BadGlyphTestFont : public FTFont
{
    public:
        BadGlyphTestFont( const char* fontFilePath)
        :   FTFont( fontFilePath)
        {}
        
        FTGlyph* MakeGlyph( unsigned int g)
        {
            FT_GlyphSlot ftGlyph = face.Glyph( g, FT_LOAD_NO_HINTING);
        
            if( ftGlyph)
            {
                TestGlyph* tempGlyph = new TestGlyph( ftGlyph);
                return tempGlyph;
            }
        
            err = face.Error();
            return NULL;
        }
        
    private:
        bool CheckGlyph( const unsigned int chr)
        {
            static bool succeed = false;
            
            if( succeed == false)
            {
                succeed = true;
                return false;
            }
            
            return true;
        }

};


class FTFontTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTFontTest);
        CPPUNIT_TEST( testOpenFont);
        CPPUNIT_TEST( testOpenFontFromMemory);
        CPPUNIT_TEST( testAttachFile);
        CPPUNIT_TEST( testAttachData);
        CPPUNIT_TEST( testSetFontSize);
        CPPUNIT_TEST( testSetCharMap);
        CPPUNIT_TEST( testGetCharmapList);
        CPPUNIT_TEST( testBoundingBox);
        CPPUNIT_TEST( testCheckGlyphFailure);
        CPPUNIT_TEST( testAdvance);
        CPPUNIT_TEST( testRender);
    CPPUNIT_TEST_SUITE_END();

    public:
        FTFontTest() : CppUnit::TestCase( "FTFont test") {};
        FTFontTest( const std::string& name) : CppUnit::TestCase(name) {};
        
        
        void testOpenFont()
        {
            TestFont badFont( BAD_FONT_FILE);
            CPPUNIT_ASSERT( badFont.Error() == 0x06); // invalid argument
        
            TestFont goodFont( GOOD_FONT_FILE);
            CPPUNIT_ASSERT( goodFont.Error() == 0);        
        }
        
        
        void testOpenFontFromMemory()
        {
            TestFont badFont( (unsigned char*)100, 0);
            CPPUNIT_ASSERT( badFont.Error() == 0x02);
        
            TestFont goodFont( HPGCalc_pfb.dataBytes, HPGCalc_pfb.numBytes);
            CPPUNIT_ASSERT( goodFont.Error() == 0);        
        }
        
        
        void testAttachFile()
        {
            testFont->Attach( TYPE1_AFM_FILE);
            CPPUNIT_ASSERT( testFont->Error() == 0x07); // unimplemented feature
        }
        
        
        void testAttachData()
        {
            testFont->Attach( (unsigned char*)100, 0);
            CPPUNIT_ASSERT( testFont->Error() == 0x07); // unimplemented feature       
        }
        
        
        void testSetFontSize()
        {
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, testFont->Ascender(), 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, testFont->Descender(), 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, testFont->LineHeight(), 0.01);

            float advance = testFont->Advance( GOOD_UNICODE_TEST_STRING);
            CPPUNIT_ASSERT( advance == 0);
        
            CPPUNIT_ASSERT( testFont->FaceSize( FONT_POINT_SIZE));
            CPPUNIT_ASSERT( testFont->Error() == 0);
            
            CPPUNIT_ASSERT( testFont->FaceSize() == FONT_POINT_SIZE);
            
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  52, testFont->Ascender(), 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( -15, testFont->Descender(), 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  81.86, testFont->LineHeight(), 0.01);
        
            CPPUNIT_ASSERT( testFont->FaceSize( FONT_POINT_SIZE * 2));
            CPPUNIT_ASSERT( testFont->Error() == 0);
        
            CPPUNIT_ASSERT( testFont->FaceSize() == FONT_POINT_SIZE * 2);
        
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 104, testFont->Ascender(), 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( -29, testFont->Descender(), 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 163.72, testFont->LineHeight(), 0.01);
        }
        
        
        void testSetCharMap()
        {
            CPPUNIT_ASSERT( true == testFont->CharMap( ft_encoding_unicode));
            CPPUNIT_ASSERT( testFont->Error() == 0);        
            CPPUNIT_ASSERT( false == testFont->CharMap( ft_encoding_johab));
            CPPUNIT_ASSERT( testFont->Error() == 0x06); // invalid argument
        }
        
        
        void testGetCharmapList()
        {
            CPPUNIT_ASSERT( testFont->CharMapCount() == 2);
            
            FT_Encoding* charmapList = testFont->CharMapList();
            
            CPPUNIT_ASSERT( charmapList[0] == ft_encoding_unicode);
            CPPUNIT_ASSERT( charmapList[1] == ft_encoding_adobe_standard);
        }
        
        
        void testBoundingBox()
        {
            CPPUNIT_ASSERT( testFont->FaceSize( FONT_POINT_SIZE));
            CPPUNIT_ASSERT( testFont->Error() == 0);
            
            float llx, lly, llz, urx, ury, urz;
            
            testFont->BBox( GOOD_ASCII_TEST_STRING, llx, lly, llz, urx, ury, urz);
        
            CPPUNIT_ASSERT_DOUBLES_EQUAL(   1.21, llx, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( -15.12, lly, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(   0.00, llz, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 307.43, urx, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  51.54, ury, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(   0.00, urz, 0.01);
            
            testFont->BBox( BAD_ASCII_TEST_STRING, llx, lly, llz, urx, ury, urz);
        
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, llx, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, lly, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, llz, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, urx, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, ury, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, urz, 0.01);
            
            testFont->BBox( GOOD_UNICODE_TEST_STRING, llx, lly, llz, urx, ury, urz);
        
            CPPUNIT_ASSERT_DOUBLES_EQUAL(   2.15, llx, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  -6.12, lly, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(   0.00, llz, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 134.28, urx, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(  61.12, ury, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(   0.00, urz, 0.01);

            testFont->BBox( BAD_UNICODE_TEST_STRING, llx, lly, llz, urx, ury, urz);

            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, llx, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, lly, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, llz, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, urx, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, ury, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, urz, 0.01);
            
            testFont->BBox( (char*)0, llx, lly, llz, urx, ury, urz);

            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, llx, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, lly, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, llz, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, urx, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, ury, 0.01);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, urz, 0.01);
        }
        
        void testCheckGlyphFailure()
        {
            BadGlyphTestFont* testFont = new BadGlyphTestFont(GOOD_FONT_FILE);

            float advance = testFont->Advance( GOOD_ASCII_TEST_STRING);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, advance, 0.01);            
        }
        
        void testAdvance()
        {
            CPPUNIT_ASSERT( testFont->FaceSize( FONT_POINT_SIZE));
            CPPUNIT_ASSERT( testFont->Error() == 0);
            
            float advance = testFont->Advance( GOOD_ASCII_TEST_STRING);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 312.10, advance, 0.01);
        
            advance = testFont->Advance( BAD_ASCII_TEST_STRING);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, advance, 0.01);
        
            advance = testFont->Advance( GOOD_UNICODE_TEST_STRING);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 144, advance, 0.01);
            
            advance = testFont->Advance( BAD_UNICODE_TEST_STRING);
            CPPUNIT_ASSERT_DOUBLES_EQUAL( 0, advance, 0.01);
        }
        
        void testRender()
        {
            testFont->Render(GOOD_ASCII_TEST_STRING);
            CPPUNIT_ASSERT( testFont->Error() == 0);    
        }
        
        
        void setUp() 
        {
            testFont = new TestFont( GOOD_FONT_FILE);
        }
        
        
        void tearDown() 
        {
            delete testFont;
        }
        
    private:
        TestFont* testFont;

};

CPPUNIT_TEST_SUITE_REGISTRATION( FTFontTest);

