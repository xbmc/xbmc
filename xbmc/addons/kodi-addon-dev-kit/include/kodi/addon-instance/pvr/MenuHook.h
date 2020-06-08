/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr/pvr_menu_hook.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class PVRMenuhook : public CStructHdl<PVRMenuhook, PVR_MENUHOOK>
{
public:
  PVRMenuhook(unsigned int hookId, unsigned int localizedStringId, PVR_MENUHOOK_CAT category)
  {
    m_cStructure->iHookId = hookId;
    m_cStructure->iLocalizedStringId = localizedStringId;
    m_cStructure->category = category;
  }

  PVRMenuhook()
  {
    m_cStructure->iHookId = 0;
    m_cStructure->iLocalizedStringId = 0;
    m_cStructure->category = PVR_MENUHOOK_UNKNOWN;
  }
  PVRMenuhook(const PVRMenuhook& data) : CStructHdl(data) {}
  PVRMenuhook(const PVR_MENUHOOK* data) : CStructHdl(data) {}
  PVRMenuhook(PVR_MENUHOOK* data) : CStructHdl(data) {}

  void SetHookId(unsigned int hookId) { m_cStructure->iHookId = hookId; }
  unsigned int GetHookId() const { return m_cStructure->iHookId; }

  void SetLocalizedStringId(unsigned int localizedStringId)
  {
    m_cStructure->iLocalizedStringId = localizedStringId;
  }
  unsigned int GetLocalizedStringId() const { return m_cStructure->iLocalizedStringId; }

  void SetCategory(PVR_MENUHOOK_CAT category) { m_cStructure->category = category; }
  PVR_MENUHOOK_CAT GetCategory() const { return m_cStructure->category; }
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
