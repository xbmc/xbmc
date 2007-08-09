#ifndef     __FTGLPolygonFont__
#define     __FTGLPolygonFont__


#include "FTFont.h"
#include "FTGL.h"

class FTGlyph;


/**
 * FTGLPolygonFont is a specialisation of the FTFont class for handling
 * tesselated Polygon Mesh fonts
 *
 * @see     FTFont
 */
class FTGL_EXPORT FTGLPolygonFont : public FTFont
{
    public:
        /**
         * Open and read a font file. Sets Error flag.
         *
         * @param fontFilePath  font file path.
         */
        FTGLPolygonFont( const char* fontFilePath);
        
        /**
         * Open and read a font from a buffer in memory. Sets Error flag.
         *
         * @param pBufferBytes  the in-memory buffer
         * @param bufferSizeInBytes  the length of the buffer in bytes
         */
        FTGLPolygonFont( const unsigned char *pBufferBytes, size_t bufferSizeInBytes);
        
        /**
         * Destructor
         */
        ~FTGLPolygonFont();
        
    private:
        /**
         * Construct a FTPolyGlyph.
         *
         * @param g The glyph index NOT the char code.
         * @return  An FTPolyGlyph or <code>null</code> on failure.
         */
        inline virtual FTGlyph* MakeGlyph( unsigned int g);
        
};


#endif  //  __FTGLPolygonFont__

