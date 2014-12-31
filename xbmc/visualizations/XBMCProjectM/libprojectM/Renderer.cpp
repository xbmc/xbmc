#include "Renderer.hpp"
#include "wipemalloc.h"
#include "math.h"
#include "Common.hpp"
#include "CustomShape.hpp"
#include "CustomWave.hpp"
#include "KeyHandler.hpp"
#include "TextureManager.hpp"
#include <iostream>
#include <cassert>
#include "SectionLock.h"

class Preset;

Renderer::Renderer(int width, int height, int gx, int gy, int texsize, BeatDetect *beatDetect, std::string _presetURL, std::string _titlefontURL, std::string _menufontURL, int xpos, int ypos, bool usefbo): title_fontURL(_titlefontURL), menu_fontURL(_menufontURL), presetURL(_presetURL), m_presetName("None"), vw(width), vh(height), gx(gx), gy(gy), texsize(texsize), vx(xpos), vy(ypos), useFBO(usefbo)
{
	int x; int y;
	
	//  this->gx=gx;
	//  this->gy=gy;
#ifdef _USE_THREADS
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_renderer_lock, &attr);
    pthread_mutexattr_destroy(&attr);
#endif

	
	this->totalframes = 1;
	this->noSwitch = false;
	this->showfps = false;
	this->showtitle = false;
	this->showpreset = false;
	this->showhelp = false;
	this->showstats = false;
	this->studio = false;
	this->realfps=0;
	
	this->drawtitle=0;
	
	this->title = "Unknown";
	
	/** Other stuff... */
	this->correction = true;
	this->aspect=1.33333333;
	
	this->gridx=(float **)wipemalloc(gx * sizeof(float *));
	for(x = 0; x < gx; x++)
	{
		this->gridx[x] = (float *)wipemalloc(gy * sizeof(float));
	}
	this->gridy=(float **)wipemalloc(gx * sizeof(float *));
	for(x = 0; x < gx; x++)
	{
		this->gridy[x] = (float *)wipemalloc(gy * sizeof(float));
	}
	
	this->origx2=(float **)wipemalloc(gx * sizeof(float *));
	for(x = 0; x < gx; x++)
	{
		this->origx2[x] = (float *)wipemalloc(gy * sizeof(float));
	}
	this->origy2=(float **)wipemalloc(gx * sizeof(float *));
	for(x = 0; x < gx; x++)
	{
		this->origy2[x] = (float *)wipemalloc(gy * sizeof(float));
	}
	
	//initialize reference grid values
	for (x=0;x<gx;x++)
	{
		for(y=0;y<gy;y++)
		{
			
			float origx=x/(float)(gx-1);
			float origy=-((y/(float)(gy-1))-1);
			this->gridx[x][y]=origx;
			this->gridy[x][y]=origy;
			this->origx2[x][y]=( origx-.5)*2;
			this->origy2[x][y]=( origy-.5)*2;
			
		}
	}
	
	/// @bug put these on member init list
	this->renderTarget = new RenderTarget( texsize, width, height, useFBO );
	this->textureManager = new TextureManager(presetURL);
	this->beatDetect = beatDetect;
	
	
#ifdef USE_FTGL
	/**f Load the standard fonts */
	
	title_font = new FTGLPixmapFont(title_fontURL.c_str());
	other_font = new FTGLPixmapFont(menu_fontURL.c_str());
	other_font->UseDisplayList(true);
	title_font->UseDisplayList(true);
	
	
	poly_font = new FTGLExtrdFont(title_fontURL.c_str());
	
	poly_font->UseDisplayList(true);
	poly_font->Depth(20);
	poly_font->FaceSize(72);
	
	poly_font->UseDisplayList(true);
	
#endif /** USE_FTGL */
	
	
}

void Renderer::ResetTextures()
{
        CSectionLock lock(&_renderer_lock);
	textureManager->Clear();
	
	delete(renderTarget);
	renderTarget = new RenderTarget(texsize, vw, vh, useFBO);
	reset(vw, vh);
	
	textureManager->Preload();
}

void Renderer::RenderFrame(PresetOutputs *presetOutputs, PresetInputs *presetInputs)
{
	/** Save original view state */
	// TODO: check there is sufficient room on the stack
        CSectionLock lock(&_renderer_lock);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	totalframes++;		
	
	//BEGIN PASS 1
	//
	//This pass is used to render our texture
	//the texture is drawn to a FBO or a subsection of the framebuffer
	//and then we perform our manipulations on it in pass 2 we
	//will copy the image into texture memory and render the final image
	
	
	//Lock FBO
	renderTarget->lock();
	
	glViewport( 0, 0, renderTarget->texsize, renderTarget->texsize );
	
	glEnable( GL_TEXTURE_2D );
	
	//If using FBO, sitch to FBO texture
	if(this->renderTarget->useFBO)
	{
		glBindTexture( GL_TEXTURE_2D, renderTarget->textureID[1] );
	}
	else
	{
		glBindTexture( GL_TEXTURE_2D, renderTarget->textureID[0] );
	}
	
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	
	
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
#ifdef USE_GLES1
	glOrthof(0.0, 1, 0.0, 1, -40, 40);
#else
	glOrtho(0.0, 1, 0.0, 1, -40, 40);
#endif
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
		
	
	if(this->renderTarget->useFBO)
	{
		//draw_motion_vectors();        //draw motion vectors
		//unlockPBuffer( this->renderTarget);
		//lockPBuffer( this->renderTarget, PBUFFER_PASS1 );
	}
	
	Interpolation(presetOutputs, presetInputs);
	
	//    if(!this->renderTarget->useFBO)
	{
		draw_motion_vectors(presetOutputs);
	}
	
	draw_shapes(presetOutputs);
	draw_custom_waves(presetOutputs);
	draw_waveform(presetOutputs);
	if(presetOutputs->bDarkenCenter)darken_center();
	draw_borders(presetOutputs);
	draw_title_to_texture();
	/** Restore original view state */
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
	
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	
	renderTarget->unlock();
	
	
#ifdef DEBUG
	GLint msd = 0, psd = 0;
	glGetIntegerv( GL_MODELVIEW_STACK_DEPTH, &msd );
	glGetIntegerv( GL_PROJECTION_STACK_DEPTH, &psd );
	DWRITE( "end pass1: modelview matrix depth: %d\tprojection matrix depth: %d\n", msd, psd );
	DWRITE( "begin pass2\n" );
#endif
	
	//BEGIN PASS 2
	//
	//end of texture rendering
	//now we copy the texture from the FBO or framebuffer to
	//video texture memory and render fullscreen.
	
	/** Reset the viewport size */
#ifdef USE_FBO
	if(renderTarget->renderToTexture)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, this->renderTarget->fbuffer[1]);
		glViewport( 0, 0, this->renderTarget->texsize, this->renderTarget->texsize );
	}
	else 
#endif
		glViewport( viewport[0], viewport[1], viewport[2], viewport[3] );
	
	
	
	glBindTexture( GL_TEXTURE_2D, this->renderTarget->textureID[0] );
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
#ifdef USE_GLES1
	glOrthof(-0.5, 0.5, -0.5, 0.5, -40, 40);	
#else
	glOrtho(-0.5, 0.5, -0.5, 0.5, -40, 40);	
#endif
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glLineWidth( this->renderTarget->texsize < 512 ? 1 : this->renderTarget->texsize/512.0);
	if(this->studio%2)render_texture_to_studio(presetOutputs, presetInputs);
	else render_texture_to_screen(presetOutputs);
	
	
	glMatrixMode(GL_MODELVIEW);
	glTranslatef(-0.5, -0.5, 0);
	
	// When console refreshes, there is a chance the preset has been changed by the user
	refreshConsole();
	draw_title_to_screen(false);
	if(this->showhelp%2) draw_help();
	if(this->showtitle%2) draw_title();
	if(this->showfps%2) draw_fps(this->realfps);
	if(this->showpreset%2) draw_preset();
	if(this->showstats%2) draw_stats(presetInputs);
	glTranslatef(0.5 , 0.5, 0);
	
#ifdef USE_FBO
	if(renderTarget->renderToTexture)
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#endif
}


void Renderer::Interpolation(PresetOutputs *presetOutputs, PresetInputs *presetInputs)
{
        CSectionLock lock(&_renderer_lock);
	//Texture wrapping( clamp vs. wrap)
	if (presetOutputs->bTexWrap==0)
	{
#ifdef USE_GLES1
	  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#endif
	}
	else
	{ glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);}
	
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	
	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
	
	glColor4f(1.0, 1.0, 1.0, presetOutputs->decay);
	
	glEnable(GL_TEXTURE_2D);
	
	int size = presetInputs->gy;

