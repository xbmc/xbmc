#pragma once

#include "tinyxml/tinyxml.h"
#include "key.h"
#include <xtl.h>
#include "stdstring.h"
#include <map>

struct CButtonAction
{
	WORD wID;
	CStdString strID;	// needed for "XBMC.ActivateWindow()" type actions
};
// class to map from buttons to actions
class CButtonTranslator
{
public:
	CButtonTranslator();
	virtual ~CButtonTranslator();

	bool Load();
	void GetAction(WORD wWindow, const CKey &key, CAction &action);

private:
	typedef multimap<WORD,CButtonAction> buttonMap;	// our button map to fill in
	map<WORD, buttonMap> translatorMap;							// mapping of windows to button maps
	void MapAction(WORD wAction, const CStdString &strAction, TiXmlNode *pNode, buttonMap &map);
	WORD GetActionCode(WORD wWindow, const CKey &key, CStdString &strAction);
};

extern CButtonTranslator g_buttonTranslator;