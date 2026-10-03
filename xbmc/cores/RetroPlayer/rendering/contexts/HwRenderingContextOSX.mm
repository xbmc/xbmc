/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HwRenderingContextOSX.h"

#include "IHwRenderingContext.h"
#include "cores/RetroPlayer/buffers/IRenderBufferPool.h"
#include "utils/log.h"

#import <AppKit/NSOpenGL.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>

using namespace KODI::RETRO;

namespace
{
class CPreviousContext
{
public:
  CPreviousContext()
    : m_nsContext([NSOpenGLContext currentContext]),
      m_cglContext(CGLGetCurrentContext())
  {
    if (m_cglContext)
      CGLRetainContext(m_cglContext);
  }

  ~CPreviousContext()
  {
    if (!m_restored)
      Restore();
    if (m_cglContext)
      CGLReleaseContext(m_cglContext);
  }

  bool Restore()
  {
    if (m_nsContext)
      [m_nsContext makeCurrentContext];
    else
      [NSOpenGLContext clearCurrentContext];

    // CGL callers can have a current context without an NSOpenGLContext wrapper.
    if (CGLGetCurrentContext() != m_cglContext)
      CGLSetCurrentContext(m_cglContext);
    m_restored = true;
    if ([NSOpenGLContext currentContext] == m_nsContext && CGLGetCurrentContext() == m_cglContext)
      return true;

    [NSOpenGLContext clearCurrentContext];
    CGLSetCurrentContext(nullptr);
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to restore the native OpenGL context");
    return false;
  }

private:
  NSOpenGLContext* m_nsContext;
  CGLContextObj m_cglContext;
  bool m_restored{false};
};

bool BindContext(NSOpenGLContext* context)
{
  [context makeCurrentContext];
  return [NSOpenGLContext currentContext] == context &&
         CGLGetCurrentContext() == [context CGLContextObj];
}

class CHwRenderingContextOSX : public IHwRenderingContext
{
public:
  explicit CHwRenderingContextOSX(NSOpenGLContext* shareContext) : m_shareContext(shareContext)
  {
    m_supported = CreateSharedContext();
  }

  ~CHwRenderingContextOSX() override { Destroy(); }

  bool SupportsHardwareRendering() const override { return m_supported; }

  bool Create(const HwContextProperties& properties) override
  {
    if (properties.debugContext)
    {
      CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Native macOS OpenGL cannot create a debug context");
      return false;
    }

    if (!m_supported || m_created || properties.embedded || !properties.coreProfile)
      return false;

    if (!m_context && !CreateSharedContext())
      return false;

    if (properties.versionMajor > m_major ||
        (properties.versionMajor == m_major && properties.versionMinor > m_minor))
    {
      CLog::Log(LOGERROR,
                "RetroPlayer[RENDER]: Requested OpenGL {}.{} core exceeds native OpenGL {}.{}",
                properties.versionMajor, properties.versionMinor, m_major, m_minor);
      return false;
    }

    m_created = true;
    CLog::Log(LOGINFO,
              "RetroPlayer[RENDER]: Created OpenGL {}.{} core context for game client, sharing "
              "Kodi's objects (requested minimum {}.{})",
              m_major, m_minor, properties.versionMajor, properties.versionMinor);
    return true;
  }

  bool IsCreated() const override { return m_created; }

  bool MakeCurrent() override
  {
    if (!m_created || !m_context || m_previousContext)
      return false;

    auto previousContext = std::make_unique<CPreviousContext>();
    if (!BindContext(m_context))
      return false;

    m_previousContext = std::move(previousContext);
    return true;
  }

  void RestoreCurrent() override { m_previousContext.reset(); }

  void Destroy() override
  {
    RestoreCurrent();
    m_context = nil;
    m_created = false;
  }

private:
  bool CreateSharedContext()
  {
    if (!m_shareContext || ![m_shareContext CGLContextObj])
      return false;

    // Identical pixel formats preserve the profile and virtual-screen list required for sharing.
    NSOpenGLPixelFormat* format = [m_shareContext pixelFormat];
    GLint profile = 0;
    [format getValues:&profile forAttribute:NSOpenGLPFAOpenGLProfile forVirtualScreen:0];
    if (profile != NSOpenGLProfileVersion3_2Core && profile != NSOpenGLProfileVersion4_1Core)
      return false;

    NSOpenGLContext* context = [[NSOpenGLContext alloc] initWithFormat:format
                                                          shareContext:m_shareContext];
    if (!context)
      return false;
    [context setCurrentVirtualScreen:[m_shareContext currentVirtualScreen]];

    GLint accelerated = 0;
    [format getValues:&accelerated
            forAttribute:NSOpenGLPFAAccelerated
        forVirtualScreen:[context currentVirtualScreen]];
    const CGLShareGroupObj shareGroup = CGLGetShareGroup([context CGLContextObj]);
    if (!accelerated || !shareGroup ||
        shareGroup != CGLGetShareGroup([m_shareContext CGLContextObj]))
      return false;

    CPreviousContext previousContext;
    if (!BindContext(context))
      return false;

    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
    const bool supported =
        (major > 3 || (major == 3 && minor >= 2)) && (profile & GL_CONTEXT_CORE_PROFILE_BIT) != 0;
    if (!previousContext.Restore() || !supported)
      return false;

    m_major = major;
    m_minor = minor;
    m_context = context;
    return true;
  }

  NSOpenGLContext* m_shareContext;
  NSOpenGLContext* m_context;
  std::unique_ptr<CPreviousContext> m_previousContext;
  unsigned int m_major{0};
  unsigned int m_minor{0};
  bool m_supported{false};
  bool m_created{false};
};
} // namespace

std::unique_ptr<IHwRenderingContext> KODI::RETRO::CreateHwRenderingContextOSX(
    NSOpenGLContext* shareContext)
{
  return std::make_unique<CHwRenderingContextOSX>(shareContext);
}
