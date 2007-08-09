#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>

#include "FTVectoriser.h"


void CALLBACK ftglError( GLenum errCode, FTMesh* mesh);
void CALLBACK ftglVertex( void* data, FTMesh* mesh);
void CALLBACK ftglBegin( GLenum type, FTMesh* mesh);
void CALLBACK ftglEnd( FTMesh* mesh);
void CALLBACK ftglCombine( FTGL_DOUBLE coords[3], void* vertex_data[4], GLfloat weight[4], void** outData, FTMesh* mesh);


static float POINT_DATA[] = 
{
    10, 3, 0.7,
    -53, 2000, 23,
    77, -2.4, 765,
    117.5,  0.02, -99,
    10,    3,    -0.87,
    117.5, 0.02, 34.76,
    0.27,  44.4, 3000,
    10,    3,    0
};

class FTMeshTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE( FTMeshTest);
        CPPUNIT_TEST( testGetTesselation);
        CPPUNIT_TEST( testAddPoint);
        CPPUNIT_TEST( testTooManyPoints);
    CPPUNIT_TEST_SUITE_END();
        
    public:
        FTMeshTest() : CppUnit::TestCase( "FTMesh Test")
        {}
        
        FTMeshTest( const std::string& name) : CppUnit::TestCase(name)
        {}

        void testGetTesselation()
        {
            FTMesh mesh;

            CPPUNIT_ASSERT( mesh.Tesselation(0) == NULL);

            ftglBegin( GL_TRIANGLES, &mesh);
            ftglVertex( &POINT_DATA[0], &mesh);
            ftglVertex( &POINT_DATA[3], &mesh);
            ftglVertex( &POINT_DATA[6], &mesh);
            ftglVertex( &POINT_DATA[9], &mesh);
            ftglEnd( &mesh);

            CPPUNIT_ASSERT( mesh.Tesselation(0));
            CPPUNIT_ASSERT( mesh.Tesselation(10) == NULL);
        }
        
        
        void testAddPoint()
        {
            FTGL_DOUBLE testPoint[3] = { 1, 2, 3};
            FTGL_DOUBLE* hole[] = { 0, 0, 0, 0};

            FTMesh mesh;
            CPPUNIT_ASSERT( mesh.TesselationCount() == 0);
            
            ftglBegin( GL_TRIANGLES, &mesh);
            ftglVertex( &POINT_DATA[0], &mesh);
            ftglVertex( &POINT_DATA[3], &mesh);
            ftglVertex( &POINT_DATA[6], &mesh);
            ftglVertex( &POINT_DATA[9], &mesh);
            ftglEnd( &mesh);
            
            CPPUNIT_ASSERT( mesh.TesselationCount() == 1);
            CPPUNIT_ASSERT( mesh.Tesselation(0)->PolygonType() == GL_TRIANGLES);
            CPPUNIT_ASSERT( mesh.Tesselation(0)->PointCount() == 4);
            CPPUNIT_ASSERT( mesh.Error() == 0);
            
            ftglBegin( GL_QUADS, &mesh);
            ftglVertex( &POINT_DATA[12], &mesh);
            ftglVertex( &POINT_DATA[15], &mesh);
            ftglError( 2, &mesh);
            ftglVertex( &POINT_DATA[18], &mesh);
            ftglCombine( testPoint, NULL, NULL, (void**)hole, &mesh);
            ftglVertex( &POINT_DATA[21], &mesh);
            ftglError( 3, &mesh);
            ftglEnd( &mesh);

            CPPUNIT_ASSERT( mesh.TesselationCount() == 2);
            CPPUNIT_ASSERT( mesh.Tesselation(0)->PointCount() == 4);
            CPPUNIT_ASSERT( mesh.Tesselation(1)->PolygonType() == GL_QUADS);
            CPPUNIT_ASSERT( mesh.Tesselation(1)->PointCount() == 4);
            CPPUNIT_ASSERT( mesh.Error() == 3);
            
            CPPUNIT_ASSERT( mesh.TesselationCount() == 2);
        }
        
        
        void testTooManyPoints()
        {
            FTGL_DOUBLE testPoint[3] = { 1, 2, 3};

            FTGL_DOUBLE* testOutput[] = { 0, 0, 0, 0};
            FTGL_DOUBLE* hole[] = { 0, 0, 0, 0};
            
            FTMesh mesh;
			unsigned int x;
        
            ftglBegin( GL_TRIANGLES, &mesh);
            ftglCombine( testPoint, NULL, NULL, (void**)testOutput, &mesh);
            
            for( x = 0; x < 200; ++x)
            {
                ftglCombine( testPoint, NULL, NULL, (void**)hole, &mesh);
            }

            CPPUNIT_ASSERT( *testOutput == static_cast<const FTGL_DOUBLE*>(mesh.TempPointList().front()));
            
            for( x = 201; x < 300; ++x)
            {
                ftglCombine( testPoint, NULL, NULL, (void**)hole, &mesh);            
            }

            ftglEnd( &mesh);
            
            CPPUNIT_ASSERT( *testOutput == static_cast<const FTGL_DOUBLE*>(mesh.TempPointList().front()));
        }
        
        void setUp() 
        {}
        
        
        void tearDown() 
        {}
        
    private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( FTMeshTest);

