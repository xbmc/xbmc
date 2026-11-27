/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CFileItem;
class CFileItemList;

namespace KODI::PLAYLIST
{

/*!
 * \brief What a library adds to turning a selection into playlist entries. The walk through
 * folders and playlist files is CApplicationPlayLists::ExpandToEntries.
 */
class IEntryRules
{
public:
  virtual ~IEntryRules() = default;

  //! What to expand in place of a folder: the folder itself if nothing else, nullptr for nothing.
  virtual std::shared_ptr<CFileItem> Redirect(const std::shared_ptr<CFileItem>& folder)
  {
    return folder;
  }

  //! Whether a locked source may be expanded.
  virtual bool IsUnlocked(CFileItem& source) = 0;

  /*!
   * \brief Order a folder's listing and drop what is not queued from it.
   * \param startAt Where playing starts; the rules may choose it if nobody did.
   */
  virtual void Arrange(const CFileItem& folder,
                       CFileItemList& items,
                       std::shared_ptr<CFileItem>& startAt) = 0;

  /*!
   * \brief The entry a file becomes.
   * \param entries What has been expanded so far.
   * \return nullptr if the file is not queued.
   */
  virtual std::shared_ptr<CFileItem> Accept(const std::shared_ptr<CFileItem>& file,
                                            const CFileItemList& entries) = 0;
};

/*!
 * \brief Every file that can be an entry, in the order it is listed, with no lock of its own: for
 * a playlist file or smart playlist played as it stands.
 */
class CEntriesAsListed final : public IEntryRules
{
public:
  bool IsUnlocked(CFileItem& source) override;
  void Arrange(const CFileItem& folder,
               CFileItemList& items,
               std::shared_ptr<CFileItem>& startAt) override;
  std::shared_ptr<CFileItem> Accept(const std::shared_ptr<CFileItem>& file,
                                    const CFileItemList& entries) override;
};

} // namespace KODI::PLAYLIST
