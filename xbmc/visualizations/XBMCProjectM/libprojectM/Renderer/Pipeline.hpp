#ifndef Pipeline_HPP
#define Pipeline_HPP

#include <vector>
#include "PerPixelMesh.hpp"
#include "Renderable.hpp"
#include "Filters.hpp"
#include "PipelineContext.hpp"
#include "Shader.hpp"
#include "../Common.hpp"
//This class is the input to projectM's renderer
//
//Most implemenatations should implement PerPixel in order to get multi-threaded
//dynamic PerPixel equations.  If you MUST (ie Milkdrop compatability), you can use the
//setStaticPerPixel function and fill in x_mesh and y_mesh yourself.
class Pipeline
{
public:

	 //static per pixel stuff
	 bool staticPerPixel;
	 int gx;
	 int gy;

	 float** x_mesh;
	 float** y_mesh;
	 //end static per pixel

	 bool  textureWrap;
	 float screenDecay;

	 //variables passed to pixel shaders
	 float q[NUM_Q_VARIABLES];

	 //blur settings n=bias x=scale
	 float blur1n;
	 float blur2n;
	 float blur3n;
	 float blur1x;
	 float blur2x;
	 float blur3x;
	 float blur1ed;

	 Shader warpShader;
	 Shader compositeShader;

	 std::vector<RenderItem*> drawables;
	 std::vector<RenderItem*> compositeDrawables;

	 Pipeline();
	 void setStaticPerPixel(int gx, int gy);
	 virtual ~Pipeline();
	 virtual Point PerPixel(Point p, const PerPixelContext context);
};

#endif
