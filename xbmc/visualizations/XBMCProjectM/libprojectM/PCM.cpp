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
/**
 * $Id: PCM.c,v 1.3 2006/03/13 20:35:26 psperl Exp $
 *
 * Takes sound data from wherever and hands it back out.
 * Returns PCM Data or spectrum data, or the derivative of the PCM data
 */

#include <stdlib.h>
#include <stdio.h>

#include "Common.hpp"
#include "wipemalloc.h"
#include "fftsg.h"
#include "PCM.hpp"
#include <cassert>

int PCM::maxsamples = 2048;

//initPCM(int samples)
//
//Initializes the PCM buffer to
// number of samples specified.
#include <iostream>
PCM::PCM() {
    initPCM( 2048 );
  }

void PCM::initPCM(int samples) {
  int i; 

    waveSmoothing = 0;

  //Allocate memory for PCM data buffer
    assert(samples == 2048);
  PCMd = (float **)wipemalloc(2 * sizeof(float *));
  PCMd[0] = (float *)wipemalloc(samples * sizeof(float));
  PCMd[1] = (float *)wipemalloc(samples * sizeof(float));
  
  //maxsamples=samples;
  newsamples=0;
    numsamples = maxsamples;

  //Initialize buffers to 0
  for (i=0;i<samples;i++)
    {
      PCMd[0][i]=0;
      PCMd[1][i]=0;
    }

  start=0;

  //Allocate FFT workspace
  w=  (double *)wipemalloc(maxsamples*sizeof(double));
  ip= (int *)wipemalloc(maxsamples*sizeof(int));
  ip[0]=0;


    /** PCM data */
//    this->maxsamples = 2048;
//    this->numsamples = 0;
//    this->pcmdataL = NULL;
//    this->pcmdataR = NULL;

    /** Allocate PCM data structures */
    pcmdataL=(float *)wipemalloc(this->maxsamples*sizeof(float));
    pcmdataR=(float *)wipemalloc(this->maxsamples*sizeof(float));
 
}

PCM::~PCM() {

	free(pcmdataL);
	free(pcmdataR);
	free(w);
	free(ip);
	
	free(PCMd[0]);
	free(PCMd[1]);
	free(PCMd);

}

#include <iostream>

void PCM::addPCMfloat(const float *PCMdata, int samples) const
{
  int i,j;

  for(i=0;i<samples;i++)
    {
      j=i+start;

      if (PCMdata[i] != 0 ) {
	
	PCMd[0][j%maxsamples] = PCMdata[i];
	PCMd[1][j%maxsamples] = PCMdata[i];
	
      }
      else 
	{
	  PCMd[0][j % maxsamples] = 0;
	  PCMd[1][j % maxsamples] = 0;
	}
    }
  
  start+=samples;
  start=start%maxsamples;
  
 newsamples+=samples;
 if (newsamples>maxsamples) newsamples=maxsamples;
  numsamples = getPCMnew(pcmdataR,1,0,waveSmoothing,0,0);
    getPCMnew(pcmdataL,0,0,waveSmoothing,0,1);
    getPCM(vdataL,512,0,1,0,0);
    getPCM(vdataR,512,1,1,0,0);
}

void PCM::addPCM16Data(const short* pcm_data, short samples) const {
   int i, j;

   for (i = 0; i < samples; ++i) {
      j=i+start;
      PCMd[0][j % maxsamples]=(pcm_data[i * 2 + 0]/16384.0);
      PCMd[1][j % maxsamples]=(pcm_data[i * 2 + 1]/16384.0);
   }

   start = (start + samples) % maxsamples;

   newsamples+=samples;
   if (newsamples>maxsamples) newsamples=maxsamples;
     numsamples = getPCMnew(pcmdataR,1,0,waveSmoothing,0,0);
    getPCMnew(pcmdataL,0,0,waveSmoothing,0,1);
    getPCM(vdataL,512,0,1,0,0);
    getPCM(vdataR,512,1,1,0,0);
}


void PCM::addPCM16(short PCMdata[2][512]) const
{
  int i,j;
  int samples=512;
 
	 for(i=0;i<samples;i++)
	   {
	     j=i+start;
         if ( PCMdata[0][i] != 0 && PCMdata[1][i] != 0 ) {
	         PCMd[0][j%maxsamples]=(PCMdata[0][i]/16384.0);
	         PCMd[1][j%maxsamples]=(PCMdata[1][i]/16384.0);  
          } else {
             PCMd[0][j % maxsamples] = (float)0;
             PCMd[1][j % maxsamples] = (float)0;
          }
	   }
 
 start+=samples;
 start=start%maxsamples;

 newsamples+=samples;
 if (newsamples>maxsamples) newsamples=maxsamples;

    numsamples = getPCMnew(pcmdataR,1,0,waveSmoothing,0,0);
    getPCMnew(pcmdataL,0,0,waveSmoothing,0,1);
    getPCM(vdataL,512,0,1,0,0);
    getPCM(vdataR,512,1,1,0,0);
}


