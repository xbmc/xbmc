#include "PresetMerge.hpp"

#include <iostream>

void PresetMerger::MergePresets(PresetOutputs & A, PresetOutputs & B, double ratio, int gx, int gy)
{  

double invratio = 1.0 - ratio;
  //Merge Simple Waveforms
  //
  // All the mess is because of Waveform 7, which is two lines. 
  //
  A.wave_rot = A.wave_rot* invratio + B.wave_rot*ratio;
  A.wave_scale = A.wave_scale* invratio + B.wave_scale*ratio;
  
  if (!B.draw_wave_as_loop) A.draw_wave_as_loop = false; 

  if (A.two_waves && B.two_waves)
    {
      for (int x = 0; x<A.wave_samples;x++)
	{
	  A.wavearray[x][0] = A.wavearray[x][0]* invratio + B.wavearray[x][0]*ratio;
	  A.wavearray[x][1] = A.wavearray[x][1]* invratio + B.wavearray[x][1]*ratio;
	  A.wavearray2[x][0] = A.wavearray2[x][0]* invratio + B.wavearray2[x][0]*ratio;
	  A.wavearray2[x][1] = A.wavearray2[x][1]* invratio + B.wavearray2[x][1]*ratio;
	}
    }
  else if (A.two_waves)
    {
      for (int x = 0; x<A.wave_samples;x++)
	{
	  A.wavearray[x][0] = A.wavearray[x][0]* invratio + B.wavearray[x][0]*ratio;
	  A.wavearray[x][1] = A.wavearray[x][1]* invratio + B.wavearray[x][1]*ratio;
	  A.wavearray2[x][0] = A.wavearray2[x][0]* invratio + B.wavearray[x][0]*ratio;
	  A.wavearray2[x][1] = A.wavearray2[x][1]* invratio + B.wavearray[x][1]*ratio;
	}
    }
  else if (B.two_waves)
    {
      A.two_waves=true;
      for (int x = 0; x<A.wave_samples;x++)
	{
	  A.wavearray[x][0] = A.wavearray[x][0]* invratio + B.wavearray[x][0]*ratio;
	  A.wavearray[x][1] = A.wavearray[x][1]* invratio + B.wavearray[x][1]*ratio;
	  A.wavearray2[x][0] = A.wavearray[x][0]* invratio + B.wavearray[x][0]*ratio;
	  A.wavearray2[x][1] = A.wavearray[x][1]* invratio + B.wavearray[x][1]*ratio;
	}
    }
  else
    {
      for (int x = 0; x<A.wave_samples;x++)
	{
	  A.wavearray[x][0] = A.wavearray[x][0]* invratio + B.wavearray[x][0]*ratio;
	  A.wavearray[x][1] = A.wavearray[x][1]* invratio + B.wavearray[x][1]*ratio;
	}
    }
 
  //Merge Custom Shapes and Custom Waves 

  for (PresetOutputs::cshape_container::iterator pos = A.customShapes.begin();
	pos != A.customShapes.end(); ++pos) 
    {
       (*pos)->a *= invratio;
       (*pos)->a2 *= invratio;
       (*pos)->border_a *= invratio;
    }
  
  for (PresetOutputs::cshape_container::iterator pos = B.customShapes.begin();
	pos != B.customShapes.end(); ++pos) 
    {
       (*pos)->a *= ratio;
       (*pos)->a2 *= ratio;
       (*pos)->border_a *= ratio;

        A.customShapes.push_back(*pos);

    }
 for (PresetOutputs::cwave_container::iterator pos = A.customWaves.begin();
	pos != A.customWaves.end(); ++pos) 
    {
       (*pos)->a *= invratio;
      for (int x=0; x <   (*pos)->samples; x++)
	{
	   (*pos)->a_mesh[x]= (*pos)->a_mesh[x]*invratio;
	}
    }

  for (PresetOutputs::cwave_container::iterator pos = B.customWaves.begin();
	pos != B.customWaves.end(); ++pos) 
    {
       (*pos)->a *= ratio;        
      for (int x=0; x < (*pos)->samples; x++)
	{
	   (*pos)->a_mesh[x]= (*pos)->a_mesh[x]*ratio;
	}
       A.customWaves.push_back(*pos);
    }


  //Interpolate Per-Pixel mesh

  for (int x=0;x<gx;x++)
    {
      for(int y=0;y<gy;y++)
	{
	  A.x_mesh[x][y]  = A.x_mesh[x][y]* invratio + B.x_mesh[x][y]*ratio;
	}
    }
 for (int x=0;x<gx;x++)
    {
      for(int y=0;y<gy;y++)
	{
	  A.y_mesh[x][y]  = A.y_mesh[x][y]* invratio + B.y_mesh[x][y]*ratio;
	}
    }


 
 //Interpolate PerFrame floats

  A.decay = A.decay * invratio + B.decay * ratio;
  
  A.wave_r = A.wave_r* invratio + B.wave_r*ratio;
  A.wave_g = A.wave_g* invratio + B.wave_g*ratio;
  A.wave_b = A.wave_b* invratio + B.wave_b*ratio;
  A.wave_o = A.wave_o* invratio + B.wave_o*ratio;
  A.wave_x = A.wave_x* invratio + B.wave_x*ratio;
  A.wave_y = A.wave_y* invratio + B.wave_y*ratio;
  A.wave_mystery = A.wave_mystery* invratio + B.wave_mystery*ratio;
  
  A.ob_size = A.ob_size* invratio + B.ob_size*ratio;
  A.ob_r = A.ob_r* invratio + B.ob_r*ratio;
  A.ob_g = A.ob_g* invratio + B.ob_g*ratio;
  A.ob_b = A.ob_b* invratio + B.ob_b*ratio;
  A.ob_a = A.ob_a* invratio + B.ob_a*ratio;
  
  A.ib_size = A.ib_size* invratio + B.ib_size*ratio;
  A.ib_r = A.ib_r* invratio + B.ib_r*ratio;
  A.ib_g = A.ib_g* invratio + B.ib_g*ratio;
  A.ib_b = A.ib_b* invratio + B.ib_b*ratio;
  A.ib_a = A.ib_a* invratio + B.ib_a*ratio;
  
  A.mv_a  = A.mv_a* invratio + B.mv_a*ratio;
  A.mv_r  = A.mv_r* invratio + B.mv_r*ratio;
  A.mv_g  = A.mv_g* invratio + B.mv_g*ratio;
  A.mv_b  = A.mv_b* invratio + B.mv_b*ratio;
  A.mv_l = A.mv_l* invratio + B.mv_l*ratio;
  A.mv_x = A.mv_x* invratio + B.mv_x*ratio;
  A.mv_y = A.mv_y* invratio + B.mv_y*ratio;
  A.mv_dy = A.mv_dy* invratio + B.mv_dy*ratio;
  A.mv_dx = A.mv_dx* invratio + B.mv_dx*ratio;

  
  A.fRating = A.fRating* invratio + B.fRating*ratio;
  A.fGammaAdj = A.fGammaAdj* invratio + B.fGammaAdj*ratio;
  A.fVideoEchoZoom = A.fVideoEchoZoom* invratio + B.fVideoEchoZoom*ratio;
  A.fVideoEchoAlpha = A.fVideoEchoAlpha* invratio + B.fVideoEchoAlpha*ratio;
 
  A.fWaveAlpha = A.fWaveAlpha* invratio + B.fWaveAlpha*ratio;
  A.fWaveScale = A.fWaveScale* invratio + B.fWaveScale*ratio;
  A.fWaveSmoothing = A.fWaveSmoothing* invratio + B.fWaveSmoothing*ratio;
  A.fWaveParam = A.fWaveParam* invratio + B.fWaveParam*ratio;
  A.fModWaveAlphaStart = A.fModWaveAlphaStart* invratio + B.fModWaveAlphaStart*ratio;
  A.fModWaveAlphaEnd = A.fModWaveAlphaEnd * invratio + B.fModWaveAlphaEnd *ratio;
  A.fWarpAnimSpeed = A.fWarpAnimSpeed* invratio + B.fWarpAnimSpeed*ratio;
  A.fWarpScale = A.fWarpScale* invratio + B.fWarpScale*ratio;
  A.fShader = A.fShader* invratio + B.fShader*ratio;   

  //Switch bools and discrete values halfway.  Maybe we should do some interesting stuff here.
 
  if (ratio > 0.5)
    {
      A.nVideoEchoOrientation = B.nVideoEchoOrientation;
      A.nWaveMode = B.nWaveMode;
      A.bAdditiveWaves = B.bAdditiveWaves;
      A.bWaveDots = B.bWaveDots;
      A.bWaveThick = B.bWaveThick;
      A.bModWaveAlphaByVolume = B.bModWaveAlphaByVolume;
      A.bMaximizeWaveColor = B.bMaximizeWaveColor;
      A.bTexWrap = B.bTexWrap;
      A.bDarkenCenter = B.bDarkenCenter;
      A.bRedBlueStereo = B.bRedBlueStereo;
      A.bBrighten = B.bBrighten;
      A.bDarken = B.bDarken;
      A.bSolarize = B.bSolarize;
      A.bInvert = B.bInvert;
      A.bMotionVectorsOn = B.bMotionVectorsOn;
    }   

  return;
}

