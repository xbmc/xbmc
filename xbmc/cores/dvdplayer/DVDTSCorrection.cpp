/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "DVDTSCorrection.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "utils/log.h"

#include <cmath>

#define MAXERR DVD_MSEC_TO_TIME(2.5)

using namespace std;

CPullupCorrection::CPullupCorrection()
{
  Flush();
}

void CPullupCorrection::Flush()
{
  m_pattern.clear();
  m_ringpos       = 0;
  m_ptscorrection = 0.0;
  m_prevpts       = DVD_NOPTS_VALUE;
  m_patternpos    = 0;
  m_ringfill      = 0;
  m_haspattern    = false;
  m_patternlength = 0;
  m_frameduration = DVD_NOPTS_VALUE;
  m_trackingpts   = DVD_NOPTS_VALUE;
  memset(m_diffring, 0, sizeof(m_diffring));
}

void CPullupCorrection::Add(double pts)
{
  //can't get a diff with just one pts
  if (m_prevpts == DVD_NOPTS_VALUE)
  {
    m_prevpts = pts;
    return;
  }

  //increase the ringbuffer position
  m_ringpos = (m_ringpos + 1) % DIFFRINGSIZE;
  //add the current diff to the ringbuffer
  m_diffring[m_ringpos] = pts - m_prevpts;
  //save the pts
  m_prevpts = pts;

  if (m_ringfill < DIFFRINGSIZE)
    m_ringfill++;

  //only search for patterns if we have full ringbuffer
  if (m_ringfill < DIFFRINGSIZE)
    return;

  //get the current pattern in the ringbuffer
  vector<double> pattern;
  GetPattern(pattern);

  //check if the pattern is the same as the saved pattern
  //and if it is actually a pattern
  if (!CheckPattern(pattern))
  {
    if (m_haspattern)
    {
      CLog::Log(LOGDEBUG, "CPullupCorrection: pattern lost on diff %f", GetDiff(0));
      Flush();
    }

    //no pattern detected or current pattern broke/changed
    //save detected pattern so we can check it with the next iteration
    m_pattern = pattern;

    return;
  }
  else
  {
    //the saved pattern should have moved 1 diff into the past
    m_patternpos = (m_patternpos + 1) % m_pattern.size();

    //we save the pattern, in case it changes very slowly
    for (unsigned int i = 0; i < m_pattern.size(); i++)
      m_pattern[i] = pattern[(m_patternpos + i) % pattern.size()];

    if (!m_haspattern)
    {
      m_haspattern = true;
      m_patternlength = m_pattern.size();

      double frameduration = CalcFrameDuration();
      CLog::Log(LOGDEBUG, "CPullupCorrection: detected pattern of length %i: %s, frameduration: %f",
                (int)pattern.size(), GetPatternStr().c_str(), frameduration);
    }
  }

  //calculate where we are in the pattern
  double ptsinpattern = 0.0;
  for (int i = 0; i < m_patternpos; i++)
    ptsinpattern += m_pattern[m_pattern.size() - i - 1];

  m_frameduration = CalcFrameDuration();

  //correct the last pts based on where we should be according to the frame duration
  m_ptscorrection = (m_frameduration * m_patternpos) - ptsinpattern;

  double corrpts = pts + m_ptscorrection;
  if (m_trackingpts != DVD_NOPTS_VALUE)
  {
    //set the tracked pts a frame duration forward
    m_trackingpts += m_frameduration;

    //if the tracked pts differs too much from the corrected one,
    //move the tracked pts slowly towards the corrected one
    //this allows the timestamps to wobble slightly
    if (fabs(m_trackingpts - corrpts) > MAXERR)
      m_trackingpts += (corrpts - m_trackingpts) * 0.005;

    //set m_ptscorrection so that pts + m_ptscorrection = m_trackingpts
    m_ptscorrection = m_trackingpts - pts;
  }
  else
  {
    m_trackingpts = corrpts;
  }
}

//gets a diff diffnr into the past
inline double CPullupCorrection::GetDiff(int diffnr)
{
  //m_ringpos is the last added diff, so if we want to go in the past we have to move back in the ringbuffer
  int pos = m_ringpos - diffnr;
  if (pos < 0) pos += DIFFRINGSIZE;

  return m_diffring[pos];
}

