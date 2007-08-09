#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>

#include "FTContour.h"

// FT_Curve_Tag_On           1
// FT_Curve_Tag_Conic        0
// FT_Curve_Tag_Cubic        2

static FT_Vector shortLine[2] =
{
    { 1, 1},
    { 2, 2},
};

static FT_Vector straightLinePoints[3] = 
{
    { 0,  0},
    { 6,  7},
    { 9, -2}
};

static char straightLineTags[3] =
{
    FT_Curve_Tag_On,
    FT_Curve_Tag_On,
    FT_Curve_Tag_On
};

static char brokenTags[3] =
{
    FT_Curve_Tag_Conic,
    69,
    FT_Curve_Tag_On
};

static FT_Vector simpleConicPoints[3] = 
{
    { 0,  0},
    { 6,  7},
    { 9, -2}
};

static FT_Vector brokenPoints[3] = 
{
    { 0, 0},
    { 0, 0},
    { 0, 0}
};

static char simpleConicTags[3] = 
{
    FT_Curve_Tag_Conic,
    FT_Curve_Tag_On,
    FT_Curve_Tag_On
};

static FT_Vector doubleConicPoints[4] = 
{
    { 0,  0},
    { 6,  7},
    { 9, -2},
    { 4,  0}
};

static char doubleConicTags[4] = 
{
    FT_Curve_Tag_On,
    FT_Curve_Tag_On,
    FT_Curve_Tag_Conic,
    FT_Curve_Tag_Conic
};

static FT_Vector cubicPoints[4] = 
{
    { 0,  0},
    { 6,  7},
    { 9, -2},
    { 4,  0}
};

static char cubicTags[4] = 
{
    FT_Curve_Tag_On,
    FT_Curve_Tag_On,
    FT_Curve_Tag_Cubic,
    FT_Curve_Tag_Cubic
};


// ARIAl 'd'
static FT_Vector compositePoints[18] = 
{
	{ 1856,    0},
	{ 1856,  279},
	{ 1625,  -64},
	{ 1175,  -64},
	{  884,  -64},
	{  396,  251},
	{  128,  815},
	{  128, 1182},
	{  128, 1539},
	{  370, 2121},
	{  855, 2432},
	{ 1156, 2432},
	{ 1375, 2432},
	{ 1718, 2257},
	{ 1826, 2118},
	{ 1826, 3264},
	{ 2240, 3264},
	{ 2240,    0},
};

static char compositeTags[18] = 
{
    FT_Curve_Tag_On,
    FT_Curve_Tag_On,
    FT_Curve_Tag_Conic,
    FT_Curve_Tag_On,
    FT_Curve_Tag_Conic,
    FT_Curve_Tag_Conic,
    FT_Curve_Tag_Conic,
    FT_Curve_Tag_On,
    FT_Curve_Tag_Conic,
    FT_Curve_Tag_Conic,
    FT_Curve_Tag_Conic,
    FT_Curve_Tag_On,
    FT_Curve_Tag_Conic,
    FT_Curve_Tag_Conic,
    FT_Curve_Tag_On,
    FT_Curve_Tag_On,
    FT_Curve_Tag_On,
    FT_Curve_Tag_On
};

class FTContourTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTContourTest);
        CPPUNIT_TEST( testNullCurve);
        CPPUNIT_TEST( testBrokenCurve);
        CPPUNIT_TEST( testStraightLine);
        CPPUNIT_TEST( testConicCurve);
        CPPUNIT_TEST( testDoubleConicCurve);
        CPPUNIT_TEST( testCubicCurve);
        CPPUNIT_TEST( testCompositeCurve);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTContourTest() : CppUnit::TestCase( "FTContour Test")
        {}
        
        FTContourTest( const std::string& name) : CppUnit::TestCase(name)
        {}

        void testNullCurve()
        {
            FTContour contour( NULL, NULL, 0);
            CPPUNIT_ASSERT( contour.PointCount() == 0);
        }


        void testBrokenCurve()
        {
            FTContour contour( brokenPoints, simpleConicTags, 3);
            CPPUNIT_ASSERT( contour.PointCount() == 1);

            FTContour shortContour( shortLine, simpleConicTags, 2);
            CPPUNIT_ASSERT( shortContour.PointCount() == 6);

            FTContour reallyShortContour( shortLine, simpleConicTags, 1);
            CPPUNIT_ASSERT( reallyShortContour.PointCount() == 1);

            FTContour brokenTagtContour( shortLine, brokenTags, 3);
            CPPUNIT_ASSERT( brokenTagtContour.PointCount() == 7);
        }


        void testStraightLine()
        {
            FTContour contour( straightLinePoints, straightLineTags, 3);
            CPPUNIT_ASSERT( contour.PointCount() == 3);
        }


        void testConicCurve()
        {
            FTContour contour( simpleConicPoints, simpleConicTags, 3);
            CPPUNIT_ASSERT( contour.PointCount() == 7);
        }


        void testDoubleConicCurve()
        {
            FTContour contour( doubleConicPoints, doubleConicTags, 4);
            CPPUNIT_ASSERT( contour.PointCount() == 12);
        }


        void testCubicCurve()
        {
            FTContour contour( cubicPoints, cubicTags, 4);
            CPPUNIT_ASSERT( contour.PointCount() == 7);
        }


        void testCompositeCurve()
        {
            FTContour contour( compositePoints, compositeTags, 18);
            CPPUNIT_ASSERT( contour.PointCount() == 50);
        }
        
        
        void setUp() 
        {}
        
        
        void tearDown() 
        {}
        
    private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTContourTest);

