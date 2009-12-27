/*
 * RenderItemDistanceMetric.h
 *
 *  Created on: Feb 16, 2009
 *      Author: struktured
 */

#ifndef RenderItemDISTANCEMETRIC_H_
#define RenderItemDISTANCEMETRIC_H_

#include "Common.hpp"
#include "Renderable.hpp"
#include <limits>
#include <functional>
#include <map>


/// Compares two render items and returns zero if they are virtually equivalent and large values
/// when they are dissimilar. If two render items cannot be compared, NOT_COMPARABLE_VALUE is returned.
class RenderItemDistanceMetric : public std::binary_function<const RenderItem*, const RenderItem*, double> {
public:
  const static double NOT_COMPARABLE_VALUE;
  virtual double operator()(const RenderItem * r1, const RenderItem * r2) const = 0;
  virtual TypeIdPair typeIdPair() const = 0;
};

// A base class to construct render item distance metrics. Just specify your two concrete
// render item types as template parameters and override the computeDistance() function.
template <class R1, class R2>
class RenderItemDistance : public RenderItemDistanceMetric {

protected:
// Override to create your own distance fmetric for your specified custom types.
virtual double computeDistance(const R1 * r1, const R2 * r2) const = 0;

public:

inline virtual double operator()(const RenderItem * r1, const RenderItem * r2) const {
	if (supported(r1, r2))
		return computeDistance(dynamic_cast<const R1*>(r1), dynamic_cast<const R2*>(r2));
	else if (supported(r2,r1))
		return computeDistance(dynamic_cast<const R1*>(r2), dynamic_cast<const R2*>(r1));
	else
		return NOT_COMPARABLE_VALUE;
}

// Returns true if and only if r1 and r2 are the same type as or derived from R1, R2 respectively
inline bool supported(const RenderItem * r1, const RenderItem * r2) const {
	return dynamic_cast<const R1*>(r1) && dynamic_cast<const R2*>(r2);
	//return typeid(r1) == typeid(const R1 *) && typeid(r2) == typeid(const R2 *);
}

inline TypeIdPair typeIdPair() const {
	return TypeIdPair(typeid(const R1*).name(), typeid(const R2*).name());
}

};


class RTIRenderItemDistance : public RenderItemDistance<RenderItem, RenderItem> {
public:

	RTIRenderItemDistance() {}
	virtual ~RTIRenderItemDistance() {}

protected:
	virtual inline double computeDistance(const RenderItem * lhs, const RenderItem * rhs) const {
		if (typeid(*lhs) == typeid(*rhs)) {
			//std::cerr << typeid(*lhs).name() << " and " << typeid(*rhs).name() <<  "are comparable" << std::endl;

			return 0.0;
		}
		else {
			//std::cerr << typeid(*lhs).name() << " and " << typeid(*rhs).name() <<  "not comparable" << std::endl;
			return NOT_COMPARABLE_VALUE;
		}
	}


};



class ShapeXYDistance : public RenderItemDistance<Shape, Shape> {

public:

	ShapeXYDistance() {}
	virtual ~ShapeXYDistance() {}

protected:

	virtual inline double computeDistance(const Shape * lhs, const Shape * rhs) const {
			return (meanSquaredError(lhs->x, rhs->x) + meanSquaredError(lhs->y, rhs->y)) / 2;
	}

};


class MasterRenderItemDistance : public RenderItemDistance<RenderItem, RenderItem> {

typedef std::map<TypeIdPair, RenderItemDistanceMetric*> DistanceMetricMap;
public:

	MasterRenderItemDistance() {}
	virtual ~MasterRenderItemDistance() {}

	inline void addMetric(RenderItemDistanceMetric * fun) {
		_distanceMetricMap[fun->typeIdPair()] = fun;
	}

protected:
	virtual inline double computeDistance(const RenderItem * lhs, const RenderItem * rhs) const {

		RenderItemDistanceMetric * metric;

		TypeIdPair pair(typeid(lhs), typeid(rhs));


		// If specialized metric exists, use it to get higher granularity
		// of correctness
		if (_distanceMetricMap.count(pair)) {
			metric = _distanceMetricMap[pair];
		} else if (_distanceMetricMap.count(pair = TypeIdPair(typeid(rhs), typeid(lhs)))) {
			metric = _distanceMetricMap[pair];
		} else { // Failing that, use rtti && shape distance if its a shape type

			const double rttiError = _rttiDistance(lhs,rhs);

			/// @bug This is a non elegant approach to supporting shape distance
			if (rttiError == 0 && _shapeXYDistance.supported(lhs,rhs))
			   return _shapeXYDistance(lhs, rhs);
			else return rttiError;
		}

		return (*metric)(lhs, rhs);
	}

private:
	mutable RTIRenderItemDistance _rttiDistance;
	mutable ShapeXYDistance _shapeXYDistance;
	mutable DistanceMetricMap _distanceMetricMap;
};

#endif /* RenderItemDISTANCEMETRIC_H_ */