#ifdef _WIN32PC
  // guessed value. solved in current projectM svn
  float p[30*2][2];
  float t[30*2][2];
#else
	float p[size*2][2];      
	float t[size*2][2];
#endif

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2,GL_FLOAT,0,p);
	glTexCoordPointer(2,GL_FLOAT,0,t);
    	
	for (int x=0;x<presetInputs->gx - 1;x++)
	{	       
		for(int y=0;y<presetInputs->gy;y++)
		{		
		  t[y*2][0] = presetOutputs->x_mesh[x][y];
		  t[y*2][1] = presetOutputs->y_mesh[x][y];

		  p[y*2][0] = this->gridx[x][y];
		  p[y*2][1] = this->gridy[x][y];			    

		  t[(y*2)+1][0] = presetOutputs->x_mesh[x+1][y];
		  t[(y*2)+1][1] = presetOutputs->y_mesh[x+1][y];

		  p[(y*2)+1][0] = this->gridx[x+1][y];
		  p[(y*2)+1][1] = this->gridy[x+1][y];			    

		}
	       	glDrawArrays(GL_TRIANGLE_STRIP,0,size*2);
	}



	glDisable(GL_TEXTURE_2D);
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
}


Renderer::~Renderer()
{
	
	int x;
	
	
	if (renderTarget)
		delete(renderTarget);
	if (textureManager)
		delete(textureManager);
	
	assert(gx > 0);
	for(x = 0; x < this->gx; x++)
	{
		free(this->gridx[x]);
		free(this->gridy[x]);
		free(this->origx2[x]);
		free(this->origy2[x]);
	}
	
	
	//std::cerr << "freeing grids" << std::endl;
	free(this->origx2);
	free(this->origy2);
	free(this->gridx);
	free(this->gridy);
	
//std:cerr << "grid assign begin " << std::endl;
	this->origx2 = NULL;
	this->origy2 = NULL;
	this->gridx = NULL;
	this->gridy = NULL;
	
//std::cerr << "grid assign end" << std::endl;
	
#ifdef USE_FTGL
//	std::cerr << "freeing title fonts" << std::endl;
	if (title_font)
		delete title_font;
	if (poly_font)
		delete poly_font;
	if (other_font)
		delete other_font;
//	std::cerr << "freeing title fonts finished" << std::endl;
#endif
//	std::cerr << "exiting destructor" << std::endl;
#ifdef _USE_THREADS
        pthread_mutex_destroy(&_renderer_lock);
#endif
}


void Renderer::PerPixelMath(PresetOutputs * presetOutputs, PresetInputs * presetInputs)
{
	
	int x, y;
	float fZoom2, fZoom2Inv;
        CSectionLock lock(&_renderer_lock);
	
	
	for (x=0;x<this->gx;x++)
	{
		for(y=0;y<this->gy;y++)
		{
			fZoom2 = powf( presetOutputs->zoom_mesh[x][y], powf( presetOutputs->zoomexp_mesh[x][y], presetInputs->rad_mesh[x][y]*2.0f - 1.0f));
			fZoom2Inv = 1.0f/fZoom2;
			presetOutputs->x_mesh[x][y]= this->origx2[x][y]*0.5f*fZoom2Inv + 0.5f;
			presetOutputs->y_mesh[x][y]= this->origy2[x][y]*0.5f*fZoom2Inv + 0.5f;
		}
	}
	
	for (x=0;x<this->gx;x++)
	{
		for(y=0;y<this->gy;y++)
		{
			presetOutputs->x_mesh[x][y]  = ( presetOutputs->x_mesh[x][y] - presetOutputs->cx_mesh[x][y])/presetOutputs->sx_mesh[x][y] + presetOutputs->cx_mesh[x][y];
		}
	}
	
	for (x=0;x<this->gx;x++)
	{
		for(y=0;y<this->gy;y++)
		{
			presetOutputs->y_mesh[x][y] = ( presetOutputs->y_mesh[x][y] - presetOutputs->cy_mesh[x][y])/presetOutputs->sy_mesh[x][y] + presetOutputs->cy_mesh[x][y];
		}
	}
	
	float fWarpTime = presetInputs->time * presetOutputs->fWarpAnimSpeed;
	float fWarpScaleInv = 1.0f / presetOutputs->fWarpScale;
	float f[4];
	f[0] = 11.68f + 4.0f*cosf(fWarpTime*1.413f + 10);
	f[1] =  8.77f + 3.0f*cosf(fWarpTime*1.113f + 7);
	f[2] = 10.54f + 3.0f*cosf(fWarpTime*1.233f + 3);
	f[3] = 11.49f + 4.0f*cosf(fWarpTime*0.933f + 5);
	
	for (x=0;x<this->gx;x++)
	{
		for(y=0;y<this->gy;y++)
		{
			presetOutputs->x_mesh[x][y] += presetOutputs->warp_mesh[x][y]*0.0035f*sinf(fWarpTime*0.333f + fWarpScaleInv*(this->origx2[x][y]*f[0] - this->origy2[x][y]*f[3]));
			presetOutputs->y_mesh[x][y] += presetOutputs->warp_mesh[x][y]*0.0035f*cosf(fWarpTime*0.375f - fWarpScaleInv*(this->origx2[x][y]*f[2] + this->origy2[x][y]*f[1]));
			presetOutputs->x_mesh[x][y] += presetOutputs->warp_mesh[x][y]*0.0035f*cosf(fWarpTime*0.753f - fWarpScaleInv*(this->origx2[x][y]*f[1] - this->origy2[x][y]*f[2]));
			presetOutputs->y_mesh[x][y] += presetOutputs->warp_mesh[x][y]*0.0035f*sinf(fWarpTime*0.825f + fWarpScaleInv*(this->origx2[x][y]*f[0] + this->origy2[x][y]*f[3]));
		}
	}
	for (x=0;x<this->gx;x++)
	{
		for(y=0;y<this->gy;y++)
		{
			float u2 = presetOutputs->x_mesh[x][y] - presetOutputs->cx_mesh[x][y];
			float v2 = presetOutputs->y_mesh[x][y] - presetOutputs->cy_mesh[x][y];
			
			float cos_rot = cosf(presetOutputs->rot_mesh[x][y]);
			float sin_rot = sinf(presetOutputs->rot_mesh[x][y]);
			
			presetOutputs->x_mesh[x][y] = u2*cos_rot - v2*sin_rot + presetOutputs->cx_mesh[x][y];
			presetOutputs->y_mesh[x][y] = u2*sin_rot + v2*cos_rot + presetOutputs->cy_mesh[x][y];
			
		}
	}
	
	
	for (x=0;x<this->gx;x++)
	{
		for(y=0;y<this->gy;y++)
		{
			presetOutputs->x_mesh[x][y] -= presetOutputs->dx_mesh[x][y];
		}
	}
	
	
	
	for (x=0;x<this->gx;x++)
	{
		for(y=0;y<this->gy;y++)
		{
			presetOutputs->y_mesh[x][y] -= presetOutputs->dy_mesh[x][y];
		}
	}
	
}



void Renderer::reset(int w, int h)
{
        CSectionLock lock(&_renderer_lock);
	this->aspect=(float)h / (float)w;
	this -> vw = w;
	this -> vh = h;
	
	glShadeModel( GL_SMOOTH);
	
	glCullFace( GL_BACK );
	//glFrontFace( GL_CCW );
	
	glClearColor( 0, 0, 0, 0 );
	
	glViewport( 0, 0, w, h );
	
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
#ifndef USE_GLES1
	glDrawBuffer(GL_BACK);
	glReadBuffer(GL_BACK);
#endif
	glEnable(GL_BLEND);
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glEnable( GL_LINE_SMOOTH );
	
	
//	glEnable(GL_POINT_SMOOTH);
	glClear(GL_COLOR_BUFFER_BIT);
	
#ifndef USE_GLES1
	glLineStipple(2, 0xAAAA);       
#endif

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_MODULATE);
	
	//glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	//glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	
	if (!this->renderTarget->useFBO)
	{
		this->renderTarget->fallbackRescale(w, h);
	}
}


