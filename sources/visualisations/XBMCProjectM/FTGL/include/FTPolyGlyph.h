#ifndef     __FTPolyGlyph__
#define     __FTPolyGlyph__


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "FTGL.h"
#include "FTGlyph.h"

class FTVectoriser;

/**
 * FTPolyGlyph is a specialisation of FTGlyph for creating tessellated
 * polygon glyphs.
 * 
 * @see FTGlyphContainer
 * @see FTVectoriser
 *
 */
class FTGL_EXPORT FTPolyGlyph : public FTGlyph
{
    public:
        /**
         * Constructor. Sets the Error to Invalid_Outline if the glyphs isn't an outline.
         *
         * @param glyph The Freetype glyph to be processed
         * @param glyph The Freetype glyph to be processed
         * @param useDisplayList Enable or disable the use of Display Lists for this glyph
         *                       <code>true</code> turns ON display lists.
         *                       <code>false</code> turns OFF display lists.
         */
        FTPolyGlyph( FT_GlyphSlot glyph, bool useDisplayList);

        /**
         * Destructor
         */
        virtual ~FTPolyGlyph();

        /**
         * Renders this glyph at the current pen position.
         *
         * @param pen   The current pen position.
         * @return      The advance distance for this glyph.
         */
        virtual const FTPoint& Render( const FTPoint& pen);
        
    private:
        /**
         * OpenGL display list
         */
        GLuint glList;
    
};


#endif  //  __FTPolyGlyph__

