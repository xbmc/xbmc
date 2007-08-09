#include    "FTVectoriser.h"
#include    "FTGL.h"

#ifndef CALLBACK
#define CALLBACK
#endif

#ifdef __APPLE_CC__    
    typedef GLvoid (*GLUTesselatorFunction)(...);
#elif defined( __mips ) || defined( __linux__ ) || defined( __FreeBSD__ ) || defined( __OpenBSD__ ) || defined( __sun ) || defined (__CYGWIN__)
    typedef GLvoid (*GLUTesselatorFunction)();
#elif defined ( WIN32)
    typedef GLvoid (CALLBACK *GLUTesselatorFunction)( );
#else
    #error "Error - need to define type GLUTesselatorFunction for this platform/compiler"
#endif


void CALLBACK ftglError( GLenum errCode, FTMesh* mesh)
{
    mesh->Error( errCode);
}


void CALLBACK ftglVertex( void* data, FTMesh* mesh)
{
    FTGL_DOUBLE* vertex = static_cast<FTGL_DOUBLE*>(data);
    mesh->AddPoint( vertex[0], vertex[1], vertex[2]);
}


void CALLBACK ftglCombine( FTGL_DOUBLE coords[3], void* vertex_data[4], GLfloat weight[4], void** outData, FTMesh* mesh)
{
    const FTGL_DOUBLE* vertex = static_cast<const FTGL_DOUBLE*>(coords);
    *outData = const_cast<FTGL_DOUBLE*>(mesh->Combine( vertex[0], vertex[1], vertex[2]));
}
        

void CALLBACK ftglBegin( GLenum type, FTMesh* mesh)
{
    mesh->Begin( type);
}


void CALLBACK ftglEnd( FTMesh* mesh)
{
    mesh->End();
}


FTMesh::FTMesh()
:	currentTesselation(0),
    err(0)
{
    tesselationList.reserve( 16);
}


FTMesh::~FTMesh()
{
    for( size_t t = 0; t < tesselationList.size(); ++t)
    {
        delete tesselationList[t];
    }
    
    tesselationList.clear();
}


void FTMesh::AddPoint( const FTGL_DOUBLE x, const FTGL_DOUBLE y, const FTGL_DOUBLE z)
{
    currentTesselation->AddPoint( x, y, z);
}


const FTGL_DOUBLE* FTMesh::Combine( const FTGL_DOUBLE x, const FTGL_DOUBLE y, const FTGL_DOUBLE z)
{
    tempPointList.push_back( FTPoint( x, y,z));
    return static_cast<const FTGL_DOUBLE*>(tempPointList.back());
}


void FTMesh::Begin( GLenum meshType)
{
    currentTesselation = new FTTesselation( meshType);
}


void FTMesh::End()
{
    tesselationList.push_back( currentTesselation);
}


const FTTesselation* const FTMesh::Tesselation( unsigned int index) const
{
    return ( index < tesselationList.size()) ? tesselationList[index] : NULL;
}


FTVectoriser::FTVectoriser( const FT_GlyphSlot glyph)
:   contourList(0),
    mesh(0),
    ftContourCount(0),
    contourFlag(0)
{
    if( glyph)
    {
        outline = glyph->outline;
        
        ftContourCount = outline.n_contours;
        contourList = 0;
        contourFlag = outline.flags;
        
        ProcessContours();
    }
}


FTVectoriser::~FTVectoriser()
{
    for( size_t c = 0; c < ContourCount(); ++c)
    {
        delete contourList[c];
    }

    delete [] contourList;
    delete mesh;
}


void FTVectoriser::ProcessContours()
{
    short contourLength = 0;
    short startIndex = 0;
    short endIndex = 0;
    
    contourList = new FTContour*[ftContourCount];
    
    for( short contourIndex = 0; contourIndex < ftContourCount; ++contourIndex)
    {
        FT_Vector* pointList = &outline.points[startIndex];
        char* tagList = &outline.tags[startIndex];
        
        endIndex = outline.contours[contourIndex];
        contourLength =  ( endIndex - startIndex) + 1;

        FTContour* contour = new FTContour( pointList, tagList, contourLength);
        
        contourList[contourIndex] = contour;
        
        startIndex = endIndex + 1;
    }
}


size_t FTVectoriser::PointCount()
{
    size_t s = 0;
    for( size_t c = 0; c < ContourCount(); ++c)
    {
        s += contourList[c]->PointCount();
    }
    
    return s;
}


const FTContour* const FTVectoriser::Contour( unsigned int index) const
{
    return ( index < ContourCount()) ? contourList[index] : NULL;
}


void FTVectoriser::MakeMesh( FTGL_DOUBLE zNormal)
{
    if( mesh)
    {
        delete mesh;
    }
        
    mesh = new FTMesh;
    
    GLUtesselator* tobj = gluNewTess();

    gluTessCallback( tobj, GLU_TESS_BEGIN_DATA,     (GLUTesselatorFunction)ftglBegin);
    gluTessCallback( tobj, GLU_TESS_VERTEX_DATA,    (GLUTesselatorFunction)ftglVertex);
    gluTessCallback( tobj, GLU_TESS_COMBINE_DATA,   (GLUTesselatorFunction)ftglCombine);
    gluTessCallback( tobj, GLU_TESS_END_DATA,       (GLUTesselatorFunction)ftglEnd);
    gluTessCallback( tobj, GLU_TESS_ERROR_DATA,     (GLUTesselatorFunction)ftglError);
    
    if( contourFlag & ft_outline_even_odd_fill) // ft_outline_reverse_fill
    {
        gluTessProperty( tobj, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
    }
    else
    {
        gluTessProperty( tobj, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
    }
    
    
    gluTessProperty( tobj, GLU_TESS_TOLERANCE, 0);
    gluTessNormal( tobj, 0.0f, 0.0f, zNormal);
    gluTessBeginPolygon( tobj, mesh);
    
        for( size_t c = 0; c < ContourCount(); ++c)
        {
            const FTContour* contour = contourList[c];

            gluTessBeginContour( tobj);
            
                for( size_t p = 0; p < contour->PointCount(); ++p)
                {
                    const FTGL_DOUBLE* d = contour->Point(p);
                    gluTessVertex( tobj, (GLdouble*)d, (GLdouble*)d);
                }

            gluTessEndContour( tobj);
        }
        
    gluTessEndPolygon( tobj);

    gluDeleteTess( tobj);
}