void Renderer::draw_custom_waves(PresetOutputs *presetOutputs)
{
	
	int x;
	
        CSectionLock lock(&_renderer_lock);
	
	glPointSize(this->renderTarget->texsize < 512 ? 1 : this->renderTarget->texsize/512);
	
	for (PresetOutputs::cwave_container::const_iterator pos = presetOutputs->customWaves.begin();
	pos != presetOutputs->customWaves.end(); ++pos)
	{
		
		if( (*pos)->enabled==1)
		{
			
			if ( (*pos)->bAdditive==0)  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			if ( (*pos)->bDrawThick==1)
			{ glLineWidth(this->renderTarget->texsize < 512 ? 1 : 2*this->renderTarget->texsize/512);
			  glPointSize(this->renderTarget->texsize < 512 ? 1 : 2*this->renderTarget->texsize/512);
			  
			}
			beatDetect->pcm->getPCM( (*pos)->value1, (*pos)->samples, 0, (*pos)->bSpectrum, (*pos)->smoothing, 0);
			beatDetect->pcm->getPCM( (*pos)->value2, (*pos)->samples, 1, (*pos)->bSpectrum, (*pos)->smoothing, 0);
			// printf("%f\n",pcmL[0]);
			
			
			float mult= (*pos)->scaling*presetOutputs->fWaveScale*( (*pos)->bSpectrum ? 0.015f :1.0f);
			
			for(x=0;x< (*pos)->samples;x++)
			{ (*pos)->value1[x]*=mult;}
			
			for(x=0;x< (*pos)->samples;x++)
			{ (*pos)->value2[x]*=mult;}
			
			for(x=0;x< (*pos)->samples;x++)
			{ (*pos)->sample_mesh[x]=((float)x)/((float)( (*pos)->samples-1));}
			
			// printf("mid inner loop\n");
			(*pos)->evalPerPointEqns();
			
#ifdef _WIN32PC
      char buf[1024];
      sprintf(buf, "%s: samples = %d\n", __FUNCTION__,(*pos)->samples);
      OutputDebugString( buf );
      float colors[1024][4];
			float points[1024][2];
#else
			float colors[(*pos)->samples][4];
			float points[(*pos)->samples][2];
#endif

			for(x=0;x< (*pos)->samples;x++)
			{
			  colors[x][0] = (*pos)->r_mesh[x];
			  colors[x][1] = (*pos)->g_mesh[x];
			  colors[x][2] = (*pos)->b_mesh[x];			  
			  colors[x][3] = (*pos)->a_mesh[x];			     

			  points[x][0] = (*pos)->x_mesh[x];
			  points[x][1] =  -( (*pos)->y_mesh[x]-1);
		       
			}
			
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);	 
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

			glVertexPointer(2,GL_FLOAT,0,points);
			glColorPointer(4,GL_FLOAT,0,colors);
		     		       
	
			if ( (*pos)->bUseDots==1)
		       	glDrawArrays(GL_POINTS,0,(*pos)->samples);
			else  	glDrawArrays(GL_LINE_STRIP,0,(*pos)->samples);
			
			glPointSize(this->renderTarget->texsize < 512 ? 1 : this->renderTarget->texsize/512);
			glLineWidth(this->renderTarget->texsize < 512 ? 1 : this->renderTarget->texsize/512);
#ifndef USE_GLES1
			glDisable(GL_LINE_STIPPLE);
#endif
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			//  glPopMatrix();
			
		}
	}
	
	
}

void Renderer::draw_shapes(PresetOutputs *presetOutputs)
{
	
	
        CSectionLock lock(&_renderer_lock);
	float radius;
	float xval, yval;
	float t;
	
	float aspect=this->aspect;	
	
	for (PresetOutputs::cshape_container::const_iterator pos = presetOutputs->customShapes.begin();
	pos != presetOutputs->customShapes.end(); ++pos)
	{
		
		if( (*pos)->enabled==1)
		{
			
			// printf("drawing shape %f\n", (*pos)->ang);
			(*pos)->y=-(( (*pos)->y)-1);
			radius=.5;
			(*pos)->radius= (*pos)->radius*(.707*.707*.707*1.04);
			//Additive Drawing or Overwrite
			if ( (*pos)->additive==0)  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else    glBlendFunc(GL_SRC_ALPHA, GL_ONE);			
			
			xval= (*pos)->x;
			yval= (*pos)->y;
									
			if ( (*pos)->textured)
			{
				
				if ((*pos)->getImageUrl() !="")
				{
					GLuint tex = textureManager->getTexture((*pos)->getImageUrl());
					if (tex != 0)
					{
						glBindTexture(GL_TEXTURE_2D, tex);
						aspect=1.0;
					}
				}


				
				glMatrixMode(GL_TEXTURE);
				glPushMatrix();
				glLoadIdentity();
				
				glEnable(GL_TEXTURE_2D);
					
				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_COLOR_ARRAY);	 
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);

#ifdef _WIN32PC
        char buf[1024];
        sprintf(buf, "%s: sides = %d\n", __FUNCTION__,(*pos)->sides);
        OutputDebugString( buf );
        float colors[1024][4];
				float tex[1024][2];
				float points[1024][2];
#else
				float colors[(*pos)->sides+2][4];
				float tex[(*pos)->sides+2][2];
				float points[(*pos)->sides+2][2];			
#endif
										
				//Define the center point of the shape
				colors[0][0] = (*pos)->r;
				colors[0][1] = (*pos)->g;
				colors[0][2] = (*pos)->b;
				colors[0][3] = (*pos)->a;
			  	   tex[0][0] = 0.5;
				   tex[0][1] = 0.5;
				points[0][0] = xval;
				points[0][1] = yval;     							     
				
				for ( int i=1;i< (*pos)->sides+2;i++)
				{
				  colors[i][0]= (*pos)->r2;
				  colors[i][1]=(*pos)->g2;
				  colors[i][2]=(*pos)->b2;
				  colors[i][3]=(*pos)->a2;

				  t = (i-1)/(float) (*pos)->sides;
				  tex[i][0] =0.5f + 0.5f*cosf(t*3.1415927f*2 +  (*pos)->tex_ang + 3.1415927f*0.25f)*(this->correction ? aspect : 1.0)/ (*pos)->tex_zoom;
				  tex[i][1] =  0.5f + 0.5f*sinf(t*3.1415927f*2 +  (*pos)->tex_ang + 3.1415927f*0.25f)/ (*pos)->tex_zoom;
				  points[i][0]=(*pos)->radius*cosf(t*3.1415927f*2 +  (*pos)->ang + 3.1415927f*0.25f)*(this->correction ? aspect : 1.0)+xval;
				  points[i][1]=(*pos)->radius*sinf(t*3.1415927f*2 +  (*pos)->ang + 3.1415927f*0.25f)+yval;
				  
										
				
				}
					
				glVertexPointer(2,GL_FLOAT,0,points);
				glColorPointer(4,GL_FLOAT,0,colors);
				glTexCoordPointer(2,GL_FLOAT,0,tex);

				glDrawArrays(GL_TRIANGLE_FAN,0,(*pos)->sides+2);
				
				glDisable(GL_TEXTURE_2D);
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);
				
				//Reset Texture state since we might have changed it
				if(this->renderTarget->useFBO)
				{
					glBindTexture( GL_TEXTURE_2D, renderTarget->textureID[1] );
				}
				else
				{
					glBindTexture( GL_TEXTURE_2D, renderTarget->textureID[0] );
				}
				
				
			}
			else
			{//Untextured (use color values)
				

			  glEnableClientState(GL_VERTEX_ARRAY);
			  glEnableClientState(GL_COLOR_ARRAY);	 
			  glDisableClientState(GL_TEXTURE_COORD_ARRAY);

#ifdef _WIN32PC
        char buf[1024];
        sprintf(buf, "%s: sides = %d\n", __FUNCTION__,(*pos)->sides+2);
        OutputDebugString( buf );
        float colors[1024][4];				
			  float points[1024][2];
#else
			  float colors[(*pos)->sides+2][4];				
			  float points[(*pos)->sides+2][2];			
#endif
			  
			  //Define the center point of the shape
			  colors[0][0]=(*pos)->r;
			  colors[0][1]=(*pos)->g;
			  colors[0][2]=(*pos)->b;
			  colors[0][3]=(*pos)->a;			       
			  points[0][0]=xval;
			  points[0][1]=yval;
			  
			  
			  
			  for ( int i=1;i< (*pos)->sides+2;i++)
			    {
			      colors[i][0]=(*pos)->r2;
			      colors[i][1]=(*pos)->g2;
			      colors[i][2]=(*pos)->b2;
			      colors[i][3]=(*pos)->a2;
			      t = (i-1)/(float) (*pos)->sides;
			      points[i][0]=(*pos)->radius*cosf(t*3.1415927f*2 +  (*pos)->ang + 3.1415927f*0.25f)*(this->correction ? aspect : 1.0)+xval;
			      points[i][1]=(*pos)->radius*sinf(t*3.1415927f*2 +  (*pos)->ang + 3.1415927f*0.25f)+yval;
			     			      			      
			    }
					
			  glVertexPointer(2,GL_FLOAT,0,points);
			  glColorPointer(4,GL_FLOAT,0,colors);
			  
			  
			  glDrawArrays(GL_TRIANGLE_FAN,0,(*pos)->sides+2);
			  //draw first n-1 triangular pieces
			  			  				
			}
			if (presetOutputs->bWaveThick==1)  glLineWidth(this->renderTarget->texsize < 512 ? 1 : 2*this->renderTarget->texsize/512);

			glEnableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_COLOR_ARRAY);	 

