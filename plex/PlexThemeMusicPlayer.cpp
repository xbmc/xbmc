#include "PlexThemeMusicPlayer.h"

#include "playercorefactory/PlayerCoreFactory.h"
#include "paplayer/PAPlayer.h"
#include "settings/GUISettings.h"

#include "Directory.h"

#include "Variant.h"
#include "JobManager.h"
#include "PlexJobs.h"

CPlexThemeMusicPlayer::CPlexThemeMusicPlayer()
{
  m_player = CPlayerCoreFactory::CreatePlayer(EPC_PAPLAYER, *this);
  m_player->RegisterAudioCallback(this);
  m_player->SetVolume(g_guiSettings.GetInt("backgroundmusic.bgmusicvolume") / 100);
  m_player->FadeOut(2 * 1000);

  if (!XFILE::CDirectory::Exists("special://masterprofile/ThemeMusicCache"))
    XFILE::CDirectory::Create("special://masterprofile/ThemeMusicCache");
}

void CPlexThemeMusicPlayer::stop()
{
  if (m_player->IsPlaying())
    m_player->CloseFile();
  delete m_player;
}

void CPlexThemeMusicPlayer::pauseThemeMusic()
{
  m_player->FadeOut(80);
  playForItem(CFileItem());
}

CFileItemPtr CPlexThemeMusicPlayer::getThemeItem(const CStdString &url)
{
  CFileItemPtr themeItem = CFileItemPtr(new CFileItem(url, false));
  themeItem->SetMimeType("audio/mp3");
  return themeItem;
}

void CPlexThemeMusicPlayer::playForItem(const CFileItem &item)
{
  if (g_guiSettings.GetBool("backgroundmusic.thememusicenabled"))
    CJobManager::GetInstance().AddJob(new CPlexThemeMusicPlayerJob(item), this);
}

void CPlexThemeMusicPlayer::OnQueueNextItem()
{
  m_player->OnNothingToQueueNotify();
  m_currentItem.reset();
}

void CPlexThemeMusicPlayer::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexThemeMusicPlayerJob *tjob = static_cast<CPlexThemeMusicPlayerJob*>(job);
  if (success && tjob)
  {
    CSingleLock lk(m_fadeLock);
    CFileItemPtr themeItem = getThemeItem(tjob->m_fileToPlay);
    if (!m_currentItem || m_currentItem->GetPath() != themeItem->GetPath())
    {
      m_currentItem = themeItem;
      CLog::Log(LOGDEBUG, "CPlexThemeMusicPlayer::OnJobComplete playing %s", m_currentItem->GetPath().c_str());
      m_player->FadeOut(2 * 1000);
      m_player->OpenFile(*m_currentItem.get(), CPlayerOptions());
    }
  }
  else if (m_player->IsPlaying() && !m_player->IsPaused())
  {
    CSingleLock lk(m_fadeLock);
    CLog::Log(LOGDEBUG, "CPlexThemeMusicPlayer::OnJobComplete fading out...");
    m_player->Pause();
    m_currentItem.reset();
    CLog::Log(LOGDEBUG, "CPlexThemeMusicPlayer::OnJobComplete fading out done...");
  }
}
