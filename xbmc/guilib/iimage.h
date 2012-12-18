#pragma once
/*
*      Copyright (C) 2012 Team XBMC
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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

class IImage
{
public:

  IImage()
  {
    m_width          = 0;
    m_height         = 0;
    m_originalWidth  = 0;
    m_originalHeight = 0;
    m_orientation    = 0;
  };
  virtual ~IImage() {};

  //virtual bool LoadImageFromFile(std::string imagePath)=0;
  virtual bool LoadImageFromMemory()=0;
  virtual bool CreateThumbnailFromSurface(unsigned char* buffer, unsigned int width, unsigned int height, unsigned int format, unsigned int pitch, const CStdString& destFile);
  virtual bool ReleaseImage()=0;

  unsigned int Width()              { return m_width; }
  unsigned int Height()             { return m_height; }
  unsigned int originalWidth()      { return m_originalWidth; }
  unsigned int originalHeight()     { return m_originalHeight; }
  unsigned int Orientation()        { return m_orientation; }

protected:

  unsigned int m_width;
  unsigned int m_height;
  unsigned int m_originalWidth;   ///< original image width before scaling or cropping
  unsigned int m_originalHeight;  ///< original image height before scaling or cropping
  unsigned int m_orientation;
 
};