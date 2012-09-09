/*
*      Copyright (C) 2011-2012 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#pragma once

#include <vector>
#include <boost/circular_buffer.hpp>

/*! \brief Class to smooth timestamps
 
 This class is designed to smooth reasonably regular timestamps into more regular timestamps.  It was designed
 to deal with per-frame timestamps, i.e. timestamps immediately following a back/front buffer flip.
 
 The main difficultes are:
 1. The timestamps are generally noisy.
 2. We do not know the vsync rate.
 3. We do not know whether or not we've missed flips, either intermittently or due to some cadence (eg 2/3/2/3
    frame durations)
 
 Primarily we're interested in what the vsync rate is, i.e. what is the duration of a single frame.  To do this,
 we solve the linear regression
 
 y_i = b_0 + b_1*x_i + e_i
 
 where y_i are the time stamps, x_i are the integer frame counts that these timestamps correspond to, b_0 is an offset,
 b_1 is the frame duration, and e_i is an error term, assumed to be independent, identically distributed as 
 Normal(0, \sigma^2) for some fixed \sigma^2.
 
 The main difficulty is we do not know the predictors x_i's - all we know is that they're an increasing sequence of
 integers.  Thus, to solve this problem we must estimate both the x_i's, b_0 and b_1.  We can eliminate b_0 by operating
 on the differences t_i = y_i - y_{i-1} and k_i = x_i - x_{i-1} rather than y_i and x_i directly.  Further, if we assume
 that the error in the differences is additive gaussian white noise, by the maximum likelihood principle we may estimate 
 the k_i's and b_1 by minimizing the least squares problem:

   min_{b_1 \in R, k \in Z^M}||t - kb_1||_2^2

 This is linearly separable, and the cost function can be concentrated to k, yielding
 
   min_{b_1 \in R}||t - b_1 round(t/b_1)||_2^2     ....(1)
 
 This allows the period b_1 to be estimated without knowledge of the x_i.  The main problem with this is we require a
 limit on the range for b_1, as clearly b_1 / j for any positive integer j also minimizes (1).  This presents a problem
 in the case where we have no knowledge what b_1 is.  Furthermore, (1) is typically not smooth, with the concave portion
 which contains it's minimum often being very small.  Thus, in order to minimize (1) we need to evaluate it over a fairly
 fine grid, between pre-defined limits.  This is infeasible.
 
 Instead, we attack the problem by first attempting to estimate the k_i's.  We do this by binning the differences t_i
 into common categories and then computing the greatest common divisor of those bins.  Given the data is noisy, the binning
 procedure (BinData) has a tolerance for the bin size.  To minimize the influence of timestamps that are out of the ordinary
 we then discard any bins with only one value in them, thus leaving only binned values that are common.  We then compute
 an approximate greatest common divisor by computing the continued fraction expansion of the ratio of pairs of bins, to
 produce integer divisors that produce a common divisor.  We restrict the size of these integer divisors so that our divisor
 is not too small - typically we're wanting to pick up simple cadences such as 1,2,1,2 and 2,3,2,3 but are unlikely to need
 to detect 5,4,5,4 etc - at that point the CPU/GPU is limiting the frame rate so much that timing issues aren't likely to be
 the largest issue.
 
 Once we have the GCD we can then compute the integers k_i.  Given k_i we can then estimate b_1 using a standard simple
 linear regression without intercept:
 
   t_i = b_1*x_i + e_i
 
 which can be solved in the standard manner.
 
 Our procedure is thus as follows:
 1. We collect K differences, where K is enough to ensure we get an estimate of the period, but not too large so that the
    computation is unnecessarily long.
 2. We then keep a running average of our period that is fairly long in order to stabilise the period over time.
 3. To allow the running average to adapt quickly to framerate changes, if the newly computed period is much different
    from our running average, we start a new running average.
 4. Given our period, we then estimate our noise-free timestamps by rounding to the nearest multiple of the period
    from past time points.  We use several time points and choose the median such point as our final point.  This allows
    for deciding whether to round up or round down in cases where it may not be particularly obvious.
 5. Finally, we ensure the timestamps always move forward by the period.
 
 This works well for the most part.  Note that we're estimating a noise-free version of the last timestamp passed in,
 we do not claim to be estimating the timestamp of the next frame time.  Such an estimate is essentially indeterminant, as
 the time to process the frame is non-constant.
 */

