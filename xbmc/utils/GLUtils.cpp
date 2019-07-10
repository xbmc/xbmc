/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GLUtils.h"

#include "ServiceBroker.h"
#include "log.h"
#include "rendering/MatrixGL.h"
#include "rendering/RenderSystem.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"

#include <map>
#include <utility>

namespace
{

#define X(VAL) std::make_pair(VAL, #VAL)
std::map<GLenum, const char*> glErrors =
{
  // please keep attributes in accordance to:
  // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetError.xhtml
  X(GL_NO_ERROR),
  X(GL_INVALID_ENUM),
  X(GL_INVALID_VALUE),
  X(GL_INVALID_OPERATION),
  X(GL_INVALID_FRAMEBUFFER_OPERATION),
  X(GL_OUT_OF_MEMORY),
#if defined(HAS_GL)
  X(GL_STACK_UNDERFLOW),
  X(GL_STACK_OVERFLOW),
#endif
};

std::map<GLenum, const char*> glErrorSource =
{
//! @todo remove TARGET_RASPBERRY_PI when Raspberry Pi updates their GL headers
#if defined(HAS_GLES) && defined(TARGET_LINUX) && !defined(TARGET_RASPBERRY_PI)
  X(GL_DEBUG_SOURCE_API_KHR),
  X(GL_DEBUG_SOURCE_WINDOW_SYSTEM_KHR),
  X(GL_DEBUG_SOURCE_SHADER_COMPILER_KHR),
  X(GL_DEBUG_SOURCE_THIRD_PARTY_KHR),
  X(GL_DEBUG_SOURCE_APPLICATION_KHR),
  X(GL_DEBUG_SOURCE_OTHER_KHR),
#endif
};

std::map<GLenum, const char*> glErrorType =
{
//! @todo remove TARGET_RASPBERRY_PI when Raspberry Pi updates their GL headers
#if defined(HAS_GLES) && defined(TARGET_LINUX) && !defined(TARGET_RASPBERRY_PI)
  X(GL_DEBUG_TYPE_ERROR_KHR),
  X(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_KHR),
  X(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_KHR),
  X(GL_DEBUG_TYPE_PORTABILITY_KHR),
  X(GL_DEBUG_TYPE_PERFORMANCE_KHR),
  X(GL_DEBUG_TYPE_OTHER_KHR),
  X(GL_DEBUG_TYPE_MARKER_KHR),
#endif
};

std::map<GLenum, const char*> glErrorSeverity =
{
//! @todo remove TARGET_RASPBERRY_PI when Raspberry Pi updates their GL headers
#if defined(HAS_GLES) && defined(TARGET_LINUX) && !defined(TARGET_RASPBERRY_PI)
  X(GL_DEBUG_SEVERITY_HIGH_KHR),
  X(GL_DEBUG_SEVERITY_MEDIUM_KHR),
  X(GL_DEBUG_SEVERITY_LOW_KHR),
  X(GL_DEBUG_SEVERITY_NOTIFICATION_KHR),
#endif
};
#undef X

} // namespace

void KODI::UTILS::GL::GlErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
  std::string sourceStr;
  std::string typeStr;
  std::string severityStr;

  auto glSource = glErrorSource.find(source);
  if (glSource != glErrorSource.end())
  {
    sourceStr = glSource->second;
  }

  auto glType = glErrorType.find(type);
  if (glType != glErrorType.end())
  {
    typeStr = glType->second;
  }

  auto glSeverity = glErrorSeverity.find(severity);
  if (glSeverity != glErrorSeverity.end())
  {
    severityStr = glSeverity->second;
  }

  CLog::Log(LOGDEBUG, "OpenGL(ES) Debugging:\nSource: {}\nType: {}\nSeverity: {}\nID: {}\nMessage: {}", sourceStr, typeStr, severityStr, id, message);
}

static void PrintMatrix(const GLfloat* matrix, std::string matrixName)
{
  CLog::Log(LOGDEBUG, "{}:\n{:> 10.3f} {:> 10.3f} {:> 10.3f} {:> 10.3f}\n{:> 10.3f} {:> 10.3f} {:> 10.3f} {:> 10.3f}\n{:> 10.3f} {:> 10.3f} {:> 10.3f} {:> 10.3f}\n{:> 10.3f} {:> 10.3f} {:> 10.3f} {:> 10.3f}",
                      matrixName,
                      matrix[0], matrix[1], matrix[2], matrix[3],
                      matrix[4], matrix[5], matrix[6], matrix[7],
                      matrix[8], matrix[9], matrix[10], matrix[11],
                      matrix[12], matrix[13], matrix[14], matrix[15]);
}

