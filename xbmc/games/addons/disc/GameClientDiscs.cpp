/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscs.h"

#include "URL.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "games/GameUtils.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscM3U.h"
#include "games/addons/disc/GameClientDiscMergeUtils.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscTransport.h"
#include "games/addons/disc/GameClientDiscXML.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <climits>
#include <mutex>
#include <vector>

using namespace KODI;
using namespace GAME;

namespace
{
std::set<std::string> GetSupportedExtensions(const CGameClient& gameClient)
{
  const auto& extensions = gameClient.GetExtensions();
  return extensions.empty() ? CGameUtils::GetGameExtensions() : extensions;
}
} // namespace

CGameClientDiscs::CGameClientDiscs(CGameClient& gameClient,
                                   AddonInstance_Game& addonStruct,
                                   CCriticalSection& clientAccess)
  : CGameClientSubsystem(gameClient, addonStruct, clientAccess),
    m_transport(std::make_unique<CGameClientDiscTransport>(gameClient, addonStruct, clientAccess)),
    m_discXml(std::make_unique<CGameClientDiscXML>()),
    m_discM3u(std::make_unique<CGameClientDiscM3U>()),
    m_discModel(std::make_unique<CGameClientDiscModel>())
{
}

CGameClientDiscs::~CGameClientDiscs() = default;

bool CGameClientDiscs::SupportsDiscControl() const
{
  return m_gameClient.SupportsDiscControl();
}

bool CGameClientDiscs::IsMediaSupported(const CGameClientDiscModel& model) const
{
  const auto supportedExtensions = GetSupportedExtensions(m_gameClient);
  for (size_t i = 0; i < model.Size(); ++i)
  {
    if (model.IsRemovedSlotByIndex(i))
      continue;

    const auto path = model.GetPathByIndex(i);
    if (std::none_of(supportedExtensions.begin(), supportedExtensions.end(),
                     [&path](const std::string& ext) { return URIUtils::HasExtension(path, ext); }))
    {
      CLog::Log(LOGWARNING, "RetroPlayer[DISC]: Unsupported media in slot {}: {}", i,
                CURL::GetRedacted(path));
      return false;
    }
  }
  return true;
}

void CGameClientDiscs::Initialize(const std::string& gamePath, bool usePersistedState)
{
  std::unique_lock lock(m_clientAccess);

  const bool replaceInitialHint = m_initialHintAttempted;
  // Disc state is per running game session. Always reset the in-memory model
  // before loading persisted state for the game being started.
  ResetSessionState();

  const auto supportedExtensions = GetSupportedExtensions(m_gameClient);

  const bool persistedStateExists = CFileUtils::Exists(CGameClientDiscXML::GetXMLPath(gamePath));
  CGameClientDiscModel restoredModel;
  bool persistedStateValid = false;
  if (usePersistedState && persistedStateExists && m_discXml->Load(gamePath, restoredModel))
  {
    persistedStateValid = IsMediaSupported(restoredModel);
    for (size_t i = 0; persistedStateValid && i < restoredModel.Size(); ++i)
    {
      if (restoredModel.IsRemovedSlotByIndex(i))
        continue;

      const std::string path = restoredModel.GetPathByIndex(i);
      if (path.empty() || !CFileUtils::Exists(path))
      {
        CLog::Log(LOGWARNING,
                  "RetroPlayer[DISC]: Ignoring persisted disc state: slot {} has {} media {}", i,
                  path.empty() ? "empty" : "missing", CURL::GetRedacted(path));
        persistedStateValid = false;
        break;
      }
    }

    if (persistedStateValid)
    {
      *m_discModel = restoredModel;
      PruneRemovedDiscs(*m_discModel);
      m_isEjected = m_discModel->IsEjected();
      m_preserveSlotTopology = true;
      m_hasPersistedState = true;

      const std::optional<size_t> startupIndex = m_discModel->GetSelectedDiscIndex();
      if (startupIndex)
      {
        m_initialHintAttempted = true;
        m_transport->SetInitialImage(static_cast<unsigned int>(*startupIndex),
                                     m_discModel->GetPathByIndex(*startupIndex));
      }
    }
  }

  if (usePersistedState && persistedStateExists && !persistedStateValid)
    CLog::Log(LOGWARNING, "RetroPlayer[DISC]: Falling back to source media for {}",
              CURL::GetRedacted(gamePath));

  CGameClientDiscModel sourceModel;
  if (URIUtils::HasExtension(gamePath, ".m3u"))
  {
    m_discM3u->Load(gamePath, sourceModel);
    PruneExtensions(sourceModel, supportedExtensions);
  }
  if (sourceModel.Empty())
    sourceModel.AddDisc(gamePath);

  if (!m_hasPersistedState)
  {
    *m_discModel = sourceModel;
    m_discModel->SetSelectedDiscByIndex(0);
  }
  else
  {
    // Source media can resolve older savestates without changing the current playlist.
    m_discModel->RememberDiscs(sourceModel);
  }

  if (!m_hasPersistedState && replaceInitialHint && !m_discModel->Empty())
  {
    m_initialHintAttempted = true;
    m_transport->SetInitialImage(0, m_discModel->GetPathByIndex(0));
  }
}

