/*
 * RenderItemDistanceMetric.cpp
 *
 *  Created on: Feb 18, 2009
 *      Author: struktured
 */

#include "RenderItemDistanceMetric.hpp"

// Assumes [0, 1] distance space because it's easy to manage with overflow
// Underflow is obviously possible though.
const double RenderItemDistanceMetric::NOT_COMPARABLE_VALUE
	(1.0);
