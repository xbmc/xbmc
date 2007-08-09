#ifndef __FTGLBitmapFont__
#define __FTGLBitmapFont__

#include "FTFont.h"
#include "FTGL.h"


class FTGlyph;

/**
 * FTGLBitmapFont is a specialisation of the FTFont class for handling
 * Bitmap fonts
 *
 * @see     FTFont
 */
class FTGL_EXPORT FTGLBitmapFont : public FTFont
{
    public:
        /**
         * Open and read a font file. Sets Error flag.
         *
         * @param fontFilePath  font file path.
         */
        FTGLBitmapFont( const char* fontFilePath);

        /**
         * Open and read a font from a buffer in memory. Sets Error flag.
         *
         * @param pBufferBytes  the in-memory buffer
         * @param bufferSizeInBytes  the length of the buffer in bytes
         */
        FTGLBitmapFont( const unsigned char *pBufferBytes, size_t bufferSizeInBytes);

        /**
         * Destructor
         */
        ~FTGLBitmapFont();
        
        /**
         * Renders a string of characters
         * 
         * @param string    'C' style string to be output.   
         */
        void Render( const char* string);

        /**
         * Renders a string of characters
         * 
         * @param string    'C' style wide string to be output.  
         */
        void Render( const wchar_t* string);

        // attributes
        
    private:
        /**
         * Construct a FTBitmapGlyph.
         *
         * @param g The glyph index NOT the char code.
         * @return  An FTBitmapGlyph or <code>null</code> on failure.
         */
        inline virtual FTGlyph* MakeGlyph( unsigned int g);
                
};
#endif  //  __FTGLBitmapFont__
