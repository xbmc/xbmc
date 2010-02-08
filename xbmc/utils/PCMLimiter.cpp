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

//based on the fast lookahead limiter from http://plugin.org.uk/

//original description:

/*
This is a limiter with an attack time of 5ms. It adds just over 5ms of
lantecy to the input signal, but it guatantees that there will be no signals
over the limit, and tries to get the minimum ammount of distortion.
*/

#include "system.h"
#include "MathUtils.h"
#include "PCMLimiter.h"

CPCMLimiter::CPCMLimiter(int samplerate, int channels)
{
  //delay buffer of 5 milliseconds
  m_delaybuffsize = MathUtils::round_int((double)samplerate * DELAYTIME);
  m_channels      = channels;
  m_samplerate    = samplerate; //save samplerate as float so we don't cast every sample
  m_release       = 2.0; //default release time 2 seconds

  m_delaybuff = new float[m_delaybuffsize * m_channels];
  memset(m_delaybuff, 0, m_delaybuffsize * m_channels * sizeof(float));
  m_delaybuffpos = 0;

  m_chunknum = 0;
  m_chunkpos = 0;
  m_chunksize = m_delaybuffsize / 10; //chunk size of roughly 0.5 ms
  memset(m_chunks, 0, sizeof(m_chunks));

  m_peak    = 0.0f;
  m_atten   = 1.0f;
  m_attenlp = 1.0f;
  m_delta   = 0.0f;
}

CPCMLimiter::~CPCMLimiter()
{
  delete [] m_delaybuff;
}

void CPCMLimiter::Run(float* insamples, float* outsamples, int frames)
{
  float* inptr = insamples;
  float* outptr = outsamples;
  float* delayptr;

  for (int i = 0; i < frames; i++)
  {
    if (m_chunkpos++ == m_chunksize)
    { //we have a full chunk, apparently the attenuation is calculated here
      //I'm not completely sure what this does, it seems to base the attenuation on the rate of change
      m_delta = (1.0f - m_atten) / (m_samplerate / m_release);
      for (int j = 0; j < 10; j++)
      {
        int p = (m_chunknum - 9 + j) & (CHUNKS - 1);
        float thisdelta = (1.0f / m_chunks[p] - m_atten) / ((float)(j + 1) * m_samplerate * (DELAYTIME / 10.0f) + 1.0f);
        if (thisdelta < m_delta)
          m_delta = thisdelta;
      }
      m_chunks[m_chunknum++ & (CHUNKS - 1)] = m_peak;
      m_peak = 0.0f;
      m_chunkpos = 0;
    }

    delayptr = m_delaybuff + m_delaybuffpos * m_channels;

    for (int j = 0; j < m_channels; j++)
    {
      //store input at the beginning of the delay buffer
      delayptr[j] = *inptr++;

      //get the maximum absolute value of all the channels
      if (fabs(delayptr[j]) > m_peak)
        m_peak = fabs(delayptr[j]);
    }

    //calculate attenuation
    m_atten += m_delta;
    m_attenlp = m_atten * 0.1f + m_attenlp * 0.9f;

    //not sure what this is for
    if (m_delta > 0.0f && m_atten > 1.0f)
    {
      m_atten = 1.0f;
      m_delta = 0.0;
    }

    //increase delaybuffer position
    m_delaybuffpos = (m_delaybuffpos + 1) % m_delaybuffsize;

    //write from the end of the delay buffer to the output with attenuation
    for (int j = 0; j < m_channels; j++)
      *outptr++ = delayptr[j] * m_attenlp;
  }
}

