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
#pragma once

#include <samplerate.h>

#define MAXRATIO 30

#define PROPORTIONAL 20.0
#define PROPREF       0.01
#define PROPDIVMIN    2.0
#define PROPDIVMAX   40.0
#define INTEGRAL    200.0

//forward declaration of struct stDVDAudioFrame
typedef struct stDVDAudioFrame DVDAudioFrame;

class CDVDPlayerResampler
{
  public:
    CDVDPlayerResampler();
    ~CDVDPlayerResampler();

    void Add(DVDAudioFrame &audioframe, double pts);       //add audioframes and resample
    bool Retrieve(DVDAudioFrame &audioframe, double &pts); //get audioframes fromt the samplebuffer
    void SetRatio(double ratio);                           //ratio higher than 1.0 means more output samples than input
    void Flush();                                          //clear samplebuffer
    void SetQuality(int quality);
    void Clean();                                          //free buffers

  private:

    int        m_nrchannels;
    int        m_quality;
    SRC_STATE* m_converter;
    SRC_DATA   m_converterdata;
    double     m_ratio;

    float*     m_buffer;     //buffer for the audioframes
    int        m_bufferfill; //how many unread frames there are in the buffer
    int        m_buffersize; //size of allocated buffer in frames
    double*    m_ptsbuffer;  //ringbuffer for the pts value, each frame gets its own pts

    void CheckResampleBuffers(int channels);
    void ResizeSampleBuffer(int nrframes);

    //this makes sure value is bewteen min and max
    template <typename A, typename B, typename C>
        inline A Clamp(A value, B min, C max){ return value < max ? (value > min ? value : min) : max; }
};
