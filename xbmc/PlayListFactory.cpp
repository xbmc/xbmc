
#include "stdafx.h"
#include "playlistfactory.h"
#include "Playlistm3u.h"
#include "PlaylistPLS.h"
#include "Playlistb4S.h"
#include "util.h"
using namespace PLAYLIST;
CPlayListFactory::CPlayListFactory(void)
{
}

CPlayListFactory::~CPlayListFactory(void)
{
}


CPlayList* CPlayListFactory::Create(const CStdString& strFileName) const
{
	CStdString strExtension;
	CUtil::GetExtension(strFileName,strExtension);
	strExtension.ToLower();
	if (strExtension==".m3u")
	{
		return new CPlayListM3U();
	}
	if (strExtension==".pls" || strExtension==".strm")
	{
		return new CPlayListPLS();
	}
	if (strExtension==".b4s")
	{
		return new CPlayListB4S();
	}
	return NULL;
}