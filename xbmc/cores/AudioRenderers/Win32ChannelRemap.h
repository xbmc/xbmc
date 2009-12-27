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
 
#ifndef __WIN32CHANNELREMAP_H__ 
#define __WIN32CHANNELREMAP_H__ 
 
#if _MSC_VER > 1000 
#pragma once 
#endif // _MSC_VER > 1000 
 
class CWin32ChannelRemap 
{ 
public: 
 
  CWin32ChannelRemap(); 
  ~CWin32ChannelRemap(); 
 
protected: 
 
  void SetChannelMap(unsigned int channels, unsigned int bitsPerSample, bool passthrough, const char* strAudioCodec); 
  void MapDataIntoBuffer(unsigned char* pData, unsigned int len, unsigned char* pOut); 
 
private: 
 
  void MapData8(unsigned char* pData, unsigned int len, unsigned char* pOut); 
  void MapData16(unsigned char* pData, unsigned int len, unsigned char* pOut); 
  void MapData24(unsigned char* pData, unsigned int len, unsigned char* pOut); 
  void MapData32(unsigned char* pData, unsigned int len, unsigned char* pOut); 
 
  const unsigned char* m_pChannelMap; 
  unsigned int m_uiChannels; 
  unsigned int m_uiBitsPerSample; 
  bool m_bPassthrough; 
}; 
 
#endif // __WIN32CHANNELREMAP_H__ 