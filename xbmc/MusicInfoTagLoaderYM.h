#pragma once

#include "IMusicInfoTagLoader.h"
#include "cores/paplayer/dllstsound.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{
	class CMusicInfoTagLoaderYM: public IMusicInfoTagLoader
	{
	public:
		CMusicInfoTagLoaderYM(void);
		virtual ~CMusicInfoTagLoaderYM();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
  private:
    int m_ym;
    DllStSound m_dll;
	};
}
