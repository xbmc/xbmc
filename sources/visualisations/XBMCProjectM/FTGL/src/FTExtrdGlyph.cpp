#include <iostream>

#include    <math.h>

#include    "FTExtrdGlyph.h"
#include    "FTVectoriser.h"


FTExtrdGlyph::FTExtrdGlyph( FT_GlyphSlot glyph, float depth, bool useDisplayList)
:   FTGlyph( glyph),
    glList(0)
{
    bBox.SetDepth( -depth);
        
    if( ft_glyph_format_outline != glyph->format)
    {
        err = 0x14; // Invalid_Outline
        return;
    }

    FTVectoriser vectoriser( glyph);
    if( ( vectoriser.ContourCount() < 1) || ( vectoriser.PointCount() < 3))
    {
        return;
    }

    unsigned int tesselationIndex;
    
    if(useDisplayList)
    {
        glList = glGenLists(1);
        glNewList( glList, GL_COMPILE);
    }

    vectoriser.MakeMesh( 1.0);
    glNormal3d(0.0, 0.0, 1.0);
    
    unsigned int horizontalTextureScale = glyph->face->size->metrics.x_ppem * 64;
    unsigned int verticalTextureScale = glyph->face->size->metrics.y_ppem * 64;        
    
    const FTMesh* mesh = vectoriser.GetMesh();
    for( tesselationIndex = 0; tesselationIndex < mesh->TesselationCount(); ++tesselationIndex)
    {
        const FTTesselation* subMesh = mesh->Tesselation( tesselationIndex);
        unsigned int polyonType = subMesh->PolygonType();

        glBegin( polyonType);
            for( unsigned int pointIndex = 0; pointIndex < subMesh->PointCount(); ++pointIndex)
            {
                FTPoint point = subMesh->Point(pointIndex);

                glTexCoord2f( point.X() / horizontalTextureScale,
                              point.Y() / verticalTextureScale);
                
                glVertex3f( point.X() / 64.0f,
                            point.Y() / 64.0f,
                            0.0f);
            }
        glEnd();
    }
    
    vectoriser.MakeMesh( -1.0);
    glNormal3d(0.0, 0.0, -1.0);
    
    mesh = vectoriser.GetMesh();
    for( tesselationIndex = 0; tesselationIndex < mesh->TesselationCount(); ++tesselationIndex)
    {
        const FTTesselation* subMesh = mesh->Tesselation( tesselationIndex);
        unsigned int polyonType = subMesh->PolygonType();

        glBegin( polyonType);
            for( unsigned int pointIndex = 0; pointIndex < subMesh->PointCount(); ++pointIndex)
            {
                FTPoint point = subMesh->Point(pointIndex);

                glTexCoord2f( subMesh->Point(pointIndex).X() / horizontalTextureScale,
                              subMesh->Point(pointIndex).Y() / verticalTextureScale);
                
                glVertex3f( subMesh->Point( pointIndex).X() / 64.0f,
                            subMesh->Point( pointIndex).Y() / 64.0f,
                            -depth);
            }
        glEnd();
    }
    
    int contourFlag = vectoriser.ContourFlag();
    
    for( size_t c = 0; c < vectoriser.ContourCount(); ++c)
    {
        const FTContour* contour = vectoriser.Contour(c);
        unsigned int numberOfPoints = contour->PointCount();
        
        glBegin( GL_QUAD_STRIP);
            for( unsigned int j = 0; j <= numberOfPoints; ++j)
            {
                unsigned int pointIndex = ( j == numberOfPoints) ? 0 : j;
                unsigned int nextPointIndex = ( pointIndex == numberOfPoints - 1) ? 0 : pointIndex + 1;
                
                FTPoint point = contour->Point(pointIndex);

                FTPoint normal = GetNormal( point, contour->Point(nextPointIndex));
                if(normal != FTPoint( 0.0f, 0.0f, 0.0f))
                {                   
                    glNormal3dv(static_cast<const FTGL_DOUBLE*>(normal));
                }

                if( contourFlag & ft_outline_reverse_fill)
                {
                    glTexCoord2f( point.X() / horizontalTextureScale,
                                  point.X() / verticalTextureScale);
                
                    glVertex3f( point.X() / 64.0f, point.Y() / 64.0f, 0.0f);
                    glVertex3f( point.X() / 64.0f, point.Y() / 64.0f, -depth);
                }
                else
                {
                    glTexCoord2f( point.X() / horizontalTextureScale,
                                  point.Y() / verticalTextureScale);
                
                    glVertex3f( point.X() / 64.0f, point.Y() / 64.0f, -depth);
                    glVertex3f( point.X() / 64.0f, point.Y() / 64.0f, 0.0f);
                }
            }
        glEnd();
    }
        
    if(useDisplayList)
    {
        glEndList();
    }
}


FTExtrdGlyph::~FTExtrdGlyph()
{
    glDeleteLists( glList, 1);
}


const FTPoint& FTExtrdGlyph::Render( const FTPoint& pen)
{
    glTranslatef( pen.X(), pen.Y(), 0);
    
    if( glList)
    {
        glCallList( glList);    
    }
    
    return advance;
}


FTPoint FTExtrdGlyph::GetNormal( const FTPoint &a, const FTPoint &b)
{
    float vectorX = a.X() - b.X();
    float vectorY = a.Y() - b.Y();
                              
    float length = sqrt( vectorX * vectorX + vectorY * vectorY );
    
    if( length > 0.01f)
    {
        length = 1 / length;
    }
    else
    {
        length = 0.0f;
    }
    
    return FTPoint( -vectorY * length,
                     vectorX * length,
                     0.0f);
}

