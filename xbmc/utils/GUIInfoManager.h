/*!
	\file GUIInfoManager.h
	\brief 
	*/

#ifndef GUILIB_GUIInfoManager_H
#define GUILIB_GUIInfoManager_H
#pragma once

#include "stdstring.h"
#include "../MusicInfoTag.h"
#include "../FileItem.h"

using namespace std;

/*!
	\ingroup strings
	\brief 
	*/
class CGUIInfoManager
{
public:
  CGUIInfoManager(void);
  virtual ~CGUIInfoManager(void);

	wstring GetLabel(const CStdString &strInfo);
	CStdString GetImage(const CStdString &strInfo);
	LPDIRECT3DTEXTURE8 GetTexture(const CStdString &strInfo);

	wstring GetTime(bool bSeconds = false);
	wstring GetDate(bool bNumbersOnly = false);

	// Current song stuff
	/// \brief Retrieves tag info (if necessary) and fills in our current song path.
	void SetCurrentSong(const CFileItem &item);
	void SetCurrentSongTag(const CMusicInfoTag &tag) { m_currentSong.m_musicInfoTag = tag; m_currentSong.m_lStartOffset = 0;};
	const CMusicInfoTag &GetCurrentSongTag() const { return m_currentSong.m_musicInfoTag; };
	void ResetCurrentSong() { m_currentSong.Clear(); };
	long GetCurrentSongStart() { return m_currentSong.m_lStartOffset; };
	long GetCurrentSongEnd() { return m_currentSong.m_lEndOffset; };
	CStdString GetMusicLabel(const CStdString &strItem);
	CStdString GetVideoLabel(const CStdString &strItem);
	CStdString GetProgressBar(const CStdString &strItem);
	wstring GetFreeSpace(const CStdString &strDrive);
	CStdString GetCurrentPlayTime();

protected:

	// Current playing song stuff
	CFileItem m_currentSong;
};

/*!
	\ingroup strings
	\brief 
	*/
extern CGUIInfoManager g_infoManager;
#endif
