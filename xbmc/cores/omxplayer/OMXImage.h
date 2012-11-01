#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
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

#if defined(HAVE_OMXLIB)

#include "OMXCore.h"

#include <IL/OMX_Video.h>

#include "OMXClock.h"
#if defined(STANDALONE)
#define XB_FMT_A8R8G8B8 1
#include "File.h"
#else
#include "filesystem/File.h"
#include "guilib/XBTF.h"
#endif

using namespace XFILE;
using namespace std;

class COMXImage
{
public:
  COMXImage();
  virtual ~COMXImage();

  // Required overrides
  void Close(void);
  bool ClampLimits(unsigned int &width, unsigned int &height);
  void SetHardwareSizeLimits();
  bool ReadFile(const CStdString& inputFile);
  bool IsProgressive() { return m_progressive; };
  bool IsAlpha() { return m_alpha; };
  int  GetOrientation() { return m_orientation; };
  unsigned int GetOriginalWidth()  { return m_omx_image.nFrameWidth; };
  unsigned int GetOriginalHeight() { return m_omx_image.nFrameHeight; };
  unsigned int GetWidth()  { return m_width; };
  unsigned int GetHeight() { return m_height; };
  OMX_IMAGE_CODINGTYPE GetCodingType();
  const uint8_t *GetImageBuffer() { return (const uint8_t *)m_image_buffer; };
  unsigned long GetImageSize() { return m_image_size; };
  OMX_IMAGE_CODINGTYPE GetCompressionFormat() { return m_omx_image.eCompressionFormat; };
  bool Decode(unsigned int width, unsigned int height);
  bool Encode(unsigned char *buffer, int size, unsigned int width, unsigned int height);
  unsigned int GetDecodedWidth() { return (unsigned int)m_decoded_format.format.image.nFrameWidth; };
  unsigned int GetDecodedHeight() { return (unsigned int)m_decoded_format.format.image.nFrameHeight; };
  unsigned int GetDecodedStride() { return (unsigned int)m_decoded_format.format.image.nStride; };
  unsigned char *GetDecodedData();
  unsigned int GetDecodedSize();
  unsigned int GetEncodedWidth() { return (unsigned int)m_encoded_format.format.image.nFrameWidth; };
  unsigned int GetEncodedHeight() { return (unsigned int)m_encoded_format.format.image.nFrameHeight; };
  unsigned int GetEncodedStride() { return (unsigned int)m_encoded_format.format.image.nStride; };
  unsigned char *GetEncodedData();
  unsigned int GetEncodedSize();
  bool SwapBlueRed(unsigned char *pixels, unsigned int height, unsigned int pitch, 
      unsigned int elements = 4, unsigned int offset=0);
  bool CreateThumbnail(const CStdString& sourceFile, const CStdString& destFile, 
      int minx, int miny, bool rotateExif);
  bool CreateThumbnailFromMemory(unsigned char* buffer, unsigned int bufSize, 
      const CStdString& destFile, unsigned int minx, unsigned int miny);
  bool CreateThumbnailFromSurface(unsigned char* buffer, unsigned int width, unsigned int height, 
      unsigned int format, unsigned int pitch, const CStdString& destFile);
protected:
  uint8_t           *m_image_buffer;
  bool              m_is_open;
  unsigned long     m_image_size;
  unsigned int      m_width;
  unsigned int      m_height;
  bool              m_progressive;
  bool              m_alpha;
  int               m_orientation;
  XFILE::CFile      m_pFile;
  OMX_IMAGE_PORTDEFINITIONTYPE  m_omx_image;

  // Components
  COMXCoreComponent             m_omx_decoder;
  COMXCoreComponent             m_omx_encoder;
  COMXCoreComponent             m_omx_resize;
  COMXCoreTunel                 m_omx_tunnel_decode;
  OMX_BUFFERHEADERTYPE          *m_decoded_buffer;
  OMX_BUFFERHEADERTYPE          *m_encoded_buffer;
  OMX_PARAM_PORTDEFINITIONTYPE  m_decoded_format;
  OMX_PARAM_PORTDEFINITIONTYPE  m_encoded_format;

  bool                          m_decoder_open;
  bool                          m_encoder_open;
};

#endif
