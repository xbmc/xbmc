#include    "FTGLPolygonFont.h"
#include    "FTPolyGlyph.h"


FTGLPolygonFont::FTGLPolygonFont( const char* fontFilePath)
:   FTFont( fontFilePath)
{}


FTGLPolygonFont::FTGLPolygonFont( const unsigned char *pBufferBytes, size_t bufferSizeInBytes)
:   FTFont( pBufferBytes, bufferSizeInBytes)
{}


FTGLPolygonFont::~FTGLPolygonFont()
{}


FTGlyph* FTGLPolygonFont::MakeGlyph( unsigned int g)
{
    FT_GlyphSlot ftGlyph = face.Glyph( g, FT_LOAD_NO_HINTING);

    if( ftGlyph)
    {
        FTPolyGlyph* tempGlyph = new FTPolyGlyph( ftGlyph, useDisplayLists);
        return tempGlyph;
    }

    err = face.Error();
    return NULL;
}


