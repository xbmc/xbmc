#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "ImusicInfoTagLoader.h"

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderWMA: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderWMA(void);
  virtual ~CMusicInfoTagLoaderWMA();

  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);

protected:
  void SetTagValueString(const CStdString& strFrameName, const CStdString& strValue, CMusicInfoTag& tag);
  void SetTagValueUnsigned(const CStdString& strFrameName, uint32_t value, CMusicInfoTag& tag);
  void SetTagValueBinary(const CStdString& strFrameName, const unsigned char *pValue, CMusicInfoTag& tag);
  void SetTagValueBool(const CStdString& strFrameName, bool bValue, CMusicInfoTag& tag);
};
}