#ifdef _WIN32PC
      char buf[1024];
      sprintf(buf, "%s: sides = %d\n", __FUNCTION__,(*pos)->sides+1);
      OutputDebugString( buf );
      float points[1024][2];
#else
			float points[(*pos)->sides+1][2];
#endif

			glColor4f( (*pos)->border_r, (*pos)->border_g, (*pos)->border_b, (*pos)->border_a);
			
			for ( int i=0;i< (*pos)->sides;i++)
			{
				t = (i-1)/(float) (*pos)->sides;
				points[i][0]= (*pos)->radius*cosf(t*3.1415927f*2 +  (*pos)->ang + 3.1415927f*0.25f)*(this->correction ? aspect : 1.0)+xval;
				points[i][1]=  (*pos)->radius*sinf(t*3.1415927f*2 +  (*pos)->ang + 3.1415927f*0.25f)+yval;
								
			}

			glVertexPointer(2,GL_FLOAT,0,points);			 			  			  
			glDrawArrays(GL_LINE_LOOP,0,(*pos)->sides);
		      
			if (presetOutputs->bWaveThick==1)  glLineWidth(this->renderTarget->texsize < 512 ? 1 : this->renderTarget->texsize/512);
			
			
		}
	}
	
	
}

void Renderer::WaveformMath(PresetOutputs *presetOutputs, PresetInputs *presetInputs, bool isSmoothing)
{
	
	int x;
	
	float r, theta;
	
	float offset, scale;
	
	float wave_x_temp=0;
	float wave_y_temp=0;
	
	float cos_rot;
	float sin_rot;
        CSectionLock lock(&_renderer_lock);
	
	offset=presetOutputs->wave_x-.5;
	scale=505.0/512.0;
	
	presetOutputs->two_waves = false;
	presetOutputs->draw_wave_as_loop = false;

	switch(presetOutputs->nWaveMode)
	{
		
		case 0:
		  {
 		        presetOutputs->draw_wave_as_loop = true;
			presetOutputs->wave_rot =   0;
			presetOutputs->wave_scale =1.0;
			presetOutputs->wave_y=-1*(presetOutputs->wave_y-1.0);
			
			
			presetOutputs->wave_samples = isSmoothing ? 512-32 : beatDetect->pcm->numsamples;

			float inv_nverts_minus_one = 1.0f/(float)(presetOutputs->wave_samples);

	float last_value = beatDetect->pcm->pcmdataR[presetOutputs->wave_samples-1]+beatDetect->pcm->pcmdataL[presetOutputs->wave_samples-1];
			float first_value = beatDetect->pcm->pcmdataR[0]+beatDetect->pcm->pcmdataL[0];
			float offset = first_value-last_value;
		
			for ( x=0;x<presetOutputs->wave_samples;x++)
			{ 
			

			  float value = beatDetect->pcm->pcmdataR[x]+beatDetect->pcm->pcmdataL[x];
			  value += offset * (x/(float)presetOutputs->wave_samples);

			  r=(0.5 + 0.4f*.12*value*presetOutputs->fWaveScale + presetOutputs->wave_mystery)*.5;
			  theta=(x)*inv_nverts_minus_one*6.28f + presetInputs->time*0.2f;
			  
			  presetOutputs->wavearray[x][0]=(r*cos(theta)*(this->correction ? this->aspect : 1.0)+presetOutputs->wave_x);
			  presetOutputs->wavearray[x][1]=(r*sin(theta)+presetOutputs->wave_y);		       
			  
			}
		
		  }
			
			break;
			
		case 1://circularly moving waveform
			
			presetOutputs->wave_rot =   0;
			presetOutputs->wave_scale = this->vh/(float)this->vw;
			
			
			presetOutputs->wave_y=-1*(presetOutputs->wave_y-1.0);
			
			
			presetOutputs->wave_samples = 512-32;
			for ( x=0;x<(512-32);x++)
			{
				
				theta=beatDetect->pcm->pcmdataL[x+32]*0.06*presetOutputs->fWaveScale * 1.57 + presetInputs->time*2.3;
				r=(0.53 + 0.43*beatDetect->pcm->pcmdataR[x]*0.12*presetOutputs->fWaveScale+ presetOutputs->wave_mystery)*.5;
				
				presetOutputs->wavearray[x][0]=(r*cos(theta)*(this->correction ? this->aspect : 1.0)+presetOutputs->wave_x);
				presetOutputs->wavearray[x][1]=(r*sin(theta)+presetOutputs->wave_y);
			
			}
			
			
			
			break;
			
		case 2://EXPERIMENTAL
			
			
			presetOutputs->wave_y=-1*(presetOutputs->wave_y-1.0);
			presetOutputs->wave_rot =   0;
			presetOutputs->wave_scale =1.0;
			presetOutputs->wave_samples = 512-32;
			
			
			for (x=0; x<512-32; x++)
			{
				presetOutputs->wavearray[x][0]=(beatDetect->pcm->pcmdataR[x]*presetOutputs->fWaveScale*0.5*(this->correction ? this->aspect : 1.0) + presetOutputs->wave_x);
				
				presetOutputs->wavearray[x][1]=(beatDetect->pcm->pcmdataL[x+32]*presetOutputs->fWaveScale*0.5 + presetOutputs->wave_y);
			
			}
		       
			
			break;
			
		case 3://EXPERIMENTAL
			
			
			presetOutputs->wave_y=-1*(presetOutputs->wave_y-1.0);
			
			presetOutputs->wave_rot =   0;
			presetOutputs->wave_scale =1.0;
			
			
			presetOutputs->wave_samples = 512-32;
			
			for (x=0; x<512-32; x++)
			{
				presetOutputs->wavearray[x][0]=(beatDetect->pcm->pcmdataR[x] * presetOutputs->fWaveScale*0.5 + presetOutputs->wave_x);
				presetOutputs->wavearray[x][1]=( (beatDetect->pcm->pcmdataL[x+32]*presetOutputs->fWaveScale*0.5 + presetOutputs->wave_y));
			
			}
			
			
			break;
			
		case 4://single x-axis derivative waveform
		{
			
			presetOutputs->wave_rot =-presetOutputs->wave_mystery*90;
			presetOutputs->wave_scale=1.0;
			
			presetOutputs->wave_y=-1*(presetOutputs->wave_y-1.0);
			
			
			float w1 = 0.45f + 0.5f*(presetOutputs->wave_mystery*0.5f + 0.5f);
			float w2 = 1.0f - w1;
			float xx[512], yy[512];
			presetOutputs->wave_samples = 512-32;
			
			for (int i=0; i<512-32; i++)
			{
				xx[i] = -1.0f + 2.0f*(i/(512.0-32.0)) + presetOutputs->wave_x;
				yy[i] =0.4* beatDetect->pcm->pcmdataL[i]*0.47f*presetOutputs->fWaveScale + presetOutputs->wave_y;
				xx[i] += 0.4*beatDetect->pcm->pcmdataR[i]*0.44f*presetOutputs->fWaveScale;
				
				if (i>1)
				{
					xx[i] = xx[i]*w2 + w1*(xx[i-1]*2.0f - xx[i-2]);
					yy[i] = yy[i]*w2 + w1*(yy[i-1]*2.0f - yy[i-2]);
				}
				presetOutputs->wavearray[i][0]=xx[i];
				presetOutputs->wavearray[i][1]=yy[i];			    
			}											   		}
		break;
		
		case 5://EXPERIMENTAL
					       
			presetOutputs->wave_rot = 0;
			presetOutputs->wave_scale =1.0;
			
			presetOutputs->wave_y=-1*(presetOutputs->wave_y-1.0);
			
			cos_rot = cosf(presetInputs->time*0.3f);
			sin_rot = sinf(presetInputs->time*0.3f);
			presetOutputs->wave_samples = 512-32;		      
			
			for (x=0; x<512-32; x++)
			{
				float x0 = (beatDetect->pcm->pcmdataR[x]*beatDetect->pcm->pcmdataL[x+32] + beatDetect->pcm->pcmdataL[x+32]*beatDetect->pcm->pcmdataR[x]);
				float y0 = (beatDetect->pcm->pcmdataR[x]*beatDetect->pcm->pcmdataR[x] - beatDetect->pcm->pcmdataL[x+32]*beatDetect->pcm->pcmdataL[x+32]);
				presetOutputs->wavearray[x][0]=((x0*cos_rot - y0*sin_rot)*presetOutputs->fWaveScale*0.5*(this->correction ? this->aspect : 1.0) + presetOutputs->wave_x);
				presetOutputs->wavearray[x][1]=( (x0*sin_rot + y0*cos_rot)*presetOutputs->fWaveScale*0.5 + presetOutputs->wave_y);
			
			}
			
			
			
			break;
			
		case 6://single waveform
			
			
			
			wave_x_temp=-2*0.4142*(fabs(fabs(presetOutputs->wave_mystery)-.5)-.5);
			
			presetOutputs->wave_rot = -presetOutputs->wave_mystery*90;
			presetOutputs->wave_scale =1.0+wave_x_temp;					
			wave_x_temp=-1*(presetOutputs->wave_x-1.0);		
			presetOutputs->wave_samples = isSmoothing ? 512-32 : beatDetect->pcm->numsamples;
			
			for ( x=0;x<  presetOutputs->wave_samples;x++)
			{
				
				presetOutputs->wavearray[x][0]=x/(float)  presetOutputs->wave_samples;
				presetOutputs->wavearray[x][1]=beatDetect->pcm->pcmdataR[x]*.04*presetOutputs->fWaveScale+wave_x_temp;
				
			}
			//	  printf("%f %f\n",renderTarget->texsize*wave_y_temp,wave_y_temp);
			
			break;
			
		case 7://dual waveforms
			
		
			wave_x_temp=-2*0.4142*(fabs(fabs(presetOutputs->wave_mystery)-.5)-.5);
		    
			presetOutputs->wave_rot = -presetOutputs->wave_mystery*90;
			presetOutputs->wave_scale =1.0+wave_x_temp;
		     
			
			presetOutputs->wave_samples = isSmoothing ? 512-32 : beatDetect->pcm->numsamples;
			presetOutputs->two_waves = true;
		
			double y_adj = presetOutputs->wave_y*presetOutputs->wave_y*.5;
			
			wave_y_temp=-1*(presetOutputs->wave_x-1);
			
			for ( x=0;x<  presetOutputs->wave_samples ;x++)
			{
				presetOutputs->wavearray[x][0]=x/((float)  presetOutputs->wave_samples);
				presetOutputs->wavearray[x][1]= beatDetect->pcm->pcmdataL[x]*.04*presetOutputs->fWaveScale+(wave_y_temp+y_adj);
			
			}
			
			for ( x=0;x<  presetOutputs->wave_samples;x++)
			{
				
				presetOutputs->wavearray2[x][0]=x/((float)  presetOutputs->wave_samples);
				presetOutputs->wavearray2[x][1]=beatDetect->pcm->pcmdataR[x]*.04*presetOutputs->fWaveScale+(wave_y_temp-y_adj);
			
			}
			
			break;
			
		
	}

	
}

