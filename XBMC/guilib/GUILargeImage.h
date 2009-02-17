/*!
\file GUILargeImage.h
\brief 
*/

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

#include "GUIImage.h"

/*!
 \ingroup controls
 \brief 
 */

class CGUILargeImage : public CGUIImage
{
public:
  CGUILargeImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture);
  virtual ~CGUILargeImage(void);
  virtual CGUILargeImage *Clone() const { return new CGUILargeImage(*this); };

  virtual void PreAllocResources();
  virtual void FreeResources();
  virtual void Render();
  virtual void SetAspectRatio(const CAspectRatio &aspect);

protected:
  CGUIImage m_fallbackImage;
};

