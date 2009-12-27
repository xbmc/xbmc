#ifndef Renderer_HPP
#define Renderer_HPP

#include "FBO.hpp"
#include "BeatDetect.hpp"
#include <string>
#include <set>

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

#ifdef USE_FTGL
#ifdef WIN32
#include <ftgl.h>
#include <FTGLPixmapFont.h>
#include <FTGLExtrdFont.h>
#else
#include <FTGL/FTFont.h>
#include <FTGL/FTGLPixmapFont.h>
#include <FTGL/FTGLExtrdFont.h>
#endif
#endif /** USE_FTGL */


#include "Pipeline.hpp"
#include "PerPixelMesh.hpp"
#include "Transformation.hpp"
#include "ShaderEngine.hpp"

class UserTexture;
class BeatDetect;
class TextureManager;

class Renderer
{

public:

  bool showfps;
  bool showtitle;
  bool showpreset;
  bool showhelp;
  bool showstats;

  bool studio;
  bool correction;

  bool noSwitch;

  int totalframes;
  float realfps;

  std::string title;
  int drawtitle;
  int texsize;


  Renderer( int width, int height, int gx, int gy, int texsize,  BeatDetect *beatDetect, std::string presetURL, std::string title_fontURL, std::string menu_fontURL);
  ~Renderer();

  void RenderFrame(const Pipeline &pipeline, const PipelineContext &pipelineContext);
  void ResetTextures();
  void reset(int w, int h);
  GLuint initRenderToTexture();


  void SetPipeline(Pipeline &pipeline);

  void setPresetName(const std::string& theValue)
  {
    m_presetName = theValue;
  }

  std::string presetName() const
  {
    return m_presetName;
  }

private:

	PerPixelMesh mesh;
  RenderTarget *renderTarget;
  BeatDetect *beatDetect;
  TextureManager *textureManager;
  static Pipeline* currentPipe;
  RenderContext renderContext;
  //per pixel equation variables
#ifdef USE_CG
  ShaderEngine shaderEngine;
#endif
  std::string m_presetName;

  float* p;


  int vw;
  int vh;

  float aspect;

  std::string title_fontURL;
  std::string menu_fontURL;
  std::string presetURL;

#ifdef USE_FTGL
  FTGLPixmapFont *title_font;
  FTGLPixmapFont *other_font;
  FTGLExtrdFont *poly_font;
#endif /** USE_FTGL */

  void SetupPass1(const Pipeline &pipeline, const PipelineContext &pipelineContext);
  void Interpolation(const Pipeline &pipeline);
  void RenderItems(const Pipeline &pipeline, const PipelineContext &pipelineContext);
  void FinishPass1();
  void Pass2 (const Pipeline &pipeline, const PipelineContext &pipelineContext);
  void CompositeOutput(const Pipeline &pipeline, const PipelineContext &pipelineContext);

  inline static Point PerPixel(Point p, PerPixelContext &context)
  {
	  return currentPipe->PerPixel(p,context);
  }

  void rescale_per_pixel_matrices();

  void draw_fps( float realfps );
  void draw_stats();
  void draw_help();
  void draw_preset();
  void draw_title();
  void draw_title_to_screen(bool flip);
  void draw_title_to_texture();

};

#endif
