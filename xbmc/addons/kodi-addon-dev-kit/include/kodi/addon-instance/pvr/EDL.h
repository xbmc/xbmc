/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr/pvr_edl.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class PVREDLEntry : public CStructHdl<PVREDLEntry, PVR_EDL_ENTRY>
{
public:
  PVREDLEntry() { memset(m_cStructure, 0, sizeof(PVR_EDL_ENTRY)); }
  PVREDLEntry(const PVREDLEntry& type) : CStructHdl(type) {}
  PVREDLEntry(const PVR_EDL_ENTRY* type) : CStructHdl(type) {}
  PVREDLEntry(PVR_EDL_ENTRY* type) : CStructHdl(type) {}

  void SetStart(int64_t start) { m_cStructure->start = start; }
  int64_t GetStart() const { return m_cStructure->start; }

  void SetEnd(int64_t end) { m_cStructure->end = end; }
  int64_t GetEnd() const { return m_cStructure->end; }

  void SetType(PVR_EDL_TYPE type) { m_cStructure->type = type; }
  PVR_EDL_TYPE GetType() const { return m_cStructure->type; }
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