void CGameClientDiscs::Deinitialize()
{
  std::unique_lock lock(m_clientAccess);

  // Stopping a game must discard all live disc UI/model state. Persisted state
  // remains keyed by game path and will be loaded explicitly for the next game.
  ResetSessionState();
}

CGameClientDiscModel CGameClientDiscs::GetDiscs() const
{
  std::unique_lock lock(m_clientAccess);
  return *m_discModel;
}

void CGameClientDiscs::SetDiscModel(const CGameClientDiscModel& model)
{
  std::unique_lock lock(m_clientAccess);
  if (!m_preserveSlotTopology || !(*m_discModel == model))
    m_restoredImageCount.reset();
  if (*m_discModel == model)
    m_discModel->RememberDiscs(model);
  else
  {
    CGameClientDiscModel next = model;
    next.RememberDiscs(*m_discModel);
    *m_discModel = std::move(next);
  }
  m_isEjected = model.IsEjected();
  m_preserveSlotTopology = true;
}

bool CGameClientDiscs::PrepareForDeserialize()
{
  std::unique_lock lock(m_clientAccess);
  return !m_transport->GetEjectState() || m_transport->SetEjectState(false);
}

void CGameClientDiscs::InvalidateRestoreCache()
{
  std::unique_lock lock(m_clientAccess);
  m_restoredImageCount.reset();
}

