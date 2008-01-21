#pragma once

#include "Profile.h"
#include "../guilib/tinyXML/tinyxml.h"
#include <vector>
#include <map>
#include "../guilib/GraphicContext.h"

// Render Methods
#define RENDER_LQ_RGB_SHADER   0
#define RENDER_OVERLAYS      1
#define RENDER_HQ_RGB_SHADER   2
#define RENDER_HQ_RGB_SHADERV2   3

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

#define VIDEO_NORMAL 0
#define VIDEO_LETTERBOX 1
#define VIDEO_WIDESCREEN 2

// LCD settings
#define LCD_TYPE_NONE        0
#define LCD_TYPE_LCD_HD44780 1
#define LCD_TYPE_LCD_KS0073  2
#define LCD_TYPE_VFD         3

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

#define FRAME_RATE_LEAVE_AS_IS  0
#define FRAME_RATE_CONVERT      1
#define FRAME_RATE_USE_PAL60    2

#define LED_PLAYBACK_OFF     0
#define LED_PLAYBACK_VIDEO    1
#define LED_PLAYBACK_MUSIC    2
#define LED_PLAYBACK_VIDEO_MUSIC 3

#define SPIN_DOWN_NONE  0
#define SPIN_DOWN_MUSIC  1
#define SPIN_DOWN_VIDEO  2
#define SPIN_DOWN_BOTH  3

#define AAM_QUIET 1
#define AAM_FAST  0

#define APM_HIPOWER 0
#define APM_LOPOWER 1
#define APM_HIPOWER_STANDBY 2
#define APM_LOPOWER_STANDBY 3

#define NETWORK_DASH   0
#define NETWORK_DHCP   1
#define NETWORK_STATIC  2

#define SETTINGS_TYPE_BOOL   1
#define SETTINGS_TYPE_FLOAT   2
#define SETTINGS_TYPE_INT    3
#define SETTINGS_TYPE_STRING  4
#define SETTINGS_TYPE_HEX    5
#define SETTINGS_TYPE_SEPARATOR 6

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
#define BUTTON_CONTROL_PATH_INPUT 11
#define SEPARATOR_CONTROL 12

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
  CSetting(int iOrder, const char *strSetting, int iLabel, int iControlType) { m_iOrder = iOrder; m_strSetting = strSetting; m_iLabel = iLabel; m_iControlType = iControlType; m_advanced = false; };
  virtual ~CSetting() {};
  virtual int GetType() { return 0; };
  int GetControlType() { return m_iControlType; };
  virtual void FromString(const CStdString &strValue) {};
  virtual CStdString ToString() { return ""; };
  const char *GetSetting() { return m_strSetting.c_str(); };
  int GetLabel() { return m_iLabel; };
  int GetOrder() const { return m_iOrder; };
  void SetAdvanced() { m_advanced = true; };
  bool IsAdvanced() { return m_advanced; };
private:
  int m_iControlType;
  int m_iLabel;
  int m_iOrder;
  bool m_advanced;
  CStdString m_strSetting;
};

class CSettingBool : public CSetting
{
public:
  CSettingBool(int iOrder, const char *strSetting, int iLabel, bool bData, int iControlType): CSetting(iOrder, strSetting, iLabel, iControlType) { m_bData = bData; };
  virtual ~CSettingBool() {};

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
  virtual ~CSettingFloat() {};

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
  virtual ~CSettingInt() {};

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
  virtual ~CSettingHex() {};
  virtual void FromString(const CStdString &strValue);
  virtual CStdString ToString();
  virtual int GetType() { return SETTINGS_TYPE_HEX; };
};

class CSettingString : public CSetting
{
public:
  CSettingString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString);
  virtual ~CSettingString() {};

  virtual int GetType() { return SETTINGS_TYPE_STRING; };
  virtual void FromString(const CStdString &strValue);
  virtual CStdString ToString();

  void SetData(const char *strData) { m_strData = strData; };
  const CStdString &GetData() const { return m_strData; };

  bool m_bAllowEmpty;
  int m_iHeadingString;
private:
  CStdString m_strData;
};

class CSettingSeparator : public CSetting
{
public:
  CSettingSeparator(int iOrder, const char *strSetting);
  virtual ~CSettingSeparator() {};

  virtual int GetType() { return SETTINGS_TYPE_SEPARATOR; };
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
  void GetCategories(vecSettingsCategory &vecCategories);
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
  bool GetBool(const char *strSetting) const;
  void SetBool(const char *strSetting, bool bSetting);
  void ToggleBool(const char *strSetting);

  void AddFloat(int iOrder, const char *strSetting, int iLabel, float fSetting, float fMin, float fStep, float fMax, int iControlType = SPIN_CONTROL_FLOAT);
  float GetFloat(const char *strSetting) const;
  void SetFloat(const char *strSetting, float fSetting);

  void AddInt(int iOrder, const char *strSetting, int iLabel, int fSetting, int iMin, int iStep, int iMax, int iControlType, const char *strFormat = NULL);
  void AddInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin=-1);
  int GetInt(const char *strSetting) const;
  void SetInt(const char *strSetting, int fSetting);

  void AddHex(int iOrder, const char *strSetting, int iLabel, int fSetting, int iMin, int iStep, int iMax, int iControlType, const char *strFormat = NULL);

  void AddString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType = BUTTON_CONTROL_INPUT, bool bAllowEmpty = false, int iHeadingString = -1);
  const CStdString &GetString(const char *strSetting, bool bPrompt=true) const;
  void SetString(const char *strSetting, const char *strData);

  void AddSeparator(int iOrder, const char *strSetting);

  CSetting *GetSetting(const char *strSetting);

  void GetSettingsGroup(const char *strGroup, vecSettings &settings);
  void LoadXML(TiXmlElement *pRootElement, bool hideSettings = false);
  void SaveXML(TiXmlNode *pRootNode);
  void LoadMasterLock(TiXmlElement *pRootElement);

  //m_LookAndFeelResolution holds the real gui resolution,
  //also when g_guiSettings.GetInt("videoscreen.resolution") is set to AUTORES
  RESOLUTION m_LookAndFeelResolution;
  ReplayGainSettings m_replayGain;

  void Clear();

private:
  typedef std::map<CStdString, CSetting*>::iterator mapIter;
  typedef std::map<CStdString, CSetting*>::const_iterator constMapIter;
  std::map<CStdString, CSetting*> settingsMap;
  std::vector<CSettingsGroup *> settingsGroups;
  void LoadFromXML(TiXmlElement *pRootElement, mapIter &it, bool advanced = false);
};

extern class CGUISettings g_guiSettings;
