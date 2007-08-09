#include <cassert>
#include <string> // For memset

#include "FTGLTextureFont.h"
#include "FTTextureGlyph.h"


inline GLuint NextPowerOf2( GLuint in)
{
     in -= 1;

     in |= in >> 16;
     in |= in >> 8;
     in |= in >> 4;
     in |= in >> 2;
     in |= in >> 1;

     return in + 1;
}


FTGLTextureFont::FTGLTextureFont( const char* fontFilePath)
:   FTFont( fontFilePath),
    maximumGLTextureSize(0),
    textureWidth(0),
    textureHeight(0),
    glyphHeight(0),
    glyphWidth(0),
    padding(3),
    xOffset(0),
    yOffset(0)
{
    remGlyphs = numGlyphs = face.GlyphCount();
}


FTGLTextureFont::FTGLTextureFont( const unsigned char *pBufferBytes, size_t bufferSizeInBytes)
:   FTFont( pBufferBytes, bufferSizeInBytes),
    maximumGLTextureSize(0),
    textureWidth(0),
    textureHeight(0),
    glyphHeight(0),
    glyphWidth(0),
    padding(3),
    xOffset(0),
    yOffset(0)
{
    remGlyphs = numGlyphs = face.GlyphCount();
}


FTGLTextureFont::~FTGLTextureFont()
{
    glDeleteTextures( textureIDList.size(), (const GLuint*)&textureIDList[0]);
}


FTGlyph* FTGLTextureFont::MakeGlyph( unsigned int glyphIndex)
{
    FT_GlyphSlot ftGlyph = face.Glyph( glyphIndex, FT_LOAD_NO_HINTING);
    
    if( ftGlyph)
    {
        glyphHeight = static_cast<int>( charSize.Height());
        glyphWidth = static_cast<int>( charSize.Width());
        
        if( textureIDList.empty())
        {
            textureIDList.push_back( CreateTexture());
            xOffset = yOffset = padding;
        }
        
        if( xOffset > ( textureWidth - glyphWidth))
        {
            xOffset = padding;
            yOffset += glyphHeight;
            
            if( yOffset > ( textureHeight - glyphHeight))
            {
                textureIDList.push_back( CreateTexture());
                yOffset = padding;
            }
        }
        
        FTTextureGlyph* tempGlyph = new FTTextureGlyph( ftGlyph, textureIDList[textureIDList.size() - 1],
                                                        xOffset, yOffset, textureWidth, textureHeight);
        xOffset += static_cast<int>( tempGlyph->BBox().upperX - tempGlyph->BBox().lowerX + padding);
        
        --remGlyphs;
        return tempGlyph;
    }
    
    err = face.Error();
    return NULL;
}


void FTGLTextureFont::CalculateTextureSize()
{
    if( !maximumGLTextureSize)
    {
        glGetIntegerv( GL_MAX_TEXTURE_SIZE, (GLint*)&maximumGLTextureSize);
        assert(maximumGLTextureSize); // If you hit this then you have an invalid OpenGL context.
    }
    
    textureWidth = NextPowerOf2( (remGlyphs * glyphWidth) + ( padding * 2));
    textureWidth = textureWidth > maximumGLTextureSize ? maximumGLTextureSize : textureWidth;
    
    int h = static_cast<int>( (textureWidth - ( padding * 2)) / glyphWidth);
        
    textureHeight = NextPowerOf2( (( numGlyphs / h) + 1) * glyphHeight);
    textureHeight = textureHeight > maximumGLTextureSize ? maximumGLTextureSize : textureHeight;
}


GLuint FTGLTextureFont::CreateTexture()
{   
    CalculateTextureSize();
    
    int totalMemory = textureWidth * textureHeight;
    unsigned char* textureMemory = new unsigned char[totalMemory];
    memset( textureMemory, 0, totalMemory);

    GLuint textID;
    glGenTextures( 1, (GLuint*)&textID);

    glBindTexture( GL_TEXTURE_2D, textID);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, textureWidth, textureHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, textureMemory);

    delete [] textureMemory;

    return textID;
}


bool FTGLTextureFont::FaceSize( const unsigned int size, const unsigned int res)
{
    if( !textureIDList.empty())
    {
        glDeleteTextures( textureIDList.size(), (const GLuint*)&textureIDList[0]);
        textureIDList.clear();
        remGlyphs = numGlyphs = face.GlyphCount();
    }

    return FTFont::FaceSize( size, res);
}


void FTGLTextureFont::Render( const char* string)
{   
    glPushAttrib( GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    
    glEnable(GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_ONE

    FTTextureGlyph::ResetActiveTexture();
    
    FTFont::Render( string);

    glPopAttrib();
}


void FTGLTextureFont::Render( const wchar_t* string)
{   
    glPushAttrib( GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    
    glEnable(GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_ONE
    
    FTTextureGlyph::ResetActiveTexture();
    
    FTFont::Render( string);
    
    glPopAttrib();
}

