#pragma once

#include "GUIDialog.h"
#include "GUISpinControlEx.h"
#include "GUIRadioButtonControl.h"
#include "GUIButtonControl.h"
#include "GUISettingsSliderControl.h"

class VideoSetting
{
public:
  enum SETTING_TYPE { NONE=0, BUTTON, CHECK, SPIN, SLIDER, SLIDER_INT, SEPARATOR };
  VideoSetting()
  {
    id = 0;
    data = NULL;
    type = NONE;
  };
  SETTING_TYPE type;
  CStdString name;
  unsigned int id;
  void *data;
  float min;
  float max;
  float interval;
  CStdString format;
  std::vector<CStdString> entry;
};

class CGUIDialogVideoSettings :
      public CGUIDialog
{
public:
  CGUIDialogVideoSettings(void);
  virtual ~CGUIDialogVideoSettings(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();
  virtual bool HasID(DWORD dwID);
protected:
  virtual void SetupPage();
  void CreateSettings();
  void UpdateSetting(unsigned int setting);
  void OnSettingChanged(unsigned int setting);
  virtual void FreeControls();
  void OnClick(int iControlID);
  void AddSetting(VideoSetting &setting, int iPosX, int iPosY, int iWidth, int iControlID);

  void AddButton(unsigned int it, int label);
  void AddBool(unsigned int id, int label, bool *on);
  void AddSpin(unsigned int id, int label, int *current, unsigned int max, const int *entries);
  void AddSpin(unsigned int id, int label, int *current, unsigned int min, unsigned int max);
  void AddSlider(unsigned int id, int label, float *current, float min, float interval, float max, const char *format = NULL);
  void AddSlider(unsigned int id, int label, int *current, int min, int max);
  void AddAudioStreams(unsigned int id);
  void AddSubtitleStreams(unsigned int id);
  void AddSeparator(unsigned int id);

  int m_iLastControl;
  int m_iScreen;      // current screen
  int m_iPageOffset;  // offset into the settings list of our current page.
  int m_iCurrentPage;
  int m_iNumPages;
  int m_iNumPerPage;

  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalSettingsButton;
  CGUISettingsSliderControl *m_pOriginalSlider;
  CGUIImage *m_pOriginalSeparator;

  vector<VideoSetting> m_settings;

  int m_flickerFilter;
  bool m_soften;
  float m_volume;
  int m_audioStream;
  int m_subtitleStream;
};