class CTimeSmoother
{
public:
  CTimeSmoother();

  /*! \brief Add a valid time stamp to the time smoother
   This function will add a time stamp to the smoother and use it to update the current average frame rate
   and estimate the current cadence.  The next frame time can be retrieved using GetNextFrameTime.
   \param currentTime the current time stamp to add to the smoother
   \sa GetNextFrameTime
   */
  void AddTimeStamp(unsigned int currentTime);

  /*! \brief Retreive an estimate of the next frame time, based on the current time
   This function uses previously estimated average frame rates and the current cadence to estimate the next
   frame time.
   \param currentTime the current time stamp to use to estimate the next frame time
   \return the estimated time the next frame will be displayed
   \sa AddTimeStamp
   */
  unsigned int GetNextFrameTime(unsigned int currentTime);

protected:
  /*! \brief Bin data into separate clusters, determined by a given threshold and minimum bin size.
   \param data a circular buffer of data points
   \param bins the bins to return
   \param threshold the threshold to determine whether a data point is close to a given bin as a proportion of bin mean
   \param minbinsize the minimum bin size of each bin
   */
  void BinData(const boost::circular_buffer<double> &data, std::vector<double> &bins, const double threshold, const unsigned int minbinsize);

  /*! \brief Given a real value, find a rational convergent
   Uses a continued fraction expansion of value to determine the numerator and denominator of a rational convergent
   where min(num, denom) does not exceed maxnumdem
   \param value real data value. Must be no less than 1.
   \param num [out] the numerator
   \param denom [out] the denominator
   \param maxnumden the maximal value of min(num, denom)
   */
  void GetConvergent(double value, unsigned int &num, unsigned int &denom, const unsigned int maxnumden);

  /*! \brief Given a set of data, find integer multipliers such that data[i] \sim quotient[i] * gcd(data)
   Uses rational convergents to data[i]/min(data) to find integer multipliers to the (approximate) greatest common divisor
   of the data.
   \param data a vector of data
   \param multipliers the output multipliers
   \param maxminmult the maximal value of multiplier[min(data)]
   \sa GetConvergent
   */
  void GetGCDMultipliers(const std::vector<double> &data, std::vector<unsigned int> &multipliers, const unsigned int maxminmult);

  /*! \brief Given a set of bins and integer values associated with each bin, find the integer representation of some data
   This allows noisy data to be approximated by a set of clean data, and to compute the integer representation of that data.
   \param data the data on which to compute the integer representation
   \param intData [out] the integer representation of the data
   \param bins the bins to use for approximating the data
   \param intBins the integer representation of the bins
   */
  void GetIntRepresentation(const boost::circular_buffer<double> &data, std::vector<unsigned int> &intData, const std::vector<double> &bins, const std::vector<unsigned int> &intBins);

  /*! \brief Given a set of data, and an integer representation of that data, estimate the period of the data
   Essentially we solve a linear regression d_i = \theta*z_i, where d_i is the original data, and z_i is the integer
   representation of that data.  Note that no intercept is included, so this is appropriate for operating on difference data
   (such as the difference between timestamps following flipping of buffers during rendering - the integers represent the vsync
   sequence).
   \param data noisy data to estimate the period of
   \param intData an integral representation of the data
   \return the period of the data
   */
  double EstimatePeriod(const boost::circular_buffer<double> &data, const std::vector<unsigned int> &intData);

  /*! \brief Compute the next frame time
   \param currentTime the current time
   \return the next frame time
   */
  double EstimateFrameTime(unsigned int currentTime);

private:
  static const unsigned int num_diffs = 10;        ///< \brief number of differences to keep for evaluating period
  static const unsigned int num_periods = 100;     ///< \brief number of previous period estimates to use for the average period
  static const unsigned int num_stamps = 3;        ///< \brief number of previous time stamps to keep to optimize next time point

  boost::circular_buffer<double> m_diffs;   ///< \brief the recently received differences
  boost::circular_buffer<double> m_periods; ///< \brief the recently evaluated periods
  double m_period;                          ///< \brief the running average of m_periods

  double m_lastFrameTime;                   ///< \brief the last frame time

  boost::circular_buffer<double> m_prevIn;  ///< \brief the previous timestamps coming in
  boost::circular_buffer<double> m_prevOut; ///< \brief the previous timestamps going out
};
