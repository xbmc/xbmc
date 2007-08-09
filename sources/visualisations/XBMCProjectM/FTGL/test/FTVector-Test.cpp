#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>

#include "FTVector.h"


class FTVectorTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTVectorTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testReserve);
        CPPUNIT_TEST( testPushBack);
        CPPUNIT_TEST( testOperatorSquareBrackets);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTVectorTest() : CppUnit::TestCase( "FTVector Test")
        {}
        
        FTVectorTest( const std::string& name) : CppUnit::TestCase(name) {}

        void testConstructor()
        {
            FTVector<float> floatVector;

            CPPUNIT_ASSERT( floatVector.size() == 0);
            CPPUNIT_ASSERT( floatVector.empty());
            CPPUNIT_ASSERT( floatVector.capacity() == 0);
        }
        
        
        void testReserve()
        {
            FTVector<float> floatVector;

            floatVector.reserve(128);
            CPPUNIT_ASSERT( floatVector.capacity() == 256);
            CPPUNIT_ASSERT( floatVector.empty());
            CPPUNIT_ASSERT( floatVector.size() == 0);
        }
        
        
        void testPushBack()
        {
            FTVector<int> integerVector;

            CPPUNIT_ASSERT( integerVector.size() == 0);
            
            integerVector.push_back(0);
            integerVector.push_back(1);
            integerVector.push_back(2);
            integerVector.push_back(3);
            
            CPPUNIT_ASSERT( !integerVector.empty());
            CPPUNIT_ASSERT( integerVector.size() == 4);
        }
        
        
        void testOperatorSquareBrackets()
        {
            FTVector<int> integerVector;

            integerVector.push_back(1);
            integerVector.push_back(2);
            integerVector.push_back(4);
            integerVector.push_back(8);
            
            CPPUNIT_ASSERT( integerVector[0] == 1);
            CPPUNIT_ASSERT( integerVector[2] == 4);
        }
        
        
        void setUp() 
        {}
        
        
        void tearDown() 
        {}
        
    private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTVectorTest);

