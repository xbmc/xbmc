#include "boost/foreach.hpp"
#include "PlexGlobalCacher.h"
#include "Client/PlexServerDataLoader.h"
#include "LocalizeStrings.h"
#include "GUIDialogPlexGlobalCacher.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIDialogPlexGlobalCacher::CGUIDialogPlexGlobalCacher() : CGUIDialogSelect(WINDOW_DIALOG_PLEX_GLOBAL_CACHER) {}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexGlobalCacher::LoadSections()
{
  m_SelectedSections = CFileItemListPtr(new CFileItemList());

  // grab all the sections
  CFileItemListPtr pAllSharedSections = g_plexApplication.dataLoader->GetAllSharedSections();
  CLog::Log(LOGNOTICE, "Global Cache : found %d Shared Sections", pAllSharedSections->Size());

  // get all the sections names
  m_Sections = g_plexApplication.dataLoader->GetAllSections();
  m_Sections->Append(*pAllSharedSections);
  CLog::Log(LOGNOTICE, "Global Cache : found %d Regular Sections", m_Sections->Size());

  CFileItemList items;
  std::set<CStdString> servers;

  // build the servers list
  for (int iSection = 0; iSection < m_Sections->Size(); iSection++)
  {
    CFileItemPtr section = m_Sections->Get(iSection);

    CStdString servername = section->GetProperty("serverName").asString();

    if (servers.find(servername) == servers.end())
    {
      servers.insert(servername);
      CFileItemPtr item(new CFileItem("", false));
      item->SetLabel(servername);
      item->SetProperty("serverName", servername);
      items.Add(item);
    }
  }

  // setup the window
  Reset();
  SetHeading(g_localizeStrings.Get(44400));
  SetItems(&items);
  SetMultiSelection(true);
  EnableButton(true, 209);
  SetUseDetails(true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexGlobalCacher::ProcessSelection()
{
  // buils the selected sections list
  const CFileItemList& selectedItems = GetSelectedItems();

  CLog::Log(LOGDEBUG, "CGUIDialogPlexGlobalCacher::ProcessSelection %d items selected", selectedItems.Size());

  for (int iItem = 0; iItem < selectedItems.Size(); iItem++)
  {
    CFileItemPtr serveritem = selectedItems.Get(iItem);

    for (int iSection = 0; iSection < m_Sections->Size(); iSection++)
    {
      CFileItemPtr section = m_Sections->Get(iSection);
      CLog::Log(LOGDEBUG, "Checking item %s vs section %s", serveritem->GetProperty("servername").asString().c_str(), section->GetProperty("serverName").asString().c_str());

      if (serveritem->GetProperty("servername").asString() == section->GetProperty("serverName").asString())
        m_SelectedSections->Add(section);
    }
  }

  CLog::Log(LOGDEBUG, "CGUIDialogPlexGlobalCacher::ProcessSelection %d Sections selected", m_SelectedSections->Size());

  if (m_SelectedSections->Size() > 0)
  {
    // Give the section List to global cacher
    CPlexGlobalCacher* cacher = CPlexGlobalCacher::GetInstance();
    cacher->SetSections(m_SelectedSections);

    // And start it
    if (!cacher->IsRunning())
    {
      cacher->Start();
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexGlobalCacher::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
      LoadSections();
      break;

    case GUI_MSG_WINDOW_DEINIT:
      bool ret = CGUIDialogSelect::OnMessage(message);
      ProcessSelection();
      return ret;
      break;
  }

  return CGUIDialogSelect::OnMessage(message);
}
