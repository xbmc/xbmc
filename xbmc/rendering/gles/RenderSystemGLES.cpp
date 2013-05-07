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

#if HAS_GLES == 2

#include "guilib/GraphicContext.h"
#include "settings/AdvancedSettings.h"
#include "RenderSystemGLES.h"
#include "guilib/MatrixGLES.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "utils/TimeUtils.h"
#include "utils/SystemInfo.h"
#include "utils/MathUtils.h"
#include "guilib/Texture.h"
#include "threads/Thread.h"
#include "rendering/SceneGraph.h"
static const char* ShaderNames[SM_ESHADERCOUNT] =
    {"guishader_frag_default.glsl",
     "guishader_frag_texture.glsl",
     "guishader_frag_multi.glsl",
     "guishader_frag_fonts.glsl",
     "guishader_frag_texture_noblend.glsl",
     "guishader_frag_multi_blendcolor.glsl",
     "guishader_frag_rgba.glsl",
     "guishader_frag_rgba_blendcolor.glsl"
    };

CRenderSystemGLES::CRenderSystemGLES()
 : CRenderSystemBase()
 , m_pGUIshader(0)
 , m_method(SM_NONE)
{
  m_enumRenderingSystem = RENDERING_SYSTEM_OPENGLES;
}

CRenderSystemGLES::~CRenderSystemGLES()
{
}

bool CRenderSystemGLES::InitRenderSystem()
{
  GLint maxTextureSize;

  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

  m_maxTextureSize = maxTextureSize;
  m_bVSync = false;
  m_iVSyncMode = 0;
  m_iSwapStamp = 0;
  m_iSwapTime = 0;
  m_iSwapRate = 0;
  m_bVsyncInit = false;
  m_renderCaps = 0;
  m_needsClear = true;
  // Get the GLES version number
  m_RenderVersionMajor = 0;
  m_RenderVersionMinor = 0;
  const char* ver = (const char*)glGetString(GL_VERSION);
  if (ver != 0)
  {
    sscanf(ver, "%d.%d", &m_RenderVersionMajor, &m_RenderVersionMinor);
    if (!m_RenderVersionMajor)
      sscanf(ver, "%*s %*s %d.%d", &m_RenderVersionMajor, &m_RenderVersionMinor);
    m_RenderVersion = ver;
  }
  
  // Get our driver vendor and renderer
  m_RenderVendor = (const char*) glGetString(GL_VENDOR);
  m_RenderRenderer = (const char*) glGetString(GL_RENDERER);

  m_RenderExtensions  = " ";
  m_RenderExtensions += (const char*) glGetString(GL_EXTENSIONS);
  m_RenderExtensions += " ";

  LogGraphicsInfo();
  
  if (IsExtSupported("GL_TEXTURE_NPOT"))
  {
    m_renderCaps |= RENDER_CAPS_NPOT;
  }

  if (IsExtSupported("GL_EXT_texture_format_BGRA8888"))
  {
    m_renderCaps |= RENDER_CAPS_BGRA;
  }

  if (IsExtSupported("GL_IMG_texture_format_BGRA8888"))
  {
    m_renderCaps |= RENDER_CAPS_BGRA;
  }

  if (IsExtSupported("GL_APPLE_texture_format_BGRA8888"))
  {
    m_renderCaps |= RENDER_CAPS_BGRA_APPLE;
  }



  m_bRenderCreated = true;
  
  InitialiseGUIShader();

  return true;
}

bool CRenderSystemGLES::ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate)
{
  m_width = width;
  m_height = height;
  
  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
  CalculateMaxTexturesize();

  CRect rect( 0, 0, width, height );
  SetViewPort( rect );

  glEnable(GL_SCISSOR_TEST); 

  g_matrices.MatrixMode(MM_PROJECTION);
  g_matrices.LoadIdentity();

#ifdef TARGET_RASPBERRY_PI
  g_matrices.Ortho(0.0f, width-1, height-1, 0.0f, +1.0f, 1.0f);
#else
  g_matrices.Ortho(0.0f, width-1, height-1, 0.0f, -1.0f, 1.0f);
#endif

  g_matrices.MatrixMode(MM_MODELVIEW);
  g_matrices.LoadIdentity();
  
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);          // Turn Blending On
  glDisable(GL_DEPTH_TEST);  
    
  return true;
}

