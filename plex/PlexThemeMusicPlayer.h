#ifndef PLEXTHEMEMUSICPLAYER_H
#define PLEXTHEMEMUSICPLAYER_H

#include "FileItem.h"
#include "cores/IPlayer.h"
#include "threads/Timer.h"
#include "JobManager.h"

class CPlexThemeMusicPlayer : public IPlayerCallback, public IAudioCallback, public IJobCallback
{
  public:
    CPlexThemeMusicPlayer();

    void Stop();

    void playForItem(const CFileItem &item);
    void stop();
    void pauseThemeMusic();
    CFileItemPtr getThemeItem(const CStdString &item);

    /* IPlayerCallback */
    virtual void OnPlayBackEnded() {}
    virtual void OnPlayBackStarted() {}
    virtual void OnPlayBackStopped() {}
    virtual void OnQueueNextItem();

    /* IAudioCallback */
    virtual void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample) {}
    virtual void OnAudioData(const float* pAudioData, int iAudioDataLength) {}

    /* IJobCallback */
    virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

    void initPlayer();
    void destroy(int fadeOut = 0);
  private:
    CCriticalSection m_fadeLock;
    CFileItemPtr m_currentItem;
    IPlayer *m_player;
};

#endif // PLEXTHEMEMUSICPLAYER_H
