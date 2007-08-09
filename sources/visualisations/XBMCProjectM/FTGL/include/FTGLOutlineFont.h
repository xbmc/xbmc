#ifndef     __FTGLOutlineFont__
#define     __FTGLOutlineFont__


#include "FTFont.h"
#include "FTGL.h"

class FTGlyph;


/**
 * FTGLOutlineFont is a specialisation of the FTFont class for handling
 * Vector Outline fonts
 *
 * @see     FTFont
 */
class FTGL_EXPORT FTGLOutlineFont : public FTFont
{
    public:
        /**
         * Open and read a font file. Sets Error flag.
         *
         * @param fontFilePath  font file path.
         */
        FTGLOutlineFont( const char* fontFilePath);
        
        /**
         * Open and read a font from a buffer in memory. Sets Error flag.
         *
         * @param pBufferBytes  the in-memory buffer
         * @param bufferSizeInBytes  the length of the buffer in bytes
         */
        FTGLOutlineFont( const unsigned char *pBufferBytes, size_t bufferSizeInBytes);
        
        /**
         * Destructor
         */
        ~FTGLOutlineFont();
        
        /**
         * Renders a string of characters
         * 
         * @param string    'C' style string to be output.   
         */
        void Render( const char* string);
        
        /**
         * Renders a string of characters
         * 
         * @param string    wchar_t string to be output.     
         */
        void Render( const wchar_t* string);

    private:
        /**
         * Construct a FTOutlineGlyph.
         *
         * @param g The glyph index NOT the char code.
         * @return  An FTOutlineGlyph or <code>null</code> on failure.
         */
        inline virtual FTGlyph* MakeGlyph( unsigned int g);
        
};
#endif // __FTGLOutlineFont__