bool CRenderSystemGLES::DestroyRenderSystem()
{
  CLog::Log(LOGDEBUG, "GUI Shader - Destroying Shader : %p", m_pGUIshader);

  if (m_pGUIshader)
  {
    for (int i = 0; i < SM_ESHADERCOUNT; i++)
    {
      if (m_pGUIshader[i])
      {
        m_pGUIshader[i]->Free();
        delete m_pGUIshader[i];
        m_pGUIshader[i] = NULL;
      }
    }
    delete[] m_pGUIshader;
    m_pGUIshader = NULL;
  }

  m_bRenderCreated = false;

  return true;
}

bool CRenderSystemGLES::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGLES::EndRender()
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGLES::ClearBuffers(color_t color)
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

bool CRenderSystemGLES::IsExtSupported(const char* extension)
{
  if (strcmp( extension, "GL_EXT_framebuffer_object" ) == 0)
  {
    // GLES has FBO as a core element, not an extension!
    return true;
  }
  else if (strcmp( extension, "GL_TEXTURE_NPOT" ) == 0)
  {
    // GLES supports non-power-of-two textures as standard.
	return true;
	/* Note: The wrap mode can only be GL_CLAMP_TO_EDGE and the minification filter can only be
	 * GL_NEAREST or GL_LINEAR (in other words, not mipmapped). The extension GL_OES_texture_npot
	 * relaxes these restrictions and allows wrap modes of GL_REPEAT and GL_MIRRORED_REPEAT and
	 * also	allows npot textures to be mipmapped with the full set of minification filters
	 */
  }
  else
  {
    CStdString name;
    name  = " ";
    name += extension;
    name += " ";

    bool supported = m_RenderExtensions.find(name) != std::string::npos;
    CLog::Log(LOGDEBUG, "GLES: Extension Support Test - %s %s", extension, supported ? "YES" : "NO");
    return supported;
  }
}

static int64_t abs64(int64_t a)
{
  if(a < 0)
    return -a;
  return a;
}

bool CRenderSystemGLES::PresentRender(const CDirtyRegionList &dirty)
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

    if (abs64(diff - m_iSwapRate) < abs64(diff))
      CLog::Log(LOGDEBUG, "%s - missed requested swap",__FUNCTION__);
  }
  m_needsClear = true; 
  return result;
}

void CRenderSystemGLES::SetVSync(bool enable)
{
  if (m_bVSync==enable && m_bVsyncInit == true)
    return;

  if (!m_bRenderCreated)
    return;
  
  if (enable)
    CLog::Log(LOGINFO, "GLES: Enabling VSYNC");
  else
    CLog::Log(LOGINFO, "GLES: Disabling VSYNC");

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
      CLog::Log(LOGINFO, "GLES: Using artificial vsync sleep with rate %f", rate);
      if(!m_iVSyncMode)
        m_iVSyncMode = 1;
    }
  }
    
  if (!m_iVSyncMode)
    CLog::Log(LOGERROR, "GLES: Vertical Blank Syncing unsupported");
  else
    CLog::Log(LOGINFO, "GLES: Selected vsync mode %d", m_iVSyncMode);
}

void CRenderSystemGLES::CaptureStateBlock()
{
  if (!m_bRenderCreated)
    return;

  g_matrices.MatrixMode(MM_PROJECTION);
  g_matrices.PushMatrix();
  g_matrices.MatrixMode(MM_TEXTURE);
  g_matrices.PushMatrix();
  g_matrices.MatrixMode(MM_MODELVIEW);
  g_matrices.PushMatrix();
  glDisable(GL_SCISSOR_TEST); // fixes FBO corruption on Macs
  glActiveTexture(GL_TEXTURE0);
//TODO - NOTE: Only for Screensavers & Visualisations
//  glColor3f(1.0, 1.0, 1.0);
}

void CRenderSystemGLES::ApplyStateBlock()
{
  if (!m_bRenderCreated)
    return;

  g_matrices.MatrixMode(MM_PROJECTION);
  g_matrices.PopMatrix();
  g_matrices.MatrixMode(MM_TEXTURE);
  g_matrices.PopMatrix();
  g_matrices.MatrixMode(MM_MODELVIEW);
  g_matrices.PopMatrix();
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_BLEND);
  glEnable(GL_SCISSOR_TEST);  
  glClear(GL_DEPTH_BUFFER_BIT);
}

