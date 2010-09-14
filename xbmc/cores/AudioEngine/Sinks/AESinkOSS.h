#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "AESink.h"
#include <stdint.h>

#include "CriticalSection.h"

class CAESinkOSS : public IAESink
{
public:
  virtual const char *GetName() { return "OSS"; }

  CAESinkOSS();
  virtual ~CAESinkOSS();

  virtual bool Initialize  (AEAudioFormat &format, CStdString &device);
  virtual void Deinitialize();
  virtual bool IsCompatible(const AEAudioFormat format, const CStdString device);

  virtual void         Stop          ();
  virtual float        GetDelay      ();
  virtual unsigned int AddPackets    (uint8_t *data, unsigned int frames);

private:
  int m_fd;
  CStdString    m_device;
  AEAudioFormat m_initFormat;
  AEAudioFormat m_format;

  CStdString GetDeviceUse(AEAudioFormat format, CStdString device);
};