void _VerifyGLState(const char* szfile, const char* szfunction, int lineno)
{
  GLenum err = glGetError();
  if (err == GL_NO_ERROR)
  {
    return;
  }

  auto error = glErrors.find(err);
  if (error != glErrors.end())
  {
    CLog::Log(LOGERROR, "GL(ES) ERROR: {}", error->second);
  }

  if (szfile && szfunction)
  {
    CLog::Log(LOGERROR, "In file: {} function: {} line: {}", szfile, szfunction, lineno);
  }

  GLboolean scissors;
  glGetBooleanv(GL_SCISSOR_TEST, &scissors);
  CLog::Log(LOGDEBUG, "Scissor test enabled: {}", scissors == GL_TRUE ? "True" : "False");

  GLfloat matrix[16];
  glGetFloatv(GL_SCISSOR_BOX, matrix);
  CLog::Log(LOGDEBUG, "Scissor box: {}, {}, {}, {}", matrix[0], matrix[1], matrix[2], matrix[3]);

  glGetFloatv(GL_VIEWPORT, matrix);
  CLog::Log(LOGDEBUG, "Viewport: {}, {}, {}, {}", matrix[0], matrix[1], matrix[2], matrix[3]);

  PrintMatrix(glMatrixProject.Get(), "Projection Matrix");
  PrintMatrix(glMatrixModview.Get(), "Modelview Matrix");
}

void LogGraphicsInfo()
{
#if defined(HAS_GL) || defined(HAS_GLES)
  const GLubyte *s;

  s = glGetString(GL_VENDOR);
  if (s)
    CLog::Log(LOGNOTICE, "GL_VENDOR = %s", s);
  else
    CLog::Log(LOGNOTICE, "GL_VENDOR = NULL");

  s = glGetString(GL_RENDERER);
  if (s)
    CLog::Log(LOGNOTICE, "GL_RENDERER = %s", s);
  else
    CLog::Log(LOGNOTICE, "GL_RENDERER = NULL");

  s = glGetString(GL_VERSION);
  if (s)
    CLog::Log(LOGNOTICE, "GL_VERSION = %s", s);
  else
    CLog::Log(LOGNOTICE, "GL_VERSION = NULL");

  s = glGetString(GL_SHADING_LANGUAGE_VERSION);
  if (s)
    CLog::Log(LOGNOTICE, "GL_SHADING_LANGUAGE_VERSION = %s", s);
  else
    CLog::Log(LOGNOTICE, "GL_SHADING_LANGUAGE_VERSION = NULL");

  //GL_NVX_gpu_memory_info extension
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX          0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
#define GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX            0x904A
#define GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX            0x904B

  if (CServiceBroker::GetRenderSystem()->IsExtSupported("GL_NVX_gpu_memory_info"))
  {
    GLint mem = 0;

    glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &mem);
    CLog::Log(LOGNOTICE, "GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX = %i", mem);

    //this seems to be the amount of ram on the videocard
    glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &mem);
    CLog::Log(LOGNOTICE, "GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX = %i", mem);
  }

  std::string extensions;
#if defined(HAS_GL)
  unsigned int renderVersionMajor, renderVersionMinor;
  CServiceBroker::GetRenderSystem()->GetRenderVersion(renderVersionMajor, renderVersionMinor);
  if (renderVersionMajor > 3 ||
      (renderVersionMajor == 3 && renderVersionMinor >= 2))
  {
    GLint n;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    if (n > 0)
    {
      GLint i;
      for (i = 0; i < n; i++)
      {
        extensions += (const char*)glGetStringi(GL_EXTENSIONS, i);
        extensions += " ";
      }
    }
  }
  else
#endif
  {
    extensions += (const char*) glGetString(GL_EXTENSIONS);
  }

  if (!extensions.empty())
    CLog::Log(LOGNOTICE, "GL_EXTENSIONS = %s", extensions.c_str());
  else
    CLog::Log(LOGNOTICE, "GL_EXTENSIONS = NULL");


#else /* !HAS_GL */
  CLog::Log(LOGNOTICE,
            "Please define LogGraphicsInfo for your chosen graphics library");
#endif /* !HAS_GL */
}

int glFormatElementByteCount(GLenum format)
{
  switch (format)
  {
#ifdef HAS_GL
  case GL_BGRA:
    return 4;
  case GL_RED:
    return 1;
  case GL_GREEN:
    return 1;
  case GL_RG:
    return 2;
  case GL_BGR:
    return 3;
#endif
  case GL_RGBA:
    return 4;
  case GL_RGB:
    return 3;
  case GL_LUMINANCE_ALPHA:
    return 2;
  case GL_LUMINANCE:
  case GL_ALPHA:
    return 1;
  default:
    CLog::Log(LOGERROR, "glFormatElementByteCount - Unknown format %u", format);
    return 1;
  }
}
