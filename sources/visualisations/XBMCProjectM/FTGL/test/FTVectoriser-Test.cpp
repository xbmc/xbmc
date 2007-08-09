#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <assert.h>

#include "Fontdefs.h"
#include "FTVectoriser.h"


static double testOutline[] = 
{
    28, 0, 0.0,
    28, 4.53125, 0.0,
    26.4756, 2.54, 0.0,
    24.69, 0.99125, 0.0,
    22.6431, -0.115, 0.0,
    20.335, -0.77875, 0.0,
    17.7656, -1, 0.0,
    16.0434, -0.901563, 0.0,
    14.3769, -0.60625, 0.0,
    12.7659, -0.114062, 0.0,
    11.2106, 0.575, 0.0,
    9.71094, 1.46094, 0.0,
    8.30562, 2.52344, 0.0,
    7.03344, 3.74219, 0.0,
    5.89437, 5.11719, 0.0,
    4.88844, 6.64844, 0.0,
    4.01562, 8.33594, 0.0,
    3.29, 10.1538, 0.0,
    2.72563, 12.0759, 0.0,
    2.3225, 14.1025, 0.0,
    2.08063, 16.2334, 0.0,
    2, 18.4688, 0.0,
    2.07312, 20.6591, 0.0,
    2.2925, 22.7675, 0.0,
    2.65812, 24.7941, 0.0,
    3.17, 26.7388, 0.0,
    3.82812, 28.6016, 0.0,
    4.6325, 30.3381, 0.0,
    5.58313, 31.9041, 0.0,
    6.68, 33.2994, 0.0,
    7.92313, 34.5241, 0.0,
    9.3125, 35.5781, 0.0,
    10.8088, 36.45, 0.0,
    12.3725, 37.1281, 0.0,
    14.0038, 37.6125, 0.0,
    15.7025, 37.9031, 0.0,
    17.4688, 38, 0.0,
    18.7647, 37.9438, 0.0,
    20.0025, 37.775, 0.0,
    21.1822, 37.4938, 0.0,
    22.3038, 37.1, 0.0,
    23.3672, 36.5938, 0.0,
    24.3631, 35.9975, 0.0,
    25.2822, 35.3338, 0.0,
    26.1244, 34.6025, 0.0,
    26.8897, 33.8037, 0.0,
    27.5781, 32.9375, 0.0,
    27.5781, 51, 0.0,
    34, 51, 0.0,
    34, 0, 0.0,
    8.375, 18.4844, 0.0,
    8.49312, 15.7491, 0.0,
    8.8475, 13.3056, 0.0,
    9.43813, 11.1541, 0.0,
    10.265, 9.29437, 0.0,
    11.3281, 7.72656, 0.0,
    12.5519, 6.44688, 0.0,
    13.8606, 5.45156, 0.0,
    15.2544, 4.74062, 0.0,
    16.7331, 4.31406, 0.0,
    18.2969, 4.17188, 0.0,
    19.8669, 4.30781, 0.0,
    21.3394, 4.71563, 0.0,
    22.7144, 5.39531, 0.0,
    23.9919, 6.34688, 0.0,
    25.1719, 7.57031, 0.0,
    26.19, 9.07312, 0.0,
    26.9819, 10.8628, 0.0,
    27.5475, 12.9394, 0.0,
    27.8869, 15.3028, 0.0,
    28, 17.9531, 0.0,
    27.8847, 20.8591, 0.0,
    27.5388, 23.4394, 0.0,
    26.9622, 25.6941, 0.0,
    26.155, 27.6231, 0.0,
    25.1172, 29.2266, 0.0,
    23.9106, 30.5231, 0.0,
    22.5972, 31.5316, 0.0,
    21.1769, 32.2519, 0.0,
    19.6497, 32.6841, 0.0,
    18.0156, 32.8281, 0.0,
    16.4203, 32.69, 0.0,
    14.9344, 32.2756, 0.0,
    13.5578, 31.585, 0.0,
    12.2906, 30.6181, 0.0,
    11.1328, 29.375, 0.0,
    10.14, 27.8344, 0.0,
    9.36781, 25.975, 0.0,
    8.81625, 23.7969, 0.0,
    8.48531, 21.3, 0.0,
    8.375, 18.4844, 0.0
};


