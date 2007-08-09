#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>

#include "FTList.h"


class FTListTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTListTest);
        CPPUNIT_TEST( testConstructor);
        CPPUNIT_TEST( testPushBack);
        CPPUNIT_TEST( testGetBack);
        CPPUNIT_TEST( testGetFront);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTListTest() : CppUnit::TestCase( "FTList Test")
        {}
        
        FTListTest( const std::string& name) : CppUnit::TestCase(name) {}

        void testConstructor()
        {
            FTList<float> listOfFloats;
            
            CPPUNIT_ASSERT( listOfFloats.size() == 0);
        }
        
        
        void testPushBack()
        {
            FTList<float> listOfFloats;
            
            CPPUNIT_ASSERT( listOfFloats.size() == 0);
            
            listOfFloats.push_back( 0.1);
            listOfFloats.push_back( 1.2);
            listOfFloats.push_back( 2.3);

            CPPUNIT_ASSERT( listOfFloats.size() == 3);            
        }
        
        
        void testGetBack()
        {
            FTList<int> listOfIntegers;

            listOfIntegers.push_back( 0);
            listOfIntegers.push_back( 1);
            listOfIntegers.push_back( 2);

            CPPUNIT_ASSERT( listOfIntegers.back() == 2);
        }
        
        
        void testGetFront()
        {
            FTList<char> listOfChars;
            
            listOfChars.push_back( 'a');
            listOfChars.push_back( 'b');
            listOfChars.push_back( 'c');
    
            CPPUNIT_ASSERT( listOfChars.front() == 'a');       
        }
        
                
        void setUp() 
        {}
        
        
        void tearDown() 
        {}
        
    private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTListTest);

