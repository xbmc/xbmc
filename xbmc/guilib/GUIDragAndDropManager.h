#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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
#include "GUIListItem.h"
#include "GUIMessage.h"
#include "Key.h"
#include "IMsgTargetCallback.h"
#include "boost/shared_ptr.hpp"

class CGUIControl;



class CGUIDragAndDropManager : public IMsgTargetCallback
{
public:
  CGUIDragAndDropManager() {}
  ~CGUIDragAndDropManager() {}
  virtual bool OnMessage(CGUIMessage& message);
  /*! \brief Returns all the stored drag and drop Info
   \sa DragAndDropInfo 
   */
  inline boost::shared_ptr<DragAndDropInfo> GetMessageInfo() { return m_dndInfo; }
  
protected:
  void DraggingStart(CGUIListItemPtr draggedFileItem, int windowID);
  void DraggingStop();
  
  boost::shared_ptr<DragAndDropInfo> m_dndInfo;
};

extern CGUIDragAndDropManager g_dragAndDropManager;
