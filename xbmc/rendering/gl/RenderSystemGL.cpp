/*
*      Copyright (C) 2005-2013 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/


#include "system.h"

#ifdef HAS_GL

#include "RenderSystemGL.h"
#include "guilib/GraphicContext.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "utils/TimeUtils.h"
#include "utils/SystemInfo.h"
#include "utils/MathUtils.h"
#include "guilib/Texture.h"
#include "rendering/SceneGraph.h"

CRenderSystemGL::CRenderSystemGL() : CRenderSystemBase()
{
  m_enumRenderingSystem = RENDERING_SYSTEM_OPENGL;
  m_glslMajor = 0;
  m_glslMinor = 0;
}

CRenderSystemGL::~CRenderSystemGL()
{
}

void CRenderSystemGL::CheckOpenGLQuirks()

{
#ifdef __APPLE__	
  if (m_RenderVendor.Find("NVIDIA") > -1)
  {             
    // Nvidia 7300 (AppleTV) and 7600 cannot do DXT with NPOT under OSX
    // Nvidia 9400M is slow as a dog
    if (m_renderCaps & RENDER_CAPS_DXT_NPOT)
    {
      const char *arr[3]= { "7300","7600","9400M" };
      for(int j = 0; j < 3; j++)
      {
        if((int(m_RenderRenderer.find(arr[j])) > -1))
        {
          m_renderCaps &= ~ RENDER_CAPS_DXT_NPOT;
          break;
        }
      }
    }
  }
#ifdef __ppc__
  // ATI Radeon 9600 on osx PPC cannot do NPOT
  if (m_RenderRenderer.Find("ATI Radeon 9600") > -1)
  {
    m_renderCaps &= ~ RENDER_CAPS_NPOT;
    m_renderCaps &= ~ RENDER_CAPS_DXT_NPOT;
  }
#endif
#endif
  if (m_RenderVendor.ToLower() == "nouveau")
    m_renderQuirks |= RENDER_QUIRKS_YV12_PREFERED;

  if (m_RenderVendor.Equals("Tungsten Graphics, Inc.")
  ||  m_RenderVendor.Equals("Tungsten Graphics, Inc"))
  {
    unsigned major, minor, micro;
    if (sscanf(m_RenderVersion.c_str(), "%*s Mesa %u.%u.%u", &major, &minor, &micro) == 3)
    {

      if((major  < 7)
      || (major == 7 && minor  < 7)
      || (major == 7 && minor == 7 && micro < 1))
        m_renderQuirks |= RENDER_QUIRKS_MAJORMEMLEAK_OVERLAYRENDERER;
    }
    else
      CLog::Log(LOGNOTICE, "CRenderSystemGL::CheckOpenGLQuirks - unable to parse mesa version string");

    if(m_RenderRenderer.Find("Poulsbo") >= 0)
      m_renderCaps &= ~RENDER_CAPS_DXT_NPOT;

    m_renderQuirks |= RENDER_QUIRKS_BROKEN_OCCLUSION_QUERY;
  }
}	

bool CRenderSystemGL::InitRenderSystem()
{
  m_bVSync = false;
  m_iVSyncMode = 0;
  m_iSwapStamp = 0;
  m_iSwapTime = 0;
  m_iSwapRate = 0;
  m_bVsyncInit = false;
  m_maxTextureSize = 2048;
  m_renderCaps = 0;
  m_needsClear = true;
  // init glew library
  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
    // Problem: glewInit failed, something is seriously wrong
    CLog::Log(LOGERROR, "InitRenderSystem() glewInit returned %i: %s", err, glewGetErrorString(err));
    return false;
  }

  // Get the GL version number
  m_RenderVersionMajor = 0;
  m_RenderVersionMinor = 0;

  const char* ver = (const char*)glGetString(GL_VERSION);
  if (ver != 0)
  {
    sscanf(ver, "%d.%d", &m_RenderVersionMajor, &m_RenderVersionMinor);
    m_RenderVersion = ver;
  }

  if (glewIsSupported("GL_ARB_shading_language_100"))
  {
    ver = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    if (ver)
    {
      sscanf(ver, "%d.%d", &m_glslMajor, &m_glslMinor);
    }
    else
    {
      m_glslMajor = 1;
      m_glslMinor = 0;
    }
  }

  // Get our driver vendor and renderer
  m_RenderVendor = (const char*) glGetString(GL_VENDOR);
  m_RenderRenderer = (const char*) glGetString(GL_RENDERER);

  // grab our capabilities
  if (glewIsSupported("GL_EXT_texture_compression_s3tc"))
    m_renderCaps |= RENDER_CAPS_DXT;

  if (glewIsSupported("GL_ARB_texture_non_power_of_two"))
  {
    m_renderCaps |= RENDER_CAPS_NPOT;
    if (m_renderCaps & RENDER_CAPS_DXT) 
      m_renderCaps |= RENDER_CAPS_DXT_NPOT;
  }
  //Check OpenGL quirks and revert m_renderCaps as needed
  CheckOpenGLQuirks();
	
  m_RenderExtensions  = " ";
  m_RenderExtensions += (const char*) glGetString(GL_EXTENSIONS);
  m_RenderExtensions += " ";

  LogGraphicsInfo();

  m_bRenderCreated = true;

  return true;
}

bool CRenderSystemGL::ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate)
{
  m_width = width;
  m_height = height;

  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

  CalculateMaxTexturesize();

  glViewport(0, 0, width, height);
  glScissor(0, 0, width, height);

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_SCISSOR_TEST);

  //ati doesn't init the texture matrix correctly
  //so we have to do it ourselves
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  if (glewIsSupported("GL_ARB_multitexture"))
  {
    //clear error flags
    ResetGLErrors();

    GLint maxtex;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &maxtex);

    //some sanity checks
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
      CLog::Log(LOGERROR, "ResetRenderSystem() GL_MAX_TEXTURE_IMAGE_UNITS_ARB returned error %i", (int)error);
      maxtex = 3;
    }
    else if (maxtex < 1 || maxtex > 32)
    {
      CLog::Log(LOGERROR, "ResetRenderSystem() GL_MAX_TEXTURE_IMAGE_UNITS_ARB returned invalid value %i", (int)maxtex);
      maxtex = 3;
    }

    //reset texture matrix for all textures
    for (GLint i = 0; i < maxtex; i++)
    {
      glActiveTextureARB(GL_TEXTURE0 + i);
      glLoadIdentity();
    }
    glActiveTextureARB(GL_TEXTURE0);
  }

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glOrtho(0.0f, width-1, height-1, 0.0f, -1.0f, 1.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);          // Turn Blending On
  glDisable(GL_DEPTH_TEST);

  return true;
}

bool CRenderSystemGL::DestroyRenderSystem()
{
  m_bRenderCreated = false;

  return true;
}

bool CRenderSystemGL::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGL::EndRender()
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGL::ClearBuffers(color_t color)
{
  if (!m_bRenderCreated)
    return false;

  float r = GET_R(color) / 255.0f;
  float g = GET_G(color) / 255.0f;
  float b = GET_B(color) / 255.0f;
  float a = GET_A(color) / 255.0f;

  glClearColor(r, g, b, a);

  GLbitfield flags = GL_COLOR_BUFFER_BIT;
  glClear(flags);

  return true;
}

bool CRenderSystemGL::IsExtSupported(const char* extension)
{
  CStdString name;
  name  = " ";
  name += extension;
  name += " ";

  return m_RenderExtensions.find(name) != std::string::npos;;
}

bool CRenderSystemGL::PresentRender(const CDirtyRegionList& dirty)
{
  if (!m_bRenderCreated)
    return false;

  if (m_iVSyncMode != 0 && m_iSwapRate != 0)
  {
    int64_t curr, diff, freq;
    curr = CurrentHostCounter();
    freq = CurrentHostFrequency();

    if(m_iSwapStamp == 0)
      m_iSwapStamp = curr;

    /* calculate our next swap timestamp */
    diff = curr - m_iSwapStamp;
    diff = m_iSwapRate - diff % m_iSwapRate;
    m_iSwapStamp = curr + diff;

    /* sleep as close as we can before, assume 1ms precision of sleep *
     * this should always awake so that we are guaranteed the given   *
     * m_iSwapTime to do our swap                                     */
    diff = (diff - m_iSwapTime) * 1000 / freq;
    if (diff > 0)
      Sleep((DWORD)diff);
  }

  bool result = PresentRenderImpl(dirty);

  if (m_iVSyncMode && m_iSwapRate != 0)
  {
    int64_t curr, diff;
    curr = CurrentHostCounter();

    diff = curr - m_iSwapStamp;
    m_iSwapStamp = curr;

    if (MathUtils::abs(diff - m_iSwapRate) < MathUtils::abs(diff))
      CLog::Log(LOGDEBUG, "%s - missed requested swap",__FUNCTION__);
  }

  m_needsClear = true;
  return result;
}

