/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef __COREAUDIOAEHAL_H__
#define __COREAUDIOAEHAL_H__

#include "AEAudioFormat.h"
#include "AE.h"

class ICoreAudioAEHAL;

/**
 * ICoreAudioAEHAL Interface
 */
class ICoreAudioAEHAL
{
protected:  
	ICoreAudioAEHAL() {}
  virtual ~ICoreAudioAEHAL() {}

public:
  virtual bool	Initialize(IAE *ae, bool passThrough, AEAudioFormat &format, CStdString &device) = 0;
	virtual void	Deinitialize() = 0;
	virtual void	EnumerateOutputDevices(AEDeviceList &devices, bool passthrough) = 0;
	virtual void	Stop() = 0;
	virtual bool	Start() = 0;
	virtual float GetDelay() = 0;
};

#endif