/*!
	\file GUIList.h
	\brief 
	*/

#ifndef GUILIB_GUIListEx_H
#define GUILIB_GUIListEx_H

#pragma once
#include "GUIItem.h"

class CGUIList
{
public:
	typedef bool (*GUILISTITEMCOMPARISONFUNC) (CGUIItem* pObject1, CGUIItem* pObject2);
	typedef vector<CGUIItem*> GUILISTITEMS;
	typedef vector<CGUIItem*> ::iterator GUILISTITERATOR;

	CGUIList();
	virtual ~CGUIList(void);

	virtual void Add(CGUIItem* aListItem);
	virtual void Remove(CStdString aListItemLabel);
	virtual void Clear();
	virtual int  Size();

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