bool CGameClientDiscs::RestoreDiscList()
{
  std::unique_lock lock(m_clientAccess);
  ++m_restoreGeneration;

  unsigned int imageCount = m_transport->GetImageCount();
  bool listMatches = imageCount >= m_discModel->Size();
  for (unsigned int i = 0; i < imageCount; ++i)
  {
    const std::string path = m_transport->GetImagePath(i);
    const std::string targetPath = m_discModel->GetPathByIndex(i);
    if (path.empty())
    {
      // Missing path metadata cannot invalidate an unchanged, successfully restored model.
      if (m_restoredImageCount != imageCount)
        listMatches = false;
    }
    else if (!URIUtils::PathEquals(path, targetPath))
      listMatches = false;
  }
  const auto selectedIndex = m_discModel->GetSelectedDiscIndex();
  const unsigned int currentIndex = m_transport->GetImageIndex();
  // Libretro permits any out-of-range index to represent no disc.
  const bool selectionMatches =
      selectedIndex ? currentIndex == *selectedIndex : currentIndex >= imageCount;
  const bool finalEjected = m_discModel->IsEjected();

  if (!listMatches || !selectionMatches)
  {
    if (!m_transport->GetEjectState() && !m_transport->SetEjectState(true))
    {
      CLog::Log(LOGERROR, "RetroPlayer[DISC]: Failed to eject before restoring disc list");
      return false;
    }
    if (!m_transport->GetEjectState())
    {
      CLog::Log(LOGERROR, "RetroPlayer[DISC]: Core did not report an ejected tray");
      return false;
    }

    if (!listMatches)
    {
      // Some cores retain removed slots. Empty excess slots must never re-enter the frontend model.
      for (unsigned int i = imageCount; m_preserveSlotTopology && i > m_discModel->Size(); --i)
      {
        if (!m_transport->RemoveImageIndex(i - 1))
        {
          CLog::Log(LOGERROR, "RetroPlayer[DISC]: Failed to remove excess slot {}", i - 1);
          return false;
        }
      }
      imageCount = m_transport->GetImageCount();
      while (imageCount < m_discModel->Size())
      {
        if (!m_transport->AddImageIndex())
        {
          CLog::Log(LOGERROR, "RetroPlayer[DISC]: Failed to add slot {}", imageCount);
          return false;
        }
        const unsigned int newCount = m_transport->GetImageCount();
        if (newCount <= imageCount)
        {
          CLog::Log(LOGERROR, "RetroPlayer[DISC]: Core did not add requested slot {}", imageCount);
          return false;
        }
        imageCount = newCount;
      }

      for (size_t i = 0; i < m_discModel->Size(); ++i)
      {
        if (m_discModel->IsRemovedSlotByIndex(i))
        {
          if (!m_transport->RemoveImageIndex(static_cast<unsigned int>(i)))
          {
            CLog::Log(LOGERROR, "RetroPlayer[DISC]: Failed to clear removed slot {}", i);
            return false;
          }
          if (m_transport->GetImageCount() != imageCount)
          {
            CLog::Log(LOGERROR, "RetroPlayer[DISC]: Core cannot restore removed slot {}", i);
            return false;
          }
        }
        else
        {
          const std::string imagePath = m_discModel->GetPathByIndex(i);
          if (imagePath.empty())
          {
            CLog::Log(LOGERROR, "RetroPlayer[DISC]: Cannot restore empty media in slot {}", i);
            return false;
          }
          const std::string currentPath = m_transport->GetImagePath(static_cast<unsigned int>(i));
          if (!URIUtils::PathEquals(currentPath, imagePath) &&
              !m_transport->ReplaceImageIndex(static_cast<unsigned int>(i), imagePath))
          {
            CLog::Log(LOGERROR, "RetroPlayer[DISC]: Failed to restore media in slot {}", i);
            return false;
          }
        }
      }
    }

    if (!m_transport->SetImageIndex(selectedIndex ? static_cast<unsigned int>(*selectedIndex)
                                                  : m_transport->GetImageCount()))
    {
      CLog::Log(LOGERROR, "RetroPlayer[DISC]: Failed to restore selected disc");
      return false;
    }
  }

  if (m_transport->GetEjectState() != finalEjected && !m_transport->SetEjectState(finalEjected))
  {
    CLog::Log(LOGERROR, "RetroPlayer[DISC]: Failed to restore tray state");
    return false;
  }
  m_isEjected = m_transport->GetEjectState();
  const unsigned int restoredIndex = m_transport->GetImageIndex();
  const bool restored = m_isEjected == finalEjected &&
                        (selectedIndex ? restoredIndex == *selectedIndex
                                       : restoredIndex >= m_transport->GetImageCount());
  if (restored)
    m_restoredImageCount = m_transport->GetImageCount();
  return restored;
}

void CGameClientDiscs::RefreshDiscState()
{
  std::unique_lock lock(m_clientAccess);
  RefreshDiscStateLive();
  SaveDiscState();
}

void CGameClientDiscs::RefreshDiscStateLive()
{
  std::unique_lock lock(m_clientAccess);

  CGameClientDiscModel coreModel;
  LoadModelFromCore(coreModel);
  if (m_preserveSlotTopology)
  {
    // Libretro can clear media without shrinking its physical slot array.
    while (coreModel.Size() > m_discModel->Size())
      coreModel.EraseDiscByIndex(coreModel.Size() - 1);
  }

  *m_discModel = CGameClientDiscMergeUtils::ReconcileModels(*m_discModel, coreModel);
  m_isEjected = m_discModel->IsEjected();
}

