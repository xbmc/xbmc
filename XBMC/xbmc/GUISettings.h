#pragma once

#define CONFIG_VERSION 0x000F

#include "Profile.h"
#include "..\guilib\tinyxml/tinyxml.h"
#include <vector>
#include <map>
#include "..\guilib\GraphicContext.h"

// Render Methods
#define RENDER_LQ_RGB_SHADER   0
#define RENDER_OVERLAYS      1
#define RENDER_HQ_RGB_SHADER   2

// Subtitle colours

#define SUBTITLE_COLOR_START  0
#define SUBTITLE_COLOR_END    5

// CDDA ripper defines
#define CDDARIP_ENCODER_LAME     0
#define CDDARIP_ENCODER_VORBIS   1
#define CDDARIP_ENCODER_WAV      2

#define CDDARIP_QUALITY_CBR      0
#define CDDARIP_QUALITY_MEDIUM   1
#define CDDARIP_QUALITY_STANDARD 2
#define CDDARIP_QUALITY_EXTREME  3

#define AUDIO_ANALOG      0
#define AUDIO_DIGITAL      1

// LCD settings
#define LCD_MODE_NONE   0
#define LCD_MODE_NORMAL  1
#define LCD_MODE_NOTV     2

#define LCD_TYPE_LCD_HD44780 0
#define LCD_TYPE_LCD_KS0073  1
#define LCD_TYPE_VFD         2

#define MODCHIP_SMARTXX   0
#define MODCHIP_XENIUM    1
#define MODCHIP_XECUTER3  2

// LED settings
#define LED_COLOUR_NO_CHANGE 0
#define LED_COLOUR_GREEN   1
#define LED_COLOUR_ORANGE   2
#define LED_COLOUR_RED    3
#define LED_COLOUR_CYCLE   4
#define LED_COLOUR_OFF    5

#define SYS_DAY_SUNDAY		0
#define SYS_DAY_MONDAY		1
#define SYS_DAY_TUESDAY		2
#define SYS_DAY_WEDNESDAY	3
#define SYS_DAY_THURSDAY	4
#define SYS_DAY_FRIDAY		5
#define SYS_DAY_SATURDAY	6

#define SYS_MONTH_JANUARY	1
#define SYS_MONTH_FEBRUARY	2
#define SYS_MONTH_MARCH		3
#define SYS_MONTH_APRIL		4
#define SYS_MONTH_MAY		5
#define SYS_MONTH_JUNE		6
#define SYS_MONTH_JULY		7
#define SYS_MONTH_AUGUST	8
#define SYS_MONTH_SEPTEMBER	9
#define SYS_MONTH_OCTOBER	10
#define SYS_MONTH_NOVEMBER	11
#define SYS_MONTH_DECEMBER	12

// GeminiServer: Add SMB Shares to [1+2+4+8=15]
#define SMB_SHARE_MUSIC         0 // Musik
#define SMB_SHARE_VIDEO         1 // Video
#define SMB_SHARE_PICTURES      2 // Pictures
#define SMB_SHARE_FILES         3 // Files
#define SMB_SHARE_MU_VI         4 // Musik & Video
#define SMB_SHARE_MU_PIC        5 // Musik & Picture
#define SMB_SHARE_MU_FIL        6 // Musik & Files
#define SMB_SHARE_VI_PIC        7 // Video & Pcitures
#define SMB_SHARE_VI_FIL        8 // Video & Files
#define SMB_SHARE_PIC_FIL       9 // Picture & Files
#define SMB_SHARE_MU_VI_PIC     10 // Musik & Video & Pictures
#define SMB_SHARE_FIL_VI_MU     11 // Files & Video & Musik
#define SMB_SHARE_FIL_PIC_MU    12 // Files & Pictures & Musik
#define SMB_SHARE_FIL_PIC_VI    13 // Musik & Picture & Video
#define SMB_SHARE_MU_VI_PIC_FIL 14 // Musik & Video & Pictures & Files

// GeminiServer: Add. Lock Home Media 
#define LOCK_DISABLED           0 // Disabled
#define LOCK_MUSIC              1 // Musik
#define LOCK_VIDEO              2 // Video
#define LOCK_PICTURES           3 // Pictures
#define LOCK_PROGRAMS           4 // Programs
#define LOCK_MU_VI              5 // Musik & Video
#define LOCK_MU_PIC             6 // Musik & Picture
#define LOCK_MU_PROG            7 // Musik & Programs
#define LOCK_VI_PIC             8 // Video & Pcitures
#define LOCK_VI_PROG            9 // Video & Programs
#define LOCK_PIC_PROG           10 // Picture & Programs
#define LOCK_MU_VI_PIC          11 // Musik & Video & Pictures
#define LOCK_PROG_VI_MU         12 // Programs & Video & Musik
#define LOCK_PROG_PIC_MU        13 // Programs & Pictures & Musik
#define LOCK_PROG_PIC_VI        14 // Musik & Picture & Video
#define LOCK_MU_VI_PIC_PROG     15 // Musik & Video & Pictures & Programs



