#include "PresetFrameIO.hpp"
#include "wipemalloc.h"
#include <math.h>
#include <cassert>
#include <iostream>
#include "Renderer/BeatDetect.hpp"

PresetInputs::PresetInputs() : PipelineContext()
{
}

void PresetInputs::update(const BeatDetect & music, const PipelineContext & context) {

    // Reflect new values form the beat detection unit
    this->bass = music.bass;
    this->mid = music.mid;
    this->treb = music.treb;
    this->bass_att = music.bass_att;
    this->mid_att = music.mid_att;
    this->treb_att = music.treb_att;

    // Reflect new values from the pipeline context
    this->fps = context.fps;
    this->time = context.time;

    this->frame = context.frame;
    this->progress = context.progress;
}

void PresetInputs::Initialize ( int gx, int gy )
{
	int x, y;

	this->gx =gx;
	this->gy= gy;


	/// @bug no clue if this block belongs here
	// ***
	progress = 0;
	frame = 1;

	x_per_pixel = 0;
	y_per_pixel = 0;
	rad_per_pixel = 0;
	ang_per_pixel = 0;
	// ***

	this->x_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->x_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->y_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x <gx; x++ )
	{
		this->y_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->rad_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->rad_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->theta_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x <gx; x++ )
	{
		this->theta_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}

	this->origtheta= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->origtheta[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->origrad= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->origrad[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->origx= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->origx[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->origy= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->origy[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}

	for ( x=0;x<gx;x++ )
	{
		for ( y=0;y<gy;y++ )
		{
			this->origx[x][y]=x/ ( float ) ( gx-1 );
			this->origy[x][y]=- ( ( y/ ( float ) ( gy-1 ) )-1 );
			this->origrad[x][y]=hypot ( ( this->origx[x][y]-.5 ) *2, ( this->origy[x][y]-.5 ) *2 ) * .7071067;
			this->origtheta[x][y]=atan2 ( ( ( this->origy[x][y]-.5 ) *2 ), ( ( this->origx[x][y]-.5 ) *2 ) );
		}
	}



}

PresetOutputs::PresetOutputs() : Pipeline()
{}

PresetOutputs::~PresetOutputs()
{
	assert(this->gx > 0);

	for ( int x = 0; x < this->gx; x++ )
	{
		free(this->sx_mesh[x]);
		free(this->sy_mesh[x]);
		free(this->dy_mesh[x]);
		free(this->dx_mesh[x]);
		free(this->cy_mesh[x]);
		free(this->cx_mesh[x]);

		free(this->warp_mesh[x]);
		free(this->zoom_mesh[x]);
		free(this->zoomexp_mesh[x]);
		free(this->rot_mesh[x]);
		free(this->orig_x[x]);
		free(this->orig_y[x]);
		free(this->rad_mesh[x]);
	}

		free(this->rad_mesh);
		free(this->sx_mesh);
		free(this->sy_mesh);
		free(this->dy_mesh);
		free(this->dx_mesh);
		free(this->cy_mesh);
		free(this->cx_mesh);
		free(this->warp_mesh);
		free(this->zoom_mesh);
		free(this->zoomexp_mesh);
		free(this->rot_mesh);
		free(this->orig_x);
		free(this->orig_y);

}

void PresetOutputs::Render(const BeatDetect &music, const PipelineContext &context)
{
	PerPixelMath(context);

	drawables.clear();

	drawables.push_back(&mv);

	for (PresetOutputs::cshape_container::iterator pos = customShapes.begin();
			pos != customShapes.end(); ++pos)
			{
				if( (*pos)->enabled==1)	drawables.push_back((*pos));
			}

	for (PresetOutputs::cwave_container::iterator pos = customWaves.begin();
			pos != customWaves.end(); ++pos)
			{
				if( (*pos)->enabled==1)	drawables.push_back((*pos));
			}

    	drawables.push_back(&wave);
	if(bDarkenCenter==1) drawables.push_back(&darkenCenter);
	drawables.push_back(&border);

	compositeDrawables.clear();
	compositeDrawables.push_back(&videoEcho);

	if (bBrighten==1)
		compositeDrawables.push_back(&brighten);

	if (bDarken==1)
		compositeDrawables.push_back(&darken);

	if (bSolarize==1)
		compositeDrawables.push_back(&solarize);

	if (bInvert==1)
		compositeDrawables.push_back(&invert);
}


void PresetOutputs::PerPixelMath(const PipelineContext &context)
{

	int x, y;
	float fZoom2, fZoom2Inv;

	for (x = 0; x < gx; x++)
	{
		for (y = 0; y < gy; y++)
		{
			fZoom2 = powf(this->zoom_mesh[x][y], powf(this->zoomexp_mesh[x][y],
					rad_mesh[x][y] * 2.0f - 1.0f));
			fZoom2Inv = 1.0f / fZoom2;
			this->x_mesh[x][y] = this->orig_x[x][y] * 0.5f * fZoom2Inv + 0.5f;
			this->y_mesh[x][y] = this->orig_y[x][y] * 0.5f * fZoom2Inv + 0.5f;
		}
	}

	for (x = 0; x < gx; x++)
	{
		for (y = 0; y < gy; y++)
		{
			this->x_mesh[x][y] = (this->x_mesh[x][y] - this->cx_mesh[x][y])
					/ this->sx_mesh[x][y] + this->cx_mesh[x][y];
		}
	}

	for (x = 0; x < gx; x++)
	{
		for (y = 0; y < gy; y++)
		{
			this->y_mesh[x][y] = (this->y_mesh[x][y] - this->cy_mesh[x][y])
					/ this->sy_mesh[x][y] + this->cy_mesh[x][y];
		}
	}

	float fWarpTime = context.time * this->fWarpAnimSpeed;
	float fWarpScaleInv = 1.0f / this->fWarpScale;
	float f[4];
	f[0] = 11.68f + 4.0f * cosf(fWarpTime * 1.413f + 10);
	f[1] = 8.77f + 3.0f * cosf(fWarpTime * 1.113f + 7);
	f[2] = 10.54f + 3.0f * cosf(fWarpTime * 1.233f + 3);
	f[3] = 11.49f + 4.0f * cosf(fWarpTime * 0.933f + 5);

	for (x = 0; x < gx; x++)
	{
		for (y = 0; y < gy; y++)
		{
			this->x_mesh[x][y] += this->warp_mesh[x][y] * 0.0035f * sinf(fWarpTime * 0.333f
					+ fWarpScaleInv * (this->orig_x[x][y] * f[0] - this->orig_y[x][y] * f[3]));
			this->y_mesh[x][y] += this->warp_mesh[x][y] * 0.0035f * cosf(fWarpTime * 0.375f
					- fWarpScaleInv * (this->orig_x[x][y] * f[2] + this->orig_y[x][y] * f[1]));
			this->x_mesh[x][y] += this->warp_mesh[x][y] * 0.0035f * cosf(fWarpTime * 0.753f
					- fWarpScaleInv * (this->orig_x[x][y] * f[1] - this->orig_y[x][y] * f[2]));
			this->y_mesh[x][y] += this->warp_mesh[x][y] * 0.0035f * sinf(fWarpTime * 0.825f
					+ fWarpScaleInv * (this->orig_x[x][y] * f[0] + this->orig_y[x][y] * f[3]));
		}
	}
	for (x = 0; x < gx; x++)
	{
		for (y = 0; y < gy; y++)
		{
			float u2 = this->x_mesh[x][y] - this->cx_mesh[x][y];
			float v2 = this->y_mesh[x][y] - this->cy_mesh[x][y];

			float cos_rot = cosf(this->rot_mesh[x][y]);
			float sin_rot = sinf(this->rot_mesh[x][y]);

			this->x_mesh[x][y] = u2 * cos_rot - v2 * sin_rot + this->cx_mesh[x][y];
			this->y_mesh[x][y] = u2 * sin_rot + v2 * cos_rot + this->cy_mesh[x][y];

		}
	}

	for (x = 0; x < gx; x++)
		for (y = 0; y < gy; y++)
			this->x_mesh[x][y] -= this->dx_mesh[x][y];

	for (x = 0; x < gx; x++)
		for (y = 0; y < gy; y++)
			this->y_mesh[x][y] -= this->dy_mesh[x][y];

}


void PresetOutputs::Initialize ( int gx, int gy )
{

	assert(gx > 0);
	this->gx = gx;
	this->gy= gy;

	staticPerPixel = true;
	setStaticPerPixel(gx,gy);

	assert(this->gx > 0);
	int x;
	this->x_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->x_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->y_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->y_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->sx_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->sx_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->sy_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->sy_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->dx_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->dx_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->dy_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->dy_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->cx_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->cx_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->cy_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->cy_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->zoom_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->zoom_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->zoomexp_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->zoomexp_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->rot_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->rot_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}

	this->warp_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->warp_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->rad_mesh= ( float ** ) wipemalloc ( gx * sizeof ( float * ) );
	for ( x = 0; x < gx; x++ )
	{
		this->rad_mesh[x] = ( float * ) wipemalloc ( gy * sizeof ( float ) );
	}
	this->orig_x = (float **) wipemalloc(gx * sizeof(float *));
	for (x = 0; x < gx; x++)
	{
		this->orig_x[x] = (float *) wipemalloc(gy * sizeof(float));
	}
	this->orig_y = (float **) wipemalloc(gx * sizeof(float *));
	for (x = 0; x < gx; x++)
	{
		this->orig_y[x] = (float *) wipemalloc(gy * sizeof(float));
	}

		//initialize reference grid values
		for (x = 0; x < gx; x++)
		{
			for (int y = 0; y < gy; y++)
			{
				float origx = x / (float) (gx - 1);
				float origy = -((y / (float) (gy - 1)) - 1);

				rad_mesh[x][y]=hypot ( ( origx-.5 ) *2, ( origy-.5 ) *2 ) * .7071067;
				orig_x[x][y] = (origx - .5) * 2;
				orig_y[x][y] = (origy - .5) * 2;
			}
		}
}

PresetInputs::~PresetInputs()
{
	for ( int x = 0; x < this->gx; x++ )
	{


		free ( this->origtheta[x] );
		free ( this->origrad[x] );
		free ( this->origx[x] );
		free ( this->origy[x] );

		free ( this->x_mesh[x] );
		free ( this->y_mesh[x] );
		free ( this->rad_mesh[x] );
		free ( this->theta_mesh[x] );

	}


	free ( this->origx );
	free ( this->origy );
	free ( this->origrad );
	free ( this->origtheta );

	free ( this->x_mesh );
	free ( this->y_mesh );
	free ( this->rad_mesh );
	free ( this->theta_mesh );

	this->origx = NULL;
	this->origy = NULL;
	this->origtheta = NULL;
	this->origrad = NULL;

	this->x_mesh = NULL;
	this->y_mesh = NULL;
	this->rad_mesh = NULL;
	this->theta_mesh = NULL;
}


void PresetInputs::resetMesh()
{
	int x,y;

	assert ( x_mesh );
	assert ( y_mesh );
	assert ( rad_mesh );
	assert ( theta_mesh );

	for ( x=0;x<this->gx;x++ )
	{
		for ( y=0;y<this->gy;y++ )
		{
			x_mesh[x][y]=this->origx[x][y];
			y_mesh[x][y]=this->origy[x][y];
			rad_mesh[x][y]=this->origrad[x][y];
			theta_mesh[x][y]=this->origtheta[x][y];
		}
	}

}


#ifdef USE_MERGE_PRESET_CODE
void PresetMerger::MergePresets(PresetOutputs & A, PresetOutputs & B, double ratio, int gx, int gy)
{

double invratio = 1.0 - ratio;
  //Merge Simple Waveforms
  //
  // All the mess is because of Waveform 7, which is two lines.
  //


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

  A.screenDecay = A.screenDecay * invratio + B.screenDecay * ratio;

  A.wave.r = A.wave.r* invratio + B.wave.r*ratio;
  A.wave.g = A.wave.g* invratio + B.wave.g*ratio;
  A.wave.b = A.wave.b* invratio + B.wave.b*ratio;
  A.wave.a = A.wave.a* invratio + B.wave.a*ratio;
  A.wave.x = A.wave.x* invratio + B.wave.x*ratio;
  A.wave.y = A.wave.y* invratio + B.wave.y*ratio;
  A.wave.mystery = A.wave.mystery* invratio + B.wave.mystery*ratio;

  A.border.outer_size = A.border.outer_size* invratio + B.border.outer_size*ratio;
  A.border.outer_r = A.border.outer_r* invratio + B.border.outer_r*ratio;
  A.border.outer_g = A.border.outer_g* invratio + B.border.outer_g*ratio;
  A.border.outer_b = A.border.outer_b* invratio + B.border.outer_b*ratio;
  A.border.outer_a = A.border.outer_a* invratio + B.border.outer_a*ratio;

  A.border.inner_size = A.border.inner_size* invratio + B.border.inner_size*ratio;
  A.border.inner_r = A.border.inner_r* invratio + B.border.inner_r*ratio;
  A.border.inner_g = A.border.inner_g* invratio + B.border.inner_g*ratio;
  A.border.inner_b = A.border.inner_b* invratio + B.border.inner_b*ratio;
  A.border.inner_a = A.border.inner_a* invratio + B.border.inner_a*ratio;

  A.mv.a  = A.mv.a* invratio + B.mv.a*ratio;
  A.mv.r  = A.mv.r* invratio + B.mv.r*ratio;
  A.mv.g  = A.mv.g* invratio + B.mv.g*ratio;
  A.mv.b  = A.mv.b* invratio + B.mv.b*ratio;
  A.mv.length = A.mv.length* invratio + B.mv.length*ratio;
  A.mv.x_num = A.mv.x_num* invratio + B.mv.x_num*ratio;
  A.mv.y_num = A.mv.y_num* invratio + B.mv.y_num*ratio;
  A.mv.y_offset = A.mv.y_offset* invratio + B.mv.y_offset*ratio;
  A.mv.x_offset = A.mv.x_offset* invratio + B.mv.x_offset*ratio;


  A.fRating = A.fRating* invratio + B.fRating*ratio;
  A.fGammaAdj = A.fGammaAdj* invratio + B.fGammaAdj*ratio;
  A.videoEcho.zoom = A.videoEcho.zoom* invratio + B.videoEcho.zoom*ratio;
  A.videoEcho.a = A.videoEcho.a* invratio + B.videoEcho.a*ratio;


  A.fWarpAnimSpeed = A.fWarpAnimSpeed* invratio + B.fWarpAnimSpeed*ratio;
  A.fWarpScale = A.fWarpScale* invratio + B.fWarpScale*ratio;
  A.fShader = A.fShader* invratio + B.fShader*ratio;

  //Switch bools and discrete values halfway.  Maybe we should do some interesting stuff here.

  if (ratio > 0.5)
    {
      A.videoEcho.orientation = B.videoEcho.orientation;
      A.textureWrap = B.textureWrap;
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
#endif
