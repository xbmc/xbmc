/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#ifndef XBTF_H_
#define XBTF_H_

#include <string>
#include <vector>

#define XBTF_MAGIC "XBTF"
#define XBTF_VERSION "1"

#define XB_FMT_DXT_MASK 14
#define XB_FMT_UNKNOWN  1
#define XB_FMT_DXT1     2
#define XB_FMT_DXT3     4
#define XB_FMT_DXT5     8
#define XB_FMT_B8G8R8A8 16
#define XB_FMT_R8G8B8A8 32
#define XB_FMT_LZO      64

class CXBTFFrame
{
public:  
  CXBTFFrame();
  unsigned int GetWidth() const;
  void SetWidth(unsigned int width);
  unsigned int GetX() const;
  void SetX(unsigned int x);
  unsigned int GetY() const;
  void SetY(unsigned int y);
  unsigned int GetHeight() const;
  void SetHeight(unsigned int height);
  unsigned long long GetUnpackedSize() const;
  void SetUnpackedSize(unsigned long long size);
  unsigned long long GetPackedSize() const;
  void SetPackedSize(unsigned long long size);
  unsigned long long GetOffset() const;
  void SetOffset(unsigned long long offset);
  unsigned long long GetHeaderSize() const;
  unsigned int GetDuration() const;
  void SetDuration(unsigned int duration);

private:
  unsigned int       m_width;
  unsigned int       m_height;
  unsigned int       m_x;
  unsigned int       m_y;
  unsigned long long m_packedSize;
  unsigned long long m_unpackedSize;
  unsigned long long m_offset;
  unsigned int       m_duration;
};

class CXBTFFile
{
public:
  CXBTFFile();
  CXBTFFile(const CXBTFFile& ref);
  char* GetPath();
  void SetPath(const std::string& path);
  int GetLoop() const;
  void SetLoop(int loop);
  unsigned int GetFormat() const;
  void SetFormat(unsigned int format);  
  
  std::vector<CXBTFFrame>& GetFrames();  
  unsigned long long GetHeaderSize() const;
  
private:
  char         m_path[256];
  int          m_loop;
  unsigned int m_format;    
  std::vector<CXBTFFrame> m_frames;
};

class CXBTF
{
public:
  CXBTF();
  unsigned long long GetHeaderSize() const;
  std::vector<CXBTFFile>& GetFiles();  
  
private:
  std::vector<CXBTFFile> m_files;
};

#endif