#define LED_PLAYBACK_OFF     0
#define LED_PLAYBACK_VIDEO    1
#define LED_PLAYBACK_MUSIC    2
#define LED_PLAYBACK_VIDEO_MUSIC 3

#define SPIN_DOWN_NONE  0
#define SPIN_DOWN_MUSIC  1
#define SPIN_DOWN_VIDEO  2
#define SPIN_DOWN_BOTH  3

#define NETWORK_DASH   0
#define NETWORK_DHCP   1
#define NETWORK_STATIC  2

#define SETTINGS_TYPE_BOOL   1
#define SETTINGS_TYPE_FLOAT   2
#define SETTINGS_TYPE_INT    3
#define SETTINGS_TYPE_STRING  4
#define SETTINGS_TYPE_HEX    5

#define CHECKMARK_CONTROL    1
#define SPIN_CONTROL_FLOAT   2
#define SPIN_CONTROL_INT    3
#define SPIN_CONTROL_INT_PLUS  4
#define SPIN_CONTROL_TEXT    5
#define BUTTON_CONTROL_INPUT    6
#define BUTTON_CONTROL_HIDDEN_INPUT 7
#define BUTTON_CONTROL_STANDARD   8
#define BUTTON_CONTROL_IP_INPUT   9
#define BUTTON_CONTROL_MISC_INPUT 10

#define REPLAY_GAIN_NONE 0
#define REPLAY_GAIN_ALBUM 1
#define REPLAY_GAIN_TRACK 2

// replay gain settings struct for quick access by the player multiple
// times per second (saves doing settings lookup)
struct ReplayGainSettings
{
  int iPreAmp;
  int iNoGainPreAmp;
  int iType;
  bool bAvoidClipping;
};

// base class for all settings types
class CSetting
{
public:
  CSetting(int iOrder, const char *strSetting, int iLabel, int iControlType) { m_iOrder = iOrder; m_strSetting = strSetting; m_iLabel = iLabel; m_iControlType = iControlType;};
  ~CSetting() {};
  virtual int GetType() { return 0; };
  int GetControlType() { return m_iControlType; };
  virtual void FromString(const CStdString &strValue) {};
  virtual CStdString ToString() { return ""; };
  const char *GetSetting() { return m_strSetting.c_str(); };
  int GetLabel() { return m_iLabel; };
  int GetOrder() const { return m_iOrder; };
private:
  int m_iControlType;
  int m_iLabel;
  int m_iOrder;
  CStdString m_strSetting;
};

class CSettingBool : public CSetting
{
public:
  CSettingBool(int iOrder, const char *strSetting, int iLabel, bool bData, int iControlType): CSetting(iOrder, strSetting, iLabel, iControlType) { m_bData = bData; };
  ~CSettingBool() {};

  virtual int GetType() { return SETTINGS_TYPE_BOOL; };
  virtual void FromString(const CStdString &strValue);
  virtual CStdString ToString();

  void SetData(bool bData) { m_bData = bData; };
  bool GetData() const { return m_bData; };

private:
  bool m_bData;
};

class CSettingFloat : public CSetting
{
public:
  CSettingFloat(int iOrder, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType);
  ~CSettingFloat() {};

  virtual int GetType() { return SETTINGS_TYPE_FLOAT; };
  virtual void FromString(const CStdString &strValue);
  virtual CStdString ToString();

  void SetData(float fData) { m_fData = fData; if (m_fData < m_fMin) m_fData = m_fMin; if (m_fData > m_fMax) m_fData = m_fMax;};
float GetData() const { return m_fData; };

  float m_fMin;
  float m_fStep;
  float m_fMax;

private:
  float m_fData;
};

class CSettingInt : public CSetting
{
public:
  CSettingInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat);
  CSettingInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin);
  ~CSettingInt() {};

  virtual int GetType() { return SETTINGS_TYPE_INT; };
  virtual void FromString(const CStdString &strValue);
  virtual CStdString ToString();

  void SetData(int iData) { m_iData = iData; if (m_iData < m_iMin) m_iData = m_iMin; if (m_iData > m_iMax) m_iData = m_iMax;};
int GetData() const { return m_iData; };

  int m_iMin;
  int m_iStep;
  int m_iMax;
  int m_iFormat;
  int m_iLabelMin;
  CStdString m_strFormat;

protected:
  int m_iData;
};

