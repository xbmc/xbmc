/*
 * ShaderEngine.hpp
 *
 *  Created on: Jul 18, 2008
 *      Author: pete
 */

#ifndef SHADERENGINE_HPP_
#define SHADERENGINE_HPP_


#ifdef USE_GLES1
#include <GLES/gl.h>
#else
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#endif


#ifdef USE_CG
#include <Cg/cg.h>    /* Can't include this?  Is Cg Toolkit installed! */
#include <Cg/cgGL.h>
#endif


#include "Pipeline.hpp"
#include "PipelineContext.hpp"
class ShaderEngine;
#include "TextureManager.hpp"

#include <cstdlib>
#include <iostream>
#include <map>
#include "Shader.hpp"
class ShaderEngine
{
#ifdef USE_CG


  unsigned int mainTextureId;
  int texsize;
  float aspect;
  BeatDetect *beatDetect;
  TextureManager *textureManager;

  GLuint noise_texture_lq_lite;
  GLuint noise_texture_lq;
  GLuint noise_texture_mq;
  GLuint noise_texture_hq;
  GLuint noise_texture_perlin;
  GLuint noise_texture_lq_vol;
  GLuint noise_texture_hq_vol;

  bool blur1_enabled;
  bool blur2_enabled;
  bool blur3_enabled;
  GLuint blur1_tex;
  GLuint blur2_tex;
  GLuint blur3_tex;

  float rand_preset[4];

  CGcontext   myCgContext;
  CGprofile myCgProfile;
  CGprogram   blur1Program;
  CGprogram   blur2Program;

  bool enabled;

  std::map<Shader*,CGprogram> programs;

   std::string cgTemplate;
   std::string blurProgram;

 bool LoadCgProgram(Shader &shader);
 bool checkForCgCompileError(const char *situation);
 void checkForCgError(const char *situation);

 void SetupCg();
 void SetupCgVariables(CGprogram program, const Pipeline &pipeline, const PipelineContext &pipelineContext);
 void SetupCgQVariables(CGprogram program, const Pipeline &pipeline);

 void SetupUserTexture(CGprogram program, const UserTexture* texture);
 void SetupUserTextureState(const UserTexture* texture);



#endif
public:
	ShaderEngine();
	virtual ~ShaderEngine();
#ifdef USE_CG
    void RenderBlurTextures(const Pipeline  &pipeline, const PipelineContext &pipelineContext, const int texsize);
	void loadShader(Shader &shader);

	void setParams(const int texsize, const unsigned int texId, const float aspect, BeatDetect *beatDetect, TextureManager *textureManager);
	void enableShader(Shader &shader, const Pipeline &pipeline, const PipelineContext &pipelineContext);
	void disableShader();
	void reset();
	void setAspect(float aspect);
    std::string profileName;

#endif
};

#endif /* SHADERENGINE_HPP_ */