void CRenderSystemGL::SetVSync(bool enable)
{
  if (m_bVSync==enable && m_bVsyncInit == true)
    return;

  if (!m_bRenderCreated)
    return;

  if (enable)
    CLog::Log(LOGINFO, "GL: Enabling VSYNC");
  else
    CLog::Log(LOGINFO, "GL: Disabling VSYNC");

  m_iVSyncMode   = 0;
  m_iVSyncErrors = 0;
  m_iSwapRate    = 0;
  m_bVSync       = enable;
  m_bVsyncInit   = true;

  SetVSyncImpl(enable);

  if (!enable)
    return;

  if (g_advancedSettings.m_ForcedSwapTime != 0.0)
  {
    /* some hardware busy wait on swap/glfinish, so we must manually sleep to avoid 100% cpu */
    double rate = g_graphicsContext.GetFPS();
    if (rate <= 0.0 || rate > 1000.0)
    {
      CLog::Log(LOGWARNING, "Unable to determine a valid horizontal refresh rate, vsync workaround disabled %.2g", rate);
      m_iSwapRate = 0;
    }
    else
    {
      int64_t freq;
      freq = CurrentHostFrequency();
      m_iSwapRate   = (int64_t)((double)freq / rate);
      m_iSwapTime   = (int64_t)(0.001 * g_advancedSettings.m_ForcedSwapTime * freq);
      m_iSwapStamp  = 0;
      CLog::Log(LOGINFO, "GL: Using artificial vsync sleep with rate %f", rate);
      if(!m_iVSyncMode)
        m_iVSyncMode = 1;
    }
  }

  if (!m_iVSyncMode)
    CLog::Log(LOGERROR, "GL: Vertical Blank Syncing unsupported");
  else
    CLog::Log(LOGINFO, "GL: Selected vsync mode %d", m_iVSyncMode);
}