void CRenderSystemGLES::SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight)
{ 
  if (!m_bRenderCreated)
    return;
  
  g_graphicsContext.BeginPaint();
  
  CPoint offset = camera - CPoint(screenWidth*0.5f, screenHeight*0.5f);
  
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  float w = (float)viewport[2]*0.5f;
  float h = (float)viewport[3]*0.5f;

  g_matrices.MatrixMode(MM_MODELVIEW);
  g_matrices.LoadIdentity();
  g_matrices.Translatef(-(viewport[0] + w + offset.x), +(viewport[1] + h + offset.y), 0);
  g_matrices.LookAt(0.0, 0.0, -2.0*h, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0);
  g_matrices.MatrixMode(MM_PROJECTION);
  g_matrices.LoadIdentity();
  g_matrices.Frustum( (-w - offset.x)*0.5f, (w - offset.x)*0.5f, (-h + offset.y)*0.5f, (h + offset.y)*0.5f, h, 100*h);
  g_matrices.MatrixMode(MM_MODELVIEW);

  glGetIntegerv(GL_VIEWPORT, m_viewPort);
  GLfloat* matx;
  matx = g_matrices.GetMatrix(MM_MODELVIEW);
  memcpy(m_view, matx, 16 * sizeof(GLfloat));
  matx = g_matrices.GetMatrix(MM_PROJECTION);
  memcpy(m_projection, matx, 16 * sizeof(GLfloat));

  g_graphicsContext.EndPaint();
}

void CRenderSystemGLES::Project(float &x, float &y, float &z)
{
  GLfloat coordX, coordY, coordZ;
  if (g_matrices.Project(x, y, z, m_view, m_projection, m_viewPort, &coordX, &coordY, &coordZ))
  {
    x = coordX;
    y = (float)(m_viewPort[3] - coordY);
    z = 0;
  }
}

bool CRenderSystemGLES::TestRender()
{
  static float theta = 0.0;

  //RESOLUTION_INFO resInfo = CDisplaySettings::Get().GetCurrentResolutionInfo();
  //glViewport(0, 0, resInfo.iWidth, resInfo.iHeight);

  g_matrices.PushMatrix();
  g_matrices.Rotatef( theta, 0.0f, 0.0f, 1.0f );

  EnableGUIShader(SM_DEFAULT);

  GLfloat col[3][4];
  GLfloat ver[3][2];
  GLint   posLoc = GUIShaderGetPos();
  GLint   colLoc = GUIShaderGetCol();

  glVertexAttribPointer(posLoc,  2, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(colLoc,  4, GL_FLOAT, 0, 0, col);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);

  // Setup Colour values
  col[0][0] = col[0][3] = col[1][1] = col[1][3] = col[2][2] = col[2][3] = 1.0f;
  col[0][1] = col[0][2] = col[1][0] = col[1][2] = col[2][0] = col[2][1] = 0.0f;

  // Setup vertex position values
  ver[0][0] =  0.0f;
  ver[0][1] =  1.0f;
  ver[1][0] =  0.87f;
  ver[1][1] = -0.5f;
  ver[2][0] = -0.87f;
  ver[2][1] = -0.5f;

  glDrawArrays(GL_TRIANGLES, 0, 3);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);

  DisableGUIShader();

  g_matrices.PopMatrix();

  theta += 1.0f;

  return true;
}

void CRenderSystemGLES::ApplyHardwareTransform(const TransformMatrix &finalMatrix)
{ 
  if (!m_bRenderCreated)
    return;

  g_matrices.MatrixMode(MM_MODELVIEW);
  g_matrices.PushMatrix();
  GLfloat matrix[4][4];

  for(int i = 0; i < 3; i++)
    for(int j = 0; j < 4; j++)
      matrix[j][i] = finalMatrix.m[i][j];

  matrix[0][3] = 0.0f;
  matrix[1][3] = 0.0f;
  matrix[2][3] = 0.0f;
  matrix[3][3] = 1.0f;

  g_matrices.MultMatrixf(&matrix[0][0]);
}

void CRenderSystemGLES::RestoreHardwareTransform()
{
  if (!m_bRenderCreated)
    return;

  g_matrices.MatrixMode(MM_MODELVIEW);
  g_matrices.PopMatrix();
}

void CRenderSystemGLES::CalculateMaxTexturesize()
{
  // GLES cannot do PROXY textures to determine maximum size,
  CLog::Log(LOGINFO, "GLES: Maximum texture width: %u", m_maxTextureSize);
}

void CRenderSystemGLES::GetViewPort(CRect& viewPort)
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

