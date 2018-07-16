/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

#define DIFFRINGSIZE 120
#define VFR_DETECTION_THRESHOLD 3
#define VFR_PATTERN_THRESHOLD 2

class CPtsTracker
{
  public:
    CPtsTracker();
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
    static bool MatchDifftype(int diffs1[], int diffs2[], int nrdiffs); //checks if the difftypes match

    //checks if the current pattern matches with the saved m_pattern with offset m_patternpos
    bool CheckPattern(std::vector<double>& pattern);

    double CalcFrameDuration(); //calculates the frame duration from m_pattern

    std::vector<double> m_pattern, m_lastPattern; //the last saved pattern
    double m_frameduration;        //frameduration exposed to VideoPlayer, used for calculating the fps
    double m_maxframeduration;     //Max value detected for frame duration (for VFR files case)
    double m_minframeduration;     //Min value detected for frame duration (for VFR files case)
    bool m_haspattern;             //for the log and detecting VFR files case
    int m_patternlength;           //for the codec info
    int m_VFRCounter;              //retry counter for VFR detection
    int m_patternCounter;
    std::string GetPatternStr();   //also for the log
};
