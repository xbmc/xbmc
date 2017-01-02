#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <string>
#include <vector>

#define DIFFRINGSIZE 120
#define VFR_DETECTION_THRESHOLD 3
#define VFR_PATTERN_THRESHOLD 2

class CPullupCorrection
{
  public:
    CPullupCorrection();
    void   Add(double pts);
    void   Flush(); //flush the saved pattern and the ringbuffer
    void   ResetVFRDetection(void);

    int    GetPatternLength() { return m_patternlength;            }
    double GetFrameDuration() { return m_frameduration;            }
    double GetMaxFrameDuration(void) { return m_maxframeduration;  }
    double GetMinFrameDuration(void) { return m_minframeduration;  }
    bool   HasFullBuffer()    { return m_ringfill == DIFFRINGSIZE; }
    bool   VFRDetection(void) { return ((m_VFRCounter >= VFR_DETECTION_THRESHOLD) && (m_patternCounter >= VFR_PATTERN_THRESHOLD)); }

  private:
    double m_prevpts;                //last pts added
    double m_diffring[DIFFRINGSIZE]; //ringbuffer of differences between pts'
    int    m_ringpos;                //position of last diff added to ringbuffer
    int    m_ringfill;               //how many diffs we have in the ringbuffer
    double GetDiff(int diffnr);      //gets diffs from now to the past

    void GetPattern(std::vector<double>& pattern);     //gets the current pattern

    static bool MatchDiff(double diff1, double diff2); //checks if two diffs match by MAXERR
    static bool MatchDifftype(int* diffs1, int* diffs2, int nrdiffs); //checks if the difftypes match

    //checks if the current pattern matches with the saved m_pattern with offset m_patternpos
    bool CheckPattern(std::vector<double>& pattern);

    double CalcFrameDuration(); //calculates the frame duration from m_pattern

    std::vector<double> m_pattern, m_lastPattern; //the last saved pattern
    int m_patternpos;              //the position of the pattern in the ringbuffer, moves one to the past each time a pts is added
    double m_frameduration;        //frameduration exposed to VideoPlayer, used for calculating the fps
    double m_maxframeduration;     //Max value detected for frame duration (for VFR files case)
    double m_minframeduration;     //Min value detected for frame duration (for VFR files case)
    bool m_haspattern;             //for the log and detecting VFR files case
    int m_patternlength;           //for the codec info
    int m_VFRCounter;              //retry counter for VFR detection
    int m_patternCounter;
    std::string GetPatternStr();   //also for the log
};
