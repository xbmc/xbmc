/*
 * Pipeline.cpp
 *
 *  Created on: Jun 17, 2008
 *      Author: pete
 */
#include "Pipeline.hpp"
#include "wipemalloc.h"

Pipeline::Pipeline() : staticPerPixel(false),gx(0),gy(0),blur1n(1), blur2n(1), blur3n(1),
blur1x(1), blur2x(1), blur3x(1),
blur1ed(1){}

void Pipeline::setStaticPerPixel(int gx, int gy)
{
	 staticPerPixel = true;
	 this->gx = gx;
	 this->gy = gy;

		this->x_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
		for ( int x = 0; x < gx; x++ )
		{
			this->x_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
		}
		this->y_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
		for ( int x = 0; x < gx; x++ )
		{
			this->y_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
		}

}

Pipeline::~Pipeline()
{
if (staticPerPixel)
{
	for ( int x = 0; x < this->gx; x++ )
	{
		free(this->x_mesh[x]);
		free(this->y_mesh[x]);
	}
	free(x_mesh);
	free(y_mesh);
}
}

//void Pipeline::Render(const BeatDetect &music, const PipelineContext &context){}
Point Pipeline::PerPixel(Point p, const PerPixelContext context)
{return p;}
