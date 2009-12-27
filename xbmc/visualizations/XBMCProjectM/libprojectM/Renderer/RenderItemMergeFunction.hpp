/*
 * RenderItemMergeFunction.hpp
 *
 *  Created on: Feb 16, 2009
 *      Author: struktured
 */

#ifndef RenderItemMergeFunction_HPP_
#define RenderItemMergeFunction_HPP_

#include "Common.hpp"
#include "Renderable.hpp"
#include "Waveform.hpp"
#include <limits>
#include <functional>
#include <map>


template <class T>
inline T interpolate(T a, T b, float ratio)
{
    return (ratio*a + (1-ratio)*b) * 0.5;
}

template <>
inline int interpolate(int a, int b, float ratio)
{
    return (int)(ratio*(float)a + (1-ratio)*(float)b) * 0.5;
}

template <>
inline bool interpolate(bool a, bool b, float ratio)
{
    return (ratio >= 0.5) ? a : b;
}


/// Merges two render items and returns zero if they are virtually equivalent and large values
/// when they are dissimilar. If two render items cannot be compared, NOT_COMPARABLE_VALUE is returned.
class RenderItemMergeFunction {
public:
  virtual RenderItem * operator()(const RenderItem * r1, const RenderItem * r2, double ratio) const = 0;
  virtual TypeIdPair typeIdPair() const = 0;
};

/// A base class to construct render item distance mergeFunctions. Just specify your two concrete
/// render item types as template parameters and override the computeMerge() function.
template <class R1, class R2=R1, class R3=R2>
class RenderItemMerge : public RenderItemMergeFunction {

protected:
/// Override to create your own distance mergeFunction for your specified custom types.
virtual R3 * computeMerge(const R1 * r1, const R2 * r2, double ratio) const = 0;

public:

inline virtual R3 * operator()(const RenderItem * r1, const RenderItem * r2, double ratio) const {
	if (supported(r1, r2))
		return computeMerge(dynamic_cast<const R1*>(r1), dynamic_cast<const R2*>(r2), ratio);
	else if (supported(r2,r1))
		return computeMerge(dynamic_cast<const R1*>(r2), dynamic_cast<const R2*>(r1), ratio);
	else
		return 0;
}

/// Returns true if and only if r1 and r2 are of type R1 and R2 respectively.
inline bool supported(const RenderItem * r1, const RenderItem * r2) const {
	return typeid(r1) == typeid(const R1 *) && typeid(r2) == typeid(const R2 *);
}

inline TypeIdPair typeIdPair() const {
	return TypeIdPair(typeid(const R1*).name(), typeid(const R2*).name());
}

};


class ShapeMerge : public RenderItemMerge<Shape> {

public:

	ShapeMerge() {}
	virtual ~ShapeMerge() {}

protected:

	virtual inline Shape * computeMerge(const Shape * lhs, const Shape * rhs, double ratio) const {

    	Shape * ret = new Shape();
	Shape & target = *ret;

	target.x = interpolate(lhs->x, rhs->x, ratio);
        target.y = interpolate(lhs->y, rhs->y, ratio);
	target.a = interpolate(lhs->a, rhs->a, ratio);
        target.a2 = interpolate(lhs->a2, rhs->a2, ratio);
        target.r = interpolate(lhs->r, rhs->r, ratio);
        target.r2 = interpolate(lhs->r2, rhs->r2, ratio);
        target.g = interpolate(lhs->g, rhs->g, ratio);
        target.g2 = interpolate(lhs->g2, rhs->g2, ratio);
        target.b = interpolate(lhs->b, rhs->b, ratio);
        target.b2 = interpolate(lhs->b2, rhs->b2, ratio);

        target.ang = interpolate(lhs->ang, rhs->ang, ratio);
        target.radius = interpolate(lhs->radius, rhs->radius, ratio);

        target.tex_ang = interpolate(lhs->tex_ang, rhs->tex_ang, ratio);
        target.tex_zoom = interpolate(lhs->tex_zoom, rhs->tex_zoom, ratio);

        target.border_a = interpolate(lhs->border_a, rhs->border_a, ratio);
        target.border_r = interpolate(lhs->border_r, rhs->border_r, ratio);
        target.border_g = interpolate(lhs->border_g, rhs->border_g, ratio);
        target.border_b = interpolate(lhs->border_b, rhs->border_b, ratio);

        target.sides = interpolate(lhs->sides, rhs->sides, ratio);

        target.additive = interpolate(lhs->additive, rhs->additive, ratio);
        target.textured = interpolate(lhs->textured, rhs->textured, ratio);
        target.thickOutline = interpolate(lhs->thickOutline, rhs->thickOutline, ratio);
        target.enabled = interpolate(lhs->enabled, rhs->enabled, ratio);

        target.masterAlpha = interpolate(lhs->masterAlpha, rhs->masterAlpha, ratio);
        target.imageUrl = (ratio > 0.5) ? lhs->imageUrl : rhs->imageUrl, ratio;

        return ret;
	}
};

