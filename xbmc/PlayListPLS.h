#pragma once
#include "playlist.h"
using namespace PLAYLIST;
namespace PLAYLIST
{
	class CPlayListPLS :
		public CPlayList
	{
	public:
		CPlayListPLS(void);
		virtual ~CPlayListPLS(void);
		virtual bool 	Load(const CStdString& strFileName);
		virtual void 	Save(const CStdString& strFileName) const;
	};
};