void Renderer::draw_waveform(PresetOutputs * presetOutputs)
{
	
        CSectionLock lock(&_renderer_lock);
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	
	modulate_opacity_by_volume(presetOutputs);
	maximize_colors(presetOutputs);
	
#ifndef USE_GLES1
	if(presetOutputs->bWaveDots==1) glEnable(GL_LINE_STIPPLE);
#endif
	
	//Thick wave drawing
	if (presetOutputs->bWaveThick==1)  glLineWidth( (this->renderTarget->texsize < 512 ) ? 2 : 2*this->renderTarget->texsize/512);
	else glLineWidth( (this->renderTarget->texsize < 512 ) ? 1 : this->renderTarget->texsize/512);
	
	//Additive wave drawing (vice overwrite)
	if (presetOutputs->bAdditiveWaves==0)  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	else    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	
	glTranslatef(.5, .5, 0);
	glRotatef(presetOutputs->wave_rot, 0, 0, 1);
	glScalef(presetOutputs->wave_scale, 1.0, 1.0);
	glTranslatef(-.5, -.5, 0);
	

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2,GL_FLOAT,0,presetOutputs->wavearray);	

	if (presetOutputs->draw_wave_as_loop) 
	  glDrawArrays(GL_LINE_LOOP,0,presetOutputs->wave_samples);
	else
	  glDrawArrays(GL_LINE_STRIP,0,presetOutputs->wave_samples);


	if (presetOutputs->two_waves)
	  {
	    glVertexPointer(2,GL_FLOAT,0,presetOutputs->wavearray2);
	    if (presetOutputs->draw_wave_as_loop) 
	      glDrawArrays(GL_LINE_LOOP,0,presetOutputs->wave_samples);
	    else
	      glDrawArrays(GL_LINE_STRIP,0,presetOutputs->wave_samples);
	  }
	
	
#ifndef USE_GLES1
	if(presetOutputs->bWaveDots==1) glDisable(GL_LINE_STIPPLE);
#endif	

	glPopMatrix();
}

void Renderer::maximize_colors(PresetOutputs *presetOutputs)
{
	
	float wave_r_switch=0, wave_g_switch=0, wave_b_switch=0;
        CSectionLock lock(&_renderer_lock);
	//wave color brightening
	//
	//forces max color value to 1.0 and scales
	// the rest accordingly
	if(presetOutputs->nWaveMode==2 || presetOutputs->nWaveMode==5)
	{
		switch(this->renderTarget->texsize)
		{
			case 256:  presetOutputs->wave_o *= 0.07f; break;
			case 512:  presetOutputs->wave_o *= 0.09f; break;
			case 1024: presetOutputs->wave_o *= 0.11f; break;
			case 2048: presetOutputs->wave_o *= 0.13f; break;
		}
	}
	
	else if(presetOutputs->nWaveMode==3)
	{
		switch(this->renderTarget->texsize)
		{
			case 256:  presetOutputs->wave_o *= 0.075f; break;
			case 512:  presetOutputs->wave_o *= 0.15f; break;
			case 1024: presetOutputs->wave_o *= 0.22f; break;
			case 2048: presetOutputs->wave_o *= 0.33f; break;
		}
		presetOutputs->wave_o*=1.3f;
		presetOutputs->wave_o*=powf(beatDetect->treb , 2.0f);
	}
	
	if (presetOutputs->bMaximizeWaveColor==1)
	{
		if(presetOutputs->wave_r>=presetOutputs->wave_g && presetOutputs->wave_r>=presetOutputs->wave_b)   //red brightest
		{
			wave_b_switch=presetOutputs->wave_b*(1/presetOutputs->wave_r);
			wave_g_switch=presetOutputs->wave_g*(1/presetOutputs->wave_r);
			wave_r_switch=1.0;
		}
		else if   (presetOutputs->wave_b>=presetOutputs->wave_g && presetOutputs->wave_b>=presetOutputs->wave_r)         //blue brightest
		{
			wave_r_switch=presetOutputs->wave_r*(1/presetOutputs->wave_b);
			wave_g_switch=presetOutputs->wave_g*(1/presetOutputs->wave_b);
			wave_b_switch=1.0;
			
		}
		
		else  if (presetOutputs->wave_g>=presetOutputs->wave_b && presetOutputs->wave_g>=presetOutputs->wave_r)         //green brightest
		{
			wave_b_switch=presetOutputs->wave_b*(1/presetOutputs->wave_g);
			wave_r_switch=presetOutputs->wave_r*(1/presetOutputs->wave_g);
			wave_g_switch=1.0;
		}
		
		
		glColor4f(wave_r_switch, wave_g_switch, wave_b_switch, presetOutputs->wave_o);
	}
	else
	{
		glColor4f(presetOutputs->wave_r, presetOutputs->wave_g, presetOutputs->wave_b, presetOutputs->wave_o);
	}
	
}

