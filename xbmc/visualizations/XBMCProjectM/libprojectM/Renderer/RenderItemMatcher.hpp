/*
 * RenderItemMatcher.hpp
 *
 *  Created on: Feb 16, 2009
 *      Author: struktured
 */

#ifndef RenderItemMatcher_HPP
#define RenderItemMatcher_HPP

#include "RenderItemDistanceMetric.hpp"
#include <vector>
#include <map>
#include <iostream>
#include "HungarianMethod.hpp"

typedef std::vector<std::pair<RenderItem*, RenderItem*> > RenderItemMatchList;

class MatchResults;

class RenderItemMatcher : public std::binary_function<RenderItemList, RenderItemList, MatchResults> {

public:

struct MatchResults {
  RenderItemMatchList matches;
  std::vector<RenderItem*> unmatchedLeft;
  std::vector<RenderItem*> unmatchedRight;

  double error;
};

	static const std::size_t MAXIMUM_SET_SIZE = 1000;

	/// Computes an optimal matching between two renderable item sets.
	/// @param lhs the "left-hand side" list of render items.
	/// @param rhs the "right-hand side" list of render items.
	/// @returns a list of match pairs, possibly self referencing, and an error estimate of the matching.
	inline virtual void operator()(const RenderItemList & lhs, const RenderItemList & rhs) const {
		
		// Ensure the first argument is greater than next to aid the helper function's logic.
		if (lhs.size() >= rhs.size()) {
		  _results.error = computeMatching(lhs, rhs);
		  setMatches(lhs, rhs);
		} else {
		  _results.error = computeMatching(rhs, lhs);
		  setMatches(rhs, lhs);
		}
		
	
	}

	RenderItemMatcher() {}
	virtual ~RenderItemMatcher() {}

	inline MatchResults & matchResults() { return _results; }

	inline double weight(int i, int j) const { return _weights[i][j]; }

	MasterRenderItemDistance & distanceFunction() { return _distanceFunction; }

private:
	mutable HungarianMethod<MAXIMUM_SET_SIZE> _hungarianMethod;
	mutable double _weights[MAXIMUM_SET_SIZE][MAXIMUM_SET_SIZE];

	mutable MatchResults _results;

	/// @idea interface this entirely allow overriding of its type.
	mutable MasterRenderItemDistance _distanceFunction;
	
	double computeMatching(const RenderItemList & lhs, const RenderItemList & rhs) const;

	void setMatches(const RenderItemList & lhs_src, const RenderItemList & rhs_src) const;

};

#endif
