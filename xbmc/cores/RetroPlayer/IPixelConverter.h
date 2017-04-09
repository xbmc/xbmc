/*
 *      Copyright (C) 2016-2017 Team Kodi
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

#include "libavutil/pixfmt.h"

#include <stdint.h>

struct VideoPicture;

class IPixelConverter
{
public:
  virtual ~IPixelConverter() = default;

  /*!
   * \brief Open a context for converting pixels
   * \param pixfmt The source format
   * \param target The target format
   * \param with The width of the frame
   * \param height The height of the frame
   * \return true if this object is ready to convert pixels
   */
  virtual bool Open(AVPixelFormat pixfmt, AVPixelFormat target, unsigned int width, unsigned int height) = 0;

  /*!
   * \brief Release the resources used by this class
   */
  virtual void Dispose() = 0;

  /*!
   * \brief Send a frame to the pixel converter
   * \param pData A pointer to the pixel data
   * \param size The size of the data
   * \return true if a picture is ready to be read, false otherwise
   */
  virtual bool Decode(const uint8_t* pData, unsigned int size) = 0;

  /*!
   * \brief Get the results of processing the pixels
   * \param dvdVideoPicture a container for the resulting pixel data
   */
  virtual void GetPicture(VideoPicture& dvdVideoPicture) = 0;
};