//calculate the current pattern in the ringbuffer
void CPullupCorrection::GetPattern(std::vector<double>& pattern)
{
  int difftypesbuff[DIFFRINGSIZE]; //difftypes of the diffs, difftypesbuff[0] is the last added diff,
                                   //difftypesbuff[1] the one added before that etc

  //get the difftypes
  vector<double> difftypes;
  GetDifftypes(difftypes);

  //mark each diff with what difftype it is
  for (int i = 0; i < m_ringfill; i++)
  {
    for (unsigned int j = 0; j < difftypes.size(); j++)
    {
      if (MatchDiff(GetDiff(i), difftypes[j]))
      {
        difftypesbuff[i] = j;
        break;
      }
    }
  }

  bool checkexisting = m_pattern.size() > 0;

  //we check for patterns to the length of DIFFRINGSIZE / 2
  for (int i = 1; i <= m_ringfill / 2; i++)
  {
    //check the existing pattern length first
    int length = checkexisting ? m_pattern.size() : i;

    bool hasmatch = true;
    for (int j = 1; j <= m_ringfill / length; j++)
    {
      int nrdiffs = length;
      //we want to check the full buffer to see if the pattern repeats
      //but we can't go beyond the buffer
      if (j * length + length > m_ringfill)
        nrdiffs = m_ringfill - j * length;

      if (nrdiffs < 1)  //if the buffersize can be cleanly divided by i we're done here
        break;

      if (!MatchDifftype(difftypesbuff, difftypesbuff + j * length, nrdiffs))
      {
        hasmatch = false;
        break;
      }
    }

    if (checkexisting)
    {
      checkexisting = false;
      i--;
    }

    if (hasmatch)
    {
      BuildPattern(pattern, length);
      break;
    }
  }
}

//calculate the different types of diffs we have
void CPullupCorrection::GetDifftypes(vector<double>& difftypes)
{
  for (int i = 0; i < m_ringfill; i++)
  {
    bool hasmatch = false;
    for (unsigned int j = 0; j < difftypes.size(); j++)
    {
      if (MatchDiff(GetDiff(i), difftypes[j]))
      {
        hasmatch = true;
        break;
      }
    }

    //if we don't have a match with a saved difftype, we add it as a new one
    if (!hasmatch)
      difftypes.push_back(GetDiff(i));
  }
}

//builds a pattern of timestamps in the ringbuffer
void CPullupCorrection::BuildPattern(std::vector<double>& pattern, int patternlength)
{
  for (int i = 0; i < patternlength; i++)
  {
    double avgdiff = 0.0;
    for (int j = 0; j < m_ringfill / patternlength; j++)
      avgdiff += GetDiff(j * patternlength + i);

    avgdiff /= m_ringfill / patternlength;
    pattern.push_back(avgdiff);
  }
}

inline bool CPullupCorrection::MatchDiff(double diff1, double diff2)
{
  return fabs(diff1 - diff2) < MAXERR;
}

//check if diffs1 is the same as diffs2
inline bool CPullupCorrection::MatchDifftype(int* diffs1, int* diffs2, int nrdiffs)
{
  for (int i = 0; i < nrdiffs; i++)
  {
    if (diffs1[i] != diffs2[i])
      return false;
  }
  return true;
}

//check if our current detected pattern is the same as the one we saved
bool CPullupCorrection::CheckPattern(std::vector<double>& pattern)
{
  //if no pattern was detected or if the size of the patterns differ we don't have a match
  if (pattern.size() != m_pattern.size() || pattern.size() < 1)
    return false;

  if (pattern.size() == 1)
  {
    if (pattern[0] < MAXERR)
      return false; //all diffs are too close to 0, can't use this
  }

  //the saved pattern should have moved 1 diff into the past
  int patternpos = (m_patternpos + 1) % m_pattern.size();

  //check if the current pattern matches the saved pattern, with an offset of 1
  for (unsigned int i = 0; i < m_pattern.size(); i++)
  {
    double diff = pattern[(patternpos + i) % pattern.size()];

    if (!MatchDiff(diff, m_pattern[i]))
      return false;
  }

  return true;
}

//calculate how long each frame should last from the saved pattern
double CPullupCorrection::CalcFrameDuration()
{
  if (m_pattern.size() > 0)
  {
    //take the average of all diffs in the pattern
    double frameduration = 0.0;
    for (unsigned int i = 0; i < m_pattern.size(); i++)
      frameduration += m_pattern[i];

    frameduration /= m_pattern.size();

    //frameduration is not completely correct, use a common one if it's close
    return CDVDCodecUtils::NormalizeFrameduration(frameduration);
  }

  return DVD_NOPTS_VALUE;
}

//looks pretty in the log
CStdString CPullupCorrection::GetPatternStr()
{
  CStdString patternstr;

  for (unsigned int i = 0; i < m_pattern.size(); i++)
    patternstr.AppendFormat("%.2f ", m_pattern[i]);

  patternstr.Trim();

  return patternstr;
}

