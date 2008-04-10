#include "stdafx.h"
#include "FileLastFM.h"

namespace XFILE
{

CFileLastFM::CFileLastFM() : CFileCurl()
{
  SetUserAgent("");
  SetBufferSize(8192);
}

CFileLastFM::~CFileLastFM()
{
}

}

