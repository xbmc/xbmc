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

	protected:
		bool			LoadFromWeb(CStdString& strURL);
		bool			LoadAsxInfo(CStdString& strData);
		bool			LoadAsxIniInfo(CStdString& strData);
		bool			LoadRAMInfo(CStdString& strData);
	};
};