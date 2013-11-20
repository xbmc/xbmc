#include "PlexThemeMusicPlayer.h"

#include "playercorefactory/PlayerCoreFactory.h"
#include "paplayer/PAPlayer.h"
#include "settings/GUISettings.h"

#include "cores/AudioEngine/AEFactory.h"

#include "Directory.h"

#include "Variant.h"
#include "JobManager.h"
#include "PlexJobs.h"

#include "Application.h"

CPlexThemeMusicPlayer::CPlexThemeMusicPlayer() : m_player(NULL)
{
  if (!XFILE::CDirectory::Exists("special://masterprofile/ThemeMusicCache"))
    XFILE::CDirectory::Create("special://masterprofile/ThemeMusicCache");
}

void CPlexThemeMusicPlayer::initPlayer()
{
  if (!m_player)
  {
    m_player = CPlayerCoreFactory::CreatePlayer(EPC_PAPLAYER, *this);
    m_player->RegisterAudioCallback(this);
    m_player->SetVolume(g_guiSettings.GetInt("backgroundmusic.bgmusicvolume") / 100.0);
    m_player->FadeOut(2 * 1000);
  }
}

void CPlexThemeMusicPlayer::destroy(int fadeOut)
{
  CSingleLock lk(m_fadeLock);
  if (m_player)
  {
    CLog::Log(LOGDEBUG, "CPlexThemeMusicPlayer::OnJobComplete fading out during %d ms...", fadeOut);
    m_player->FadeOut(fadeOut);
    delete m_player;
    CLog::Log(LOGDEBUG, "CPlexThemeMusicPlayer::OnJobComplete fading out done...");

    CAEFactory::GarbageCollect();

    m_player = NULL;
    m_currentItem.reset();
  }
}

void CPlexThemeMusicPlayer::stop()
{
  destroy();
}

void CPlexThemeMusicPlayer::pauseThemeMusic()
{
  if (m_player)
  {
    destroy();
  }
}

CFileItemPtr CPlexThemeMusicPlayer::getThemeItem(const CStdString &url)
{
  CFileItemPtr themeItem = CFileItemPtr(new CFileItem(url, false));
  themeItem->SetMimeType("audio/mp3");
  return themeItem;
}

void CPlexThemeMusicPlayer::playForItem(const CFileItem &item)
{
  if (g_guiSettings.GetBool("backgroundmusic.thememusicenabled") && !g_application.IsPlaying())
    CJobManager::GetInstance().AddJob(new CPlexThemeMusicPlayerJob(item), this);
}

void CPlexThemeMusicPlayer::OnQueueNextItem()
{
  if (m_player)
  {
    m_player->OnNothingToQueueNotify();
  }
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

      initPlayer();
      m_player->OpenFile(*m_currentItem.get(), CPlayerOptions());
    }
  }
  else if (m_player && m_player->IsPlaying() && !m_player->IsPaused())
  {
    destroy(2 * 1000);
  }
}
