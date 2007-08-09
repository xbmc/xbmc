#ifndef     __FTGLTextureFont__
#define     __FTGLTextureFont__

#include "FTFont.h"
#include "FTVector.h"
#include "FTGL.h"

class FTTextureGlyph;


/**
 * FTGLTextureFont is a specialisation of the FTFont class for handling
 * Texture mapped fonts
 *
 * @see     FTFont
 */
class  FTGL_EXPORT FTGLTextureFont : public FTFont
{
    public:
        /**
         * Open and read a font file. Sets Error flag.
         *
         * @param fontFilePath  font file path.
         */
        FTGLTextureFont( const char* fontFilePath);
        
        /**
         * Open and read a font from a buffer in memory. Sets Error flag.
         *
         * @param pBufferBytes  the in-memory buffer
         * @param bufferSizeInBytes  the length of the buffer in bytes
         */
        FTGLTextureFont( const unsigned char *pBufferBytes, size_t bufferSizeInBytes);
        
        /**
         * Destructor
         */
        virtual ~FTGLTextureFont();

        /**
            * Set the char size for the current face.
         *
         * @param size      the face size in points (1/72 inch)
         * @param res       the resolution of the target device.
         * @return          <code>true</code> if size was set correctly
         */
        virtual bool FaceSize( const unsigned int size, const unsigned int res = 72);

        /**
         * Renders a string of characters
         * 
         * @param string    'C' style string to be output.   
         */
        virtual void Render( const char* string);
        
        /**
         * Renders a string of characters
         * 
         * @param string    wchar_t string to be output.     
         */
        virtual void Render( const wchar_t* string);

        
    private:
        /**
         * Construct a FTTextureGlyph.
         *
         * @param glyphIndex The glyph index NOT the char code.
         * @return  An FTTextureGlyph or <code>null</code> on failure.
         */
        inline virtual FTGlyph* MakeGlyph( unsigned int glyphIndex);
                
        /**
         * Get the size of a block of memory required to layout the glyphs
         *
         * Calculates a width and height based on the glyph sizes and the
         * number of glyphs. It over estimates.
         */
        inline void CalculateTextureSize();

        /**
         * Creates a 'blank' OpenGL texture object.
         *
         * The format is GL_ALPHA and the params are
         * GL_TEXTURE_WRAP_S = GL_CLAMP
         * GL_TEXTURE_WRAP_T = GL_CLAMP
         * GL_TEXTURE_MAG_FILTER = GL_LINEAR
         * GL_TEXTURE_MIN_FILTER = GL_LINEAR
         * Note that mipmapping is NOT used
         */
        inline GLuint CreateTexture();
        
        /**
         * The maximum texture dimension on this OpenGL implemetation
         */
        GLsizei maximumGLTextureSize;
        
        /**
         * The minimum texture width required to hold the glyphs
         */
        GLsizei textureWidth;
        
        /**
         * The minimum texture height required to hold the glyphs
         */
        GLsizei textureHeight;
        
        /**
         *An array of texture ids
         */
         FTVector<GLuint> textureIDList;
        
        /**
         * The max height for glyphs in the current font
         */
        int glyphHeight;

        /**
         * The max width for glyphs in the current font
         */
        int glyphWidth;

        /**
         * A value to be added to the height and width to ensure that
         * glyphs don't overlap in the texture
         */
        unsigned int padding;
        
        /**
         *
         */
         unsigned int numGlyphs;
        
        /**
         */
        unsigned int remGlyphs;

        /**
         */
        int xOffset;

        /**
         */
        int yOffset;

};


#endif // __FTGLTextureFont__


