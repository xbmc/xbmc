/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class IAudioDeviceChangedCallback
{
public:
  virtual void  Initialize(int iDevice)=0;
  virtual void  DeInitialize(int iDevice)=0;
  virtual ~IAudioDeviceChangedCallback() {}
};

