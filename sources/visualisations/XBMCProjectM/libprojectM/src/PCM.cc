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
#include "wipemalloc.h"
#include "fftsg.h"



float **PCMd;    //data structure to store PCM data  PCM[channels][maxsamples]
int maxsamples;   //size of PCM buffer
int start;        //where to add data next

int *ip;          //working space for FFT routines
double *w;        //lookup table for FFT routines
int newsamples;          //how many new samples

#ifdef DEBUG
extern FILE *debugFile;
#endif

//initPCM(int samples)
//
//Initializes the PCM buffer to
// number of samples specified.

void initPCM(int samples)
{
  int i; 

#ifdef DEBUG
    if ( debugFile != NULL ) {
        fprintf( debugFile, "initPCM()\n" );
        fflush( debugFile );
      }
#endif 

  //Allocate memory for PCM data buffer
  PCMd = (float **)wipemalloc(2 * sizeof(float *));
  PCMd[0] = (float *)wipemalloc(samples * sizeof(float));
  PCMd[1] = (float *)wipemalloc(samples * sizeof(float));
  
  maxsamples=samples;
  newsamples=0;

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
}


 void addPCMfloat(float *PCMdata, int samples)
{
  int i,j;


	 for(i=0;i<samples;i++)
	   {
	     j=i+start;

           

         if ( PCMdata[i] != 0 && PCMdata[i] != 0 ) {
	   
	    PCMd[0][j%maxsamples]=(float)PCMdata[i];
	    PCMd[1][j%maxsamples]=(float)PCMdata[i];  
	 
          } else {
             PCMd[0][j % maxsamples] =(float) 0;
             PCMd[1][j % maxsamples] =(float) 0;
          }
	   }
	  
 
	 // printf("Added %d samples %d %d %f\n",samples,start,(start+samples)%maxsamples,PCM[0][start+10]); 

 start+=samples;
 start=start%maxsamples;

 newsamples+=samples;
 if (newsamples>maxsamples) newsamples=maxsamples;
}

 void addPCM16Data(const short* pcm_data, short samples) {
   int i, j;

   for (i = 0; i < samples; ++i) {
      j=i+start;
      PCMd[0][j % maxsamples]=(pcm_data[i * 2 + 0]/16384.0);
      PCMd[1][j % maxsamples]=(pcm_data[i * 2 + 1]/16384.0);
   }

   start = (start + samples) % maxsamples;

   newsamples+=samples;
   if (newsamples>maxsamples) newsamples=maxsamples;
}


 void addPCM16(short PCMdata[2][512])
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
       
 
	 // printf("Added %d samples %d %d %f\n",samples,start,(start+samples)%maxsamples,PCM[0][start+10]); 

 start+=samples;
 start=start%maxsamples;

 newsamples+=samples;
 if (newsamples>maxsamples) newsamples=maxsamples;
}


 void addPCM8( unsigned char PCMdata[2][512])
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
       
 
	 // printf("Added %d samples %d %d %f\n",samples,start,(start+samples)%maxsamples,PCM[0][start+10]); 

 start+=samples;
 start=start%maxsamples;

 newsamples+=samples;
 if (newsamples>maxsamples) newsamples=maxsamples;
}


//puts sound data requested at provided pointer
//
//samples is number of PCM samples to return
//freq = 0 gives PCM data
//freq = 1 gives FFT data
//smoothing is the smoothing coefficient

//returned values are normalized from -1 to 1

void getPCM(float *PCMdata, int samples, int channel, int freq, float smoothing, int derive)
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

int getPCMnew(float *PCMdata, int channel, int freq, float smoothing, int derive, int reset)
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
void freePCM()
{
  free(PCMd[0]);
  free(PCMd[1]);
  free(PCMd);
  free(ip);
  free(w);

  PCMd = NULL;
  ip = NULL;
  w = NULL;
}
