#pragma once

#include "IMusicInfoTagLoader.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{
	class CMusicInfoTagLoaderNSF: public IMusicInfoTagLoader
	{
    struct NSFDLL 
    {
      int (__cdecl *DLL_LoadNSF)(const char* szFileName);
      void (__cdecl *DLL_FreeNSF)(int nsf);
      int (__cdecl *DLL_GetTitle)(int nsf);
      int (__cdecl *DLL_GetArtist)(int nsf);
      int (__cdecl *DLL_GetNumberOfSongs)(int nsf);
    };

	public:
		CMusicInfoTagLoaderNSF(void);
		virtual ~CMusicInfoTagLoaderNSF();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
	  virtual int GetStreamCount(const CStdString& strFileName);
  private:
    bool LoadDLL();

    int m_nsf;
    bool m_bDllLoaded;
    NSFDLL m_dll;
	};
};

extern CStdString strNSFDLL;