// FIXME make me const so that I can accept temporary objects
void CRenderSystemGLES::SetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  glScissor((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
  glViewport((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
}

void CRenderSystemGLES::SetScissors(const CRect &rect)
{
  if (!m_bRenderCreated)
    return;
  GLint x1 = MathUtils::round_int(rect.x1);
  GLint y1 = MathUtils::round_int(rect.y1);
  GLint x2 = MathUtils::round_int(rect.x2);
  GLint y2 = MathUtils::round_int(rect.y2);
  glScissor(x1, m_height - y2, x2-x1, y2-y1);
}

void CRenderSystemGLES::ResetScissors()
{
  SetScissors(CRect(0, 0, (float)m_width, (float)m_height));
}

void CRenderSystemGLES::InitialiseGUIShader()
{
  if (!m_pGUIshader)
  {
    m_pGUIshader = new CGUIShader*[SM_ESHADERCOUNT];
    for (int i = 0; i < SM_ESHADERCOUNT; i++)
    {
      m_pGUIshader[i] = new CGUIShader( ShaderNames[i] );

      if (!m_pGUIshader[i]->CompileAndLink())
      {
        m_pGUIshader[i]->Free();
        delete m_pGUIshader[i];
        m_pGUIshader[i] = NULL;
        CLog::Log(LOGERROR, "GUI Shader [%s] - Initialise failed", ShaderNames[i]);
      }
      else
      {
        CLog::Log(LOGDEBUG, "GUI Shader [%s]- Initialise successful : %p", ShaderNames[i], m_pGUIshader[i]);
      }
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "GUI Shader - Tried to Initialise again. Was this intentional?");
  }
}

void CRenderSystemGLES::EnableGUIShader(ESHADERMETHOD method)
{
  if (method == m_method)
    return;
  if (m_pGUIshader[method])
  {
    m_pGUIshader[method]->Enable();
    m_method = method;
  }
  else
  {
    CLog::Log(LOGERROR, "Invalid GUI Shader selected - [%s]", ShaderNames[(int)method]);
  }
}

void CRenderSystemGLES::DisableGUIShader()
{
  if (m_method != SM_NONE && m_pGUIshader[m_method])
  {
    m_pGUIshader[m_method]->Disable();
  }
  m_method = SM_NONE;
}

GLint CRenderSystemGLES::GUIShaderGetPos()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetPosLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCol()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetColLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCoord0()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetCord0Loc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCoord1()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetCord1Loc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetUniCol()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetUniColLoc();

  return -1;
}

void CRenderSystemGLES::DrawSceneGraphImpl(const CSceneGraph *sceneGraph, const CDirtyRegionList *dirtyRegions)
{

  if (!m_bRenderCreated)
    return;
  int range;

  if(g_Windowing.UseLimitedColor())
    range = 235 - 16;
  else
    range = 255 -  0;

  GLint posLoc  = -1;
  GLint tex0Loc = -1;
  GLint tex1Loc = -1;
  GLint uniColLoc = -1;

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
    unsigned int r,g,b,a = 0;

    const PackedVerticesPtr vertices = (*i)->GetVertices();
    CBaseTexture *texture = (CBaseTexture*)(*i)->GetTexture();
    CBaseTexture *diffuseTexture = (CBaseTexture*)(*i)->GetDiffuseTexture();
    int32_t color = (*i)->GetColor();
    unsigned int triangleVerts = 6 * (vertices->size() / 4);
    GLushort idx[triangleVerts];
    GLushort *itr = idx;
    for(unsigned int j=0; j < vertices->size(); j+=4)
    {
      *itr++ = j + 0;
      *itr++ = j + 1;
      *itr++ = j + 2;
      *itr++ = j + 0;
      *itr++ = j + 2;
      *itr++ = j + 3;
     }

    if (texture)
      BindToUnit(texture, 0);

    r = GET_R(color) * range / 255;
    g = GET_G(color) * range / 255;
    b = GET_B(color) * range / 255;
    a = GET_A(color);

    bool hasAlpha = a < 255 || (texture && texture->HasAlpha());

    if (diffuseTexture)
    {
      BindToUnit(diffuseTexture, 1);

      hasAlpha |= diffuseTexture->HasAlpha();

      if (r == 255 && g == 255 && b == 255 && a == 255 )
      {
        EnableGUIShader(SM_MULTI);
      }
      else
      {
        EnableGUIShader(SM_MULTI_BLENDCOLOR);
      }
    }
    else if (texture)
    {
      EnableGUIShader(SM_TEXTURE);
    }
    else
    {
      EnableGUIShader(SM_DEFAULT);
    }

    posLoc  = GUIShaderGetPos();
    tex0Loc = GUIShaderGetCoord0();
    tex1Loc = GUIShaderGetCoord1();
    uniColLoc = GUIShaderGetUniCol();

    glUniform4f(uniColLoc, (r / 255.0f), (g / 255.0f), (b / 255.0f), (a / 255.0f));

    if (diffuseTexture)
    {
      glVertexAttribPointer(tex1Loc, 2, GL_FLOAT,         GL_FALSE, sizeof(PackedVertex), (char*)&vertices->at(0) + offsetof(PackedVertex, u2));
      glEnableVertexAttribArray(tex1Loc);
    }

    if ( hasAlpha )
    {
      glEnable( GL_BLEND );
      glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    }
    else
    {
      glDisable(GL_BLEND);
    }

    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc,  3, GL_FLOAT,         GL_FALSE, sizeof(PackedVertex), (char*)&vertices->at(0) + offsetof(PackedVertex, x));
    if (texture)
    {
      glEnableVertexAttribArray(tex0Loc);
      glVertexAttribPointer(tex0Loc, 2, GL_FLOAT,         GL_FALSE, sizeof(PackedVertex), (char*)&vertices->at(0) + offsetof(PackedVertex, u1));
    }

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
    // Don't reset state until we're done with all batches. Only reset what
    // could affect the next set of textures.
    // TODO: Create wrappers for things like blending on/off and shader
    // switching to prevent unnecessary changes.

    if (diffuseTexture)
    {
      glDisableVertexAttribArray(tex1Loc);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (texture)
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, 0);
      glDisableVertexAttribArray(tex0Loc);
    }
    glDisableVertexAttribArray(posLoc);
  }

  glDisableVertexAttribArray(tex0Loc);
  glDisableVertexAttribArray(tex1Loc);
  DisableGUIShader();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

