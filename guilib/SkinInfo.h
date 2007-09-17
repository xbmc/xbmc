#pragma once
#include "GraphicContext.h" // needed for the RESOLUTION members
#include "GUIIncludes.h"    // needed for the GUIInclude member

#define CREDIT_LINE_LENGTH 50

class CSkinInfo
{
public:
  class CStartupWindow
  {
  public:
    CStartupWindow(int id, const CStdString &name)
    {
      m_id = id; m_name = name;
    };
    int m_id;
    CStdString m_name;
  };

  CSkinInfo();
  ~CSkinInfo();

  void Load(const CStdString& strSkinDir); // load the skin.xml file if it exists, and configure our directories etc.
  bool Check(const CStdString& strSkinDir); // checks if everything is present and accounted for without loading the skin
  
  CStdString GetSkinPath(const CStdString& strFile, RESOLUTION *res, const CStdString& strBaseDir="");  // retrieve the best skin file for the resolution we are in - res will be made the resolution we are loading from
  wchar_t* GetCreditsLine(int i);

  CStdString GetDirFromRes(RESOLUTION res);
  CStdString GetBaseDir();
  double GetMinVersion();
  double GetVersion(){ return m_Version;};
  int GetStartWindow();

  void ResolveIncludes(TiXmlElement *node, const CStdString &type = "");
  bool ResolveConstant(const CStdString &constant, float &value);

  double GetEffectsSlowdown() const { return m_effectsSlowDown; };

  const vector<CStartupWindow> &GetStartupWindows() { return m_startupWindows; };

  bool OnlyAnimateToHome() { return m_onlyAnimateToHome; };

  inline float GetSkinZoom() { return m_skinzoom; };

  inline const RESOLUTION& GetDefaultWideResolution() { return m_DefaultResolutionWide; };
  inline const RESOLUTION& GetDefaultResolution() { return m_DefaultResolution; };

//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  void LoadIncludes(const TiXmlElement *element);
//#endif
protected:
  void LoadIncludes();
  bool LoadStartupWindows(const TiXmlElement *startup);

  wchar_t credits[6][CREDIT_LINE_LENGTH];  // credits info
  int m_iNumCreditLines;  // number of credit lines
  RESOLUTION m_DefaultResolution; // default resolution for the skin in 4:3 modes
  RESOLUTION m_DefaultResolutionWide; // default resolution for the skin in 16:9 modes
  CStdString m_strBaseDir;
  double m_Version;

  double m_effectsSlowDown;
  CGUIIncludes m_includes;

  vector<CStartupWindow> m_startupWindows;
  bool m_onlyAnimateToHome;

  float m_skinzoom;
};

extern CSkinInfo g_SkinInfo;
