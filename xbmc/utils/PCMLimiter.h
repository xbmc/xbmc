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

#define CHUNKS 16
#define DELAYTIME 0.005f

class CPCMLimiter
{
  public:
    CPCMLimiter(int samplerate, int channels);
    ~CPCMLimiter();

    void   Run(float* insamples, float* outsamples, int frames);
    void   SetRelease(float release) { m_release = release; }
    double GetDelay() { return DELAYTIME; }

  private:
    float* m_delaybuff;
    int    m_delaybuffsize; //buffer size in frames
    int    m_delaybuffpos;  //m_delaybuff is used as a ringbuffer
    float  m_samplerate;
    int    m_channels;
    float  m_release; //release time in seconds

    int    m_chunknum;
    int    m_chunkpos;
    int    m_chunksize;
    float  m_chunks[CHUNKS];

    float  m_peak;
    float  m_atten;
    float  m_attenlp;
    float  m_delta;
};
