#include "stdafx.h"
#include "FileLastFM.h"
#include "../Application.h"

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

