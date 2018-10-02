/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AEResampleFactory.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEResampleFFMPEG.h"
#if defined(TARGET_RASPBERRY_PI)
  #include "ServiceBroker.h"
  #include "settings/Settings.h"
#include "settings/SettingsComponent.h"
  #include "cores/AudioEngine/Engines/ActiveAE/ActiveAEResamplePi.h"
#endif

namespace ActiveAE
{

IAEResample *CAEResampleFactory::Create(uint32_t flags /* = 0 */)
{
#if defined(TARGET_RASPBERRY_PI)
  if (!(flags & AERESAMPLEFACTORY_QUICK_RESAMPLE) && CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_AUDIOOUTPUT_PROCESSQUALITY) == AE_QUALITY_GPU)
    return new CActiveAEResamplePi();
#endif
  return new CActiveAEResampleFFMPEG();
}

}
