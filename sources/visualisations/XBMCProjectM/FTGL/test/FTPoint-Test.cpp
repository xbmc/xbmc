#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>

#include "FTPoint.h"


class FTPointTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTPointTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testOperatorEqual);
        CPPUNIT_TEST( testOperatorPlus);
        CPPUNIT_TEST( testOperatorMultiply);
        CPPUNIT_TEST( testOperatorNotEqual);
        CPPUNIT_TEST( testOperatorPlusEquals);
        CPPUNIT_TEST( testOperatorDouble);
        CPPUNIT_TEST( testSetters);
        CPPUNIT_TEST( testGetters);        
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTPointTest() : CppUnit::TestCase( "FTPoint Test")
        {}
        
        FTPointTest( const std::string& name) : CppUnit::TestCase(name) {}
        
        void testConstructor()
        {
            FTPoint point1;

            CPPUNIT_ASSERT( point1.X() == 0.0f);
            CPPUNIT_ASSERT( point1.Y() == 0.0f);
            CPPUNIT_ASSERT( point1.Z() == 0.0f);
            
            FTPoint point2( 1.0f, 2.0f, 3.0f);

            CPPUNIT_ASSERT( point2.X() == 1.0f);
            CPPUNIT_ASSERT( point2.Y() == 2.0f);
            CPPUNIT_ASSERT( point2.Z() == 3.0f);
            
            FT_Vector ftVector;
            ftVector.x = 4;
            ftVector.y = 23;
            
            FTPoint point3( ftVector);
            
            CPPUNIT_ASSERT( point3.X() ==  4.0f);
            CPPUNIT_ASSERT( point3.Y() == 23.0f);
            CPPUNIT_ASSERT( point3.Z() ==  0.0f); 
        }

        
        void testOperatorEqual()
        {
            FTPoint point1(  1.0f, 2.0f, 3.0f);
            FTPoint point2(  1.0f, 2.0f, 3.0f);
            FTPoint point3( -1.0f, 2.3f, 23.0f);
            
            CPPUNIT_ASSERT(   point1 == point1);
            CPPUNIT_ASSERT(   point1 == point2);
            CPPUNIT_ASSERT( !(point1 == point3));
        }
        
        
        void testOperatorNotEqual()
        {
            FTPoint point1(  1.0f, 2.0f, 3.0f);
            FTPoint point2(  1.0f, 2.0f, 3.0f);
            FTPoint point3( -1.0f, 2.3f, 23.0f);
            
            CPPUNIT_ASSERT( !(point1 != point1));
            CPPUNIT_ASSERT( !(point1 != point2));
            CPPUNIT_ASSERT(   point1 != point3);
        }
        
        
        void testOperatorPlus()
        {
            FTPoint point1(  1.0f, 2.0f, 3.0f);
            FTPoint point2(  1.0f, 2.0f, 3.0f);
            
            FTPoint point3(  2.0f, 4.0f, 6.0f);
            FTPoint point4 = point1 + point2;
            
            CPPUNIT_ASSERT( point4 == point3);
        }
        
        
        void testOperatorMultiply()
        {
            FTPoint point1(  1.0f, 2.0f, 3.0f);
            FTPoint point2(  1.0f, 2.0f, 3.0f);
            
            FTPoint point3(  2.0f, 4.0f, 6.0f);
            FTPoint point4 = point1 * 2.0;

            CPPUNIT_ASSERT( point4 == point3);

            point4 = 2.0 * point2;
            
            CPPUNIT_ASSERT( point4 == point3);
        }
        
        
        void testOperatorPlusEquals()
        {
            FTPoint point1(  1.0f, 2.0f, 3.0f);
            FTPoint point2(  -2.0f, 21.0f, 0.0f);
            FTPoint point3( -1.0f, 23.0f, 3.0f);
            
            point1 += point2;
            
            CPPUNIT_ASSERT( point1 == point3);
        }
        
        
        void testOperatorDouble()
        {
            FTPoint point1(  1.0f, 2.0f, 3.0f);
        
            const double* pointer = static_cast<const double*>(point1);
            CPPUNIT_ASSERT(pointer[0] == 1.0f);
            CPPUNIT_ASSERT(pointer[1] == 2.0f);
            CPPUNIT_ASSERT(pointer[2] == 3.0f);
        }
        
        void testSetters()
        {
            FTPoint point;
            FTPoint point1( 1, 2, 3);
            
            point.X(1);
            point.Y(2);
            point.Z(3);
            
            CPPUNIT_ASSERT(point == point1);
        }
        
        void testGetters()
        {
            FTPoint point( 1.0f, 2.0f, 3.0f);
            
            CPPUNIT_ASSERT(point.X() == 1.0);
            CPPUNIT_ASSERT(point.Y() == 2.0);
            CPPUNIT_ASSERT(point.Z() == 3.0);
        }
        
        
        void setUp() 
        {}
        
        
        void tearDown() 
        {}
        
    private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTPointTest);

