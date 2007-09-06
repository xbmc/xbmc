/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowPrograms.h"
#include "Util.h"
#include "Shortcut.h"
#include "FileSystem/HDDirectory.h"
#include "GUIPassword.h"
#ifdef HAS_TRAINER
#include "GUIDialogTrainerSettings.h"
#endif
#include "GUIDialogMediaSource.h"
#ifdef HAS_XBOX_HARDWARE
#include "xbox/xbeheader.h"
#endif
#ifdef HAS_TRAINER
#include "utils/Trainer.h"
#endif
#ifdef HAS_KAI
#include "utils/KaiClient.h"
#endif
#include "Autorun.h"
#include "utils/LabelFormatter.h"
#include "Autorun.h"

using namespace XFILE;
using namespace DIRECTORY;

#define CONTROL_BTNVIEWASICONS 2
#define CONTROL_BTNSORTBY      3
#define CONTROL_BTNSORTASC     4
#define CONTROL_LIST          50
#define CONTROL_THUMBS        51
#define CONTROL_LABELFILES    12

CGUIWindowPrograms::CGUIWindowPrograms(void)
    : CGUIMediaWindow(WINDOW_PROGRAMS, "MyPrograms.xml")
{
  m_thumbLoader.SetObserver(this);
  m_dlgProgress = NULL;
  m_rootDir.AllowNonLocalShares(false); // no nonlocal shares for this window please
}


CGUIWindowPrograms::~CGUIWindowPrograms(void)
{
}

bool CGUIWindowPrograms::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_thumbLoader.IsLoading())
        m_thumbLoader.StopThread();
      m_database.Close();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_iRegionSet = 0;
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      // check for a passed destination path
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
        // reset directory path, as we have effectively cleared it here
        m_history.ClearPathHistory();
      }
      // is this the first time accessing this window?
      // a quickpath overrides the a default parameter
      if (m_vecItems.m_strPath == "?" && strDestination.IsEmpty())
      {
        m_vecItems.m_strPath = strDestination = g_stSettings.m_szDefaultPrograms;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      m_database.Open();
      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // open root
        if (strDestination.Equals("$ROOT"))
        {
          m_vecItems.m_strPath = "";
          CLog::Log(LOGINFO, "  Success! Opening root listing.");
        }
        else
        {
          // default parameters if the jump fails
          m_vecItems.m_strPath = "";

          bool bIsSourceName = false;
          SetupShares();
          VECSHARES shares;
          m_rootDir.GetShares(shares);
          int iIndex = CUtil::GetMatchingShare(strDestination, shares, bIsSourceName);
          if (iIndex > -1)
          {
            bool bDoStuff = true;
            if (shares[iIndex].m_iHasLock == 2)
            {
              CFileItem item(shares[iIndex]);
              if (!g_passwordManager.IsItemUnlocked(&item,"myprograms"))
              {
                m_vecItems.m_strPath = ""; // no u don't
                bDoStuff = false;
                CLog::Log(LOGINFO, "  Failure! Failed to unlock destination path: %s", strDestination.c_str());
              }
            }
            // set current directory to matching share
            if (bDoStuff)
            {
              if (bIsSourceName)
                m_vecItems.m_strPath=shares[iIndex].strPath;
              else
                m_vecItems.m_strPath=strDestination;
              CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
            }
          }
          else
          {
            CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid source!", strDestination.c_str());
          }
        }
        SetHistoryForPath(m_vecItems.m_strPath);
      }

      return CGUIMediaWindow::OnMessage(message);
    }
  break;

  case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_BTNSORTBY)
      {
        // need to update shortcuts manually
        if (CGUIMediaWindow::OnMessage(message))
        {
          LABEL_MASKS labelMasks;
          m_guiState->GetSortMethodLabelMasks(labelMasks);
          CLabelFormatter formatter("", labelMasks.m_strLabel2File);
          for (int i=0;i<m_vecItems.Size();++i)
          {
            if (m_vecItems[i]->IsShortCut())
              formatter.FormatLabel2(m_vecItems[i]);
          }
          return true;
        }
        else
          return false;
      }
      if (m_viewControl.HasControl(message.GetSenderId()))  // list/thumb control
      {
        if (message.GetParam1() == ACTION_PLAYER_PLAY)
        {
          OnPlayMedia(m_viewControl.GetSelectedItem());
          return true;
        }
      }
    }
    break;
  }

  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowPrograms::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems.Size())
    return;
  CFileItem *item = m_vecItems[itemNumber];
  if ( m_vecItems.IsVirtualDirectoryRoot() )
  {
    // get the usual shares
    CShare *share = CGUIDialogContextMenu::GetShare("myprograms", item);
    CGUIDialogContextMenu::GetContextButtons("myprograms", share, buttons);
  }
  else
  {
    if (item->IsXBE() || item->IsShortCut())
    {
      CStdString strLaunch = g_localizeStrings.Get(518); // Launch
      if (g_guiSettings.GetBool("myprograms.gameautoregion"))
      {
        int iRegion = GetRegion(itemNumber);
        if (iRegion == VIDEO_NTSCM)
          strLaunch += " (NTSC-M)";
        if (iRegion == VIDEO_NTSCJ)
          strLaunch += " (NTSC-J)";
        if (iRegion == VIDEO_PAL50)
          strLaunch += " (PAL)";
        if (iRegion == VIDEO_PAL60)
          strLaunch += " (PAL-60)";
      }
      buttons.Add(CONTEXT_BUTTON_LAUNCH, strLaunch);

      DWORD dwTitleId = CUtil::GetXbeID(item->m_strPath);
      CStdString strTitleID;
      CStdString strGameSavepath;
      strTitleID.Format("%08X",dwTitleId);
      CUtil::AddFileToFolder("E:\\udata\\",strTitleID,strGameSavepath);

      if (CDirectory::Exists(strGameSavepath))
        buttons.Add(CONTEXT_BUTTON_GAMESAVES, 20322);         // Goto GameSaves

      if (g_guiSettings.GetBool("myprograms.gameautoregion"))
        buttons.Add(CONTEXT_BUTTON_LAUNCH_IN, 519); // launch in video mode

      if (g_passwordManager.IsMasterLockUnlocked(false) || g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases())
      {
        if (item->IsShortCut())
          buttons.Add(CONTEXT_BUTTON_RENAME, 16105); // rename
        else
          buttons.Add(CONTEXT_BUTTON_RENAME, 520); // edit xbe title
      }
#ifdef HAS_TRAINER
      if (m_database.ItemHasTrainer(dwTitleId))
        buttons.Add(CONTEXT_BUTTON_TRAINER_OPTIONS, 12015); // trainer options
#endif
    }
    buttons.Add(CONTEXT_BUTTON_SCAN_TRAINERS, 12012); // scan trainers

    buttons.Add(CONTEXT_BUTTON_GOTO_ROOT, 20128); // Go to Root
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
  buttons.Add(CONTEXT_BUTTON_SETTINGS, 5);      // Settings
}

