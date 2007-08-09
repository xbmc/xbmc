#ifndef     __FTSize__
#define     __FTSize__


#include <ft2build.h>
#include FT_FREETYPE_H

#include "FTGL.h"



/**
 * FTSize class provides an abstraction layer for the Freetype Size.
 *
 * @see "Freetype 2 Documentation"
 *
 */
class FTGL_EXPORT FTSize
{
    public:
        /**
         * Default Constructor
         */
         FTSize();
        
        /**
         * Destructor
         */
        virtual ~FTSize();
        
        /**
         * Sets the char size for the current face.
         *
         * This doesn't guarantee that the size was set correctly. Clients
         * should check errors.
         *
         * @param face           Parent face for this size object
         * @param point_size     the face size in points (1/72 inch)
         * @param x_resolution   the horizontal resolution of the target device.
         * @param y_resolution   the vertical resolution of the target device.
         * @return          <code>true</code> if the size has been set. Clients should check Error() for more information if this function returns false()
         */
        bool CharSize( FT_Face* face, unsigned int point_size, unsigned int x_resolution, unsigned int y_resolution);
        
        /**
         * get the char size for the current face.
         *
         * @return The char size in points
         */
        unsigned int CharSize() const;
        
        /**
         * Gets the global ascender height for the face in pixels.
         *
         * @return  Ascender height
         */
        float Ascender() const;
        
        /**
         * Gets the global descender height for the face in pixels.
         *
         * @return  Ascender height
         */
        float Descender() const;
        
        /**
         * Gets the global face height for the face.
         *
         * If the face is scalable this returns the height of the global
         * bounding box which ensures that any glyph will be less than or
         * equal to this height. If the font isn't scalable there is no
         * guarantee that glyphs will not be taller than this value.
         *
         * @return  height in pixels.
         */
        float Height() const;
        
        /**
         * Gets the global face width for the face.
         *
         * If the face is scalable this returns the width of the global
         * bounding box which ensures that any glyph will be less than or
         * equal to this width. If the font isn't scalable this value is
         * the max_advance for the face.
         *
         * @return  width in pixels.
         */
        float Width() const;
        
        /**
         * Gets the underline position for the face.
         *
         * @return  underline position in pixels
         */
        float Underline() const;

        /**
         * Queries for errors.
         *
         * @return  The current error code.
         */
        FT_Error Error() const { return err; }
        
    private:
        /**
         * The current Freetype face that this FTSize object relates to.
         */
        FT_Face* ftFace;
        
        /**
         *  The Freetype size.
         */
        FT_Size ftSize;
        
        /**
         *  The size in points.
         */
        unsigned int size;

        /**
         *  The horizontal resolution.
         */
        unsigned int xResolution;

        /**
         *  The vertical resolution.
         */
        unsigned int yResolution;

        /**
         * Current error code. Zero means no error.
         */
        FT_Error err;
        
};

#endif  //  __FTSize__

