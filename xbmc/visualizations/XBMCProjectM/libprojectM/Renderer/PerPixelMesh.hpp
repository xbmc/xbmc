#ifndef PerPixelMesh_HPP
#define PerPixelMesh_HPP

#include <vector>

struct Point
{
	float x;
	float y;

	Point(float x, float y);
};

struct PerPixelContext
{
	float x;
	float y;
	float rad;
	float theta;

	int i;
	int j;

	PerPixelContext(float x, float y, float rad, float theta, int i, int j);
};

class PerPixelMesh
{
public:
	int width;
	int height;
	int size;

	std::vector<Point> p;
	std::vector<Point> p_original;
	std::vector<PerPixelContext> identity;

	PerPixelMesh(int width, int height);

	void Reset();
};



#endif
