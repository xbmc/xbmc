#pragma once

#include "ImusicInfoTagLoader.h"
#include "cores/paplayer/DllGensApu.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{
	class CMusicInfoTagLoaderGYM: public IMusicInfoTagLoader
	{
	public:
		CMusicInfoTagLoaderGYM(void);
		virtual ~CMusicInfoTagLoaderGYM();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
  private:
    int m_gym;
    DllGensApu m_dll;
	};
}
