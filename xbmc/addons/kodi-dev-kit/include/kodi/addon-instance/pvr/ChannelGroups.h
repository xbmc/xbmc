/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr.h"

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" Definitions group 3 - PVR channel group
#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroup class PVRChannelGroup
/// @ingroup cpp_kodi_addon_pvr_Defs_ChannelGroup
/// @brief **PVR add-on channel group**\n
/// To define a group for channels, this becomes be asked from
/// @ref kodi::addon::CInstancePVRClient::GetChannelGroups() and used on
/// @ref kodi::addon::CInstancePVRClient::GetChannelGroupMembers() to get his
/// content with @ref cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupMember "PVRChannelGroupMember".
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroup_Help
///
///@{
class PVRChannelGroup : public CStructHdl<PVRChannelGroup, PVR_CHANNEL_GROUP>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRChannelGroup() { memset(m_cStructure, 0, sizeof(PVR_CHANNEL_GROUP)); }
  PVRChannelGroup(const PVRChannelGroup& channel) : CStructHdl(channel) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroup_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroup
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroup :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **Group name** | `std::string` | @ref PVRChannelGroup::SetGroupName "SetGroupName" | @ref PVRChannelGroup::GetGroupName "GetGroupName" | *required to set*
  /// | **Is radio** | `bool` | @ref PVRChannelGroup::SetIsRadio "SetIsRadio" | @ref PVRChannelGroup::GetIsRadio "GetIsRadio" | *required to set*
  /// | **Position** | `unsigned int` | @ref PVRChannelGroup::SetPosition "SetPosition" | @ref PVRChannelGroup::GetPosition "GetPosition" | *optional*
  ///

  /// @ingroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroup
  ///@{

  /// @brief **required**\n
  /// Name of this channel group.
  void SetGroupName(const std::string& groupName)
  {
    strncpy(m_cStructure->strGroupName, groupName.c_str(), sizeof(m_cStructure->strGroupName) - 1);
  }

  /// @brief To get with @ref SetGroupName changed values.
  std::string GetGroupName() const { return m_cStructure->strGroupName; }

  /// @brief **required**\n
  /// **true** If this is a radio channel group, **false** otherwise.
  void SetIsRadio(bool isRadio) { m_cStructure->bIsRadio = isRadio; }

  /// @brief To get with @ref SetIsRadio changed values.
  bool GetIsRadio() const { return m_cStructure->bIsRadio; }

  /// @brief **optional**\n
  /// Sort position of the group (<b>`0`</b> indicates that the backend doesn't
  /// support sorting of groups).
  void SetPosition(unsigned int position) { m_cStructure->iPosition = position; }

  /// @brief To get with @ref SetPosition changed values.
  unsigned int GetPosition() const { return m_cStructure->iPosition; }

  ///@}

private:
  PVRChannelGroup(const PVR_CHANNEL_GROUP* channel) : CStructHdl(channel) {}
  PVRChannelGroup(PVR_CHANNEL_GROUP* channel) : CStructHdl(channel) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupsResultSet class PVRChannelGroupsResultSet
/// @ingroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroup
/// @brief **PVR add-on channel group member transfer class**\n
/// To transfer the content of @ref kodi::addon::CInstancePVRClient::GetChannelGroups().
///
///@{
class PVRChannelGroupsResultSet
{
public:
  /*! \cond PRIVATE */
  PVRChannelGroupsResultSet() = delete;
  PVRChannelGroupsResultSet(const AddonInstance_PVR* instance, PVR_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }
  /*! \endcond */

  /// @addtogroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupsResultSet
  ///@{

  /// @brief To add and give content from addon to Kodi on related call.
  ///
  /// @param[in] tag The to transferred data.
  void Add(const kodi::addon::PVRChannelGroup& tag)
  {
    m_instance->toKodi->TransferChannelGroup(m_instance->toKodi->kodiInstance, m_handle, tag);
  }

