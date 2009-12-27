/*
 * MilkdropWaveform.cpp
 *
 *  Created on: Jun 25, 2008
 *      Author: pete
 */
#include <iostream>

#ifdef LINUX
#include <GL/gl.h>
#endif
#ifdef WIN32
#include "glew.h"
#endif
#ifdef __APPLE__
#include <GL/gl.h>
#endif

#include "MilkdropWaveform.hpp"
#include "math.h"
#include "BeatDetect.hpp"

MilkdropWaveform::MilkdropWaveform(): RenderItem(),
	x(0.5), y(0.5), r(1), g(0), b(0), a(1), mystery(0), mode(Line), scale(10), smoothing(0), rot(0), samples(0),modOpacityStart(0),modOpacityEnd(1),
	modulateAlphaByVolume(false), maximizeColors(false), additive(false), dots(false), thick(false), loop(false) {}

void MilkdropWaveform::Draw(RenderContext &context)
{
	  WaveformMath(context);

	glMatrixMode( GL_MODELVIEW );
		glPushMatrix();
		glLoadIdentity();

		if(modulateAlphaByVolume) ModulateOpacityByVolume(context);
		else temp_a = a;
		MaximizeColors(context);

	#ifndef USE_GLES1
		if(dots==1) glEnable(GL_LINE_STIPPLE);
	#endif

		//Thick wave drawing
		if (thick==1)  glLineWidth( (context.texsize < 512 ) ? 2 : 2*context.texsize/512);
		else glLineWidth( (context.texsize < 512 ) ? 1 : context.texsize/512);

		//Additive wave drawing (vice overwrite)
		if (additive==1)glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


		glTranslatef(.5, .5, 0);
		glRotatef(rot, 0, 0, 1);
		glScalef(aspectScale, 1.0, 1.0);
		glTranslatef(-.5, -.5, 0);


		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glVertexPointer(2,GL_FLOAT,0,wavearray);

		if (loop)
		  glDrawArrays(GL_LINE_LOOP,0,samples);
		else
		  glDrawArrays(GL_LINE_STRIP,0,samples);


		if (two_waves)
		  {
		    glVertexPointer(2,GL_FLOAT,0,wavearray2);
		    if (loop)
		      glDrawArrays(GL_LINE_LOOP,0,samples);
		    else
		      glDrawArrays(GL_LINE_STRIP,0,samples);
		  }


	#ifndef USE_GLES1
		if(dots==1) glDisable(GL_LINE_STIPPLE);
	#endif

		glPopMatrix();
}

void MilkdropWaveform::ModulateOpacityByVolume(RenderContext &context)
{

	//modulate volume by opacity
	//
	//set an upper and lower bound and linearly
	//calculate the opacity from 0=lower to 1=upper
	//based on current volume

	if (context.beatDetect->vol<= modOpacityStart)  temp_a=0.0;
	 else if (context.beatDetect->vol>=modOpacityEnd) temp_a=a;
	 else temp_a=a*((context.beatDetect->vol-modOpacityStart)/(modOpacityEnd-modOpacityStart));

}

void MilkdropWaveform::MaximizeColors(RenderContext &context)
{

	float wave_r_switch=0, wave_g_switch=0, wave_b_switch=0;
	//wave color brightening
	//
	//forces max color value to 1.0 and scales
	// the rest accordingly
	if(mode==Blob2 || mode==Blob5)
		switch(context.texsize)
		{
			case 256:  temp_a *= 0.07f; break;
			case 512:  temp_a *= 0.09f; break;
			case 1024: temp_a *= 0.11f; break;
			case 2048: temp_a *= 0.13f; break;
		}
	else if(mode==Blob3)
	{
		switch(context.texsize)
		{
			case 256:  temp_a *= 0.075f; break;
			case 512:  temp_a *= 0.15f; break;
			case 1024: temp_a *= 0.22f; break;
			case 2048: temp_a *= 0.33f; break;
		}
		temp_a*=1.3f;
		temp_a*=powf(context.beatDetect->treb , 2.0f);
	}

	if (maximizeColors==true)
	{
		if(r>=g && r>=b)   //red brightest
		{
			wave_b_switch=b*(1/r);
			wave_g_switch=g*(1/r);
			wave_r_switch=1.0;
		}
		else if   (b>=g && b>=r)         //blue brightest
		{
			wave_r_switch=r*(1/b);
			wave_g_switch=g*(1/b);
			wave_b_switch=1.0;

		}

		else  if (g>=b && g>=r)         //green brightest
		{
			wave_b_switch=b*(1/g);
			wave_r_switch=r*(1/g);
			wave_g_switch=1.0;
		}


		glColor4f(wave_r_switch, wave_g_switch, wave_b_switch, temp_a * masterAlpha);
	}
	else
	{
		glColor4f(r, g, b, temp_a * masterAlpha);
	}
}