bool CGUIWindowPrograms::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItem *item = (itemNumber >= 0 && itemNumber < m_vecItems.Size()) ? m_vecItems[itemNumber] : NULL;

  if (item && m_vecItems.IsVirtualDirectoryRoot())
  {
    CShare *share = CGUIDialogContextMenu::GetShare("myprograms", item);
    if (CGUIDialogContextMenu::OnContextButton("myprograms", share, button))
    {
      Update("");
      return true;
    }
  }
  switch (button)
  {
  case CONTEXT_BUTTON_RENAME:
    {
      CStdString strDescription;
      CShortcut cut;
      if (item->IsShortCut())
      {
        cut.Create(item->m_strPath);
        strDescription = cut.m_strLabel;
      }
      else
        strDescription = item->GetLabel();

      if (CGUIDialogKeyboard::ShowAndGetInput(strDescription, g_localizeStrings.Get(16008), false))
      {
        if (item->IsShortCut())
        {
          cut.m_strLabel = strDescription;
          cut.Save(item->m_strPath);
        }
        else
        {
          // SetXBEDescription will truncate to 40 characters.
          CUtil::SetXBEDescription(item->m_strPath,strDescription);
          m_database.SetDescription(item->m_strPath,strDescription);
        }
        Update(m_vecItems.m_strPath);
      }
      return true;
    }

#ifdef HAS_TRAINER
  case CONTEXT_BUTTON_TRAINER_OPTIONS:
    {
      DWORD dwTitleId = CUtil::GetXbeID(item->m_strPath);
      if (CGUIDialogTrainerSettings::ShowForTitle(dwTitleId,&m_database))
        Update(m_vecItems.m_strPath);
      return true;
    }

  case CONTEXT_BUTTON_SCAN_TRAINERS:
    {
      PopulateTrainersList();
      Update(m_vecItems.m_strPath);
      return true;
    }
#endif

  case CONTEXT_BUTTON_SETTINGS:
    m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYPROGRAMS);
    return true;

  case CONTEXT_BUTTON_GOTO_ROOT:
    Update("");
    return true;

  case CONTEXT_BUTTON_LAUNCH:
    OnClick(itemNumber);
    return true;

  case CONTEXT_BUTTON_GAMESAVES:
    {
      CStdString strTitleID;
      CStdString strGameSavepath;
      strTitleID.Format("%08X",CUtil::GetXbeID(item->m_strPath));
      CUtil::AddFileToFolder("E:\\udata\\",strTitleID,strGameSavepath);
      m_gWindowManager.ActivateWindow(WINDOW_GAMESAVES,strGameSavepath);
      return true;
    }
  case CONTEXT_BUTTON_LAUNCH_IN:
    OnChooseVideoModeAndLaunch(itemNumber);
    return true;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowPrograms::OnChooseVideoModeAndLaunch(int item)
{
  if (item < 0 || item >= m_vecItems.Size()) return false;
  // calculate our position
  float posX = 200;
  float posY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }

  // grab the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return false;

  pMenu->Initialize();

  int btn_PAL;
  int btn_NTSCM;
  int btn_NTSCJ;
  int btn_PAL60;
  CStdString strPAL, strNTSCJ, strNTSCM, strPAL60;
  strPAL = "PAL";
  strNTSCM = "NTSC-M";
  strNTSCJ = "NTSC-J";
  strPAL60 = "PAL-60";
  int iRegion = GetRegion(item,true);

  if (iRegion == VIDEO_NTSCM)
    strNTSCM += " (default)";
  if (iRegion == VIDEO_NTSCJ)
    strNTSCJ += " (default)";
  if (iRegion == VIDEO_PAL50)
    strPAL += " (default)";

  btn_PAL = pMenu->AddButton(strPAL);
  btn_NTSCM = pMenu->AddButton(strNTSCM);
  btn_NTSCJ = pMenu->AddButton(strNTSCJ);
  btn_PAL60 = pMenu->AddButton(strPAL60);

  pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
  pMenu->DoModal();
  int btnid = pMenu->GetButton();

  if (btnid == btn_NTSCM)
  {
    m_iRegionSet = VIDEO_NTSCM;
    m_database.SetRegion(m_vecItems[item]->m_strPath,1);
  }
  if (btnid == btn_NTSCJ)
  {
    m_iRegionSet = VIDEO_NTSCJ;
    m_database.SetRegion(m_vecItems[item]->m_strPath,2);
  }
  if (btnid == btn_PAL)
  {
    m_iRegionSet = VIDEO_PAL50;
    m_database.SetRegion(m_vecItems[item]->m_strPath,4);
  }
  if (btnid == btn_PAL60)
  {
    m_iRegionSet = VIDEO_PAL60;
    m_database.SetRegion(m_vecItems[item]->m_strPath,8);
  }

  if (btnid > -1)
    return OnClick(item);

  return true;
}

