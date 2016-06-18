#pragma once
/*
 *      Copyright (C) 2005-2015 Team KODI
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <utility>
#include <vector>

#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSPProcess.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

namespace ActiveAE
{
  class CGUIDialogAudioDSPSettings : public CGUIDialogSettingsManualBase
  {
  public:
    CGUIDialogAudioDSPSettings();
    virtual ~CGUIDialogAudioDSPSettings();

    // specializations of CGUIControl
    virtual bool OnMessage(CGUIMessage &message);
    virtual bool OnBack(int actionID);

    // specialization of CGUIWindow
    virtual void FrameMove();

    static std::string FormatDelay(float value, float interval);
    static std::string FormatDecibel(float value);
    static std::string FormatPercentAsDecibel(float value);

  protected:
    // implementations of ISettingCallback
    virtual void OnSettingChanged(const CSetting *setting);
    virtual void OnSettingAction(const CSetting *setting);

    // specialization of CGUIDialogSettingsBase
    virtual bool AllowResettingSettings() const { return false; }
    virtual void Save();
    virtual void SetupView();

    // specialization of CGUIDialogSettingsManualBase
    virtual void InitializeSettings();

    bool SupportsAudioFeature(int feature);

    static void AudioModeOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

    static std::string SettingFormatterDelay(const CSettingControlSlider *control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum);
    static std::string SettingFormatterPercentAsDecibel(const CSettingControlSlider *control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum);

    std::string GetSettingsLabel(CSetting *pSetting);

  private:
    typedef struct
    {
      int              addonId;
      AE_DSP_MENUHOOK  hook;
    } MenuHookMember;
    typedef struct
    {
      std::string      MenuName;
      int              MenuListPtr;
      std::string      CPUUsage;
    } ActiveModeData;
    typedef std::vector<int> Features;

    void OpenMenu(const std::string &id);
    bool HaveActiveMenuHooks(AE_DSP_MENUHOOK_CAT category);
    void GetAudioDSPMenus(CSettingGroup *group, AE_DSP_MENUHOOK_CAT category);
    bool OpenAudioDSPMenu(unsigned int setupEntry);
    int FindCategoryIndex(const std::string &catId);

    AE_DSP_STREAM_ID                            m_ActiveStreamId;                         /*!< The on dialog selectable stream identifier */
    ActiveAE::CActiveAEDSPProcessPtr            m_ActiveStreamProcess;                    /*!< On dialog adjustable dsp processing class */
    AE_DSP_STREAMTYPE                           m_streamTypeUsed;                         /*!< The currently available stream type */
    AE_DSP_BASETYPE                             m_baseTypeUsed;                           /*!< The currently detected and used base type */
    int                                         m_modeTypeUsed;                           /*!< The currently selected mode type */
    std::vector<ActiveAE::CActiveAEDSPModePtr>  m_ActiveModes;                            /*!< The process modes currently active on dsp processing stream */
    std::vector<ActiveModeData>                 m_ActiveModesData;                        /*!< The process modes currently active on dsp processing stream info*/
    std::vector<ActiveAE::CActiveAEDSPModePtr>  m_MasterModes[AE_DSP_ASTREAM_MAX];        /*!< table about selectable and usable master processing modes */
    std::map<std::string, int>                  m_MenuPositions;                          /*!< The differnet menu selection positions */
    std::vector<int>                            m_MenuHierarchy;                          /*!< Menu selection flow hierachy */
    std::vector<MenuHookMember>                 m_Menus;                                  /*!< storage about present addon menus on currently selected submenu */
    std::vector< std::pair<std::string, int> >  m_ModeList;                               /*!< currently present modes */
    bool                                        m_GetCPUUsage;                            /*!< if true cpu usage detection is active */
    Features                                    m_audioCaps;                              /*!< the on current playback on KODI supported audio features */
    int                                         m_MenuName;                               /*!< current menu name, needed to get after the dialog was closed for addon */

    /*! Settings control selection and information data */
    std::string                                 m_InputChannels;
    std::string                                 m_InputChannelNames;
    std::string                                 m_InputSamplerate;
    std::string                                 m_OutputChannels;
    std::string                                 m_OutputChannelNames;
    std::string                                 m_OutputSamplerate;
    std::string                                 m_CPUUsage;
    float                                       m_volume;
  };
}