class BorderMerge : public RenderItemMerge<Border> {

    public:

        BorderMerge() {}
        virtual ~BorderMerge() {}

    protected:

        virtual inline Border * computeMerge(const Border * lhs, const Border * rhs, double ratio) const
        {
            Border * ret = new Border();
		
	    Border & target = *ret;

            target.inner_a = interpolate(lhs->inner_a, rhs->inner_a, ratio);
            target.inner_r = interpolate(lhs->inner_r, rhs->inner_r, ratio);
            target.inner_g = interpolate(lhs->inner_g, rhs->inner_g, ratio);
            target.inner_b = interpolate(lhs->inner_b, rhs->inner_b, ratio);
            target.inner_size = interpolate(lhs->inner_size, rhs->inner_size, ratio);

            target.outer_a = interpolate(lhs->outer_a, rhs->outer_a, ratio);
            target.outer_r = interpolate(lhs->outer_r, rhs->outer_r, ratio);
            target.outer_g = interpolate(lhs->outer_g, rhs->outer_g, ratio);
            target.outer_b = interpolate(lhs->outer_b, rhs->outer_b, ratio);
            target.outer_size = interpolate(lhs->outer_size, rhs->outer_size, ratio);

            target.masterAlpha = interpolate(lhs->masterAlpha, rhs->masterAlpha, ratio);

            return ret;
        }
};


class WaveformMerge : public RenderItemMerge<Waveform> {

    public:

        WaveformMerge() {}
        virtual ~WaveformMerge() {}

    protected:

	/// @BUG unimplemented
        virtual inline Waveform * computeMerge(const Waveform * lhs, const Waveform * rhs, double ratio) const
        {
		return 0;
/*
            Waveform * ret = new Waveform();
	    Waveform & target = *ret;

            target.additive = interpolate(lhs->additive, rhs->additive, ratio);
            target.dots = interpolate(lhs->dots, rhs->dots, ratio);
            target.samples = (rhs->samples > lhs-> samples) ? lhs->samples : rhs->samples;
            target.scaling = interpolate(lhs->scaling, rhs->scaling, ratio);
            target.sep = interpolate(lhs->sep, rhs->sep, ratio);
            target.smoothing = interpolate(lhs->smoothing, rhs->smoothing, ratio);
            target.spectrum = interpolate(lhs->spectrum, rhs->spectrum, ratio);
            target.thick = interpolate(lhs->thick, rhs->thick, ratio);
            target.masterAlpha = interpolate(lhs->masterAlpha, rhs->masterAlpha, ratio);

            return ret;
*/
        }
};


/// Use as the top level merge function. It stores a map of all other
/// merge functions, using the function that fits best with the
/// incoming type parameters.
class MasterRenderItemMerge : public RenderItemMerge<RenderItem> {

typedef std::map<TypeIdPair, RenderItemMergeFunction*> MergeFunctionMap;
public:

	MasterRenderItemMerge() {}
	virtual ~MasterRenderItemMerge() {}

	inline void add(RenderItemMergeFunction * fun) {
		_mergeFunctionMap[fun->typeIdPair()] = fun;
	}

protected:
	virtual inline RenderItem * computeMerge(const RenderItem * lhs, const RenderItem * rhs,  double ratio) const {

		RenderItemMergeFunction * mergeFunction;

		TypeIdPair pair(typeid(lhs), typeid(rhs));
		if (_mergeFunctionMap.count(pair)) {
			mergeFunction = _mergeFunctionMap[pair];
		} else if (_mergeFunctionMap.count(pair = TypeIdPair(typeid(rhs), typeid(lhs)))) {
			mergeFunction = _mergeFunctionMap[pair];
		} else {
			mergeFunction  = 0;
		}

		// If specialized mergeFunction exists, use it to get higher granularity
		// of correctness
		if (mergeFunction)
			return (*mergeFunction)(lhs, rhs, ratio);
		else
			return 0;
	}

private:
	mutable MergeFunctionMap _mergeFunctionMap;
};

#endif /* RenderItemMergeFunction_HPP_ */
