#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <vector>

#define DIFFRINGSIZE 120

class CPullupCorrection
{
  public:
    CPullupCorrection();
    void   Add(double pts);
    double Correction() {return m_ptscorrection;}
    void   Flush(); //flush the saved pattern and the ringbuffer

    int    GetPatternLength() {return m_patternlength;}
    double CalcFrameDuration(); //calculates the frame duration from m_pattern

  private:
    double m_prevpts;                //last pts added
    double m_diffring[DIFFRINGSIZE]; //ringbuffer of differences between pts'
    int    m_ringpos;                //position of last diff added to ringbuffer
    int    m_ringfill;               //how many diffs we have in the ringbuffer
    int    m_leadin;                 //how many timestamps we ignored
    double GetDiff(int diffnr);      //gets diffs from now to the past

    void GetPattern(std::vector<double>& pattern);     //gets the current pattern
    void GetDifftypes(std::vector<double>& difftypes); //gets the difftypes from the ringbuffer

    bool MatchDiff(double diff1, double diff2); //checks if two diffs match by MAXERR
    bool MatchDifftype(int* diffs1, int* diffs2, int nrdiffs); //checks if the difftypes match

    //builds a pattern of timestamps in the ringbuffer
    void BuildPattern(std::vector<double>& pattern, int patternlength);

    //checks if the current pattern matches with the saved m_pattern with offset m_patternpos
    bool CheckPattern(std::vector<double>& pattern);

    std::vector<double> m_pattern; //the last saved pattern
    int    m_patternpos;           //the position of the pattern in the ringbuffer, moves one to the past each time a pts is added
    double m_ptscorrection;        //the correction needed for the last added pts
    bool   m_haspattern;           //for the log
    int    m_patternlength;        //for the codec info
    CStdString GetPatternStr();   //also for the log
};
