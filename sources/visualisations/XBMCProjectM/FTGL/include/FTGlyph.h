#ifndef     __FTGlyph__
#define     __FTGlyph__

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "FTBBox.h"
#include "FTPoint.h"
#include "FTGL.h"


/**
 * FTGlyph is the base class for FTGL glyphs.
 *
 * It provides the interface between Freetype glyphs and their openGL
 * renderable counterparts. This is an abstract class and derived classes
 * must implement the <code>render</code> function. 
 * 
 * @see FTGlyphContainer
 * @see FTBBox
 * @see FTPoint
 *
 */
class FTGL_EXPORT FTGlyph
{
    public:
        /**
         * Constructor
         *
         * @param glyph The Freetype glyph to be processed
         * @param useDisplayList Enable or disable the use of Display Lists for this glyph
         *                       <code>true</code> turns ON display lists.
         *                       <code>false</code> turns OFF display lists.
         */
        FTGlyph( FT_GlyphSlot glyph, bool useDisplayList = true);

        /**
         * Destructor
         */
        virtual ~FTGlyph();

        /**
         * Renders this glyph at the current pen position.
         *
         * @param pen   The current pen position.
         * @return      The advance distance for this glyph.
         */
        virtual const FTPoint& Render( const FTPoint& pen) = 0;
        
        /**
         * Return the advance width for this glyph.
         *
         * @return  advance width.
         */
        const FTPoint& Advance() const { return advance;}
        
        /**
         * Return the bounding box for this glyph.
         *
         * @return  bounding box.
         */
        const FTBBox& BBox() const { return bBox;}
        
        /**
         * Queries for errors.
         *
         * @return  The current error code.
         */
        FT_Error Error() const { return err;}
        
    protected:
        /**
         * The advance distance for this glyph
         */
        FTPoint advance;

        /**
         * The bounding box of this glyph.
         */
        FTBBox bBox;
        
        /**
         * Flag to enable or disable the use of Display Lists inside FTGL
         * <code>true</code> turns ON display lists.
         * <code>false</code> turns OFF display lists.
         */
        bool useDisplayList;
        
        /**
         * Current error code. Zero means no error.
         */
        FT_Error err;
        
    private:

};


#endif  //  __FTGlyph__

