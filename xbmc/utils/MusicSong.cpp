#include ".\musicsong.h"
using namespace MUSIC_GRABBER;

CMusicSong::CMusicSong(int iTrack, const CStdString& strName, int iDuration)
{
	m_iTrack=iTrack;
	m_strSongName=strName;
	m_iDuration=iDuration;
}
CMusicSong::CMusicSong(void)
{
	m_iTrack=0;
	m_strSongName="";
	m_iDuration=0;
}

CMusicSong::~CMusicSong(void)
{
}

const CStdString& CMusicSong::GetSongName() const
{
	return m_strSongName;
}

int	CMusicSong::GetTrack() const
{
	return m_iTrack;
}

int CMusicSong::GetDuration() const
{
	return m_iDuration;
}

bool CMusicSong::Parse(const CStdString& strHTML)
{
	return false;
}