void CRenderSystemGL::CaptureStateBlock()
{
  if (!m_bRenderCreated)
    return;
  
  glGetIntegerv(GL_VIEWPORT, m_viewPort);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glDisable(GL_SCISSOR_TEST); // fixes FBO corruption on Macs
  if (glActiveTextureARB)
    glActiveTextureARB(GL_TEXTURE0_ARB);
  glDisable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glColor3f(1.0, 1.0, 1.0);
}

void CRenderSystemGL::ApplyStateBlock()
{
  if (!m_bRenderCreated)
    return;

  glViewport(m_viewPort[0], m_viewPort[1], m_viewPort[2], m_viewPort[3]);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_TEXTURE);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  if (glActiveTextureARB)
    glActiveTextureARB(GL_TEXTURE0_ARB);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glEnable(GL_BLEND);
  glEnable(GL_SCISSOR_TEST);
}

void CRenderSystemGL::SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight)
{
  if (!m_bRenderCreated)
    return;

  g_graphicsContext.BeginPaint();

  CPoint offset = camera - CPoint(screenWidth*0.5f, screenHeight*0.5f);

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  float w = (float)viewport[2]*0.5f;
  float h = (float)viewport[3]*0.5f;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(-(viewport[0] + w + offset.x), +(viewport[1] + h + offset.y), 0);
  gluLookAt(0.0, 0.0, -2.0*h, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum( (-w - offset.x)*0.5f, (w - offset.x)*0.5f, (-h + offset.y)*0.5f, (h + offset.y)*0.5f, h, 100*h);
  glMatrixMode(GL_MODELVIEW);

  glGetIntegerv(GL_VIEWPORT, m_viewPort);
  glGetDoublev(GL_MODELVIEW_MATRIX, m_view);
  glGetDoublev(GL_PROJECTION_MATRIX, m_projection);

  g_graphicsContext.EndPaint();
}

void CRenderSystemGL::Project(float &x, float &y, float &z)
{
  GLdouble coordX, coordY, coordZ;
  if (gluProject(x, y, z, m_view, m_projection, m_viewPort, &coordX, &coordY, &coordZ) == GLU_TRUE)
  {
    x = (float)coordX;
    y = (float)(m_viewPort[3] - coordY);
    z = 0;
  }
}

bool CRenderSystemGL::TestRender()
{
  static float theta = 0.0;

  //RESOLUTION_INFO resInfo = CDisplaySettings::Get().GetResolutionInfo(CDisplaySettings::Get().GetCurrentResolution()];
  //glViewport(0, 0, resInfo.iWidth, resInfo.iHeight);

  glPushMatrix();
  glRotatef( theta, 0.0f, 0.0f, 1.0f );
  glBegin( GL_TRIANGLES );
  glColor3f( 1.0f, 0.0f, 0.0f ); glVertex2f( 0.0f, 1.0f );
  glColor3f( 0.0f, 1.0f, 0.0f ); glVertex2f( 0.87f, -0.5f );
  glColor3f( 0.0f, 0.0f, 1.0f ); glVertex2f( -0.87f, -0.5f );
  glEnd();
  glPopMatrix();

  theta += 1.0f;

  return true;
}

void CRenderSystemGL::ApplyHardwareTransform(const TransformMatrix &finalMatrix)
{
  if (!m_bRenderCreated)
    return;

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  GLfloat matrix[4][4];

  for(int i=0;i<3;i++)
    for(int j=0;j<4;j++)
      matrix[j][i] = finalMatrix.m[i][j];

  matrix[0][3] = 0.0f;
  matrix[1][3] = 0.0f;
  matrix[2][3] = 0.0f;
  matrix[3][3] = 1.0f;

  glMultMatrixf(&matrix[0][0]);
}

void CRenderSystemGL::RestoreHardwareTransform()
{
  if (!m_bRenderCreated)
    return;

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

void CRenderSystemGL::CalculateMaxTexturesize()
{
  GLint width = 256;

  // reset any previous GL errors
  ResetGLErrors();

  // max out at 2^(8+8)
  for (int i = 0 ; i<8 ; i++)
  {
    glTexImage2D(GL_PROXY_TEXTURE_2D, 0, 4, width, width, 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, NULL);
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,
                             &width);

    // GMA950 on OS X sets error instead
    if (width == 0 || (glGetError() != GL_NO_ERROR) )
      break;

    m_maxTextureSize = width;
    width *= 2;
    if (width > 65536) // have an upper limit in case driver acts stupid
    {
      CLog::Log(LOGERROR, "GL: Could not determine maximum texture width, falling back to 2048");
      m_maxTextureSize = 2048;
      break;
    }
  }

#ifdef __APPLE__
  // Max Texture size reported on some apple machines seems incorrect
  // Displaying a picture with that resolution results in a corrupted output
  // So force it to a lower value
  // Problem noticed on:
  // iMac with ATI Radeon X1600, both on 10.5.8 (GL_VERSION: 2.0 ATI-1.5.48)
  // and 10.6.2 (GL_VERSION: 2.0 ATI-1.6.6)
  if (strcmp(m_RenderRenderer, "ATI Radeon X1600 OpenGL Engine") == 0)
    m_maxTextureSize = 2048;
  // Mac mini G4 with ATI Radeon 9200 (GL_VERSION: 1.3 ATI-1.5.48)
  else if (strcmp(m_RenderRenderer, "ATI Radeon 9200 OpenGL Engine") == 0)
    m_maxTextureSize = 1024;
#endif

  CLog::Log(LOGINFO, "GL: Maximum texture width: %u", m_maxTextureSize);
}

void CRenderSystemGL::GetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  GLint glvp[4];
  glGetIntegerv(GL_VIEWPORT, glvp);

  viewPort.x1 = glvp[0];
  viewPort.y1 = m_height - glvp[1] - glvp[3];
  viewPort.x2 = glvp[0] + glvp[2];
  viewPort.y2 = viewPort.y1 + glvp[3];
}

