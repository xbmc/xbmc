#pragma once

class CLangInfo
{
public:
  CLangInfo();
  virtual ~CLangInfo();
  bool Load(const CStdString& strFileName);
  CStdString GetGuiCharSet() const;
  CStdString GetSubtitleCharSet() const;
  CStdString GetDVDMenuLanguage() const;
  CStdString GetDVDAudioLanguage() const;
  CStdString GetDVDSubtitleLanguage() const;
  bool ForceUnicodeFont() { return m_forceUnicodeFont; }

protected:
  void Clear();

protected:
  CStdString m_strGuiCharSet;
  CStdString m_strSubtitleCharSet;
  CStdString m_strDVDMenuLanguage;
  CStdString m_strDVDAudioLanguage;
  CStdString m_strDVDSubtitleLanguage;
  bool m_forceUnicodeFont;
};


extern CLangInfo g_langInfo;
