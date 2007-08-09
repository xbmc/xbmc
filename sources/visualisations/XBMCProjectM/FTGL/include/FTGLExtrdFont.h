#ifndef __FTGLExtrdFont__
#define __FTGLExtrdFont__

#include "FTFont.h"
#include "FTGL.h"

class FTGlyph;

/**
 * FTGLExtrdFont is a specialisation of the FTFont class for handling
 * extruded Polygon fonts
 *
 * @see		FTFont
 * @see		FTGLPolygonFont
 */
class FTGL_EXPORT FTGLExtrdFont : public FTFont
{
    public:
        /**
         * Open and read a font file. Sets Error flag.
         *
         * @param fontFilePath  font file path.
         */
        FTGLExtrdFont( const char* fontFilePath);

        /**
         * Open and read a font from a buffer in memory. Sets Error flag.
         *
         * @param pBufferBytes  the in-memory buffer
         * @param bufferSizeInBytes  the length of the buffer in bytes
         */
        FTGLExtrdFont( const unsigned char *pBufferBytes, size_t bufferSizeInBytes);

        /**
         * Destructor
         */
        ~FTGLExtrdFont();
		
        /**
         * Set the extrusion distance for the font. 
         *
         * @param d  The extrusion distance.
         */
        void Depth( float d) { depth = d;}
		
    private:
        /**
         * Construct a FTPolyGlyph.
         *
         * @param glyphIndex The glyph index NOT the char code.
         * @return	An FTExtrdGlyph or <code>null</code> on failure.
         */
        inline virtual FTGlyph* MakeGlyph( unsigned int glyphIndex);
		
        /**
         * The extrusion distance for the font. 
         */
        float depth;
};


#endif	//	__FTGLExtrdFont__

