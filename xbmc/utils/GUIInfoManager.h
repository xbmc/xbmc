/*!
	\file GUIInfoManager.h
	\brief 
	*/

#ifndef GUILIB_GUIInfoManager_H
#define GUILIB_GUIInfoManager_H
#pragma once

#include "stdstring.h"
using namespace std;

/*!
	\ingroup strings
	\brief 
	*/
class CGUIInfoManager
{
public:
  CGUIInfoManager(void);
  virtual ~CGUIInfoManager(void);

	wstring GetLabel(const CStdString &strLabel);
	CStdString GetImage(const CStdString &strLabel);

	wstring GetTime();
	wstring GetDate();
protected:

};

/*!
	\ingroup strings
	\brief 
	*/
extern CGUIInfoManager g_infoManager;
#endif
