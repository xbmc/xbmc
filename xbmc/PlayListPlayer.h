#pragma once

#include "playlist.h"
#include "IMsgTargetCallback.h"

#define PLAYLIST_NONE    -1
#define PLAYLIST_MUSIC   0
#define PLAYLIST_MUSIC_TEMP 1
#define PLAYLIST_VIDEO   2
#define PLAYLIST_VIDEO_TEMP 3

using namespace PLAYLIST;

namespace PLAYLIST
{
/*!
 \ingroup windows 
 \brief Manages playlist playing.
 */
enum REPEAT_STATE { REPEAT_NONE = 0, REPEAT_ONE, REPEAT_ALL };

class CPlayListPlayer : public IMsgTargetCallback
{

public:
  CPlayListPlayer(void);
  virtual ~CPlayListPlayer(void);
  virtual bool OnMessage(CGUIMessage &message);
  void PlayNext(bool bAutoPlay = false);
  void PlayPrevious();
  void Play();
  void Play(int iSong, bool bAutoPlay = false, bool bPlayPrevious = false);
  int GetCurrentSong() const;
  int GetNextSong();
  void SetCurrentSong(int iSong);
  bool HasChanged();
  void SetCurrentPlaylist( int iPlayList );
  int GetCurrentPlaylist();
  CPlayList& GetPlaylist( int nPlayList);
  int RemoveDVDItems();
  void Reset();
  void ClearPlaylist(int iPlayList);
  void SetShuffle(int iPlaylist, bool bYesNo);
  bool IsShuffled(int iPlaylist);
  bool HasPlayedFirstFile();
  
  void SetRepeat(int iPlaylist, REPEAT_STATE state);
  REPEAT_STATE GetRepeat(int iPlaylist);

protected:
  bool Repeated(int iPlaylist);
  bool RepeatedOne(int iPlaylist);

  int NextShuffleItem();
  int PreviousShuffleItem();
  bool m_bChanged;
  bool m_bPlayedFirstFile;
  int m_iCurrentSong;
  int m_iCurrentPlayList;
  CPlayList m_PlaylistMusic;
  CPlayList m_PlaylistMusicTemp;
  CPlayList m_PlaylistVideo;
  CPlayList m_PlaylistVideoTemp;
  CPlayList m_PlaylistEmpty;
  REPEAT_STATE m_repeatState[4];
  bool m_shuffleState[4];
  int m_iFailedSongs;
};

};

/*!
 \ingroup windows 
 \brief Global instance of playlist player
 */
extern CPlayListPlayer g_playlistPlayer;

