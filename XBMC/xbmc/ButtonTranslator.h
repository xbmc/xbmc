#include "tinyxml/tinyxml.h"
#include "key.h"
#include <xtl.h>
#include "stdstring.h"
#include <map>

// class to map from buttons to actions
class CButtonTranslator
{
public:
	CButtonTranslator();
	virtual ~CButtonTranslator();

	void Load();
	void GetAction(WORD wWindow, const CKey &key, CAction &action);

private:
	typedef multimap<WORD,WORD> buttonMap;	// our button map to fill in
	map<WORD, buttonMap> translatorMap;		// mapping of windows to button maps
	void MapAction(WORD wAction, TiXmlNode *pNode, buttonMap &map);
	WORD GetActionCode(WORD wWindow, const CKey &key);
};

extern CButtonTranslator g_buttonTranslator;