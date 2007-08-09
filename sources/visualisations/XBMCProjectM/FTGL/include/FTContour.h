#ifndef     __FTContour__
#define     __FTContour__

#include "FTPoint.h"
#include "FTVector.h"
#include "FTGL.h"


/**
 * FTContour class is a container of points that describe a vector font
 * outline. It is used as a container for the output of the bezier curve
 * evaluator in FTVectoriser.
 *
 * @see FTOutlineGlyph
 * @see FTPolyGlyph
 * @see FTPoint
 */
class FTGL_EXPORT FTContour
{
    public:
        /**
         * Constructor
         *
         * @param contour
         * @param pointTags
         * @param numberOfPoints
         */
        FTContour( FT_Vector* contour, char* pointTags, unsigned int numberOfPoints);

        /**
         * Destructor
         */
        ~FTContour()
        {
            pointList.clear();
        }
        
        /**
         * Return a point at index.
         *
         * @param index of the point in the curve.
         * @return const point reference
         */
        const FTPoint& Point( unsigned int index) const { return pointList[index];}

        /**
         * How many points define this contour
         *
         * @return the number of points in this contour
         */
        size_t PointCount() const { return pointList.size();}

    private:
        /**
         * Add a point to this contour. This function tests for duplicate
         * points.
         *
         * @param point The point to be added to the contour.
         */
        inline void AddPoint( FTPoint point);
        
        inline void AddPoint( float x, float y);
        
        /**
         * De Casteljau (bezier) algorithm contributed by Jed Soane
         * Evaluates a quadratic or conic (second degree) curve
         */
        inline void evaluateQuadraticCurve();

        /**
         * De Casteljau (bezier) algorithm contributed by Jed Soane
         * Evaluates a cubic (third degree) curve
         */
        inline void evaluateCubicCurve();

        /**
         *  The list of points in this contour
         */
        typedef FTVector<FTPoint> PointVector;
        PointVector pointList;
        
        /**
         * 2D array storing values of de Casteljau algorithm.
         */
        float controlPoints[4][2];
};

#endif // __FTContour__
