#pragma once
#include "ScreenSaver.h"

class CScreenSaverFactory
{
public:
	CScreenSaverFactory();
	virtual ~CScreenSaverFactory();
	CScreenSaver* LoadScreenSaver(const CStdString& strScr) const;
};