void Renderer::darken_center()
{
	
        CSectionLock lock(&_renderer_lock);
	float unit=0.05f;
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float colors[6][4] = {{0, 0, 0, 3.0f/32.0f},
			      {0, 0, 0, 0},
			      {0, 0, 0, 0},
			      {0, 0, 0, 0},
			      {0, 0, 0, 0},
			      {0, 0, 0, 0}};

	float points[6][2] = {{ 0.5,  0.5},
			      { 0.45, 0.5},
			      { 0.5,  0.45},
			      { 0.55, 0.5},
			      { 0.5,  0.55},
			      { 0.45, 0.5}};

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);	 
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glVertexPointer(2,GL_FLOAT,0,points);
	glColorPointer(4,GL_FLOAT,0,colors);
		       
	glDrawArrays(GL_TRIANGLE_FAN,0,6);

}


void Renderer::modulate_opacity_by_volume(PresetOutputs *presetOutputs)
{
	
	//modulate volume by opacity
	//
	//set an upper and lower bound and linearly
	//calculate the opacity from 0=lower to 1=upper
	//based on current volume
	
        CSectionLock lock(&_renderer_lock);
	
	if (presetOutputs->bModWaveAlphaByVolume==1)
	{if (beatDetect->vol<=presetOutputs->fModWaveAlphaStart)  presetOutputs->wave_o=0.0;
	 else if (beatDetect->vol>=presetOutputs->fModWaveAlphaEnd) presetOutputs->wave_o=presetOutputs->fWaveAlpha;
	 else presetOutputs->wave_o=presetOutputs->fWaveAlpha*((beatDetect->vol-presetOutputs->fModWaveAlphaStart)/(presetOutputs->fModWaveAlphaEnd-presetOutputs->fModWaveAlphaStart));}
	else presetOutputs->wave_o=presetOutputs->fWaveAlpha;
}

void Renderer::draw_motion_vectors(PresetOutputs *presetOutputs)
{  	
        CSectionLock lock(&_renderer_lock);

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	
	float offsetx=presetOutputs->mv_dx, intervalx=1.0/(float)presetOutputs->mv_x;
	float offsety=presetOutputs->mv_dy, intervaly=1.0/(float)presetOutputs->mv_y;
	
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glPointSize(presetOutputs->mv_l);
	glColor4f(presetOutputs->mv_r, presetOutputs->mv_g, presetOutputs->mv_b, presetOutputs->mv_a);
	
	int numx = static_cast<int>(presetOutputs->mv_x);
	int numy = static_cast<int>(presetOutputs->mv_y);

	if (numx + numy < 600)
	  {
	int size = numx * numy;

#ifdef _WIN32PC
  // guessed value. solved in current projectM svn
  char buf[1024];
  sprintf(buf, "%s: size = %d\n", __FUNCTION__,size);
  OutputDebugString( buf );
  float points[1024][2];
#else
	float points[size][2];
#endif
	

       
	for (int x=0;x<numx;x++)
	{
		for(int y=0;y<numy;y++)
		{
			float lx, ly, lz;
			lx = offsetx+x*intervalx;
			ly = offsety+y*intervaly;
 			
			points[(x * numy) + y][0] = lx;
			points[(x * numy) + y][1] = ly;
		}
	}

	glVertexPointer(2,GL_FLOAT,0,points);
	glDrawArrays(GL_POINTS,0,size);
	  }
	
}

GLuint Renderer::initRenderToTexture()
{
	return renderTarget->initRenderToTexture();
}

void Renderer::draw_borders(PresetOutputs *presetOutputs)
{
        CSectionLock lock(&_renderer_lock);
  	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);	 
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	//Draw Borders
	float of=presetOutputs->ob_size*.5;
	float iff=presetOutputs->ib_size*.5;
	float texof=1.0-of;
	
	//no additive drawing for borders
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glColor4f(presetOutputs->ob_r, presetOutputs->ob_g, presetOutputs->ob_b, presetOutputs->ob_a);


  
	float pointsA[4][2] = {{0,0},{0,1},{of,0},{of,1}};	
	glVertexPointer(2,GL_FLOAT,0,pointsA);    
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	float pointsB[4][2] = {{of,0},{of,of},{texof,0},{texof,of}};
	glVertexPointer(2,GL_FLOAT,0,pointsB);    
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	float pointsC[4][2] = {{texof,0},{texof,1},{1,0},{1,1}};
	glVertexPointer(2,GL_FLOAT,0,pointsC);    
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	float pointsD[4][2] = {{of,1},{of,texof},{texof,1},{texof,texof}};
	glVertexPointer(2,GL_FLOAT,0,pointsD);    
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	glColor4f(presetOutputs->ib_r, presetOutputs->ib_g, presetOutputs->ib_b, presetOutputs->ib_a);

	glRectd(of, of, of+iff, texof);
	glRectd(of+iff, of, texof-iff, of+iff);
	glRectd(texof-iff, of, texof, texof);
	glRectd(of+iff, texof, texof-iff, texof-iff);
  
	float pointsE[4][2] = {{of,of},{of,texof},{of+iff,of},{of+iff,texof}};	
	glVertexPointer(2,GL_FLOAT,0,pointsE);    
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	float pointsF[4][2] = {{of+iff,of},{of+iff,of+iff},{texof-iff,of},{texof-iff,of+iff}};
	glVertexPointer(2,GL_FLOAT,0,pointsF);    
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	float pointsG[4][2] = {{texof-iff,of},{texof-iff,texof},{texof,of},{texof,texof}};
	glVertexPointer(2,GL_FLOAT,0,pointsG);    
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	float pointsH[4][2] = {{of+iff,texof},{of+iff,texof-iff},{texof-iff,texof},{texof-iff,texof-iff}};
	glVertexPointer(2,GL_FLOAT,0,pointsH);    
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	

}



void Renderer::draw_title_to_texture()
{
        CSectionLock lock(&_renderer_lock);
#ifdef USE_FTGL
	if (this->drawtitle>100)
	{
		draw_title_to_screen(true);
		this->drawtitle=0;
	}
#endif /** USE_FTGL */
}

/*
void setUpLighting()
{
        CSectionLock lock(&_renderer_lock);
	// Set up lighting.
	float light1_ambient[4]  = { 1.0, 1.0, 1.0, 1.0 };
	float light1_diffuse[4]  = { 1.0, 0.9, 0.9, 1.0 };
	float light1_specular[4] = { 1.0, 0.7, 0.7, 1.0 };
	float light1_position[4] = { -1.0, 1.0, 1.0, 0.0 };
	glLightfv(GL_LIGHT1, GL_AMBIENT,  light1_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,  light1_diffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
	glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
	//glEnable(GL_LIGHT1);
	
	float light2_ambient[4]  = { 0.2, 0.2, 0.2, 1.0 };
	float light2_diffuse[4]  = { 0.9, 0.9, 0.9, 1.0 };
	float light2_specular[4] = { 0.7, 0.7, 0.7, 1.0 };
	float light2_position[4] = { 0.0, -1.0, 1.0, 0.0 };
	glLightfv(GL_LIGHT2, GL_AMBIENT,  light2_ambient);
	glLightfv(GL_LIGHT2, GL_DIFFUSE,  light2_diffuse);
	glLightfv(GL_LIGHT2, GL_SPECULAR, light2_specular);
	glLightfv(GL_LIGHT2, GL_POSITION, light2_position);
	glEnable(GL_LIGHT2);
	
	float front_emission[4] = { 0.3, 0.2, 0.1, 0.0 };
	float front_ambient[4]  = { 0.2, 0.2, 0.2, 0.0 };
	float front_diffuse[4]  = { 0.95, 0.95, 0.8, 0.0 };
	float front_specular[4] = { 0.6, 0.6, 0.6, 0.0 };
	glMaterialfv(GL_FRONT, GL_EMISSION, front_emission);
	glMaterialfv(GL_FRONT, GL_AMBIENT, front_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, front_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, front_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, 16.0);
	glColor4fv(front_diffuse);
	
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	
	glEnable(GL_LIGHTING);
}
*/

float title_y;

