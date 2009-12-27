/*
 * Filters.hpp
 *
 *  Created on: Jun 18, 2008
 *      Author: pete
 */

#ifndef FILTERS_HPP_
#define FILTERS_HPP_

#include "Renderable.hpp"

class Brighten : public RenderItem
{
public:
	Brighten(){}
	void Draw(RenderContext &context);
};

class Darken : public RenderItem
{
public:
	Darken(){}
	void Draw(RenderContext &context);
};

class Invert : public RenderItem
{
public:
	Invert(){}
	void Draw(RenderContext &context);
};

class Solarize : public RenderItem
{
public:
	Solarize(){}
	void Draw(RenderContext &context);
};

#endif /* FILTERS_HPP_ */
