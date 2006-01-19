#pragma once
#include "thread.h"

#define MAX_ROWS 20

class ILCD : public CThread
{
public:
  enum LCD_MODE { LCD_MODE_GENERAL = 0, LCD_MODE_MUSIC, LCD_MODE_VIDEO, LCD_MODE_NAVIGATION, LCD_MODE_MAX };
  virtual void Initialize() = 0;
  virtual void Stop() = 0;
  virtual void SetBackLight(int iLight) = 0;
  virtual void SetContrast(int iContrast) = 0;
  virtual void SetLine(int iLine, const CStdString& strLine) = 0;
  CStdString GetProgressBar(double tCurrent, double tTotal);
  void LoadSkin(const CStdString &xmlFile);
  void Reset();
  void Render(LCD_MODE mode);
protected:
  virtual void Process() = 0;
  void StringToLCDCharSet(CStdString& strText);
  void LoadMode(TiXmlNode *node, LCD_MODE mode);
private:
  vector<CStdString> m_lcdMode[4];
};
extern ILCD* g_lcd;