bool CGUIWindowPrograms::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  m_thumbLoader.Load(m_vecItems);
  return true;
}

bool CGUIWindowPrograms::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return false;
  CFileItem* pItem = m_vecItems[iItem];

  if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("myprograms"))
    {
      Update("");
      return true;
    }
    return false;
  }

  if (pItem->IsDVD())
    return MEDIA_DETECT::CAutorun::PlayDisc();

  if (pItem->m_bIsFolder) return false;

  // launch xbe...
  char szPath[1024];
  char szParameters[1024];

  m_database.IncTimesPlayed(pItem->m_strPath);

  int iRegion = m_iRegionSet?m_iRegionSet:GetRegion(iItem);

  DWORD dwTitleId = 0;
  if (!pItem->IsOnDVD())
    dwTitleId = m_database.GetTitleId(pItem->m_strPath);
  if (!dwTitleId)
    dwTitleId = CUtil::GetXbeID(pItem->m_strPath);
#ifdef HAS_TRAINER
  CStdString strTrainer = m_database.GetActiveTrainer(dwTitleId);
  if (strTrainer != "")
  {
    bool bContinue=false;
    if (CKaiClient::GetInstance()->IsEngineConnected())
    {
      if (CGUIDialogYesNo::ShowAndGetInput(20023,20020,20021,20022,714,12013))
      {
	CStdString emptyStr = "";
        CKaiClient::GetInstance()->EnterVector(emptyStr, emptyStr);
      }
      else
        bContinue = true;
    }
    if (!bContinue)
    {
      CTrainer trainer;
      if (trainer.Load(strTrainer))
      {
        m_database.GetTrainerOptions(strTrainer,dwTitleId,trainer.GetOptions(),trainer.GetNumberOfOptions());
        CUtil::InstallTrainer(trainer);
      }
    }
  }
