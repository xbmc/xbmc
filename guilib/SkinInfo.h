#pragma once

#include <string>
using namespace std;

#include "GraphicContext.h"
#include "tinyxml/tinyxml.h"

class CSkinInfo
{
public:
	CSkinInfo();
	~CSkinInfo();
	
	void Load(CStdString& strSkinDir);// load the skin.xml file if it exists, and configure our directories etc.
	bool Check(CStdString& strSkinDir);	// checks if everything is present and accounted for without loading the skin

	CStdString GetSkinPath(const CStdString& strFile, RESOLUTION *res);		// retrieve the best skin file for the resolution we are in - res will be made the resolution we are loading from
	wchar_t* GetCreditsLine(int i);

protected:
	CStdString GetDirFromRes(RESOLUTION res);
	bool ConvertTo4x3(RESOLUTION *res);

	wchar_t credits[6][50];		// credits info
	int m_iNumCreditLines;		// number of credit lines
	RESOLUTION m_DefaultResolution;	// default resolution for the skin
	CStdString m_strBaseDir;
};

extern CSkinInfo g_SkinInfo;