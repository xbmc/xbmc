#include "musicinfotagloaderWMA.h"
#include "stdstring.h"
#include "sectionloader.h"

using namespace MUSIC_INFO;
using namespace XFILE;

CMusicInfoTagLoaderWMA::CMusicInfoTagLoaderWMA(void)
{
}

CMusicInfoTagLoaderWMA::~CMusicInfoTagLoaderWMA()
{
}

bool CMusicInfoTagLoaderWMA::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  tag.SetLoaded(false);
  CFile file;
  if (!file.Open(strFileName.c_str())) return false;
  
  // parse file...
	return false;
}