#endif

  m_database.Close();
  memset(szParameters, 0, sizeof(szParameters));

  strcpy(szPath, pItem->m_strPath.c_str());

  if (pItem->IsShortCut())
  {
    CUtil::RunShortcut(pItem->m_strPath.c_str());
    return false;
  }

  if (strlen(szParameters))
    CUtil::RunXBE(szPath, szParameters,F_VIDEO(iRegion));
  else
    CUtil::RunXBE(szPath,NULL,F_VIDEO(iRegion));
  return true;
}

int CGUIWindowPrograms::GetRegion(int iItem, bool bReload)
{
  if (!g_guiSettings.GetBool("myprograms.gameautoregion"))
    return 0;

#ifdef HAS_XBOX_HARDWARE
  int iRegion;
  if (bReload || m_vecItems[iItem]->IsOnDVD())
  {
    CXBE xbe;
    iRegion = xbe.ExtractGameRegion(m_vecItems[iItem]->m_strPath);
  }
  else
  {
    m_database.Open();
    iRegion = m_database.GetRegion(m_vecItems[iItem]->m_strPath);
    m_database.Close();
  }
  if (iRegion == -1)
  {
    if (g_guiSettings.GetBool("myprograms.gameautoregion"))
    {
      CXBE xbe;
      iRegion = xbe.ExtractGameRegion(m_vecItems[iItem]->m_strPath);
      if (iRegion < 1 || iRegion > 7)
        iRegion = 0;
      m_database.SetRegion(m_vecItems[iItem]->m_strPath,iRegion);
    }
    else
      iRegion = 0;
  }

  if (bReload)
    return CXBE::FilterRegion(iRegion,true);
  else
    return CXBE::FilterRegion(iRegion);
#else
  return 0;
#endif
}

