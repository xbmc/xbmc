/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2007 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */
/**
 * $Id$
 *
 * Encapsulation of raw sound buffer. Used in beat detection
 *
 * $Log$
 */

#ifndef _PCM_H
#define _PCM_H

class PCM {
public:
    mutable float **PCMd;
    mutable int start;

    /** Use wave smoothing */
    float waveSmoothing;

    int *ip;
    double *w;
    mutable int newsamples;

    mutable int numsamples; //size of new PCM info
    float *pcmdataL;     //holder for most recent pcm data 
    float *pcmdataR;     //holder for most recent pcm data 

    /** PCM data */
    mutable float vdataL[512];  //holders for FFT data (spectrum)
    mutable float vdataR[512];

    static int maxsamples;
    PCM();
    ~PCM();
    void initPCM(int maxsamples);
    void addPCMfloat(const float *PCMdata, int samples) const;
    void addPCM16(short [2][512]) const;
    void addPCM16Data(const short* pcm_data, short samples) const;
    void addPCM8( unsigned char [2][1024]) const;
	void addPCM8_512( const unsigned char [2][512]) const;
    void getPCM(float *data, int samples, int channel, int freq, float smoothing, int derive) const;
    void freePCM();
    int getPCMnew(float *PCMdata, int channel, int freq, float smoothing, int derive,int reset) const;
    

  };

#endif /** !_PCM_H */
