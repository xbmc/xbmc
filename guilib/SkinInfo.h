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
	
	void Load(const CStdString& strSkinDir);// load the skin.xml file if it exists, and configure our directories etc.
	bool Check(const CStdString& strSkinDir);	// checks if everything is present and accounted for without loading the skin

	CStdString GetSkinPath(const CStdString& strFile, RESOLUTION *res);		// retrieve the best skin file for the resolution we are in - res will be made the resolution we are loading from
	wchar_t* GetCreditsLine(int i);

	CStdString GetDirFromRes(RESOLUTION res);

	double GetMinVersion();

protected:

	wchar_t credits[6][50];		// credits info
	int m_iNumCreditLines;		// number of credit lines
	RESOLUTION m_DefaultResolution;	// default resolution for the skin in 4:3 modes
	RESOLUTION m_DefaultResolutionWide;	// default resolution for the skin in 16:9 modes
	CStdString m_strBaseDir;
};

extern CSkinInfo g_SkinInfo;