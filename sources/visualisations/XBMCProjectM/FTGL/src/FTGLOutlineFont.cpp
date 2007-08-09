#include    "FTGLOutlineFont.h"
#include    "FTOutlineGlyph.h"


FTGLOutlineFont::FTGLOutlineFont( const char* fontFilePath)
:   FTFont( fontFilePath)
{}


FTGLOutlineFont::FTGLOutlineFont( const unsigned char *pBufferBytes, size_t bufferSizeInBytes)
:   FTFont( pBufferBytes, bufferSizeInBytes)
{}


FTGLOutlineFont::~FTGLOutlineFont()
{}


FTGlyph* FTGLOutlineFont::MakeGlyph( unsigned int g)
{
    FT_GlyphSlot ftGlyph = face.Glyph( g, FT_LOAD_NO_HINTING);

    if( ftGlyph)
    {
        FTOutlineGlyph* tempGlyph = new FTOutlineGlyph( ftGlyph, useDisplayLists);
        return tempGlyph;
    }

    err = face.Error();
    return NULL;
}


void FTGLOutlineFont::Render( const char* string)
{   
    glPushAttrib( GL_ENABLE_BIT | GL_HINT_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    
    glDisable( GL_TEXTURE_2D);
    
    glEnable( GL_LINE_SMOOTH);
    glHint( GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    glEnable(GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_ONE

    FTFont::Render( string);

    glPopAttrib();
}


void FTGLOutlineFont::Render( const wchar_t* string)
{   
    glPushAttrib( GL_ENABLE_BIT | GL_HINT_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    
    glDisable( GL_TEXTURE_2D);
    
    glEnable( GL_LINE_SMOOTH);
    glHint( GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    glEnable(GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_ONE

    FTFont::Render( string);

    glPopAttrib();
}

