#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
#include <string>

class IImage
{
public:

  IImage():m_width(0), m_height(0), m_originalWidth(0), m_originalHeight(0), m_orientation(0), m_hasAlpha(false) {};
  virtual ~IImage() {};

  /*!
   \brief Load an image from memory with the format m_strMimeType to determine it's size and orientation
   \param buffer The memory location where the image data can be found
   \param bufSize The size of the buffer
   \param width The ideal width of the texture
   \param height The ideal height of the texture
   \return true if the image could be loaded
   */
  virtual bool LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize, unsigned int width, unsigned int height)=0;
  /*!
   \brief Decodes the previously loaded image data to the output buffer in 32 bit raw bits
   \param pixels The output buffer
   \param pitch The pitch of the output buffer
   \param format The format of the output buffer (JpegIO only)
   \return true if the image data could be decoded to the output buffer
   */
  virtual bool Decode(const unsigned char *pixels, unsigned int pitch, unsigned int format)=0;
  /*!
   \brief Encodes an thumbnail from raw bits of given memory location
   \remarks Caller need to call ReleaseThumbnailBuffer() afterwards to free the output buffer
   \param bufferin The memory location where the image data can be found
   \param width The width of the thumbnail
   \param height The height of the thumbnail
   \param format The format of the input buffer (JpegIO only)
   \param pitch The pitch of the input texture stored in bufferin
   \param destFile The destination path of the thumbnail to determine the image format from the extension
   \param bufferout The output buffer (will be allocated inside the method)
   \param bufferoutSize The output buffer size
   \return true if the thumbnail was successfully created
   */
  virtual bool CreateThumbnailFromSurface(unsigned char* bufferin, unsigned int width, unsigned int height, unsigned int format, unsigned int pitch, const std::string& destFile, 
                                          unsigned char* &bufferout, unsigned int &bufferoutSize)=0;
  /*!
   \brief Frees the output buffer allocated by CreateThumbnailFromSurface
   */
  virtual void ReleaseThumbnailBuffer() {return;}

  unsigned int Width() const              { return m_width; }
  unsigned int Height() const             { return m_height; }
  unsigned int originalWidth() const      { return m_originalWidth; }
  unsigned int originalHeight() const     { return m_originalHeight; }
  unsigned int Orientation() const        { return m_orientation; }
  bool hasAlpha() const                   { return m_hasAlpha; }

protected:

  unsigned int m_width;
  unsigned int m_height;
  unsigned int m_originalWidth;   ///< original image width before scaling or cropping
  unsigned int m_originalHeight;  ///< original image height before scaling or cropping
  unsigned int m_orientation;
  bool m_hasAlpha;
 
};