class CSettingHex : public CSettingInt
{
public:
  CSettingHex(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
      : CSettingInt(iOrder, strSetting, iLabel, iData, iMin, iStep, iMax, iControlType, strFormat) {};
  ~CSettingHex() {};
  virtual void FromString(const CStdString &strValue);
  virtual CStdString ToString();
  virtual int GetType() { return SETTINGS_TYPE_HEX; };
};

class CSettingString : public CSetting
{
public:
  CSettingString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty);
  ~CSettingString() {};

  virtual int GetType() { return SETTINGS_TYPE_STRING; };
  virtual void FromString(const CStdString &strValue);
  virtual CStdString ToString();

  void SetData(const char *strData) { m_strData = strData; };
  CStdString GetData() const { return m_strData; };

  bool m_bAllowEmpty;
private:
  CStdString m_strData;
};

class CSettingsCategory
{
public:
  CSettingsCategory(const char *strCategory, DWORD dwLabelID)
  {
    m_strCategory = strCategory;
    m_dwLabelID = dwLabelID;
  }
  ~CSettingsCategory() {};

  CStdString m_strCategory;
  DWORD m_dwLabelID;
};

typedef std::vector<CSettingsCategory *> vecSettingsCategory;

class CSettingsGroup
{
public:
  CSettingsGroup(DWORD dwGroupID, DWORD dwLabelID)
  {
    m_dwGroupID = dwGroupID;
    m_dwLabelID = dwLabelID;
  }
  ~CSettingsGroup()
  {
    for (unsigned int i = 0; i < m_vecCategories.size(); i++)
      delete m_vecCategories[i];
    m_vecCategories.clear();
  };

  void AddCategory(const char *strCategory, DWORD dwLabelID)
  {
    CSettingsCategory *pCategory = new CSettingsCategory(strCategory, dwLabelID);
    if (pCategory)
      m_vecCategories.push_back(pCategory);
  }
  void GetCategories(vecSettingsCategory &vecCategories)
  {
    vecCategories.clear();
    for (unsigned int i = 0; i < m_vecCategories.size(); i++)
      vecCategories.push_back(m_vecCategories[i]);
  };
DWORD GetLabelID() { return m_dwLabelID; };
  DWORD GetGroupID() { return m_dwGroupID; };
private:
  vecSettingsCategory m_vecCategories;
  DWORD m_dwGroupID;
  DWORD m_dwLabelID;
};

typedef std::vector<CSetting *> vecSettings;

class CGUISettings
{
public:
  CGUISettings();
  ~CGUISettings();

  void AddGroup(DWORD dwGroupID, DWORD dwLabelID);
  void AddCategory(DWORD dwGroupID, const char *strCategory, DWORD dwLabelID);
  CSettingsGroup *GetGroup(DWORD dwWindowID);

  void AddBool(int iOrder, const char *strSetting, int iLabel, bool bSetting, int iControlType = CHECKMARK_CONTROL);
  bool GetBool(const char *strSetting);
  void SetBool(const char *strSetting, bool bSetting);
  void ToggleBool(const char *strSetting);

  void AddFloat(int iOrder, const char *strSetting, int iLabel, float fSetting, float fMin, float fStep, float fMax, int iControlType = SPIN_CONTROL_FLOAT);
  float GetFloat(const char *strSetting);
  void SetFloat(const char *strSetting, float fSetting);

  void AddInt(int iOrder, const char *strSetting, int iLabel, int fSetting, int iMin, int iStep, int iMax, int iControlType, const char *strFormat = NULL);
  void AddInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin=-1);
  int GetInt(const char *strSetting);
  void SetInt(const char *strSetting, int fSetting);

  void AddHex(int iOrder, const char *strSetting, int iLabel, int fSetting, int iMin, int iStep, int iMax, int iControlType, const char *strFormat = NULL);

  void AddString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType = BUTTON_CONTROL_INPUT, bool bAllowEmpty = false);
  CStdString GetString(const char *strSetting);
  void SetString(const char *strSetting, const char *strData);

  CSetting *GetSetting(const char *strSetting);

  void ReadXML(TiXmlElement* pRootElement, const char *strSetting);
  void WriteXML(TiXmlNode* pRootNode , const char *strSetting);

  void GetSettingsGroup(const char *strGroup, vecSettings &settings);
  void LoadXML(TiXmlElement *pRootElement);
  void SaveXML(TiXmlNode *pRootNode);

  //m_LookAndFeelResolution holds the real gui resolution,
  //also when g_guiSettings.GetInt("LookAndFeel.Resolution") is set to AUTORES
  RESOLUTION m_LookAndFeelResolution;
  ReplayGainSettings m_replayGain;

  void Clear();

private:
  typedef std::map<CStdString, CSetting*>::iterator mapIter;
  std::map<CStdString, CSetting*> settingsMap;
  std::vector<CSettingsGroup *> settingsGroups;
};

extern class CGUISettings g_guiSettings;
