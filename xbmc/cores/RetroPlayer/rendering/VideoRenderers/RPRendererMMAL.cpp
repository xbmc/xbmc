/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "RPRendererMMAL.h"
#include "MMALProcess.h"
#include "MMALRenderer.h"
#include "cores/RetroPlayer/process/rbpi/RenderBufferMMAL.h"
#include "cores/RetroPlayer/process/rbpi/RenderBufferPoolMMAL.h"
#include "threads/SingleLock.h"
#include "platform/linux/RBP.h"

using namespace KODI;
using namespace RETRO;

// --- CRendererFactoryMMAL ----------------------------------------------------

CRPBaseRenderer *CRendererFactoryMMAL::CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererMMAL(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryMMAL::CreateBufferPools(CRenderContext &context)
{
  return { std::make_shared<CRenderBufferPoolMMAL>() };
}

// --- CRPRendererMMAL ---------------------------------------------------------

CRPRendererMMAL::CRPRendererMMAL(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) :
  CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
}

CRPRendererMMAL::~CRPRendererMMAL()
{
  Deinitialize();
}

bool CRPRendererMMAL::Supports(ERENDERFEATURE feature) const
{
  if (feature == RENDERFEATURE_STRETCH         ||
      feature == RENDERFEATURE_ZOOM            ||
      feature == RENDERFEATURE_ROTATION        ||
      feature == RENDERFEATURE_VERTICAL_SHIFT  ||
      feature == RENDERFEATURE_PIXEL_RATIO)
    return true;

  return false;
}

bool CRPRendererMMAL::SupportsScalingMethod(ESCALINGMETHOD method)
{
  if (method == VS_SCALINGMETHOD_LINEAR)
    return true;

  return false;
}

ESCALINGMETHOD CRPRendererMMAL::GetDefaultScalingMethod() const
{
  return VS_SCALINGMETHOD_LINEAR; //! @todo Add nearest neighbor support
}

void CRPRendererMMAL::Deinitialize()
{
  CSingleLock lock(m_mutex);

  if (m_renderer)
    m_renderer->UnregisterProcess();
  m_process.reset();
  m_renderer.reset();
}

bool CRPRendererMMAL::ConfigureInternal()
{
  CSingleLock lock(m_mutex);

  //! @todo
  m_bufferPool->Configure(m_format, m_sourceWidth, m_sourceHeight);

  m_renderer.reset(new CMMALRenderer(m_context, m_bufferPool.get()));
  m_process.reset(new CMMALProcess(m_bufferPool.get(), m_renderer.get()));
  m_renderer->RegisterProcess(m_process.get());
  return true;
}

void CRPRendererMMAL::RenderInternal(bool clear, uint8_t alpha)
{
  CSingleLock lock(m_mutex);

  if (!m_bufferPool)
    return;

  if (m_renderBuffer == nullptr)
    return;

  // We only want to upload frames once
  if (m_renderBuffer->IsRendered())
  {
    m_renderer->SetVideoRect();
    return;
  }

  m_renderBuffer->Acquire();
  m_process->Put(static_cast<CRenderBufferMMAL*>(m_renderBuffer));

  m_renderBuffer = nullptr;
}

void CRPRendererMMAL::FlushInternal()
{
  CSingleLock lock(m_mutex);

  if (m_renderer)
    m_renderer->Flush();
}

void CRPRendererMMAL::ManageRenderArea()
{
  CRPBaseRenderer::ManageRenderArea();

  if (m_renderer)
  {
    CRect source;
    CRect dest;
    CRect view;
    GetVideoRect(source, dest, view);
    m_renderer->SetDimensions(source, dest);
  }
}
