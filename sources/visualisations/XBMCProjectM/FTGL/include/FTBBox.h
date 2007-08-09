#ifndef     __FTBBox__
#define     __FTBBox__

#include <ft2build.h>
#include FT_FREETYPE_H
//#include FT_GLYPH_H
#include FT_OUTLINE_H

#include "FTGL.h"
#include "FTPoint.h"


/**
 * FTBBox is a convenience class for handling bounding boxes.
 */
class FTGL_EXPORT FTBBox
{
    public:
        /**
         * Default constructor. Bounding box is set to zero.
         */
        FTBBox()
        :   lowerX(0.0f),
            lowerY(0.0f),
            lowerZ(0.0f),
            upperX(0.0f),
            upperY(0.0f),
            upperZ(0.0f)
        {}
        
        /**
         * Constructor.
         */
        FTBBox( float lx, float ly, float lz, float ux, float uy, float uz)
        :   lowerX(lx),
            lowerY(ly),
            lowerZ(lz),
            upperX(ux),
            upperY(uy),
            upperZ(uz)
        {}
        
        /**
         * Constructor. Extracts a bounding box from a freetype glyph. Uses
         * the control box for the glyph. <code>FT_Glyph_Get_CBox()</code>
         *
         * @param glyph A freetype glyph
         */
        FTBBox( FT_GlyphSlot glyph)
        :   lowerX(0.0f),
            lowerY(0.0f),
            lowerZ(0.0f),
            upperX(0.0f),
            upperY(0.0f),
            upperZ(0.0f)
        {
            FT_BBox bbox;
            FT_Outline_Get_CBox( &(glyph->outline), &bbox);

            lowerX = static_cast<float>( bbox.xMin) / 64.0f;
            lowerY = static_cast<float>( bbox.yMin) / 64.0f;
            lowerZ = 0.0f;
            upperX = static_cast<float>( bbox.xMax) / 64.0f;
            upperY = static_cast<float>( bbox.yMax) / 64.0f;
            upperZ = 0.0f;
            
        }       

        /**
         * Destructor
         */
        ~FTBBox()
        {}
        

        /**
         * Move the Bounding Box by a vector.
         *
         * @param distance The distance to move the bbox in 3D space.
         */
        FTBBox& Move( FTPoint distance)
        {
            lowerX += distance.X();
            lowerY += distance.Y();
            lowerZ += distance.Z();
            upperX += distance.X();
            upperY += distance.Y();
            upperZ += distance.Z();
            return *this;
        }

        FTBBox& operator += ( const FTBBox& bbox) 
        {
            lowerX = bbox.lowerX < lowerX? bbox.lowerX: lowerX; 
            lowerY = bbox.lowerY < lowerY? bbox.lowerY: lowerY;
            lowerZ = bbox.lowerZ < lowerZ? bbox.lowerZ: lowerZ; 
            upperX = bbox.upperX > upperX? bbox.upperX: upperX; 
            upperY = bbox.upperY > upperY? bbox.upperY: upperY; 
            upperZ = bbox.upperZ > upperZ? bbox.upperZ: upperZ; 
            
            return *this;
        }
        
        void SetDepth( float depth)
        {
            upperZ = lowerZ + depth;
        }
        
        
        /**
         * The bounds of the box
         */
        // Make these ftPoints & private
        float lowerX, lowerY, lowerZ, upperX, upperY, upperZ;
    protected:
    
    
    private:

};


#endif  //  __FTBBox__

