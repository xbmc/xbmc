#pragma once
#include "ScreenSaver.h"
#include "StdString.h"

class CScreenSaverFactory
{
public:
	CScreenSaverFactory();
	virtual ~CScreenSaverFactory();
	CScreenSaver* LoadScreenSaver(const CStdString& strScr) const;
};