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
#pragma once
 
#include <samplerate.h>

#define MAXCONVSAMPLES 100000
#define RINGSIZE 1000000

#define PROPORTIONAL 20.0
#define PROPREF 0.01
#define PROPDIVMIN 2.0
#define PROPDIVMAX 40.0
#define INTEGRAL 200.0

//forward declaration of struct stDVDAudioFrame
typedef struct stDVDAudioFrame DVDAudioFrame;

class CDVDPlayerResampler
{
  public:
    CDVDPlayerResampler();
    ~CDVDPlayerResampler();
  
    void Add(DVDAudioFrame &audioframe, double pts);
    bool Retreive(DVDAudioFrame &audioframe, double &pts);
    void SetRatio(double ratio);
    void Flush();
    void SetQuality(int Quality);
    void Clean();
  
  private:
  
    int m_NrChannels;
    int m_Quality;
    SRC_STATE* m_Converter;
    SRC_DATA m_ConverterData;
  
    float*  m_RingBuffer;  //ringbuffer for the audiosamples
    int     m_RingBufferPos;  //where we are in the ringbuffer
    int     m_RingBufferFill; //how many unread samples there are in the ringbuffer, starting at RingBufferPos
    double *m_PtsRingBuffer;  //ringbuffer for the pts value, each sample gets its own pts
  
    void CheckResampleBuffers(int channels);
    
    //this makes sure value is bewteen min and max
    template <typename A, typename B, typename C>
        inline A Clamp(A value, B min, C max){ return value < max ? (value > min ? value : min) : max; }
};