TextureObject CRenderSystemGLES::CreateTextureObject() const
{
  if(!m_bRenderCreated)
    return NULL;
  GLuint *texture = new GLuint;
  glGenTextures(1, texture);
  return (TextureObject) texture;
}

void CRenderSystemGLES::DestroyTextureObject(TextureObject texture)
{
  if(!m_bRenderCreated)
    return;
  if (texture)
  {
    glDeleteTextures(1, (GLuint*) texture);
    delete (GLuint*)texture;
  }
}

bool CRenderSystemGLES::LoadToGPU(TextureObject texture, unsigned int width, unsigned int height, unsigned int pitch, unsigned int rows, unsigned int format, const unsigned char *pixels)
{
  
  if (!m_bRenderCreated)
    return false;
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
    finalWidth = maxSize;
  }

  // All incoming textures are BGRA, which GLES does not necessarily support.
  // Some (most?) hardware supports BGRA textures via an extension.
  // If not, we convert to RGBA first to avoid having to swizzle in shaders.
  // Explicitly define GL_BGRA_EXT here in the case that it's not defined by
  // system headers, and trust the extension list instead.
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

  GLint internalformat;
  GLenum pixelformat;

  switch (format)
  {
    default:
    case XB_FMT_RGBA8:
      internalformat = pixelformat = GL_RGBA;
      break;
    case XB_FMT_RGB8:
      internalformat = pixelformat = GL_RGB;
      break;
    case XB_FMT_A8:
      internalformat = pixelformat = GL_ALPHA;
      break;
    case XB_FMT_A8L8:
      internalformat = pixelformat = GL_LUMINANCE_ALPHA;
      break;
    case XB_FMT_A8R8G8B8:
      if (SupportsBGRA())
      {
        internalformat = pixelformat = GL_BGRA_EXT;
      }
      else if (SupportsBGRAApple())
      {
        // Apple's implementation does not conform to spec. Instead, they require
        // differing format/internalformat, more like GL.
        internalformat = GL_RGBA;
        pixelformat = GL_BGRA_EXT;
      }
      else
      {
        SwapBlueRed(pixels, height, pitch);
        internalformat = pixelformat = GL_RGBA;
      }
      break;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, internalformat, finalWidth, finalHeight, 0,
    pixelformat, GL_UNSIGNED_BYTE, pixels);


  VerifyGLState();
  return true;
}

bool CRenderSystemGLES::LoadToGPU(CBaseTexture *baseTexture)
{
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

void CRenderSystemGLES::BindToUnit(CBaseTexture *baseTexture, unsigned int unit)
{
  TextureObject object = baseTexture->GetTextureObject();
  if (!object)
    return;
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, *((GLuint*)object));
  VerifyGLState();
}

#endif