#ifdef HAS_TRAINER
void CGUIWindowPrograms::PopulateTrainersList()
{
  CDirectory directory;
  CFileItemList trainers;
  CFileItemList archives;
  CFileItemList inArchives;
  // first, remove any dead items
  std::vector<CStdString> vecTrainerPath;
  m_database.GetAllTrainers(vecTrainerPath);
  CGUIDialogProgress* m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  m_dlgProgress->SetLine(0,12023);
  m_dlgProgress->SetLine(1,"");
  m_dlgProgress->SetLine(2,"");
  m_dlgProgress->StartModal();
  m_dlgProgress->SetHeading(12012);
  m_dlgProgress->ShowProgressBar(true);
  m_dlgProgress->Progress();

  bool bBreak=false;
  bool bDatabaseState = m_database.IsOpen();
  if (!bDatabaseState)
    m_database.Open();
  m_database.BeginTransaction();
  for (unsigned int i=0;i<vecTrainerPath.size();++i)
  {
    m_dlgProgress->SetPercentage((int)((float)i/(float)vecTrainerPath.size()*100.f));
    CStdString strLine;
    strLine.Format("%s %i / %i",g_localizeStrings.Get(12013).c_str(), i+1,vecTrainerPath.size());
    m_dlgProgress->SetLine(1,strLine);
    m_dlgProgress->Progress();
    if (!CFile::Exists(vecTrainerPath[i]) || vecTrainerPath[i].find(g_guiSettings.GetString("myprograms.trainerpath",false)) == -1)
      m_database.RemoveTrainer(vecTrainerPath[i]);
    if (m_dlgProgress->IsCanceled())
    {
      bBreak = true;
      m_database.RollbackTransaction();
      break;
    }
  }
  if (!bBreak)
  {
    CLog::Log(LOGDEBUG,"trainerpath %s",g_guiSettings.GetString("myprograms.trainerpath",false).c_str());
    directory.GetDirectory(g_guiSettings.GetString("myprograms.trainerpath").c_str(),trainers,".xbtf|.etm");
    if (g_guiSettings.GetString("myprograms.trainerpath",false).IsEmpty())
    {
      m_database.RollbackTransaction();
      m_dlgProgress->Close();

      return;
    }

    directory.GetDirectory(g_guiSettings.GetString("myprograms.trainerpath").c_str(),archives,".rar|.zip",false); // TODO: ZIP SUPPORT
    for( int i=0;i<archives.Size();++i)
    {
      if (stricmp(CUtil::GetExtension(archives[i]->m_strPath),".rar") == 0)
      {
        g_RarManager.GetFilesInRar(inArchives,archives[i]->m_strPath,false);
        CHDDirectory dir;
        dir.SetMask(".xbtf|.etm");
        for (int j=0;j<inArchives.Size();++j)
          if (dir.IsAllowed(inArchives[j]->m_strPath))
          {
            CFileItem* item = new CFileItem(*inArchives[j]);
            CStdString strPathInArchive = item->m_strPath;
            CUtil::CreateRarPath(item->m_strPath, archives[i]->m_strPath, strPathInArchive,EXFILE_AUTODELETE,"",g_advancedSettings.m_cachePath);
            trainers.Add(item);
          }
      }
      if (stricmp(CUtil::GetExtension(archives[i]->m_strPath),".zip")==0)
      {
        // add trainers in zip
        CStdString strZipPath;
        CUtil::CreateZipPath(strZipPath,archives[i]->m_strPath,"");
        CFileItemList zipTrainers;
        directory.GetDirectory(strZipPath,zipTrainers,".etm|.xbtf");
        for (int j=0;j<zipTrainers.Size();++j)
          trainers.Add(new CFileItem(*zipTrainers[j]));
      }
    }
    if (!m_dlgProgress)
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    m_dlgProgress->SetPercentage(0);
    m_dlgProgress->ShowProgressBar(true);

    CLog::Log(LOGDEBUG,"# trainers %i",trainers.Size());
    m_dlgProgress->SetLine(1,"");
    int j=0;
    while (j < trainers.Size())
    {
      if (trainers[j]->m_bIsFolder)
        trainers.Remove(j);
      else
        j++;
    }
    for (int i=0;i<trainers.Size();++i)
    {
      CLog::Log(LOGDEBUG,"found trainer %s",trainers[i]->m_strPath.c_str());
      m_dlgProgress->SetPercentage((int)((float)(i)/trainers.Size()*100.f));
      CStdString strLine;
      strLine.Format("%s %i / %i",g_localizeStrings.Get(12013).c_str(), i+1,trainers.Size());
      m_dlgProgress->SetLine(0,strLine);
      m_dlgProgress->SetLine(2,"");
      m_dlgProgress->Progress();
      if (m_database.HasTrainer(trainers[i]->m_strPath)) // skip existing trainers
        continue;

      CTrainer trainer;
      if (trainer.Load(trainers[i]->m_strPath))
      {
        m_dlgProgress->SetLine(1,trainer.GetName());
        m_dlgProgress->SetLine(2,"");
        m_dlgProgress->Progress();
        unsigned int iTitle1, iTitle2, iTitle3;
        trainer.GetTitleIds(iTitle1,iTitle2,iTitle3);
        if (iTitle1)
          m_database.AddTrainer(iTitle1,trainers[i]->m_strPath);
        if (iTitle2)
          m_database.AddTrainer(iTitle2,trainers[i]->m_strPath);
        if (iTitle3)
          m_database.AddTrainer(iTitle3,trainers[i]->m_strPath);
      }
      if (m_dlgProgress->IsCanceled())
      {
        m_database.RollbackTransaction();
        break;
      }
    }
  }
  m_database.CommitTransaction();
  m_dlgProgress->Close();

  if (!bDatabaseState)
    m_database.Close();
  else
    Update(m_vecItems.m_strPath);
}
#endif