  ///@}

private:
  const AddonInstance_PVR* m_instance = nullptr;
  const PVR_HANDLE m_handle;
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupMember class PVRChannelGroupMember
/// @ingroup cpp_kodi_addon_pvr_Defs_ChannelGroup
/// @brief **PVR add-on channel group member**\n
/// To define the content of @ref kodi::addon::CInstancePVRClient::GetChannelGroups()
/// given groups.
///
/// This content becomes then requested with @ref kodi::addon::CInstancePVRClient::GetChannelGroupMembers().
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupMember_Help
///
///@{
class PVRChannelGroupMember : public CStructHdl<PVRChannelGroupMember, PVR_CHANNEL_GROUP_MEMBER>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRChannelGroupMember() { memset(m_cStructure, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER)); }
  PVRChannelGroupMember(const PVRChannelGroupMember& channel) : CStructHdl(channel) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupMember_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupMember
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupMember :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |-------|-------|-----------|----------|-----------
  /// | **Group name** | `std::string` | @ref PVRChannelGroupMember::SetGroupName "SetGroupName" | @ref PVRChannelGroupMember::GetGroupName "GetGroupName" | *required to set*
  /// | **Channel unique id** | `unsigned int` | @ref PVRChannelGroupMember::SetChannelUniqueId "SetChannelUniqueId" | @ref PVRChannelGroupMember::GetChannelUniqueId "GetChannelUniqueId" | *required to set*
  /// | **Channel Number** | `unsigned int` | @ref PVRChannelGroupMember::SetChannelNumber "SetChannelNumber" | @ref PVRChannelGroupMember::GetChannelNumber "GetChannelNumber" | *optional*
  /// | **Sub channel number** | `unsigned int` | @ref PVRChannelGroupMember::SetSubChannelNumber "SetSubChannelNumber"| @ref PVRChannelGroupMember::GetSubChannelNumber "GetSubChannelNumber" | *optional*
  /// | **Order** | `int` | @ref PVRChannel::SetOrder "SetOrder" | @ref PVRChannel::GetOrder "GetOrder" | *optional*
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupMember
  ///@{

  /// @brief **required**\n
  /// Name of the channel group to add the channel to.
  void SetGroupName(const std::string& groupName)
  {
    strncpy(m_cStructure->strGroupName, groupName.c_str(), sizeof(m_cStructure->strGroupName) - 1);
  }

  /// @brief To get with @ref SetGroupName changed values.
  std::string GetGroupName() const { return m_cStructure->strGroupName; }

  /// @brief **required**\n
  /// Unique id of the member.
  void SetChannelUniqueId(unsigned int channelUniqueId)
  {
    m_cStructure->iChannelUniqueId = channelUniqueId;
  }

  /// @brief To get with @ref SetChannelUniqueId changed values.
  unsigned int GetChannelUniqueId() const { return m_cStructure->iChannelUniqueId; }

  /// @brief **optional**\n
  /// Channel number within the group.
  void SetChannelNumber(unsigned int channelNumber)
  {
    m_cStructure->iChannelNumber = channelNumber;
  }

  /// @brief To get with @ref SetChannelNumber changed values.
  unsigned int GetChannelNumber() const { return m_cStructure->iChannelNumber; }

  /// @brief **optional**\n
  /// Sub channel number within the group (ATSC).
  void SetSubChannelNumber(unsigned int subChannelNumber)
  {
    m_cStructure->iSubChannelNumber = subChannelNumber;
  }

  /// @brief To get with @ref SetSubChannelNumber changed values.
  unsigned int GetSubChannelNumber() const { return m_cStructure->iSubChannelNumber; }

  /// @brief **optional**\n
  /// The value denoting the order of this channel in the <b>'All channels'</b> group.
  void SetOrder(bool order) { m_cStructure->iOrder = order; }

  /// @brief To get with @ref SetOrder changed values.
  bool GetOrder() const { return m_cStructure->iOrder; }

  ///@}

private:
  PVRChannelGroupMember(const PVR_CHANNEL_GROUP_MEMBER* channel) : CStructHdl(channel) {}
  PVRChannelGroupMember(PVR_CHANNEL_GROUP_MEMBER* channel) : CStructHdl(channel) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupMembersResultSet class PVRChannelGroupMembersResultSet
/// @ingroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupMember
/// @brief **PVR add-on channel group member transfer class**\n
/// To transfer the content of @ref kodi::addon::CInstancePVRClient::GetChannelGroupMembers().
///
///@{
class PVRChannelGroupMembersResultSet
{
public:
  /*! \cond PRIVATE */
  PVRChannelGroupMembersResultSet() = delete;
  PVRChannelGroupMembersResultSet(const AddonInstance_PVR* instance, PVR_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }
  /*! \endcond */

  /// @addtogroup cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupMembersResultSet
  ///@{

  /// @brief To add and give content from addon to Kodi on related call.
  ///
  /// @param[in] tag The to transferred data.
  void Add(const kodi::addon::PVRChannelGroupMember& tag)
  {
    m_instance->toKodi->TransferChannelGroupMember(m_instance->toKodi->kodiInstance, m_handle, tag);
  }

  ///@}

private:
  const AddonInstance_PVR* m_instance = nullptr;
  const PVR_HANDLE m_handle;
};
///@}
//------------------------------------------------------------------------------

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
