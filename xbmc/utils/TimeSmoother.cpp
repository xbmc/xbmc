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


#include "TimeSmoother.h"
#include <math.h>
#include <limits>
#include "utils/MathUtils.h"

using namespace std;

CTimeSmoother::CTimeSmoother()
: m_diffs(num_diffs),
  m_periods(num_periods),
  m_period(0),
  m_lastFrameTime(0),
  m_prevIn(num_stamps),
  m_prevOut(num_stamps)
{
}

void CTimeSmoother::AddTimeStamp(unsigned int currentTime)
{
  double diff = m_prevIn.size() ? currentTime - m_prevIn.back() : currentTime;
  if (diff)
    m_diffs.push_back(diff);

  vector<double> bins;
  BinData(m_diffs, bins, 0.15, 2);

  if (bins.size() && m_diffs.size() == num_diffs)
  {
    // have enough data to update our estimate
    vector<unsigned int> binMultipliers;
    GetGCDMultipliers(bins, binMultipliers, 2);
    assert(binMultipliers.size() == bins.size());

    vector<unsigned int> intRepresentation;
    GetIntRepresentation(m_diffs, intRepresentation, bins, binMultipliers);
    assert(intRepresentation.size() == m_diffs.size());

    double period = EstimatePeriod(m_diffs, intRepresentation);

    // update our mean period
    if (fabs(period - m_period) > m_period*0.1)
    { // more than 10 % out - kill our previous running average
      m_periods.clear();
      m_period = 0;
    }
    if (m_periods.size() < m_periods.capacity())
      m_period = (m_period * m_periods.size() + period) / (m_periods.size() + 1);
    else
      m_period += (period - m_periods[0]) / m_periods.size();
    m_periods.push_back(period);
  }
  double frameTime = EstimateFrameTime(currentTime);
  m_prevIn.push_back(currentTime);
  m_prevOut.push_back(frameTime);
}

unsigned int CTimeSmoother::GetNextFrameTime(unsigned int currentTime)
{
  if (m_period)
  {
    double frameTime = EstimateFrameTime(currentTime);
    // ensure we jump at least 1 period ahead of the last time we were called
    if (frameTime < m_lastFrameTime + m_period)
      frameTime = m_lastFrameTime + m_period;
    m_lastFrameTime = frameTime;
    return MathUtils::round_int(frameTime);
  }
  return currentTime;
}

void CTimeSmoother::BinData(const boost::circular_buffer<double> &data, vector<double> &bins, const double threshold, const unsigned int minbinsize)
{
  if (!data.size())
    return;

  bins.clear();
  vector<unsigned int> counts;

  for (boost::circular_buffer<double>::const_iterator i = data.begin(); i != data.end(); ++i)
  {
    bool found = false;
    for (unsigned int j = 0; j < bins.size(); ++j)
    {
      if (fabs(*i - bins[j]) < threshold*bins[j])
      {
        found = true;
        // update our bin mean and count
        bins[j] = (bins[j]*counts[j] + *i)/(counts[j]+1);
        counts[j]++;
        break;
      }
    }
    if (!found)
    {
      bins.push_back(*i);
      counts.push_back(1);
    }
  }
  if (minbinsize)
  {
    assert(counts.size() == bins.size());
    assert(counts.size());
    // filter out any bins that are not large enough (and any bins that aren't positive)
    for (unsigned int j = 0; j < counts.size(); )
    {
      if (counts[j] < minbinsize || bins[j] < 0.05)
      {
        bins.erase(bins.begin() + j);
        counts.erase(counts.begin() + j);
      }
      else
        j++;
    }
  }
}

void CTimeSmoother::GetConvergent(double value, unsigned int &num, unsigned int &denom, const unsigned int maxnumden)
{
  assert(value >= 1);

  unsigned int old_n = 1, old_d = 0;
  num = 0; denom = 1;

  // this while loop would typically be guaranteed to terminate as new_n, new_d are increasing non-negative
  // integers as long as f >= 1.  This in turn is guaranteed as f may never be zero as long as value > 1 and
  // value - f < 1.  Given that f = floor(value) this *should* always be true.
  // However, as f is unsigned int and thus range restricted, we can not guarantee this, and hence
  // break if value - f >= 1.
  
  // In addition, just to be on the safe side we don't allow the loop to run forever ;)
  unsigned int maxLoops = 3 * maxnumden;
  while (maxLoops--)
  {
    unsigned int f = (unsigned int)floor(value);
    if (value - f >= 1)
      break; // value out of range of unsigned int
    unsigned int new_n = f * num   + old_n;
    unsigned int new_d = f * denom + old_d;
    if (min(new_n, new_d) > maxnumden)
      break;
    old_n = num; old_d = denom;
    num = new_n; denom = new_d;
    if ((double)f == value)
      break;
    value = 1/(value - f);
  }
  // ensure num, denom are positive
  assert(num > 0 && denom > 0);
}

void CTimeSmoother::GetGCDMultipliers(const vector<double> &data, vector<unsigned int> &multipliers, const unsigned int maxminmult)
{
  vector<double>::const_iterator i = std::min_element(data.begin(), data.end());
  
  multipliers.clear();

  vector<unsigned int> num, denom;
  for (vector<double>::const_iterator j = data.begin(); j != data.end(); ++j)
  {
    if (j != i)
    {
      unsigned int n, d;
      GetConvergent(*j / *i, n, d, maxminmult);
      num.push_back(n);
      denom.push_back(d);
    }
    else
    {
      num.push_back(1);
      denom.push_back(1);
    }
  }
  vector<unsigned int>::const_iterator k = std::max_element(num.begin(), num.end());
  for (unsigned int i = 0; i < num.size(); ++i)
    multipliers.push_back(denom[i] * (*k) / num[i]);
}

void CTimeSmoother::GetIntRepresentation(const boost::circular_buffer<double> &data, vector<unsigned int> &intData, const vector<double> &bins, const vector<unsigned int> &intBins)
{
  intData.clear();
  for (boost::circular_buffer<double>::const_iterator i = data.begin(); i != data.end(); ++i)
  {
    double min_r2 = numeric_limits<double>::max();
    unsigned int min_j = 0;
    for (unsigned int j = 0; j < bins.size(); ++j)
    {
      double d = MathUtils::round_int(*i/bins[j]);
      double r2 = (*i - bins[j]*d)*(*i - bins[j]*d);
      if (r2 < min_r2)
      {
        min_j = j;
        min_r2 = r2;
      }
    }
    intData.push_back(MathUtils::round_int(*i/bins[min_j])*intBins[min_j]);
  }
}

double CTimeSmoother::EstimatePeriod(const boost::circular_buffer<double> &data, const vector<unsigned int> &intData)
{
  double sxy = 0, sxx = 0;
  for (unsigned int i = 0; i < data.size(); ++i)
  {
    sxy += intData[i] * data[i];
    sxx += intData[i] * intData[i];
  }
  return sxy/sxx;
}

double CTimeSmoother::EstimateFrameTime(unsigned int currentTime)
{
  assert(m_prevIn.size() == m_prevOut.size());
  if (m_period)
  {
    vector<double> outTimes;
    for (unsigned int i = 0; i < m_prevIn.size(); ++i)
      outTimes.push_back(m_prevOut[i] + m_period * MathUtils::round_int((currentTime - m_prevIn[i]) / m_period));
    sort(outTimes.begin(), outTimes.end());
    double outTime = outTimes[(outTimes.size()-1)/2];
    if (outTime < m_prevOut.back() + m_period)
      outTime = m_prevOut.back() + m_period;
    return outTime;
  }
  return currentTime;
}
