#include    "FTGLExtrdFont.h"
#include    "FTExtrdGlyph.h"


FTGLExtrdFont::FTGLExtrdFont( const char* fontFilePath)
:   FTFont( fontFilePath),
    depth( 0.0f)
{}


FTGLExtrdFont::FTGLExtrdFont( const unsigned char *pBufferBytes, size_t bufferSizeInBytes)
:   FTFont( pBufferBytes, bufferSizeInBytes),
    depth( 0.0f)
{}


FTGLExtrdFont::~FTGLExtrdFont()
{}


FTGlyph* FTGLExtrdFont::MakeGlyph( unsigned int glyphIndex)
{
    FT_GlyphSlot ftGlyph = face.Glyph( glyphIndex, FT_LOAD_NO_HINTING);

    if( ftGlyph)
    {
        FTExtrdGlyph* tempGlyph = new FTExtrdGlyph( ftGlyph, depth, useDisplayLists);
        return tempGlyph;
    }

    err = face.Error();
    return NULL;
}


