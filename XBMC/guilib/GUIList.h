/*!
\file GUIList.h
\brief 
*/

#ifndef GUILIB_GUIListEx_H
#define GUILIB_GUIListEx_H

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

#include "GUIItem.h"

class CGUIList
{
public:
  typedef bool (*GUILISTITEMCOMPARISONFUNC) (CGUIItem* pObject1, CGUIItem* pObject2);
  typedef std::vector<CGUIItem*> GUILISTITEMS;
  typedef std::vector<CGUIItem*> ::iterator GUILISTITERATOR;

  CGUIList();
  virtual ~CGUIList(void);

  virtual void Add(CGUIItem* aListItem);
  virtual void Remove(CStdString aListItemLabel);
  virtual void Clear();
  virtual int Size();

  GUILISTITEMS& Lock();
  CGUIItem* Find(CStdString aListItemLabel);
  void Release();

  void SetSortingAlgorithm(GUILISTITEMCOMPARISONFUNC aFunction);
  void Sort();

protected:
  CRITICAL_SECTION m_critical;
  GUILISTITEMS m_listitems;
  GUILISTITEMCOMPARISONFUNC m_comparision;
};

#endif
