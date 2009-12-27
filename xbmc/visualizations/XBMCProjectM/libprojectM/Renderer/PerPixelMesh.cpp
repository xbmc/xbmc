#include <math.h>
#include <algorithm>
#include "PerPixelMesh.hpp"
#include "omptl/omptl"
#include "omptl/omptl_algorithm"

PerPixelMesh::PerPixelMesh(int width, int height) : width(width), height(height), size (width * height),
		p(width * height, Point(0,0)),
		p_original(width * height, Point(0,0)),
		identity(width * height, PerPixelContext(0,0,0,0,0,0))
		{
		for (int j=0;j<height;j++)
			for(int i=0;i<width;i++)
			{
				int index = j * width + i;

						float xval=i/(float)(width-1);
						float yval=-((j/(float)(height-1))-1);

						p[index].x = xval;
						p[index].y = yval;

						p_original[index].x = xval;
						p_original[index].y = yval;

						identity[index].x= xval;
						identity[index].y= yval;

						//identity[index].x= (xval-.5)*2;
						//identity[index].y= (yval-.5)*2;

						identity[index].i= i;
						identity[index].j= j;

						identity[index].rad=hypot ( ( xval-.5 ) *2, ( yval-.5 ) *2 ) * .7071067;
						identity[index].theta=atan2 ( ( yval-.5 ) *2 ,  ( xval-.5 ) *2  );
			}
		}

void PerPixelMesh::Reset()
{
	omptl::copy(p_original.begin(), p_original.end(), p.begin());
}

Point::Point(float x, float y)
	: x(x), y(y) {}
PerPixelContext::PerPixelContext(float x, float y, float rad, float theta, int i, int j)
	: x(x), y(y), rad(rad), theta(theta), i(i), j(j) {}
