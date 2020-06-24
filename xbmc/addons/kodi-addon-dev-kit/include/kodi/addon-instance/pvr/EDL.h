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

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" Definitions group 8 - PVR Edit definition list (EDL)
#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_EDLEntry_PVREDLEntry class PVREDLEntry
/// @ingroup cpp_kodi_addon_pvr_Defs_EDLEntry
/// @brief **Edit definition list (EDL) entry**\n
/// Time places and type of related fields.
///
/// This used within @ref cpp_kodi_addon_pvr_EPGTag "EPG" and
/// @ref cpp_kodi_addon_pvr_Recordings "recordings".
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_EDLEntry_PVREDLEntry_Help
///
///@{
class PVREDLEntry : public CStructHdl<PVREDLEntry, PVR_EDL_ENTRY>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVREDLEntry() { memset(m_cStructure, 0, sizeof(PVR_EDL_ENTRY)); }
  PVREDLEntry(const PVREDLEntry& type) : CStructHdl(type) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_EDLEntry_PVREDLEntry_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_EDLEntry_PVREDLEntry
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_EDLEntry_PVREDLEntry :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **Start time** | `int64_t` | @ref PVREDLEntry::SetStart "SetStart" | @ref PVREDLEntry::GetStart "GetStart" | *required to set*
  /// | **End time** | `int64_t` | @ref PVREDLEntry::SetEnd "SetEnd" | @ref PVREDLEntry::GetEnd "GetEnd" | *required to set*
  /// | **Type** | @ref PVR_EDL_TYPE | @ref PVREDLEntry::SetType "SetType" | @ref PVREDLEntry::GetType "GetType" | *required to set*
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_EDLEntry_PVREDLEntry
  ///@{

  /// @brief Start time in milliseconds.
  void SetStart(int64_t start) { m_cStructure->start = start; }

  /// @brief To get with @ref SetStart() changed values.
  int64_t GetStart() const { return m_cStructure->start; }

  /// @brief End time in milliseconds.
  void SetEnd(int64_t end) { m_cStructure->end = end; }

  /// @brief To get with @ref SetEnd() changed values.
  int64_t GetEnd() const { return m_cStructure->end; }

  /// @brief The with @ref PVR_EDL_TYPE used definition list type.
  void SetType(PVR_EDL_TYPE type) { m_cStructure->type = type; }

  /// @brief To get with @ref SetType() changed values.
  PVR_EDL_TYPE GetType() const { return m_cStructure->type; }
  ///@}

private:
  PVREDLEntry(const PVR_EDL_ENTRY* type) : CStructHdl(type) {}
  PVREDLEntry(PVR_EDL_ENTRY* type) : CStructHdl(type) {}
};
///@}
//------------------------------------------------------------------------------

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
