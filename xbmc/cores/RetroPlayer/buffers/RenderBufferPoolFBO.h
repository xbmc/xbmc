/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "BaseRenderBufferPool.h"
#include "RenderBufferFBO.h"

#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>

namespace KODI
{
namespace RETRO
{
class CRenderContext;
class IHwRenderingContext;

/*!
 * \brief Framebuffers for game clients that render on the GPU themselves
 *
 * The pool owns a shared GL context and a stable client framebuffer. Completed
 * frames are blitted into separate, pooled capture buffers for the renderer.
 * Desktop GL or GLES 3 is required for framebuffer blits and fence syncs.
 */
class CRenderBufferPoolFBO : public CBaseRenderBufferPool
{
public:
  CRenderBufferPoolFBO(CRenderContext& context);
  CRenderBufferPoolFBO(
      CRenderContext& context,
      std::unique_ptr<IHwRenderingContext> hwContext,
      std::shared_ptr<CRenderBufferFBO::Sync> sync = std::make_shared<CRenderBufferFBO::Sync>());
  ~CRenderBufferPoolFBO() override;

  // Implementation of IRenderBufferPool via CBaseRenderBufferPool
  bool IsCompatible(const CRenderVideoSettings& renderSettings) const override;
  IRenderBuffer* GetBuffer(unsigned int width, unsigned int height) override;
  void Return(IRenderBuffer* buffer) override;
  void Flush() override;

  bool SupportsHardwareRendering() const override;
  bool CreateContext(const HwContextProperties& properties) override;
  bool BeginClientFrame() override;
  void EndClientFrame() override;
  void DestroyContext() override;
  void FlushRendered() override;

  IRenderBuffer* CaptureClientFrame(IRenderBuffer* clientBuffer,
                                    unsigned int width,
                                    unsigned int height) override;

protected:
  // Implementation of CBaseRenderBufferPool
  IRenderBuffer* CreateRenderBuffer(void* header = nullptr) override;
  bool ConfigureInternal() override;

  IRenderBuffer* GetCaptureBuffer(unsigned int width, unsigned int height);

  // Construction parameters
  CRenderContext& m_context;

  // Configuration parameters
  HwContextProperties m_contextProperties;

  // Context operations are serialized independently of Kodi's render thread.
  mutable std::recursive_mutex m_contextMutex;
  unsigned int m_clientFrameDepth{0};
  std::thread::id m_clientThread;

private:
  friend class CRenderBufferFBO;

  struct RenderedBuffers
  {
    void Flush(std::optional<uint64_t> expectedGeneration = {});
    std::mutex mutex;
    std::condition_variable submitted;
    uint64_t generation{0};
    std::vector<std::shared_ptr<CRenderBufferFBO::Resources>> buffers;
  };

  void MarkRendered(const std::shared_ptr<CRenderBufferFBO::Resources>& resources);
  const std::shared_ptr<CRenderBufferFBO::Sync> m_sync;
  const std::shared_ptr<RenderedBuffers> m_rendered = std::make_shared<RenderedBuffers>();
  std::unique_ptr<IHwRenderingContext> m_hwContext;
  CRenderBufferFBO* CreateFBO(CRenderBufferFBO::Type type);
  void CollectBuffers();
  std::vector<std::shared_ptr<CRenderBufferFBO::Resources>> m_resources;
  std::mutex m_captureMutex;
  std::vector<std::unique_ptr<IRenderBuffer>> m_pending;
  unsigned int m_captureWidth{0};
  unsigned int m_captureHeight{0};
};
} // namespace RETRO
} // namespace KODI
