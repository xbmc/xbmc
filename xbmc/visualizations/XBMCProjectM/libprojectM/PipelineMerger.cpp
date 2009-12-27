#include "PipelineMerger.hpp"
#include "RenderItemMatcher.hpp"
#include "RenderItemMergeFunction.hpp"

const double PipelineMerger::e(2.71828182845904523536);
const double PipelineMerger::s(0.5);

void PipelineMerger::mergePipelines(const Pipeline & a, const Pipeline & b, Pipeline & out, RenderItemMatcher::MatchResults & results, RenderItemMergeFunction & mergeFunction, float ratio)

{

	const double x = ( ratio - 0.5 ) * 20;
	const double sigmoid = 1.0 / ( 1.0 + e - s * x );
	const double invratio = 1.0 - ratio;

	out.textureWrap = ( ratio < 0.5 ) ? a.textureWrap : b.textureWrap;

	out.screenDecay = lerp ( b.screenDecay, a.screenDecay, ratio );

	out.drawables.clear();
	out.compositeDrawables.clear();

	for ( std::vector<RenderItem*>::const_iterator pos = a.drawables.begin();
	        pos != a.drawables.end(); ++pos )
	{
		( *pos )->masterAlpha = invratio;
		out.drawables.push_back ( *pos );
	}

	for ( std::vector<RenderItem*>::const_iterator pos = b.drawables.begin();
	        pos != b.drawables.end();++pos )
	{
		( *pos )->masterAlpha = ratio;
		out.drawables.push_back ( *pos );
	}

    if(ratio < 0.5)
    {
        const double local_ratio = (invratio - 0.5) * 2;

        for (std::vector<RenderItem*>::const_iterator pos = a.compositeDrawables.begin();
        pos != a.compositeDrawables.end(); ++pos)
        {
            (*pos)->masterAlpha = local_ratio;
            out.compositeDrawables.push_back(*pos);
        }
    }
    else
    {
        const double local_ratio = (ratio - 0.5) * 2;

        for (std::vector<RenderItem*>::const_iterator pos = b.compositeDrawables.begin();
        pos != b.compositeDrawables.end();++pos)
        {
            (*pos)->masterAlpha = local_ratio;
            out.compositeDrawables.push_back(*pos);
        }
    }

	/*
	for (RenderItemMatchList::iterator pos = results.matches.begin(); pos != results.matches.end(); ++pos) {

		RenderItem * itemA = pos->first;
		RenderItem * itemB = pos->second;

		RenderItem * itemC = mergeFunction(itemA, itemB, ratio);

		if (itemC == 0) {
			itemA->masterAlpha = ratio;
	        	out.drawables.push_back(itemA);
			itemB->masterAlpha = invratio;
	        	out.drawables.push_back(itemB);
		} else
			out.drawables.push_back(itemC);

	}


	for (std::vector<RenderItem*>::const_iterator pos = results.unmatchedLeft.begin();
		pos != results.unmatchedLeft.end(); ++pos)
	    {
	       (*pos)->masterAlpha = invratio;
	       out.drawables.push_back(*pos);
	    }

	for (std::vector<RenderItem*>::const_iterator pos = results.unmatchedRight.begin();
		pos != results.unmatchedRight.end(); ++pos)
		{
		  (*pos)->masterAlpha = ratio;
		   out.drawables.push_back(*pos);
		}
	*/

	if (a.staticPerPixel && b.staticPerPixel)
	{
		out.staticPerPixel = true;
		 for (int x=0;x<a.gx;x++)
		    {
		      for(int y=0;y<a.gy;y++)
			{
			  out.x_mesh[x][y]  = a.x_mesh[x][y]* invratio + b.x_mesh[x][y]*ratio;
			}
		    }
		 for (int x=0;x<a.gx;x++)
		    {
		      for(int y=0;y<a.gy;y++)
			{
			  out.y_mesh[x][y]  = a.y_mesh[x][y]* invratio + b.y_mesh[x][y]*ratio;
			}
		    }
	}

    if(ratio < 0.5)
    {
	out.compositeShader = a.compositeShader;
	out.warpShader = a.warpShader;
    }
    else
    {
      out.compositeShader = b.compositeShader;
      out.warpShader = b.warpShader;
    }
}
