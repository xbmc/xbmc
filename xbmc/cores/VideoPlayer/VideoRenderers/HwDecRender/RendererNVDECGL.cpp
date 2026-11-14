/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererNVDECGL.h"

#include "../RenderFactory.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

using namespace NVDEC;

namespace
{
bool CheckCU(CUresult result, const char* call)
{
  if (result == CUDA_SUCCESS)
    return true;

  const char* name = nullptr;
  GetCudaFunctions()->cuGetErrorName(result, &name);
  CLog::Log(LOGERROR, "NVDEC GL interop: {} failed: {}", call, name ? name : "unknown error");
  return false;
}

//! Makes a CUDA context current on this thread for the lifetime of the scope.
class CCudaContextScope
{
public:
  explicit CCudaContextScope(CUcontext context)
  {
    m_ok = CheckCU(GetCudaFunctions()->cuCtxPushCurrent(context), "cuCtxPushCurrent");
  }
  ~CCudaContextScope()
  {
    if (m_ok)
    {
      CUcontext dummy;
      GetCudaFunctions()->cuCtxPopCurrent(&dummy);
    }
  }
  CCudaContextScope(const CCudaContextScope&) = delete;
  CCudaContextScope& operator=(const CCudaContextScope&) = delete;

  bool Ok() const { return m_ok; }

private:
  bool m_ok = false;
};
} // namespace

CBaseRenderer* CRendererNVDECGL::Create(CVideoBuffer* buffer)
{
  if (dynamic_cast<CVideoBufferNVDEC*>(buffer))
    return new CRendererNVDECGL();
  return nullptr;
}

void CRendererNVDECGL::Register()
{
  if (GetCudaFunctions())
    VIDEOPLAYER::CRendererFactory::RegisterRenderer("nvdec", CRendererNVDECGL::Create);
}

CRendererNVDECGL::~CRendererNVDECGL()
{
  // renderer destruction runs on the render thread with the GL context current
  for (int i = 0; i < NUM_BUFFERS; ++i)
    DeleteTexture(i);
}

bool CRendererNVDECGL::Configure(const VideoPicture& picture, float fps, unsigned int orientation)
{
  if (!dynamic_cast<CVideoBufferNVDEC*>(picture.videoBuffer))
    return false;

  m_interopFailed = false;
  return CLinuxRendererGL::Configure(picture, fps, orientation);
}

bool CRendererNVDECGL::CreateTexture(int index)
{
  CPictureBuffer& buf = m_buffers[index];
  YuvImage& im = buf.image;
  CYuvPlane(&planes)[YuvImage::MAX_PLANES] = buf.fields[0];

  DeleteTexture(index);

  memset(&im, 0, sizeof(im));
  memset(&planes, 0, sizeof(CYuvPlane[YuvImage::MAX_PLANES]));
  im.height = m_sourceHeight;
  im.width = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;

  // The real textures are created on first upload, once the surface format is known.
  planes[0].id = 1;

  return true;
}

void CRendererNVDECGL::DeleteTexture(int index)
{
  // The video buffer stays, CLinuxRendererGL releases it when it isn't kept over a flush.
  ReleaseInterop(m_cudaTextures[index]);

  CPictureBuffer& buf = m_buffers[index];
  buf.loaded = false;
  buf.fields[FIELD_FULL][0].id = 0;
  buf.fields[FIELD_FULL][1].id = 0;
  buf.fields[FIELD_FULL][2].id = 0;
}

bool CRendererNVDECGL::SetupInterop(CudaTextures& textures, const CVideoBufferNVDEC& buffer)
{
  const AVFrame* frame = buffer.GetFrame();
  const bool deep = buffer.GetSwFormat() != AV_PIX_FMT_NV12;

  // Plane 0: luma, plane 1: interleaved chroma at half resolution.
  const GLint internalFormat[2] = {deep ? GL_R16 : GL_R8, deep ? GL_RG16 : GL_RG8};
  const GLenum pixelFormat[2] = {GL_RED, GL_RG};
  const GLenum pixelType = deep ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;
  const int width[2] = {frame->width, (frame->width + 1) / 2};
  const int height[2] = {frame->height, (frame->height + 1) / 2};

  const GLint filter = m_scalingMethod == VS_SCALINGMETHOD_NEAREST ? GL_NEAREST : GL_LINEAR;

  glGenTextures(2, textures.id);
  for (int p = 0; p < 2; ++p)
  {
    glBindTexture(GL_TEXTURE_2D, textures.id[p]);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat[p], width[p], height[p], 0, pixelFormat[p],
                 pixelType, nullptr);
    // Same filter UpdateVideoFilter() would have set, it doesn't run again for new textures.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  VerifyGLState();

  textures.context = buffer.GetCudaContext();
  textures.device = av_buffer_ref(buffer.GetDeviceRef());
  CCudaContextScope scope(textures.context);
  if (!scope.Ok())
  {
    ReleaseInterop(textures);
    return false;
  }

  for (int p = 0; p < 2; ++p)
  {
    if (!CheckCU(GetCudaFunctions()->cuGraphicsGLRegisterImage(
                     &textures.resource[p], textures.id[p], GL_TEXTURE_2D,
                     CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD),
                 "cuGraphicsGLRegisterImage"))
    {
      ReleaseInterop(textures);
      return false;
    }
  }

  textures.width = frame->width;
  textures.height = frame->height;
  textures.format = buffer.GetSwFormat();
  return true;
}

