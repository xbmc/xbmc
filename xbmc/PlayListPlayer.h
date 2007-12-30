#pragma once

#include "PlayList.h"
#include "IMsgTargetCallback.h"

#define PLAYLIST_NONE    -1
#define PLAYLIST_MUSIC   0
#define PLAYLIST_VIDEO   1

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
  void SetCurrentPlaylist(int iPlaylist);
  int GetCurrentPlaylist();
  CPlayList& GetPlaylist(int iPlaylist);
  int RemoveDVDItems();
  void Reset();
  void ClearPlaylist(int iPlaylist);
  void SetShuffle(int iPlaylist, bool bYesNo);
  bool IsShuffled(int iPlaylist);
  bool HasPlayedFirstFile();
  
  void SetRepeat(int iPlaylist, REPEAT_STATE state);
  REPEAT_STATE GetRepeat(int iPlaylist);

  // add items via the playlist player
  void Add(int iPlaylist, CPlayList::CPlayListItem& item);
  void Add(int iPlaylist, CPlayList& playlist);
  void Add(int iPlaylist, CFileItem *pItem);
  void Add(int iPlaylist, CFileItemList& items);

protected:
  bool Repeated(int iPlaylist);
  bool RepeatedOne(int iPlaylist);
  void ReShuffle(int iPlaylist, int iPosition);
  bool m_bChanged;
  bool m_bPlayedFirstFile;
  int m_iFailedSongs;
  int m_iCurrentSong;
  int m_iCurrentPlayList;
  CPlayList m_PlaylistMusic;
  CPlayList m_PlaylistVideo;
  CPlayList m_PlaylistEmpty;
  REPEAT_STATE m_repeatState[2];
};

}

/*!
 \ingroup windows 
 \brief Global instance of playlist player
 */
extern PLAYLIST::CPlayListPlayer g_playlistPlayer;