void Renderer::draw_title_to_screen(bool flip)
{
        CSectionLock lock(&_renderer_lock);
	
#ifdef USE_FTGL
	if(this->drawtitle>0)
	{
		
	  //setUpLighting();
		
		//glEnable(GL_POLYGON_SMOOTH);
		//glEnable( GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_DEPTH_BUFFER_BIT);
		
		int draw;
		if (drawtitle>=80) draw = 80;
		else draw = drawtitle;
		
		float easein = ((80-draw)*.0125);
		float easein2 = easein * easein;
		
		if(drawtitle==1)
		{
			title_y = (float)rand()/RAND_MAX;
			title_y *= 2;
			title_y -= 1;
			title_y *= .6;
		}
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glBlendFunc(GL_SRC_ALPHA_SATURATE,GL_ONE);
		glColor4f(1.0, 1.0, 1.0, 1.0);
		
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		
		glFrustum(-1, 1, -1 * (float)vh/(float)vw, 1 *(float)vh/(float)vw, 1, 1000);
		if (flip) glScalef(1, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		
		glTranslatef(-850, title_y * 850 *vh/vw  , easein2*900-900);
		
		glRotatef(easein2*360, 1, 0, 0);
		
		poly_font->Render(this->title.c_str() );
		
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		this->drawtitle++;
		
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		
		glMatrixMode(GL_MODELVIEW);
		
		glDisable( GL_CULL_FACE);
		glDisable( GL_DEPTH_TEST);
		
		glDisable(GL_COLOR_MATERIAL);
		
		glDisable(GL_LIGHTING);
		glDisable(GL_POLYGON_SMOOTH);
	}
#endif /** USE_FTGL */
}



void Renderer::draw_title()
{
#ifdef USE_FTGL
        CSectionLock lock(&_renderer_lock);
	//glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
	
	glColor4f(1.0, 1.0, 1.0, 1.0);
	//  glPushMatrix();
	//  glTranslatef(this->vw*.001,this->vh*.03, -1);
	//  glScalef(this->vw*.015,this->vh*.025,0);
	
	glRasterPos2f(0.01, 0.05);
	title_font->FaceSize( (unsigned)(20*(this->vh/512.0)));
	
	
	title_font->Render(this->title.c_str() );
	//  glPopMatrix();
	//glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
#endif /** USE_FTGL */
}

void Renderer::draw_preset()
{
#ifdef USE_FTGL
        CSectionLock lock(&_renderer_lock);
	//glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
	
	glColor4f(1.0, 1.0, 1.0, 1.0);
	//      glPushMatrix();
	//glTranslatef(this->vw*.001,this->vh*-.01, -1);
	//glScalef(this->vw*.003,this->vh*.004,0);
	
	
	glRasterPos2f(0.01, 0.01);
	
	title_font->FaceSize((unsigned)(12*(this->vh/512.0)));
	if(this->noSwitch) title_font->Render("[LOCKED]  " );
	title_font->FaceSize((unsigned)(20*(this->vh/512.0)));
	
	title_font->Render(this->presetName().c_str() );
	
	
	
	//glPopMatrix();
	// glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
#endif /** USE_FTGL */
}

void Renderer::draw_help( )
{
	
#ifdef USE_FTGL
        CSectionLock lock(&_renderer_lock);
//glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glPushMatrix();
	glTranslatef(0, 1, 0);
	//glScalef(this->vw*.02,this->vh*.02 ,0);
	
	
	title_font->FaceSize((unsigned)( 18*(this->vh/512.0)));
	
	glRasterPos2f(0.01, -0.05);
	title_font->Render("Help");
	
	glRasterPos2f(0.01, -0.09);
	title_font->Render("----------------------------");
	
	glRasterPos2f(0.01, -0.13);
	title_font->Render("F1: This help menu");
	
	glRasterPos2f(0.01, -0.17);
	title_font->Render("F2: Show song title");
	
	glRasterPos2f(0.01, -0.21);
	title_font->Render("F3: Show preset name");
	
	glRasterPos2f(0.01, -0.25);
	title_font->Render("F4: Show Rendering Settings");
	
	glRasterPos2f(0.01, -0.29);
	title_font->Render("F5: Show FPS");
	
	glRasterPos2f(0.01, -0.35);
	title_font->Render("F: Fullscreen");
	
	glRasterPos2f(0.01, -0.39);
	title_font->Render("L: Lock/Unlock Preset");
	
	glRasterPos2f(0.01, -0.43);
	title_font->Render("M: Show Menu");
	
	glRasterPos2f(0.01, -0.49);
	title_font->Render("R: Random preset");
	glRasterPos2f(0.01, -0.53);
	title_font->Render("N: Next preset");
	
	glRasterPos2f(0.01, -0.57);
	title_font->Render("P: Previous preset");
	
	glPopMatrix();
	//         glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

#endif /** USE_FTGL */
}

void Renderer::draw_stats(PresetInputs *presetInputs)
{
	
#ifdef USE_FTGL
        CSectionLock lock(&_renderer_lock);
	char buffer[128];
	float offset= (this->showfps%2 ? -0.05 : 0.0);
	// glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
	
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glPushMatrix();
	glTranslatef(0.01, 1, 0);
	glRasterPos2f(0, -.05+offset);
	other_font->Render(this->correction ? "  aspect: corrected" : "  aspect: stretched");
	sprintf( buffer, " (%f)", this->aspect);
	other_font->Render(buffer);
	
	
	
	glRasterPos2f(0, -.09+offset);
	other_font->FaceSize((unsigned)(18*(this->vh/512.0)));
	
	sprintf( buffer, " texsize: %d", this->renderTarget->texsize);
	other_font->Render(buffer);
	
	glRasterPos2f(0, -.13+offset);
	sprintf( buffer, "viewport: %d x %d", this->vw, this->vh);
	
	other_font->Render(buffer);
	glRasterPos2f(0, -.17+offset);
	other_font->Render((this->renderTarget->useFBO ? "     FBO: on" : "     FBO: off"));
	
	glRasterPos2f(0, -.21+offset);
	sprintf( buffer, "    mesh: %d x %d", presetInputs->gx, presetInputs->gy);
	other_font->Render(buffer);
	
	glRasterPos2f(0, -.25+offset);
	sprintf( buffer, "textures: %.1fkB", textureManager->getTextureMemorySize() /1000.0f);
	other_font->Render(buffer);
	
	glPopMatrix();
	// glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	
	
#endif /** USE_FTGL */
}
void Renderer::draw_fps( float realfps )
{
#ifdef USE_FTGL
        CSectionLock lock(&_renderer_lock);
	char bufferfps[20];
	sprintf( bufferfps, "%.1f fps", realfps);
	// glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
	
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glPushMatrix();
	glTranslatef(0.01, 1, 0);
	glRasterPos2f(0, -0.05);
	title_font->FaceSize((unsigned)(20*(this->vh/512.0)));
	title_font->Render(bufferfps);
	
	glPopMatrix();
	// glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
#endif /** USE_FTGL */
}


//Actually draws the texture to the screen
//
//The Video Echo effect is also applied here
void Renderer::render_texture_to_screen(PresetOutputs *presetOutputs)
{
	
        CSectionLock lock(&_renderer_lock);
	int flipx=1, flipy=1;
	
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	//Overwrite anything on the screen
	glBlendFunc(GL_ONE, GL_ZERO);
	glColor4f(1.0, 1.0, 1.0, 1.0f);
	
	glEnable(GL_TEXTURE_2D);
	


	float tex[4][2] = {{0, 1},
			   {0, 0},
			   {1, 0},
			   {1, 1}};

	float points[4][2] = {{-0.5, -0.5},
			      {-0.5,  0.5},
			      { 0.5,  0.5},
			      { 0.5,  -0.5}};

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);	 
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glVertexPointer(2,GL_FLOAT,0,points);
	glTexCoordPointer(2,GL_FLOAT,0,tex);
		       
	glDrawArrays(GL_TRIANGLE_FAN,0,4);

	//Noe Blend the Video Echo
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glMatrixMode(GL_TEXTURE);
	
	//draw video echo
	glColor4f(1.0, 1.0, 1.0, presetOutputs->fVideoEchoAlpha);
	glTranslatef(.5, .5, 0);
	glScalef(1.0/presetOutputs->fVideoEchoZoom, 1.0/presetOutputs->fVideoEchoZoom, 1);
	glTranslatef(-.5, -.5, 0);
	
	switch (((int)presetOutputs->nVideoEchoOrientation))
	{
		case 0: flipx=1;flipy=1;break;
		case 1: flipx=-1;flipy=1;break;
		case 2: flipx=1;flipy=-1;break;
		case 3: flipx=-1;flipy=-1;break;
		default: flipx=1;flipy=1; break;
	}

	float pointsFlip[4][2] = {{(float)-0.5*flipx, (float)-0.5*flipy},
				  {(float)-0.5*flipx,  (float)0.5*flipy},
				  { (float)0.5*flipx,  (float)0.5*flipy},
				  { (float)0.5*flipx, (float)-0.5*flipy}};
	
	glVertexPointer(2,GL_FLOAT,0,pointsFlip);
	glDrawArrays(GL_TRIANGLE_FAN,0,4);	
	
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (presetOutputs->bBrighten==1)
	{
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
		glDrawArrays(GL_TRIANGLE_FAN,0,4);		
		glBlendFunc(GL_ZERO, GL_DST_COLOR);
		glDrawArrays(GL_TRIANGLE_FAN,0,4);	
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
		glDrawArrays(GL_TRIANGLE_FAN,0,4);	
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
	}
	
	if (presetOutputs->bDarken==1)
	{		
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glBlendFunc(GL_ZERO, GL_DST_COLOR);
		glDrawArrays(GL_TRIANGLE_FAN,0,4);	
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		
	}
	
	
	if (presetOutputs->bSolarize)
	{		
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glBlendFunc(GL_ZERO, GL_ONE_MINUS_DST_COLOR);
		glDrawArrays(GL_TRIANGLE_FAN,0,4);		
		glBlendFunc(GL_DST_COLOR, GL_ONE);
		glDrawArrays(GL_TRIANGLE_FAN,0,4);			
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		
	}
	
	if (presetOutputs->bInvert)
	{
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
		glDrawArrays(GL_TRIANGLE_FAN,0,4);	
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	
}

void Renderer::render_texture_to_studio(PresetOutputs *presetOutputs, PresetInputs *presetInputs)
{
  /*
        CSectionLock lock(&_renderer_lock);
	int x, y;
	int flipx=1, flipy=1;
	
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glColor4f(0.0, 0.0, 0.0, 0.04);
	
	
	glBegin(GL_QUADS);
	glVertex4d(-0.5, -0.5, -1, 1);
	glVertex4d(-0.5,  0.5, -1, 1);
	glVertex4d(0.5,  0.5, -1, 1);
	glVertex4d(0.5, -0.5, -1, 1);
	glEnd();
	
	
	glColor4f(0.0, 0.0, 0.0, 1.0);
	
	glBegin(GL_QUADS);
	glVertex4d(-0.5, 0, -1, 1);
	glVertex4d(-0.5,  0.5, -1, 1);
	glVertex4d(0.5,  0.5, -1, 1);
	glVertex4d(0.5, 0, -1, 1);
	glEnd();
	
	glBegin(GL_QUADS);
	glVertex4d(0, -0.5, -1, 1);
	glVertex4d(0,  0.5, -1, 1);
	glVertex4d(0.5,  0.5, -1, 1);
	glVertex4d(0.5, -0.5, -1, 1);
	glEnd();
	
	glPushMatrix();
	glTranslatef(.25, .25, 0);
	glScalef(.5, .5, 1);
	
	glEnable(GL_TEXTURE_2D);
	
	glBlendFunc(GL_ONE, GL_ZERO);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	//Draw giant rectangle and texture it with our texture!
	glBegin(GL_QUADS);
	glTexCoord4d(0, 1, 0, 1); glVertex4d(-0.5, -0.5, -1, 1);
	glTexCoord4d(0, 0, 0, 1); glVertex4d(-0.5,  0.5, -1, 1);
	glTexCoord4d(1, 0, 0, 1); glVertex4d(0.5,  0.5, -1, 1);
	glTexCoord4d(1, 1, 0, 1); glVertex4d(0.5, -0.5, -1, 1);
	glEnd();
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	
	glMatrixMode(GL_TEXTURE);
	
	//draw video echo
	glColor4f(1.0, 1.0, 1.0, presetOutputs->fVideoEchoAlpha);
	glTranslatef(.5, .5, 0);
	glScalef(1/presetOutputs->fVideoEchoZoom, 1/presetOutputs->fVideoEchoZoom, 1);
	glTranslatef(-.5, -.5, 0);
	
	switch (((int)presetOutputs->nVideoEchoOrientation))
	{
		case 0: flipx=1;flipy=1;break;
		case 1: flipx=-1;flipy=1;break;
		case 2: flipx=1;flipy=-1;break;
		case 3: flipx=-1;flipy=-1;break;
		default: flipx=1;flipy=1; break;
	}
	glBegin(GL_QUADS);
	glTexCoord4d(0, 1, 0, 1); glVertex4f(-0.5*flipx, -0.5*flipy, -1, 1);
	glTexCoord4d(0, 0, 0, 1); glVertex4f(-0.5*flipx,  0.5*flipy, -1, 1);
	glTexCoord4d(1, 0, 0, 1); glVertex4f(0.5*flipx,  0.5*flipy, -1, 1);
	glTexCoord4d(1, 1, 0, 1); glVertex4f(0.5*flipx, -0.5*flipy, -1, 1);
	glEnd();
	
	
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	
	
	if (presetOutputs->bInvert)
	{
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
		glBegin(GL_QUADS);
		glVertex4f(-0.5*flipx, -0.5*flipy, -1, 1);
		glVertex4f(-0.5*flipx,  0.5*flipy, -1, 1);
		glVertex4f(0.5*flipx,  0.5*flipy, -1, 1);
		glVertex4f(0.5*flipx, -0.5*flipy, -1, 1);
		glEnd();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	
	//  glTranslated(.5,.5,0);
	//  glScaled(1/fVideoEchoZoom,1/fVideoEchoZoom,1);
	//   glTranslated(-.5,-.5,0);
	//glTranslatef(0,.5*vh,0);
	
	
	//glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
	
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(.25, -.25, 0);
	glScalef(.5, .5, 1);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	
	for (x=0;x<presetInputs->gx;x++)
	{
		glBegin(GL_LINE_STRIP);
		for(y=0;y<presetInputs->gy;y++)
		{
			glVertex4f((presetOutputs->x_mesh[x][y]-.5), (presetOutputs->y_mesh[x][y]-.5), -1, 1);
			//glVertex4f((origx[x+1][y]-.5) * vw, (origy[x+1][y]-.5) *vh ,-1,1);
		}
		glEnd();
	}
	
	for (y=0;y<presetInputs->gy;y++)
	{
		glBegin(GL_LINE_STRIP);
		for(x=0;x<presetInputs->gx;x++)
		{
			glVertex4f((presetOutputs->x_mesh[x][y]-.5), (presetOutputs->y_mesh[x][y]-.5), -1, 1);
			//glVertex4f((origx[x+1][y]-.5) * vw, (origy[x+1][y]-.5) *vh ,-1,1);
		}
		glEnd();
	}
	
	glEnable( GL_TEXTURE_2D );
	
	
	// glTranslated(-.5,-.5,0);     glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	// Waveform display -- bottom-left
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(-.5, 0, 0);
	
	glTranslatef(0, -0.10, 0);
	glBegin(GL_LINE_STRIP);
	glColor4f(0, 1.0, 1.0, 1.0);
	glVertex3f((((this->totalframes%256)/551.0)), beatDetect->treb_att*-7, -1);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glVertex3f((((this->totalframes%256)/551.0)), 0 , -1);
	glColor4f(.5, 1.0, 1.0, 1.0);
	glVertex3f((((this->totalframes%256)/551.0)), beatDetect->treb*7, -1);
	glEnd();
	
	glTranslatef(0, -0.13, 0);
	glBegin(GL_LINE_STRIP);
	glColor4f(0, 1.0, 0.0, 1.0);
	glVertex3f((((this->totalframes%256)/551.0)), beatDetect->mid_att*-7, -1);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glVertex3f((((this->totalframes%256)/551.0)), 0 , -1);
	glColor4f(.5, 1.0, 0.0, 0.5);
	glVertex3f((((this->totalframes%256)/551.0)), beatDetect->mid*7, -1);
	glEnd();
	
	
	glTranslatef(0, -0.13, 0);
	glBegin(GL_LINE_STRIP);
	glColor4f(1.0, 0.0, 0.0, 1.0);
	glVertex3f((((this->totalframes%256)/551.0)), beatDetect->bass_att*-7, -1);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glVertex3f((((this->totalframes%256)/551.0)), 0 , -1);
	glColor4f(.7, 0.2, 0.2, 1.0);
	glVertex3f((((this->totalframes%256)/551.0)), beatDetect->bass*7, -1);
	glEnd();
	
	glTranslatef(0, -0.13, 0);
	glBegin(GL_LINES);
	
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glVertex3f((((this->totalframes%256)/551.0)), 0 , -1);
	glColor4f(1.0, 0.6, 1.0, 1.0);
	glVertex3f((((this->totalframes%256)/551.0)), beatDetect->vol*7, -1);
	glEnd();
	
	glPopMatrix();
	
	glDisable(GL_TEXTURE_2D);
  */
}



