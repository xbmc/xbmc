#include "stdafx.h"
#include "GUIList.h"
#include "../xbmc/utils/log.h"
#include <algorithm>

CGUIList::CGUIList()
{
	InitializeCriticalSection(&m_critical);
}

CGUIList::~CGUIList(void)
{
	Clear();

	DeleteCriticalSection(&m_critical);
}

CGUIItem* CGUIList::Find(CStdString aListItemLabel)
{
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
				delete *iterator;
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

void CGUIList::Release()
{
	LeaveCriticalSection(&m_critical);
}

CGUIList::GUILISTITEMS& CGUIList::Lock()
{
	EnterCriticalSection(&m_critical);
	return m_listitems;
}

CGUISortedList::CGUISortedList() : CGUIList()
{
	m_comparision = NULL;
}

CGUISortedList::~CGUISortedList(void)
{
}

void CGUISortedList::SetSortingAlgorithm(GUILISTITEMCOMPARISONFUNC aFunction)
{
	m_comparision = aFunction;
}

void CGUISortedList::Add(CGUIItem* aListItem)
{
	EnterCriticalSection(&m_critical);
	
	m_listitems.push_back(aListItem);
	
	if (m_comparision)
	{
		sort(m_listitems.begin(), m_listitems.end(), m_comparision );
	}

	LeaveCriticalSection(&m_critical);
}