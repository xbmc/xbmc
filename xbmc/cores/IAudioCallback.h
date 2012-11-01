// IAudioCallback.h: interface for the IAudioCallback class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IAUDIOCALLBACK_H__5A6AC7CF_C60E_45B9_8113_599F036FBBF8__INCLUDED_)
#define AFX_IAUDIOCALLBACK_H__5A6AC7CF_C60E_45B9_8113_599F036FBBF8__INCLUDED_

/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class IAudioCallback
{
public:
  IAudioCallback() {};
  virtual ~IAudioCallback() {};
  virtual void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample) = 0;
  virtual void OnAudioData(const float* pAudioData, int iAudioDataLength) = 0;

};

#endif // !defined(AFX_IAUDIOCALLBACK_H__5A6AC7CF_C60E_45B9_8113_599F036FBBF8__INCLUDED_)
