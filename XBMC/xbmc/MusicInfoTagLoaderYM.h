#pragma once

#include "ImusicInfoTagLoader.h"
#include "cores/paplayer/DllStSound.h"

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