void CRendererNVDECGL::ReleaseInterop(CudaTextures& textures)
{
  if (textures.context && (textures.resource[0] || textures.resource[1]))
  {
    CCudaContextScope scope(textures.context);
    for (auto& resource : textures.resource)
    {
      if (resource && scope.Ok())
        GetCudaFunctions()->cuGraphicsUnregisterResource(resource);
      resource = nullptr;
    }
  }

  if (textures.id[0] || textures.id[1])
    glDeleteTextures(2, textures.id);

  av_buffer_unref(&textures.device);

  textures = CudaTextures{};
}

bool CRendererNVDECGL::UploadTexture(int index)
{
  CPictureBuffer& buf = m_buffers[index];
  auto* buffer = dynamic_cast<CVideoBufferNVDEC*>(buf.videoBuffer);
  if (!buffer)
    return false;

  // Render() calls this on every pass, copy each frame only once.
  if (buf.loaded)
  {
    CalculateTextureSourceRects(index, 3);
    return true;
  }

  if (m_interopFailed)
    return false;

  const AVFrame* frame = buffer->GetFrame();
  CudaTextures& textures = m_cudaTextures[index];

  if (textures.format != buffer->GetSwFormat() || textures.width != frame->width ||
      textures.height != frame->height || textures.context != buffer->GetCudaContext())
  {
    ReleaseInterop(textures);
    if (!SetupInterop(textures, *buffer))
    {
      SetInteropFailed();
      m_interopFailed = true;
      return false;
    }
  }

  {
    CCudaContextScope scope(textures.context);
    CudaFunctions* cu = GetCudaFunctions();
    bool ok = scope.Ok() && CheckCU(cu->cuGraphicsMapResources(2, textures.resource, nullptr),
                                    "cuGraphicsMapResources");
    const bool mapped = ok;

    const int bytesPerSample = textures.format == AV_PIX_FMT_NV12 ? 1 : 2;
    for (int p = 0; p < 2 && ok; ++p)
    {
      CUarray array = nullptr;
      ok = CheckCU(cu->cuGraphicsSubResourceGetMappedArray(&array, textures.resource[p], 0, 0),
                   "cuGraphicsSubResourceGetMappedArray");
      if (!ok)
        break;

      // The chroma plane holds width/2 interleaved U/V pairs, i.e. width samples per row.
      CUDA_MEMCPY2D copy{};
      copy.srcMemoryType = CU_MEMORYTYPE_DEVICE;
      copy.srcDevice = reinterpret_cast<CUdeviceptr>(frame->data[p]);
      copy.srcPitch = static_cast<size_t>(frame->linesize[p]);
      copy.dstMemoryType = CU_MEMORYTYPE_ARRAY;
      copy.dstArray = array;
      copy.WidthInBytes =
          static_cast<size_t>(p == 0 ? frame->width : ((frame->width + 1) / 2) * 2) *
          bytesPerSample;
      copy.Height = static_cast<size_t>(p == 0 ? frame->height : (frame->height + 1) / 2);
      ok = CheckCU(cu->cuMemcpy2D(&copy), "cuMemcpy2D");
    }

    // GL must not sample the textures while they are still mapped.
    if (mapped)
      ok = CheckCU(cu->cuGraphicsUnmapResources(2, textures.resource, nullptr),
                   "cuGraphicsUnmapResources") &&
           ok;
    if (!ok)
    {
      SetInteropFailed();
      m_interopFailed = true;
      return false;
    }
  }

  CYuvPlane(&planes)[3] = buf.fields[0];

  planes[0].texwidth = frame->width;
  planes[0].texheight = frame->height;
  // Same rounding as the textures allocated in SetupInterop().
  planes[1].texwidth = (frame->width + 1) / 2;
  planes[1].texheight = (frame->height + 1) / 2;
  planes[2].texwidth = planes[1].texwidth;
  planes[2].texheight = planes[1].texheight;

  for (int p = 0; p < 3; ++p)
  {
    planes[p].pixpertex_x = 1;
    planes[p].pixpertex_y = 1;
  }

  planes[0].id = textures.id[0];
  planes[1].id = textures.id[1];
  planes[2].id = textures.id[1];

  buf.loaded = true;
  CalculateTextureSourceRects(index, 3);
  return true;
}
