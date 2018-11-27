#pragma once

#include <vector>

#include <jpeglib.h>

#include "iimage.h"
#include "TextureFormats.h"

#include <iostream>

class JPEGImage : public IImage
{
public:
  JPEGImage()
  {
      m_allocated = false;
  }

  ~JPEGImage() override
  {
    if (m_allocated)
      jpeg_destroy_decompress(&m_mpoinfo);
    m_allocated = false;
  }

  bool LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize, unsigned int width, unsigned int height) override
  {
    // make a copy of data as we need it at decode time.
    m_data.resize(bufSize);
      std::cerr << "lifm " << this << " sz = " << bufSize << ", ideal w = " << width << std::endl;
    std::copy(buffer, buffer+bufSize, m_data.begin());
struct jpeg_error_mgr jerr;

    m_mpoinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&m_mpoinfo);
    std::cerr << "lifm " << this << " created decomp " <<  std::endl;
    jpeg_mem_src(&m_mpoinfo, m_data.data(), m_data.size());
    if (!jpeg_read_header(&m_mpoinfo, true))
    {
      jpeg_destroy_decompress(&m_mpoinfo);
      return false;
    }

    std::cerr << "lifm " << this << " header parsed, w =  " << m_mpoinfo.image_width <<  std::endl;

    m_allocated = true;
    m_originalWidth = m_mpoinfo.image_width;
    m_originalHeight = m_mpoinfo.image_height;

    m_mpoinfo.scale_denom = std::max(m_mpoinfo.scale_denom,
      (m_originalWidth * m_mpoinfo.scale_num) / (width * m_mpoinfo.scale_num));

    m_mpoinfo.scale_denom = std::max(m_mpoinfo.scale_denom,
      (m_originalHeight * m_mpoinfo.scale_num) / (height * m_mpoinfo.scale_num));

    m_mpoinfo.scale_denom = std::min(16u, m_mpoinfo.scale_denom);

    jpeg_calc_output_dimensions(&m_mpoinfo);

    m_width = m_mpoinfo.output_width;
    m_height = m_mpoinfo.output_height;

    std::cerr << "lifm " << this << " will scale to " << m_mpoinfo.scale_num << "/" << m_mpoinfo.scale_denom << std::endl;

    return true;
  }

  bool CreateThumbnailFromSurface(unsigned char*, unsigned int, unsigned int,
                                    unsigned int, unsigned int, const std::string&,
                                    unsigned char*&, unsigned int&) override { return false; }

  bool Decode(unsigned char * const pixels, unsigned int width, unsigned int height,
              unsigned int pitch, unsigned int format) override
  {

    struct jpeg_error_mgr jerr;

    m_mpoinfo.err = jpeg_std_error(&jerr);

    jpeg_start_decompress(&m_mpoinfo);
    JSAMPARRAY buffer;
    int const row_stride = m_mpoinfo.output_width * m_mpoinfo.output_components;
    buffer = (*m_mpoinfo.mem->alloc_sarray)((j_common_ptr)&m_mpoinfo, JPOOL_IMAGE, row_stride, 1);

    std::cerr << "deco" << this << "img height = " << m_mpoinfo.image_height << ", height = " << height
      << ", output_height = " << m_mpoinfo.output_height << std::endl;

    int line = 0;

    while (m_mpoinfo.output_scanline < m_mpoinfo.output_height) {
      size_t nl = jpeg_read_scanlines(&m_mpoinfo, buffer, 1);


      // std::cerr << "decode " << this << " output = " << m_mpoinfo.output_scanline << std::endl;
      unsigned char* dst = pixels + line * pitch;

      for (size_t i = 0; i < row_stride; i += m_mpoinfo.output_components) {
        *dst++ = buffer[0][i+2];
        *dst++ = buffer[0][i+1];
        *dst++ = buffer[0][i];
        if (format == XB_FMT_A8R8G8B8)
            *dst++ = 0xff;
      }

      line += nl;
    }

    std::cerr << "decode " << this << " done, pos = " << m_mpoinfo.output_scanline << std::endl;
    jpeg_finish_decompress(&m_mpoinfo);

    return true;
  }

private:
  bool m_allocated = false;
  jpeg_decompress_struct m_mpoinfo = {};
  std::vector<unsigned char> m_data;
};