void PCM::addPCM8( unsigned char PCMdata[2][1024]) const
{
  int i,j;
  int samples=1024;

 
	 for(i=0;i<samples;i++)
	   {
	     j=i+start;
         if ( PCMdata[0][i] != 0 && PCMdata[1][i] != 0 ) {
	         PCMd[0][j%maxsamples]=( (float)( PCMdata[0][i] - 128.0 ) / 64 );
	         PCMd[1][j%maxsamples]=( (float)( PCMdata[1][i] - 128.0 ) / 64 );  
          } else {
             PCMd[0][j % maxsamples] = 0;
             PCMd[1][j % maxsamples] = 0;
          }
	   }
 
 start+=samples;
 start=start%maxsamples;

 newsamples+=samples;
 if (newsamples>maxsamples) newsamples=maxsamples;
    numsamples = getPCMnew(pcmdataR,1,0,waveSmoothing,0,0);
    getPCMnew(pcmdataL,0,0,waveSmoothing,0,1);
    getPCM(vdataL,512,0,1,0,0);
    getPCM(vdataR,512,1,1,0,0);
}

void PCM::addPCM8_512( const unsigned char PCMdata[2][512]) const
{
  int i,j;
  int samples=512;

 
	 for(i=0;i<samples;i++)
	   {
	     j=i+start;
         if ( PCMdata[0][i] != 0 && PCMdata[1][i] != 0 ) {
	         PCMd[0][j%maxsamples]=( (float)( PCMdata[0][i] - 128.0 ) / 64 );
	         PCMd[1][j%maxsamples]=( (float)( PCMdata[1][i] - 128.0 ) / 64 );  
          } else {
             PCMd[0][j % maxsamples] = 0;
             PCMd[1][j % maxsamples] = 0;
          }
	   }
       
 start+=samples;
 start=start%maxsamples;

 newsamples+=samples;
 if (newsamples>maxsamples) newsamples=maxsamples;
    numsamples = getPCMnew(pcmdataR,1,0,waveSmoothing,0,0);
    getPCMnew(pcmdataL,0,0,waveSmoothing,0,1);
    getPCM(vdataL,512,0,1,0,0);
    getPCM(vdataR,512,1,1,0,0);
}


//puts sound data requested at provided pointer
//
//samples is number of PCM samples to return
//freq = 0 gives PCM data
//freq = 1 gives FFT data
//smoothing is the smoothing coefficient

//returned values are normalized from -1 to 1

void PCM::getPCM(float *PCMdata, int samples, int channel, int freq, float smoothing, int derive) const
{
   int i,index;
   
   index=start-1;

   if (index<0) index=maxsamples+index;

   PCMdata[0]=PCMd[channel][index];
   
   for(i=1;i<samples;i++)
     {
       index=start-1-i;
       if (index<0) index=maxsamples+index;
       
       PCMdata[i]=(1-smoothing)*PCMd[channel][index]+smoothing*PCMdata[i-1];
     }
   
   //return derivative of PCM data
   if(derive)
     {
       for(i=0;i<samples-1;i++)
	 {	   
	   PCMdata[i]=PCMdata[i]-PCMdata[i+1];
	 }
       PCMdata[samples-1]=0;
     }

   //return frequency data instead of PCM (perform FFT)
  
   if (freq) 

     {
       double temppcm[1024];
       for (int i=0;i<samples;i++)
	 {temppcm[i]=(double)PCMdata[i];}
       rdft(samples, 1, temppcm, ip, w);
       for (int j=0;j<samples;j++)
	 {PCMdata[j]=(float)temppcm[j];}
     }
}

//getPCMnew
//
//Like getPCM except it returns all new samples in the buffer
//the actual return value is the number of samples, up to maxsamples.
//the passed pointer, PCMData, must bee able to hold up to maxsamples

int PCM::getPCMnew(float *PCMdata, int channel, int freq, float smoothing, int derive, int reset) const
{
   int i,index;
   
   index=start-1;

   if (index<0) index=maxsamples+index;

   PCMdata[0]=PCMd[channel][index];
   for(i=1;i<newsamples;i++)
     {
       index=start-1-i;
       if (index<0) index=maxsamples+index;
       
       PCMdata[i]=(1-smoothing)*PCMd[channel][index]+smoothing*PCMdata[i-1];
     }
   
   //return derivative of PCM data
   if(derive)
     {
       for(i=0;i<newsamples-1;i++)
	 {	   
	   PCMdata[i]=PCMdata[i]-PCMdata[i+1];
	 }
       PCMdata[newsamples-1]=0;
     }

   //return frequency data instead of PCM (perform FFT)
   //   if (freq) rdft(samples, 1, PCMdata, ip, w);
   i=newsamples;
   if (reset)  newsamples=0;

   return i;
}

//Free stuff
void PCM::freePCM() {
  free(PCMd[0]);
  free(PCMd[1]);
  free(PCMd);
  free(ip);
  free(w);

  PCMd = NULL;
  ip = NULL;
  w = NULL;
}