static GLenum testMeshPolygonTypes[] = 
{
    GL_TRIANGLE_FAN,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_STRIP, 
    GL_TRIANGLE_FAN, 
    GL_TRIANGLE_FAN, 
    GL_TRIANGLE_FAN, 
    GL_TRIANGLE_STRIP, 
    GL_TRIANGLE_STRIP, 
    GL_TRIANGLE_FAN, 
    GL_TRIANGLE_STRIP, 
    GL_TRIANGLES
};


static unsigned int testMeshPointCount[] = 
{
    8, 7, 7, 11, 7, 9, 5, 6, 6, 7, 17, 6, 19, 3,
};


static double testMesh[] = 
{
    28, 4.53125, 0.0,
    28, 0, 0.0,
    34, 0, 0.0,
    28, 17.9531, 0.0,
    27.8869, 15.3028, 0.0,
    27.5475, 12.9394, 0.0,
    26.9819, 10.8628, 0.0,
    26.4756, 2.54, 0.0,
    26.9819, 10.8628, 0.0,
    26.19, 9.07312, 0.0,
    26.4756, 2.54, 0.0,
    25.1719, 7.57031, 0.0,
    24.69, 0.99125, 0.0,
    23.9919, 6.34688, 0.0,
    22.7144, 5.39531, 0.0,
    24.69, 0.99125, 0.0,
    22.7144, 5.39531, 0.0,
    22.6431, -0.115, 0.0,
    21.3394, 4.71563, 0.0,
    20.335, -0.77875, 0.0,
    19.8669, 4.30781, 0.0,
    18.2969, 4.17188, 0.0,
    20.335, -0.77875, 0.0,
    18.2969, 4.17188, 0.0,
    17.7656, -1, 0.0,
    16.7331, 4.31406, 0.0,
    16.0434, -0.901563, 0.0,
    15.2544, 4.74062, 0.0,
    14.3769, -0.60625, 0.0,
    13.8606, 5.45156, 0.0,
    12.7659, -0.114062, 0.0,
    12.5519, 6.44688, 0.0,
    11.3281, 7.72656, 0.0,
    12.7659, -0.114062, 0.0,
    11.3281, 7.72656, 0.0,
    11.2106, 0.575, 0.0,
    10.265, 9.29437, 0.0,
    9.71094, 1.46094, 0.0,
    9.43813, 11.1541, 0.0,
    8.8475, 13.3056, 0.0,
    8.81625, 23.7969, 0.0,
    9.3125, 35.5781, 0.0,
    8.48531, 21.3, 0.0,
    7.92313, 34.5241, 0.0,
    8.375, 18.4844, 0.0,
    8.30562, 2.52344, 0.0,
    8.49312, 15.7491, 0.0,
    9.71094, 1.46094, 0.0,
    8.8475, 13.3056, 0.0,
    34, 51, 0.0,
    27.5781, 51, 0.0,
    27.8847, 20.8591, 0.0,
    28, 17.9531, 0.0,
    34, 0, 0.0,
    27.5781, 32.9375, 0.0,
    26.8897, 33.8037, 0.0,
    26.9622, 25.6941, 0.0,
    27.5388, 23.4394, 0.0,
    27.8847, 20.8591, 0.0,
    27.5781, 51, 0.0,
    26.155, 27.6231, 0.0,
    26.9622, 25.6941, 0.0,
    26.8897, 33.8037, 0.0,
    26.1244, 34.6025, 0.0,
    25.2822, 35.3338, 0.0,
    25.1172, 29.2266, 0.0,
    22.3038, 37.1, 0.0,
    22.5972, 31.5316, 0.0,
    23.3672, 36.5938, 0.0,
    23.9106, 30.5231, 0.0,
    24.3631, 35.9975, 0.0,
    25.1172, 29.2266, 0.0,
    25.2822, 35.3338, 0.0,
    11.1328, 29.375, 0.0,
    12.2906, 30.6181, 0.0,
    12.3725, 37.1281, 0.0,
    13.5578, 31.585, 0.0,
    14.0038, 37.6125, 0.0,
    14.9344, 32.2756, 0.0,
    15.7025, 37.9031, 0.0,
    16.4203, 32.69, 0.0,
    17.4688, 38, 0.0,
    18.0156, 32.8281, 0.0,
    18.7647, 37.9438, 0.0,
    19.6497, 32.6841, 0.0,
    20.0025, 37.775, 0.0,
    21.1769, 32.2519, 0.0,
    21.1822, 37.4938, 0.0,
    22.5972, 31.5316, 0.0,
    22.3038, 37.1, 0.0,
    10.8088, 36.45, 0.0,
    9.3125, 35.5781, 0.0,
    9.36781, 25.975, 0.0,
    10.14, 27.8344, 0.0,
    11.1328, 29.375, 0.0,
    12.3725, 37.1281, 0.0,
    8.30562, 2.52344, 0.0,
    7.92313, 34.5241, 0.0,
    7.03344, 3.74219, 0.0,
    6.68, 33.2994, 0.0,
    5.89437, 5.11719, 0.0,
    5.58313, 31.9041, 0.0,
    4.88844, 6.64844, 0.0,
    4.6325, 30.3381, 0.0,
    4.01562, 8.33594, 0.0,
    3.82812, 28.6016, 0.0,
    3.29, 10.1538, 0.0,
    3.17, 26.7388, 0.0,
    2.72563, 12.0759, 0.0,
    2.65812, 24.7941, 0.0,
    2.3225, 14.1025, 0.0,
    2.2925, 22.7675, 0.0,
    2.08063, 16.2334, 0.0,
    2.07312, 20.6591, 0.0,
    2, 18.4688, 0.0,
    9.3125, 35.5781, 0.0,
    8.81625, 23.7969, 0.0,
    9.36781, 25.975, 0.0
};


class FTVectoriserTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTVectoriserTest);
        CPPUNIT_TEST( testFreetypeVersion);
        CPPUNIT_TEST( testNullGlyphProcess);
        CPPUNIT_TEST( testBadGlyphProcess);
        CPPUNIT_TEST( testSimpleGlyphProcess);
        CPPUNIT_TEST( testComplexGlyphProcess);
        CPPUNIT_TEST( testGetContour);
        CPPUNIT_TEST( testGetOutline);
        CPPUNIT_TEST( testGetMesh);
        CPPUNIT_TEST( testMakeMesh);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTVectoriserTest() : CppUnit::TestCase( "FTVectoriser Test")
        {}
        
        FTVectoriserTest( const std::string& name) : CppUnit::TestCase(name)
        {}
        
        
        void testFreetypeVersion()
        {
            setUpFreetype( NULL_CHARACTER_INDEX);

            FT_Int major;
            FT_Int minor;
            FT_Int patch;
            
            FT_Library_Version( library, &major, &minor, &patch);
            
            // If you hit these asserts then you have the wrong library version to run the tests.
            // You can still run the tests but some will fail because the hinter changed in 2.1.4 
            CPPUNIT_ASSERT( major == 2);
            CPPUNIT_ASSERT( minor == 1);
            CPPUNIT_ASSERT( patch >= 4);

            tearDownFreetype();
        }
        

        void testNullGlyphProcess()
        {
            FTVectoriser vectoriser( NULL);
            CPPUNIT_ASSERT( vectoriser.ContourCount() == 0);
        }
        
        
        void testBadGlyphProcess()
        {
            setUpFreetype( NULL_CHARACTER_INDEX);
            
            FTVectoriser vectoriser( face->glyph);
            CPPUNIT_ASSERT( vectoriser.ContourCount() == 0);
            
            tearDownFreetype();
        }
        

        void testSimpleGlyphProcess()
        {
            setUpFreetype( SIMPLE_CHARACTER_INDEX);
            
            FTVectoriser vectoriser( face->glyph);

            CPPUNIT_ASSERT( vectoriser.ContourCount() == 2);
            CPPUNIT_ASSERT( vectoriser.PointCount() == 8);
            
            tearDownFreetype();
        }
        
        
        void testComplexGlyphProcess()
        {
            setUpFreetype( COMPLEX_CHARACTER_INDEX);
            
            FTVectoriser vectoriser( face->glyph);

            CPPUNIT_ASSERT( vectoriser.ContourCount() == 2);
            CPPUNIT_ASSERT( vectoriser.PointCount() == 91);
            
            tearDownFreetype();
        }
        
        
        void testGetContour()
        {
            setUpFreetype( SIMPLE_CHARACTER_INDEX);
            
            FTVectoriser vectoriser( face->glyph);

            CPPUNIT_ASSERT( vectoriser.Contour(1));
            CPPUNIT_ASSERT( vectoriser.Contour(99) == NULL);
            
            tearDownFreetype();
        }
        
        
        void testGetOutline()
        {
            setUpFreetype( COMPLEX_CHARACTER_INDEX);
            
            FTVectoriser vectoriser( face->glyph);
            
            unsigned int d = 0;
            for( size_t c = 0; c < vectoriser.ContourCount(); ++c)
            {
                const FTContour* contour = vectoriser.Contour(c);
                
                for( size_t p = 0; p < contour->PointCount(); ++p)
                {
                    CPPUNIT_ASSERT_DOUBLES_EQUAL( *(testOutline + d),     contour->Point(p).X() / 64.0f, 0.01);
                    CPPUNIT_ASSERT_DOUBLES_EQUAL( *(testOutline + d + 1), contour->Point(p).Y() / 64.0f, 0.01);
                    d += 3;
                }
            }
            
            tearDownFreetype();
        }
        
        
        void testGetMesh()
        {
            setUpFreetype( SIMPLE_CHARACTER_INDEX);
            
            FTVectoriser vectoriser( face->glyph);
            CPPUNIT_ASSERT( vectoriser.GetMesh() == NULL);

            vectoriser.MakeMesh( FTGL_FRONT_FACING);
            
            CPPUNIT_ASSERT( vectoriser.GetMesh());
        }
        
        
        void testMakeMesh()
        {
            setUpFreetype( COMPLEX_CHARACTER_INDEX);
            
            FTVectoriser vectoriser( face->glyph);

            vectoriser.MakeMesh( FTGL_FRONT_FACING);

            int d = 0;
            const FTMesh* mesh = vectoriser.GetMesh();
            unsigned int tesselations = mesh->TesselationCount();
            CPPUNIT_ASSERT( tesselations == 14);
            
            for( unsigned int index = 0; index < tesselations; ++index)
            {
                const FTTesselation* subMesh = mesh->Tesselation( index);
                
                unsigned int polyType = subMesh->PolygonType();
                CPPUNIT_ASSERT( testMeshPolygonTypes[index] == polyType);
                
                unsigned int numberOfVertices = subMesh->PointCount();
                CPPUNIT_ASSERT( testMeshPointCount[index] == numberOfVertices);

                for( unsigned int x = 0; x < numberOfVertices; ++x)
                {
                    CPPUNIT_ASSERT_DOUBLES_EQUAL( *(testMesh + d),     subMesh->Point(x).X() / 64, 0.01);
                    CPPUNIT_ASSERT_DOUBLES_EQUAL( *(testMesh + d + 1), subMesh->Point(x).Y() / 64, 0.01);
                    d += 3;
                }
            }

            tearDownFreetype();
        }
        
        
        void setUp() 
        {}
        
        
        void tearDown() 
        {}
        
    private:
        FT_Library   library;
        FT_Face      face;

        void setUpFreetype( unsigned int characterIndex)
        {
            FT_Error error = FT_Init_FreeType( &library);
            assert(!error);
            error = FT_New_Face( library, ARIAL_FONT_FILE, 0, &face);
            assert(!error);
            
            loadGlyph( characterIndex);
        }
        
        void loadGlyph( unsigned int characterIndex)
        {
            long glyphIndex = FT_Get_Char_Index( face, characterIndex);
            
            FT_Set_Char_Size( face, 0L, FONT_POINT_SIZE * 64, RESOLUTION, RESOLUTION);
            
            FT_Error error = FT_Load_Glyph( face, glyphIndex, FT_LOAD_DEFAULT);
            assert(!error);
        }
        
        void tearDownFreetype()
        {
            FT_Done_Face( face);
            FT_Done_FreeType( library);
        }
        
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTVectoriserTest);

