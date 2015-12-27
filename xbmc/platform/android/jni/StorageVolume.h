#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
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

#include "JNIBase.h"

#include "Context.h"

class CJNIStorageVolume : public CJNIBase
{
public:
  CJNIStorageVolume(const jni::jhobject &object) : CJNIBase(object) {};
  ~CJNIStorageVolume() {};

  std::string getPath();
  std::string getDescription(const CJNIContext& context);
  int getDescriptionId();

  bool isPrimary();
  bool isRemovable();
  bool isEmulated();

  int64_t getMaxFileSize();
  std::string getUuid();
  int getFatVolumeId();

  std::string getUserLabel();
  std::string getState();

private:
  CJNIStorageVolume();
};

typedef std::vector<CJNIStorageVolume> CJNIStorageVolumes;
