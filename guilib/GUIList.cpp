#include "stdafx.h"
#include "GUIList.h"
#include "../xbmc/utils/log.h"
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

	if (m_listitems.size()>0)
	{
		GUILISTITERATOR iterator = m_listitems.begin(); 
		while (iterator != m_listitems.end())
		{
			if (aListItemLabel.Compare((*iterator)->GetName())==0)
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

	if (m_listitems.size()>0)
	{
		GUILISTITERATOR iterator = m_listitems.begin(); 
		while (iterator != m_listitems.end())
		{
			if (aListItemLabel.Compare((*iterator)->GetName())==0)
			{
				try
				{
					delete *iterator;
				}
				catch (...)
				{
				//	OutputDebugString("Unable to free stuff\r\n");
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

	if (m_listitems.size()>0)
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
