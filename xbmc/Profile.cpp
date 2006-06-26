#include "stdafx.h"
#include ".\profile.h"


CProfile::CProfile(void)
{
  _bDatabases = true;
  _bCanWrite = true;
  _bSources = true;
  _bCanWriteSources = true;
}

CProfile::~CProfile(void)
{}

