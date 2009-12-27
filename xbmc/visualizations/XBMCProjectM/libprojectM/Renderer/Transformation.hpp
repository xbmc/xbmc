#ifndef Transformation_HPP
#define Transformation_HPP

#include "PerPixelMesh.hpp"
#include <math.h>


class Transforms
{
public:

	inline static void Zoom(Point &p, const PerPixelContext &context, float zoom, float zoomExponent)
	{
		float fZoom2 = powf( zoom, powf( zoomExponent, context.rad*2.0f - 1.0f));
		float fZoom2Inv = 1.0f/fZoom2;
		p.x -= 0.5;
		p.y -= 0.5;
		p.x *= fZoom2Inv;
		p.y *= fZoom2Inv;
		p.x += 0.5;
		p.y += 0.5;
	}

	inline static void Transform(Point &p, const PerPixelContext &context, float dx, float dy)
	{
		p.x -= dx;
		p.y -= dy;
	}

	inline static void Scale(Point &p, const PerPixelContext &context, float sy, float sx, float cx, float cy)
	{
		p.x = (p.x - cx)/sx + cx;
		p.y = (p.y - cy)/sy + cy;
	}

	inline static void Rotate(Point &p, const PerPixelContext &context, float angle, float cx, float cy)
	{
		float u2 = p.x - cx;
		float v2 = p.y - cy;

		float cos_rot = cosf(angle);
		float sin_rot = sinf(angle);

		p.x = u2*cos_rot - v2*sin_rot + cx;
		p.y = u2*sin_rot + v2*cos_rot + cy;
	}

	inline static void Warp(Point &p, const PerPixelContext &context, float time, float speed, float scale, float warp)
	{
		float fWarpTime = time * speed;
		float fWarpScaleInv = 1.0f / scale;
		float f[4];
		f[0] = 11.68f + 4.0f*cosf(fWarpTime*1.413f + 10);
		f[1] =  8.77f + 3.0f*cosf(fWarpTime*1.113f + 7);
		f[2] = 10.54f + 3.0f*cosf(fWarpTime*1.233f + 3);
		f[3] = 11.49f + 4.0f*cosf(fWarpTime*0.933f + 5);

		p.x += warp*0.0035f*sinf(fWarpTime*0.333f + fWarpScaleInv*(context.x*f[0] - context.y*f[3]));
		p.y += warp*0.0035f*cosf(fWarpTime*0.375f - fWarpScaleInv*(context.x*f[2] + context.y*f[1]));
		p.x += warp*0.0035f*cosf(fWarpTime*0.753f - fWarpScaleInv*(context.x*f[1] - context.y*f[2]));
		p.y += warp*0.0035f*sinf(fWarpTime*0.825f + fWarpScaleInv*(context.x*f[0] + context.y*f[3]));
	}

};

#endif
