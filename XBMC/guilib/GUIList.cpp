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
 
#include "include.h"
#include "GUIList.h"
#include <algorithm>

CGUIList::CGUIList()
{
  m_comparision = NULL;
  InitializeCriticalSection(&m_critical);
}

CGUIList::~CGUIList(void)
{
  Clear();

  DeleteCriticalSection(&m_critical);
}

CGUIItem* CGUIList::Find(CStdString aListItemLabel)
{
  EnterCriticalSection(&m_critical);

  CGUIItem* pItem = NULL;

  if (m_listitems.size() > 0)
  {
    GUILISTITERATOR iterator = m_listitems.begin();
    while (iterator != m_listitems.end())
    {
      if (aListItemLabel.Compare((*iterator)->GetName()) == 0)
      {
        pItem = (*iterator);
        break;
      }

      iterator++;
    }
  }

  LeaveCriticalSection(&m_critical);

  return pItem;
}

void CGUIList::Add(CGUIItem* aListItem)
{
  EnterCriticalSection(&m_critical);
  m_listitems.push_back(aListItem);
  LeaveCriticalSection(&m_critical);
}

void CGUIList::Remove(CStdString aListItemLabel)
{
  EnterCriticalSection(&m_critical);

  if (m_listitems.size() > 0)
  {
    GUILISTITERATOR iterator = m_listitems.begin();
    while (iterator != m_listitems.end())
    {
      if (aListItemLabel.Compare((*iterator)->GetName()) == 0)
      {
        try
        {
          delete *iterator;
        }
        catch (...)
        {
          // OutputDebugString("Unable to free stuff\r\n");
        }

        m_listitems.erase(iterator);
        break;
      }

      iterator++;
    }
  }

  LeaveCriticalSection(&m_critical);
}

void CGUIList::Clear()
{
  EnterCriticalSection(&m_critical);

  if (m_listitems.size() > 0)
  {
    GUILISTITERATOR iterator = m_listitems.begin();
    while (iterator != m_listitems.end())
    {
      delete *iterator;
      iterator = m_listitems.erase(iterator);
    }
  }

  LeaveCriticalSection(&m_critical);
}

int CGUIList::Size()
{
  EnterCriticalSection(&m_critical);

  int sizeOfList = m_listitems.size();

  LeaveCriticalSection(&m_critical);

  return sizeOfList;
}

void CGUIList::Release()
{
  LeaveCriticalSection(&m_critical);
}

CGUIList::GUILISTITEMS& CGUIList::Lock()
{
  EnterCriticalSection(&m_critical);
  return m_listitems;
}

void CGUIList::SetSortingAlgorithm(GUILISTITEMCOMPARISONFUNC aFunction)
{
  m_comparision = aFunction;
}

void CGUIList::Sort()
{
  EnterCriticalSection(&m_critical);

  if (m_comparision)
  {
    sort(m_listitems.begin(), m_listitems.end(), m_comparision );
  }

  LeaveCriticalSection(&m_critical);
}