std::string CGameClientDiscs::GetDiscLabel() const
{
  std::unique_lock lock(m_clientAccess);

  return m_discModel->GetSelectedDiscLabel();
}

bool CGameClientDiscs::IsTrayEmpty() const
{
  std::unique_lock lock(m_clientAccess);

  return m_discModel->IsSelectedNoDisc();
}

bool CGameClientDiscs::SetEjected(bool ejected)
{
  std::unique_lock lock(m_clientAccess);

  if (!m_transport->SetEjectState(ejected))
    return false;

  RefreshDiscState();

  return true;
}

bool CGameClientDiscs::AddDisc(const std::string& filePath)
{
  std::unique_lock lock(m_clientAccess);

  if (filePath.empty())
    return true;

  // Skip duplicates in the frontend model
  if (m_discModel->GetDiscIndexByPath(filePath).has_value())
    return true;

  // Libretro only allows changing the inserted image while ejected
  if (!m_isEjected)
    return true;

  // Prefer reusing the first removed slot before growing the core list
  std::optional<size_t> removedIndex;
  for (size_t i = 0; i < m_discModel->Size(); ++i)
  {
    if (m_discModel->IsRemovedSlotByIndex(i))
    {
      removedIndex = i;
      break;
    }
  }

  if (removedIndex.has_value())
  {
    if (!m_transport->ReplaceImageIndex(static_cast<unsigned int>(*removedIndex), filePath))
      return false;

    m_discModel->SetDiscByIndex(*removedIndex, filePath);
  }
  else
  {
    const unsigned int currentImageCount = m_transport->GetImageCount();

    unsigned int newIndex;
    bool slotAdded = false;
    if (m_preserveSlotTopology && m_discModel->Size() < currentImageCount)
      newIndex = static_cast<unsigned int>(m_discModel->Size());
    else
    {
      if (!m_transport->AddImageIndex())
        return false;
      const unsigned int newImageCount = m_transport->GetImageCount();
      if (currentImageCount >= UINT_MAX || newImageCount != currentImageCount + 1)
        return false;
      newIndex = newImageCount - 1;
      slotAdded = true;
    }

    // Populate the new slot
    if (!m_transport->ReplaceImageIndex(newIndex, filePath))
    {
      if (slotAdded)
        m_transport->RemoveImageIndex(newIndex);
      return false;
    }

    // Add the image to the model. This can be overwritten when we refresh
    // the disc state, but will serve as a fallback in case the libretro
    // extended disc control interface (which allows retrieving path and label)
    // isn't supported.
    m_discModel->AddDisc(filePath);
  }

  RefreshDiscState();

  return true;
}

bool CGameClientDiscs::RemoveDisc(const std::string& filePath)
{
  std::unique_lock lock(m_clientAccess);

  if (filePath.empty())
    return true;

  const auto discIndex = m_discModel->GetDiscIndexByPath(filePath);
  if (!discIndex.has_value())
    return true;

  return RemoveDiscByIndex(*discIndex);
}

bool CGameClientDiscs::RemoveDiscByIndex(size_t index)
{
  std::unique_lock lock(m_clientAccess);

  // Libretro only allows mutating the image list while ejected
  if (!m_isEjected)
    return true;

  if (index >= m_discModel->Size())
    return true;

  const std::optional<size_t> selectedIndex = m_discModel->GetSelectedDiscIndex();
  const bool wasSelected = selectedIndex.has_value() && *selectedIndex == index;
  bool selectionUpdated = true;

  const unsigned int previousImageCount = m_transport->GetImageCount();
  if (!m_transport->RemoveImageIndex(static_cast<unsigned int>(index)))
    return false;

  // If the removed slot was currently selected, force "No disc" before refresh.
  // This makes UI behavior deterministic even if the core leaves a zombie slot
  // behind or reports stale selection state briefly.
  if (wasSelected)
  {
    const unsigned int noDiscIndex = m_transport->GetImageCount();
    selectionUpdated = m_transport->SetImageIndex(noDiscIndex);
  }

  const unsigned int imageCount = m_transport->GetImageCount();
  if (previousImageCount > 0 && imageCount == previousImageCount - 1)
    m_discModel->EraseDiscByIndex(index);
  else
    m_discModel->MarkRemovedByIndex(index);

  RefreshDiscState();

  return selectionUpdated;
}

