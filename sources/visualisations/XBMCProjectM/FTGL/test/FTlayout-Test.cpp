#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>

#include "Fontdefs.h"
#include "layout/FTLayoutFont.h"

static const int SCRIPT = 2; // arabic

class FTLayoutTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTLayoutTest);
        CPPUNIT_TEST( testConstructor);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTLayoutTest() : CppUnit::TestCase( "FTLayout Test")
        {}
        
        FTLayoutTest( const std::string& name) : CppUnit::TestCase(name) {}

        void testConstructor()
        {
            FTLayoutFont font( FONT_FILE, SCRIPT);
            CPPUNIT_ASSERT( font.Error() == 0);
        }
        
        void setUp() 
        {}
        
        
        void tearDown() 
        {}
        
    private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTLayoutTest);

