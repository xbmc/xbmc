#ifndef     __FTExtrdGlyph__
#define     __FTExtrdGlyph__

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "FTGL.h"
#include "FTGlyph.h"

class FTVectoriser;

/**
 * FTExtrdGlyph is a specialisation of FTGlyph for creating tessellated
 * extruded polygon glyphs.
 * 
 * @see FTGlyphContainer
 * @see FTVectoriser
 *
 */
class FTGL_EXPORT FTExtrdGlyph : public FTGlyph
{
    public:
        /**
         * Constructor. Sets the Error to Invalid_Outline if the glyph isn't an outline.
         *
         * @param glyph The Freetype glyph to be processed
         * @param depth The distance along the z axis to extrude the glyph
         * @param useDisplayList Enable or disable the use of Display Lists for this glyph
         *                       <code>true</code> turns ON display lists.
         *                       <code>false</code> turns OFF display lists.
         */
        FTExtrdGlyph( FT_GlyphSlot glyph, float depth, bool useDisplayList);

        /**
         * Destructor
         */
        virtual ~FTExtrdGlyph();

        /**
         * Renders this glyph at the current pen position.
         *
         * @param pen   The current pen position.
         * @return      The advance distance for this glyph.
         */
        virtual const FTPoint& Render( const FTPoint& pen);
        
    private:
        /**
         * Calculate the normal vector to 2 points. This is 2D and ignores
         * the z component. The normal will be normalised
         *
         * @param a
         * @param b
         * @return
         */
        FTPoint GetNormal( const FTPoint &a, const FTPoint &b);
        
        
        /**
         * OpenGL display list
         */
        GLuint glList;
    
};


#endif  //  __FTExtrdGlyph__

