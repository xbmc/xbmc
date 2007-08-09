#ifndef     __FTBitmapGlyph__
#define     __FTBitmapGlyph__


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "FTGL.h"
#include "FTGlyph.h"


/**
 * FTBitmapGlyph is a specialisation of FTGlyph for creating bitmaps.
 *
 * It provides the interface between Freetype glyphs and their openGL
 * Renderable counterparts. This is an abstract class and derived classes
 * must implement the <code>Render</code> function. 
 * 
 * @see FTGlyphContainer
 *
 */
class FTGL_EXPORT FTBitmapGlyph : public FTGlyph
{
    public:
        /**
         * Constructor
         *
         * @param glyph The Freetype glyph to be processed
         */
        FTBitmapGlyph( FT_GlyphSlot glyph);

        /**
         * Destructor
         */
        virtual ~FTBitmapGlyph();

        /**
         * Renders this glyph at the current pen position.
         *
         * @param pen   The current pen position.
         * @return      The advance distance for this glyph.
         */
        virtual const FTPoint& Render( const FTPoint& pen);
        
    private:
        /**
         * The width of the glyph 'image'
         */
        unsigned int destWidth;

        /**
         * The height of the glyph 'image'
         */
        unsigned int destHeight;

        /**
         * The pitch of the glyph 'image'
         */
        unsigned int destPitch;

        /**
         * Vector from the pen position to the topleft corner of the bitmap
         */
        FTPoint pos;
        
        /**
         * Pointer to the 'image' data
         */
        unsigned char* data;
        
};


#endif  //  __FTBitmapGlyph__