bool CGUIWindowPrograms::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  bool bFlattened=false;
  if (CUtil::IsDVD(strDirectory))
  {
    CStdString strPath;
    CUtil::AddFileToFolder(strDirectory,"default.xbe",strPath);
    if (CFile::Exists(strPath)) // flatten dvd
    {
      CFileItem item("default.xbe");
      item.m_strPath = strPath;
      items.Add(new CFileItem(item));
      items.m_strPath=strDirectory;
      bFlattened = true;
    }
  }
  if (!bFlattened)
    if (!CGUIMediaWindow::GetDirectory(strDirectory, items))
      return false;

  if (items.IsVirtualDirectoryRoot())
    return true;

  // flatten any folders
  m_database.BeginTransaction();
  DWORD dwTick=timeGetTime();
  bool bProgressVisible = false;
  for (int i = 0; i < items.Size(); i++)
  {
    CStdString shortcutPath;
    CFileItem *item = items[i];
    if (!bProgressVisible && timeGetTime()-dwTick>1500 && m_dlgProgress)
    { // tag loading takes more then 1.5 secs, show a progress dialog
      m_dlgProgress->SetHeading(189);
      m_dlgProgress->SetLine(0, 20120);
      m_dlgProgress->SetLine(1,"");
      m_dlgProgress->SetLine(2, item->GetLabel());
      m_dlgProgress->StartModal();
      bProgressVisible = true;
    }
    if (bProgressVisible)
    {
      m_dlgProgress->SetLine(2,item->GetLabel());
      m_dlgProgress->Progress();
    }

    if (item->m_bIsFolder && !item->IsParentFolder())
    { // folder item - let's check for a default.xbe file, and flatten if we have one
      CStdString defaultXBE;
      CUtil::AddFileToFolder(item->m_strPath, "default.xbe", defaultXBE);
      if (CFile::Exists(defaultXBE))
      { // yes, format the item up
        item->m_strPath = defaultXBE;
        item->m_bIsFolder = false;
      }
    }
    else if (item->IsShortCut())
    { // resolve the shortcut to set it's description etc.
      // and save the old shortcut path (so we can reassign it later)
      CShortcut cut;
      if (cut.Create(item->m_strPath))
      {
        shortcutPath = item->m_strPath;
        item->m_strPath = cut.m_strPath;
        item->SetThumbnailImage(cut.m_strThumb);

        LABEL_MASKS labelMasks;
        m_guiState->GetSortMethodLabelMasks(labelMasks);
        CLabelFormatter formatter("", labelMasks.m_strLabel2File);
        if (!cut.m_strLabel.IsEmpty())
        {
          item->SetLabel(cut.m_strLabel);
          __stat64 stat;
          if (CFile::Stat(item->m_strPath,&stat) == 0)
            item->m_dwSize = stat.st_size;

          formatter.FormatLabel2(item);
          item->SetLabelPreformated(true);
        }
      }
    }
    if (item->IsXBE())
    {
      if (CUtil::GetFileName(item->m_strPath).Equals("default_ffp.xbe"))
      {
        m_vecItems.Remove(i--);
        continue;
      }
      // add to database if not already there
      DWORD dwTitleID = item->IsOnDVD() ? 0 : m_database.GetProgramInfo(item);
      if (!dwTitleID)
      {
        CStdString description;
        if (CUtil::GetXBEDescription(item->m_strPath, description) && (!item->IsLabelPreformated() && !item->GetLabel().IsEmpty()))
          item->SetLabel(description);

        dwTitleID = CUtil::GetXbeID(item->m_strPath);
        if (!item->IsOnDVD())
          m_database.AddProgramInfo(item, dwTitleID);
      }

      // SetOverlayIcons()
      if (m_database.ItemHasTrainer(dwTitleID))
      {
        if (m_database.GetActiveTrainer(dwTitleID) != "")
          item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_TRAINED);
        else
          item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_HAS_TRAINER);
      }
    }
    if (!shortcutPath.IsEmpty())
      item->m_strPath = shortcutPath;
  }
  m_database.CommitTransaction();
  // set the cached thumbs
  items.SetThumbnailImage("");
  items.SetCachedProgramThumbs();
  items.SetCachedProgramThumb();
  if (!items.HasThumbnail())
    items.SetUserProgramThumb();

  if (bProgressVisible)
    m_dlgProgress->Close();

  return true;
}

#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
void CGUIWindowPrograms::OnWindowLoaded()
{
  CGUIMediaWindow::OnWindowLoaded();
  for (int i = 100; i < 110; i++)
  {
    SET_CONTROL_HIDDEN(i);
  }
}
#endif
