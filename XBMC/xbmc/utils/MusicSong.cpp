
#include "../stdafx.h"
#include ".\musicsong.h"
#include "../util.h"
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

void CMusicSong::Save(FILE* fd)
{
	CUtil::SaveString(m_strSongName,fd);
	CUtil::SaveInt(m_iTrack,fd);
	CUtil::SaveInt(m_iDuration,fd);
}
void CMusicSong::Load(FILE* fd)
{
	CUtil::LoadString(m_strSongName,fd);
	m_iTrack   =CUtil::LoadInt(fd);
	m_iDuration=CUtil::LoadInt(fd);
}