/*!
	\file GUIList.h
	\brief 
	*/

#ifndef GUILIB_GUIListEx_H
#define GUILIB_GUIListEx_H

#pragma once
#include "guiItem.h"
#include <vector>
#include "stdstring.h"
using namespace std;

class CGUIList
{
public:

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

protected:
	CRITICAL_SECTION m_critical;
	GUILISTITEMS m_listitems;
};

class CGUISortedList : public CGUIList
{
public:
	typedef bool (*GUILISTITEMCOMPARISONFUNC) (CGUIItem* pObject1, CGUIItem* pObject2);

	CGUISortedList();
	virtual ~CGUISortedList(void);

	void SetSortingAlgorithm(GUILISTITEMCOMPARISONFUNC aFunction);
	virtual void Add(CGUIItem* aListItem);

protected:
	GUILISTITEMCOMPARISONFUNC m_comparision;
};

#endif
