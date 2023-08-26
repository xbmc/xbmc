/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pictures/PictureScalingAlgorithm.h"
#include "utils/Job.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

extern "C"
{
#include <libavutil/pixfmt.h>
}

class CTexture;

class CPicture
{
public:
  static bool GetThumbnailFromSurface(const unsigned char* buffer, int width, int height, int stride, const std::string &thumbFile, uint8_t* &result, size_t& result_size);
  static bool CreateThumbnailFromSurface(const unsigned char* buffer, int width, int height, int stride, const std::string &thumbFile);

  static std::unique_ptr<CTexture> CreateTiledThumb(const std::vector<std::string>& files);

  static bool ResizeTexture(
      const std::string& image,
      CTexture* texture,
      uint32_t& dest_width,
      uint32_t& dest_height,
      uint8_t*& result,
      size_t& result_size,
      CPictureScalingAlgorithm::Algorithm scalingAlgorithm = CPictureScalingAlgorithm::NoAlgorithm);
  static bool ResizeTexture(const std::string &image, uint8_t *pixels, uint32_t width, uint32_t height, uint32_t pitch,
    uint32_t &dest_width, uint32_t &dest_height, uint8_t* &result, size_t& result_size,
    CPictureScalingAlgorithm::Algorithm scalingAlgorithm = CPictureScalingAlgorithm::NoAlgorithm);

  /*! \brief Cache a texture, resizing, rotating and flipping as needed, and saving as a JPG or PNG
   \param texture a pointer to a CTexture
   \param dest_width [in/out] maximum width in pixels of cached version - replaced with actual cached width
   \param dest_height [in/out] maximum height in pixels of cached version - replaced with actual cached height
   \param dest the output cache file
   \return true if successful, false otherwise
   */
  static bool CacheTexture(
      CTexture* texture,
      uint32_t& dest_width,
      uint32_t& dest_height,
      const std::string& dest,
      CPictureScalingAlgorithm::Algorithm scalingAlgorithm = CPictureScalingAlgorithm::NoAlgorithm);
  static bool CacheTexture(uint8_t *pixels, uint32_t width, uint32_t height, uint32_t pitch, int orientation,
    uint32_t &dest_width, uint32_t &dest_height, const std::string &dest,
    CPictureScalingAlgorithm::Algorithm scalingAlgorithm = CPictureScalingAlgorithm::NoAlgorithm);

  static void GetScale(unsigned int width, unsigned int height, unsigned int &out_width, unsigned int &out_height);
  static bool ScaleImage(
      uint8_t* in_pixels,
      unsigned int in_width,
      unsigned int in_height,
      unsigned int in_pitch,
      AVPixelFormat in_format,
      uint8_t* out_pixels,
      unsigned int out_width,
      unsigned int out_height,
      unsigned int out_pitch,
      AVPixelFormat out_format,
      CPictureScalingAlgorithm::Algorithm scalingAlgorithm = CPictureScalingAlgorithm::NoAlgorithm);

private:
  static bool OrientateImage(std::unique_ptr<uint32_t[]>& pixels,
                             unsigned int& width,
                             unsigned int& height,
                             int orientation,
                             unsigned int& stridePixels);

  static bool FlipHorizontal(std::unique_ptr<uint32_t[]>& pixels,
                             const unsigned int& width,
                             const unsigned int& height,
                             const unsigned int& stridePixels);
  static bool FlipVertical(std::unique_ptr<uint32_t[]>& pixels,
                           const unsigned int& width,
                           const unsigned int& height,
                           const unsigned int& stridePixels);
  static bool Rotate90CCW(std::unique_ptr<uint32_t[]>& pixels,
                          unsigned int& width,
                          unsigned int& height,
                          unsigned int& stridePixels);
  static bool Rotate270CCW(std::unique_ptr<uint32_t[]>& pixels,
                           unsigned int& width,
                           unsigned int& height,
                           unsigned int& stridePixels);
  static bool Rotate180CCW(std::unique_ptr<uint32_t[]>& pixels,
                           const unsigned int& width,
                           const unsigned int& height,
                           const unsigned int& stridePixels);
  static bool Transpose(std::unique_ptr<uint32_t[]>& pixels,
                        unsigned int& width,
                        unsigned int& height,
                        unsigned int& width_aligned);
  static bool TransposeOffAxis(std::unique_ptr<uint32_t[]>& pixels,
                               unsigned int& width,
                               unsigned int& height,
                               unsigned int& stridePixels);
};

//this class calls CreateThumbnailFromSurface in a CJob, so a png file can be written without halting the render thread
class CThumbnailWriter : public CJob
{
  public:
    //WARNING: buffer is deleted from DoWork()
    CThumbnailWriter(unsigned char* buffer, int width, int height, int stride, const std::string& thumbFile);
    ~CThumbnailWriter() override;
    bool DoWork() override;

  private:
    unsigned char* m_buffer;
    int            m_width;
    int            m_height;
    int            m_stride;
    std::string    m_thumbFile;
};

