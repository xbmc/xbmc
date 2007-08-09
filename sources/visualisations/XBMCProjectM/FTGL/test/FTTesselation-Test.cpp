#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>

#include "FTVectoriser.h"


class FTTesselationTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTTesselationTest);
        CPPUNIT_TEST( testAddPoint);
        CPPUNIT_TEST( testGetPoint);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTTesselationTest() : CppUnit::TestCase( "FTTesselation Test")
        {}
        
        FTTesselationTest( const std::string& name) : CppUnit::TestCase(name)
        {}

        void testAddPoint()
        {
            FTTesselation tesselation( 1);
            
            CPPUNIT_ASSERT( tesselation.PointCount() == 0);
            
            tesselation.AddPoint(  10, 3, 0.7);
            tesselation.AddPoint( -53, 2000, 23);
            tesselation.AddPoint(  77, -2.4, 765);
            tesselation.AddPoint( 117.5,  0.02, -99);

            CPPUNIT_ASSERT( tesselation.PointCount() == 4);
            
            tesselation.AddPoint(  10, 3, -0.87);
            tesselation.AddPoint( 117.5, 0.02, 34.76);
            tesselation.AddPoint(   0.27, 44.4, 3000);
            tesselation.AddPoint(  10, 3, 0);

            CPPUNIT_ASSERT( tesselation.PointCount() == 8);
        }
        
        
        void testGetPoint()
        {
            FTTesselation tesselation(1);
            
            CPPUNIT_ASSERT( tesselation.PointCount() == 0);
            
            tesselation.AddPoint(  10, 3, 0.7);
            tesselation.AddPoint( -53, 2000, 23);
            tesselation.AddPoint(  77, -2.4, 765);
            tesselation.AddPoint( 117.5,  0.02, -99);

            CPPUNIT_ASSERT( tesselation.PointCount() == 4);
            CPPUNIT_ASSERT( tesselation.Point(2) == FTPoint( 77, -2.4, 765));
            CPPUNIT_ASSERT( tesselation.Point(20) != FTPoint( 77, -2.4, 765));
        }
        
        
        void setUp() 
        {}
        
        
        void tearDown() 
        {}
        
    private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTTesselationTest);

