/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2004 projectM Team
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

#ifndef _PCM_H
#define _PCM_H

void initPCM(int maxsamples);
#ifdef __CPlUSCPLUS
extern "C" void addPCMfloat(float *PCMdata, int samples);
extern "C" void addPCM16(short [2][512]);
extern "C" void addPCM16Data(const short* pcm_data, short samples);
extern "C" void addPCM8( unsigned char [2][512]);
#else
extern void addPCMfloat(float *PCMdata, int samples);
extern void addPCM16(short [2][512]);
extern void addPCM16Data(const short* pcm_data, short samples);
extern void addPCM8( unsigned char [2][512]);
#endif
void getPCM(float *data, int samples, int channel, int freq, float smoothing, int derive);
void freePCM();
int getPCMnew(float *PCMdata, int channel, int freq, float smoothing, int derive,int reset);

#endif /** !_PCM_H */