bool CGameClientDiscs::InsertDisc(const std::string& filePath)
{
  std::unique_lock lock(m_clientAccess);

  // Libretro only allows mutating the image list while ejected
  if (!m_isEjected)
    return true;

  if (filePath.empty())
  {
    const unsigned int imageIndex = m_transport->GetImageCount(); // "No disc" sentinel
    if (!m_transport->SetImageIndex(imageIndex))
      return false;

    RefreshDiscState();
  }
  else
  {
    const auto discIndex = m_discModel->GetDiscIndexByPath(filePath);
    if (discIndex.has_value())
    {
      if (!InsertDiscByIndex(*discIndex))
        return false;
    }
  }

  return true;
}

bool CGameClientDiscs::InsertDiscByIndex(size_t index)
{
  std::unique_lock lock(m_clientAccess);

  // Libretro only allows mutating the image list while ejected
  if (!m_isEjected)
    return true;

  if (index >= m_discModel->Size())
    return true;

  if (!m_transport->SetImageIndex(static_cast<unsigned int>(index)))
    return false;

  RefreshDiscState();

  return true;
}

void CGameClientDiscs::ResetSessionState()
{
  ++m_restoreGeneration;
  m_discModel->Clear();
  m_isEjected = false;
  m_preserveSlotTopology = false;
  m_hasPersistedState = false;
  m_initialHintAttempted = false;
  m_restoredImageCount.reset();
}

void CGameClientDiscs::LoadModelFromCore(CGameClientDiscModel& model) const
{
  model.Clear();

  // Load ejected state
  model.SetEjected(m_transport->GetEjectState());

  const unsigned int imageCount = m_transport->GetImageCount();
  const unsigned int imageIndex = m_transport->GetImageIndex();
  std::optional<size_t> selectedModelIndex;

  for (unsigned int i = 0; i < imageCount; ++i)
  {
    std::string imagePath = m_transport->GetImagePath(i);
    std::string imageLabel = m_transport->GetImageLabel(i);

    const size_t modelIndex = model.Size();
    model.AddDisc(imagePath, imageLabel);

    if (i == imageIndex)
      selectedModelIndex = modelIndex;
  }

  if (model.Empty())
    return;

  if (selectedModelIndex.has_value())
    model.SetSelectedDiscByIndex(*selectedModelIndex);
  else
    model.SetSelectedNoDisc();
}

void CGameClientDiscs::SaveDiscState() const
{
  std::unique_lock lock(m_clientAccess);

  if (!m_gameClient.GetGamePath().empty())
  {
    m_discXml->Save(m_gameClient.GetGamePath(), *m_discModel);
    m_discM3u->Save(m_gameClient.GetGamePath(), *m_discModel);
  }
}

void CGameClientDiscs::PruneRemovedDiscs(CGameClientDiscModel& model)
{
  for (size_t i = 0; i < model.Size();)
  {
    if (!model.IsRemovedSlotByIndex(i))
    {
      ++i;
      continue;
    }

    model.EraseDiscByIndex(i);
  }
}

void CGameClientDiscs::PruneExtensions(CGameClientDiscModel& model,
                                       const std::set<std::string>& supportedExtensions)
{
  for (size_t i = 0; i < model.Size();)
  {
    const std::string discPath = model.GetPathByIndex(i);
    if (discPath.empty())
    {
      ++i;
      continue;
    }

    const bool isSupported = std::any_of(supportedExtensions.begin(), supportedExtensions.end(),
                                         [&discPath](const std::string& ext)
                                         { return URIUtils::HasExtension(discPath, ext); });

    if (!isSupported)
    {
      model.EraseDiscByIndex(i);
      continue;
    }

    ++i;
  }
}
