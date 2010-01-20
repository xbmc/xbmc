#ifndef LINUXRENDERERATI_RENDERER
#define LINUXRENDERERATI_RENDERER

#include "../../../guilib/Surface.h"
#include "../ffmpeg/DllSwScale.h"
#include "../ffmpeg/DllAvCodec.h"
#include "LinuxRendererGL.h"

#ifdef HAS_SDL_OPENGL

using namespace Surface;

class CLinuxRendererATI:public CLinuxRendererGL
{
public:
  CLinuxRendererATI(bool enableshaders=false);  
  virtual ~CLinuxRendererATI();

  // Player functions
  virtual void         ReleaseImage(int source, bool preserve = false);
  virtual void         FlipPage(int source);
  virtual unsigned int PreInit();
  virtual void         UnInit();

  virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);

protected:
  virtual bool CreateYV12Texture(int index, bool clear=true);
  virtual bool ValidateRenderTarget();
};

#endif

#endif

