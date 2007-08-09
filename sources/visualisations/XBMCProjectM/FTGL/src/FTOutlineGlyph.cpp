#include    "FTOutlineGlyph.h"
#include    "FTVectoriser.h"


FTOutlineGlyph::FTOutlineGlyph( FT_GlyphSlot glyph, bool useDisplayList)
:   FTGlyph( glyph),
    glList(0)
{
    if( ft_glyph_format_outline != glyph->format)
    {
        err = 0x14; // Invalid_Outline
        return;
    }

    FTVectoriser vectoriser( glyph);

    size_t numContours = vectoriser.ContourCount();
    if ( ( numContours < 1) || ( vectoriser.PointCount() < 3))
    {
        return;
    }

    if(useDisplayList)
    {
        glList = glGenLists(1);
        glNewList( glList, GL_COMPILE);
    }
    
    for( unsigned int c = 0; c < numContours; ++c)
    {
        const FTContour* contour = vectoriser.Contour(c);
        
        glBegin( GL_LINE_LOOP);
            for( unsigned int pointIndex = 0; pointIndex < contour->PointCount(); ++pointIndex)
            {
                FTPoint point = contour->Point(pointIndex);
                glVertex2f( point.X() / 64.0f, point.Y() / 64.0f);
            }
        glEnd();
    }

    if(useDisplayList)
    {
        glEndList();
    }
}


FTOutlineGlyph::~FTOutlineGlyph()
{
    glDeleteLists( glList, 1);
}


const FTPoint& FTOutlineGlyph::Render( const FTPoint& pen)
{
    glTranslatef( pen.X(), pen.Y(), 0.0f);

    if( glList)
    {
        glCallList( glList);
    }
    
    return advance;
}