void MilkdropWaveform::WaveformMath(RenderContext &context)
{

	int i;

	float r, theta;

	float offset;

	float wave_x_temp=0;
	float wave_y_temp=0;

	float cos_rot;
	float sin_rot;

	offset=x-.5;
    float temp_y;

	two_waves = false;
	loop = false;

	switch(mode)
	{

		case Circle:
		  {
 		    loop = true;
			rot =   0;
			aspectScale=1.0;
			temp_y=-1*(y-1.0);


			samples = 0? 512-32 : context.beatDetect->pcm->numsamples;

			float inv_nverts_minus_one = 1.0f/(float)(samples);

	       float last_value = context.beatDetect->pcm->pcmdataR[samples-1]+context.beatDetect->pcm->pcmdataL[samples-1];
			float first_value = context.beatDetect->pcm->pcmdataR[0]+context.beatDetect->pcm->pcmdataL[0];
			float offset = first_value-last_value;

			for ( int i=0;i<samples;i++)
			{

			  float value = context.beatDetect->pcm->pcmdataR[i]+context.beatDetect->pcm->pcmdataL[i];
			  value += offset * (i/(float)samples);

			  r=(0.5 + 0.4f*.12*value*scale + mystery)*.5;
			  theta=i*inv_nverts_minus_one*6.28f + context.time*0.2f;

			  wavearray[i][0]=(r*cos(theta)*(context.aspectCorrect? context.aspectRatio : 1.0)+x);
			  wavearray[i][1]=(r*sin(theta)+temp_y);
			}
		  }

			break;

		case RadialBlob://circularly moving waveform

			rot =   0;
			aspectScale = context.aspectRatio;

			temp_y=-1*(y-1.0);

			samples = 512-32;
			for ( int i=0;i<512-32;i++)
			{
				theta=context.beatDetect->pcm->pcmdataL[i+32]*0.06*scale * 1.57 + context.time*2.3;
				r=(0.53 + 0.43*context.beatDetect->pcm->pcmdataR[i]*0.12*scale+ mystery)*.5;

				wavearray[i][0]=(r*cos(theta)*(context.aspectCorrect ? context.aspectRatio : 1.0)+x);
				wavearray[i][1]=(r*sin(theta)+temp_y);
			}

			break;

		case Blob2://EXPERIMENTAL

			temp_y=-1*(y-1.0);
			rot =   0;
			aspectScale =1.0;
			samples = 512-32;

			for ( int i=0;i<512-32;i++)
			{
				wavearray[i][0]=(context.beatDetect->pcm->pcmdataR[i]*scale*0.5*(context.aspectCorrect ? context.aspectRatio : 1.0) + x);
				wavearray[i][1]=(context.beatDetect->pcm->pcmdataL[i+32]*scale*0.5 + temp_y);
			}

			break;

		case Blob3://EXPERIMENTAL

			temp_y=-1*(y-1.0);

			rot =   0;
			aspectScale =1.0;

			samples = 512-32;

			for ( int i=0;i<512-32;i++)
			{
				wavearray[i][0]=(context.beatDetect->pcm->pcmdataR[i] * scale*0.5 + x);
				wavearray[i][1]=( (context.beatDetect->pcm->pcmdataL[i+32]*scale*0.5 + temp_y));
			}

			break;

		case DerivativeLine://single x-axis derivative waveform
		{
			rot =-mystery*90;
			aspectScale=1.0;

			temp_y=-1*(y-1.0);

			float w1 = 0.45f + 0.5f*(mystery*0.5f + 0.5f);
			float w2 = 1.0f - w1;
			float xx[512], yy[512];
			samples = 512-32;

			for (int i=0; i<512-32; i++)
			{
				xx[i] = -1.0f + 2.0f*(i/(512.0-32.0)) + x;
				yy[i] =0.4* context.beatDetect->pcm->pcmdataL[i]*0.47f*scale + temp_y;
				xx[i] += 0.4*context.beatDetect->pcm->pcmdataR[i]*0.44f*scale;

				if (i>1)
				{
					xx[i] = xx[i]*w2 + w1*(xx[i-1]*2.0f - xx[i-2]);
					yy[i] = yy[i]*w2 + w1*(yy[i-1]*2.0f - yy[i-2]);
				}
				wavearray[i][0]=xx[i];
				wavearray[i][1]=yy[i];
			}											   		}
		break;

		case Blob5://EXPERIMENTAL

			rot = 0;
			aspectScale =1.0;

			temp_y=-1*(y-1.0);

			cos_rot = cosf(context.time*0.3f);
			sin_rot = sinf(context.time*0.3f);
			samples = 512-32;

			for ( int i=0;i<512-32;i++)
			{
				float x0 = (context.beatDetect->pcm->pcmdataR[i]*context.beatDetect->pcm->pcmdataL[i+32] + context.beatDetect->pcm->pcmdataL[i+32]*context.beatDetect->pcm->pcmdataR[i]);
				float y0 = (context.beatDetect->pcm->pcmdataR[i]*context.beatDetect->pcm->pcmdataR[i] - context.beatDetect->pcm->pcmdataL[i+32]*context.beatDetect->pcm->pcmdataL[i+32]);
				wavearray[i][0]=((x0*cos_rot - y0*sin_rot)*scale*0.5*(context.aspectCorrect ? context.aspectRatio : 1.0) + x);
				wavearray[i][1]=( (x0*sin_rot + y0*cos_rot)*scale*0.5 + temp_y);
			}
			break;

		case Line://single waveform


			wave_x_temp=-2*0.4142*(fabs(fabs(mystery)-.5)-.5);

			rot = -mystery*90;
			aspectScale =1.0+wave_x_temp;
			wave_x_temp=-1*(x-1.0);
			samples = 0 ? 512-32 : context.beatDetect->pcm->numsamples;

			for ( int i=0;i<  samples;i++)
			{

				wavearray[i][0]=i/(float)  samples;
				wavearray[i][1]=context.beatDetect->pcm->pcmdataR[i]*.04*scale+wave_x_temp;

			}
			//	  printf("%f %f\n",renderTarget->texsize*wave_y_temp,wave_y_temp);

			break;

		case DoubleLine://dual waveforms


			wave_x_temp=-2*0.4142*(fabs(fabs(mystery)-.5)-.5);

			rot = -mystery*90;
			aspectScale =1.0+wave_x_temp;


			samples = 0 ? 512-32 : context.beatDetect->pcm->numsamples;
			two_waves = true;

			double y_adj = y*y*.5;

			wave_y_temp=-1*(x-1);

			for ( int i=0;i<samples;i++)
			{
				wavearray[i][0]=i/((float)  samples);
				wavearray[i][1]= context.beatDetect->pcm->pcmdataL[i]*.04*scale+(wave_y_temp+y_adj);
			}

			for ( int i=0;i<samples;i++)
			{
				wavearray2[i][0]=i/((float)  samples);
				wavearray2[i][1]=context.beatDetect->pcm->pcmdataR[i]*.04*scale+(wave_y_temp-y_adj);
			}

			break;

	}
}
