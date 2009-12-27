#include "RenderItemMatcher.hpp"

double RenderItemMatcher::computeMatching(const RenderItemList & lhs, const RenderItemList & rhs) const {
		for (int i = 0; i < lhs.size();i++) {
			int j;
			for (j = 0; j < rhs.size();j++)
				_weights[i][j] = _distanceFunction(lhs[i], rhs[j]);
			for (; j < lhs.size();j++)
				_weights[i][j] = RenderItemDistanceMetric::NOT_COMPARABLE_VALUE;
		}

		
		const double error = _hungarianMethod(_weights, lhs.size());
		//std::cout << "[computeMatching] total error is " << error << std::endl;
		return error;
}



void RenderItemMatcher::setMatches
	(const RenderItemList & lhs_src, const RenderItemList & rhs_src) const {

	  for (int i = 0; i < lhs_src.size();i++) {
		const int j = _hungarianMethod.matching(i);

		// hack
		if (true || _weights[i][j] == RenderItemDistanceMetric::NOT_COMPARABLE_VALUE) {
 			_results.unmatchedLeft.push_back(lhs_src[i]);
			_results.unmatchedRight.push_back(rhs_src[i]);
		} else {
		    _results.matches.push_back(std::make_pair(lhs_src[i], rhs_src[j]));
		}
	  }
}