void CRenderSystemGL::SetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  glScissor((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
  glViewport((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
}

void CRenderSystemGL::SetScissors(const CRect &rect)
{
  if (!m_bRenderCreated)
    return;
  GLint x1 = MathUtils::round_int(rect.x1);
  GLint y1 = MathUtils::round_int(rect.y1);
  GLint x2 = MathUtils::round_int(rect.x2);
  GLint y2 = MathUtils::round_int(rect.y2);
  glScissor(x1, m_height - y2, x2-x1, y2-y1);
}

void CRenderSystemGL::ResetScissors()
{
  SetScissors(CRect(0, 0, (float)m_width, (float)m_height));
}

void CRenderSystemGL::GetGLSLVersion(int& major, int& minor)
{
  major = m_glslMajor;
  minor = m_glslMinor;
}

void CRenderSystemGL::ResetGLErrors()
{
  int count = 0;
  while (glGetError() != GL_NO_ERROR)
  {
    count++;
    if (count >= 100)
    {
      CLog::Log(LOGWARNING, "CRenderSystemGL::ResetGLErrors glGetError didn't return GL_NO_ERROR after %i iterations", count);
      break;
    }
  }
}

void CRenderSystemGL::DrawSceneGraphImpl( const CSceneGraph *sceneGraph, const CDirtyRegionList *dirtyRegions)
{
  unsigned int alpha, range = 0;

  if(g_Windowing.UseLimitedColor())
    range = 235 - 16;
  else
    range = 255 -  0;

  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
  glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE0);
  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
  
  for(CSceneGraph::const_iterator i = sceneGraph->begin(); i != sceneGraph->end(); ++i)
  {
    CBaseTexture *texture = (CBaseTexture*)(*i)->GetTexture();
    CBaseTexture *diffuseTexture = (CBaseTexture*)(*i)->GetDiffuseTexture();
    if (texture)
      LoadToGPU(texture);
    if (diffuseTexture)
      LoadToGPU(diffuseTexture);
  }

  // Clear directly before the first draw in each frame
  if (m_needsClear)
  {
    if (dirtyRegions)
    {
      for (CDirtyRegionList::const_iterator region = dirtyRegions->begin(); region != dirtyRegions->end(); ++region)
      {
        SetScissors(*region);
        ClearBuffers(0);
        ResetScissors();
      }
    }
    else
      ClearBuffers(0);
    m_needsClear = false;
  }

  for(CSceneGraph::const_iterator i = sceneGraph->begin(); i != sceneGraph->end(); ++i)
  {
    bool enableAlpha = false;
    const PackedVerticesPtr vertices = (*i)->GetVertices();
    CBaseTexture *texture = (CBaseTexture*)(*i)->GetTexture();
    CBaseTexture *diffuseTexture = (CBaseTexture*)(*i)->GetDiffuseTexture();
    int32_t color = (*i)->GetColor();
    unsigned int triangleVerts = 6 * (vertices->size() / 4);
    unsigned char r,g,b,a = 0;
    GLushort idx[triangleVerts];
    GLushort *itr = idx;
    bool useSingleDiffuseColor = color != 0;
    for(unsigned int j=0; j < vertices->size(); j+=4)
    {
      *itr++ = j + 0;
      *itr++ = j + 1;
      *itr++ = j + 2;
      *itr++ = j + 0;
      *itr++ = j + 2;
      *itr++ = j + 3;
     }
  

    r = GET_R(color) * range / 255;
    g = GET_G(color) * range / 255;
    b = GET_B(color) * range / 255;
    a = GET_A(color);

    if (texture)
      BindToUnit(texture, 0);

    // alpha blending
    alpha=GET_A(color);
    enableAlpha = alpha < 255 || ( texture && texture->HasAlpha());


    // diffuse color
    if (useSingleDiffuseColor)
      glColor4ub((GLubyte)r, (GLubyte)g, (GLubyte)b, (GLubyte)a);
    else
    {
      glEnableClientState(GL_COLOR_ARRAY);
      glColorPointer   (4, GL_UNSIGNED_BYTE, sizeof(PackedVertex), (char*)&vertices->at(0) + offsetof(PackedVertex, r));
    }

    // optional diffuse texture
    if (diffuseTexture)
    {
      enableAlpha |= diffuseTexture->HasAlpha();
      BindToUnit(diffuseTexture, 1);
      glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
      glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PREVIOUS);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);

      // diffuse texture coords
      glTexCoordPointer(2, GL_FLOAT        , sizeof(PackedVertex), (char*)&vertices->at(0) + offsetof(PackedVertex, u2));
    }
    
    if (enableAlpha)
      glEnable(GL_BLEND);
    else
      glDisable(GL_BLEND);

    // screen coords
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer  (3, GL_FLOAT        , sizeof(PackedVertex), (char*)&vertices->at(0) + offsetof(PackedVertex, x));

    if (texture)
    {
      // texture coords
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(2, GL_FLOAT        , sizeof(PackedVertex), (char*)&vertices->at(0) + offsetof(PackedVertex, u1));
    }

    // draw
    if (dirtyRegions)
    {
      for (CDirtyRegionList::const_iterator region = dirtyRegions->begin(); region != dirtyRegions->end(); ++region)
      {
        SetScissors(*region);
        glDrawElements(GL_TRIANGLES, triangleVerts, GL_UNSIGNED_SHORT, idx);
        ResetScissors();
      }
    }
    else
    {
      glDrawElements(GL_TRIANGLES, triangleVerts, GL_UNSIGNED_SHORT, idx);
    }
    
    if (!useSingleDiffuseColor)
      glDisableClientState(GL_COLOR_ARRAY);

    // unbind diffuse texture
    if (diffuseTexture)
    {
      glActiveTexture(1);
      glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (texture)
    {
      glActiveTexture(0);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    if (texture || diffuseTexture)
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);

  }
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisableClientState(GL_VERTEX_ARRAY);

}

TextureObject CRenderSystemGL::CreateTextureObject() const
{
  if(!m_bRenderCreated)
    return NULL;
  GLuint *texture = new GLuint;
  glGenTextures(1, texture);
  return (TextureObject) texture;
}
void CRenderSystemGL::DestroyTextureObject(TextureObject texture)
{
  if(!m_bRenderCreated)
    return;
  if (texture)
    glDeleteTextures(1, (GLuint*) texture);
}


bool CRenderSystemGL::LoadToGPU(TextureObject texture, unsigned int width, unsigned int height, unsigned int pitch, unsigned int rows, unsigned int format, const unsigned char *pixels)
{
  unsigned int finalWidth = width;
  unsigned int finalHeight = height;

  // Bind the texture object
  VerifyGLState();
  if (!texture)
    return false;
  glBindTexture(GL_TEXTURE_2D, *((GLuint*)texture));

  // Set the texture's stretching properties
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  unsigned int maxSize = GetMaxTextureSize();
  if (height > maxSize)
  {
    CLog::Log(LOGERROR, "GL: Image height %d too big to fit into single texture unit, truncating to %u", height, maxSize);
    finalHeight = maxSize;
  }
  if (width > maxSize)
  {
    CLog::Log(LOGERROR, "GL: Image width %d too big to fit into single texture unit, truncating to %u", width, maxSize);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
    finalWidth = maxSize;
  }

  GLenum glformat = GL_BGRA;
  GLint numcomponents = GL_RGBA;

  switch (format)
  {
  case XB_FMT_DXT1:
    glformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    break;
  case XB_FMT_DXT3:
    glformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    break;
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    glformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    break;
  case XB_FMT_RGB8:
    glformat = GL_RGB;
    numcomponents = GL_RGB;
    break;
  case XB_FMT_A8:
    glformat = GL_ALPHA;
    numcomponents = GL_ALPHA;
    break;
  case XB_FMT_A8L8:
    glformat = GL_LUMINANCE_ALPHA;
    numcomponents = GL_LUMINANCE_ALPHA;
    break;
  case XB_FMT_A8R8G8B8:
  default:
    break;
  }

  VerifyGLState();
  if ((format & XB_FMT_DXT_MASK) == 0)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, numcomponents, finalWidth, finalHeight, 0,
      glformat, GL_UNSIGNED_BYTE, pixels);
  }
  else
  {
    // changed from glCompressedTexImage2D to support GL < 1.3
    glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, glformat,
      finalWidth, finalHeight, 0, pitch * rows, pixels);
  }

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  VerifyGLState();
  return true;
}

bool CRenderSystemGL::LoadToGPU(CBaseTexture *baseTexture)
{
  if (!baseTexture)
    return false;
  if (baseTexture->IsLoadedToGPU())
    return true;
  unsigned int width, height, pitch, rows, format;
  width = baseTexture->GetTextureWidth();
  height = baseTexture->GetTextureHeight();
  format = baseTexture->GetFormat();
  pitch = GetPitch(format, width);
  rows = GetRows(format, height);
  TextureObject object = baseTexture->GetTextureObject();
  if(!object)
  {
    object = CreateTextureObject();
    baseTexture->SetTextureObject(object);
  }
  if (LoadToGPU(object, width, height, pitch, rows, format, (const unsigned char*)baseTexture->GetPixels()))
    baseTexture->SetLoadedToGPU();
  VerifyGLState();
  return true;
}

void CRenderSystemGL::BindToUnit(CBaseTexture *baseTexture, unsigned int unit)
{
  TextureObject object = baseTexture->GetTextureObject();
  if (!object)
    return;
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, *((GLuint*)object));
  glClientActiveTexture(GL_TEXTURE0 + unit);
  glEnable(GL_TEXTURE_2D);
  VerifyGLState();
}

#endif
