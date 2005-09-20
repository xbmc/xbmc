#pragma once
#include "../stdafx.h"
#include "DllLibCurl.h"

bool DllLibCurl::Load()
{
  if (!DllDynamic::Load())
    return false;

  if (global_init(CURL_GLOBAL_WIN32))
  {
    DllDynamic::Unload();
    CLog::Log(LOGERROR, "Error inializing libcurl");
    return false;
  }

  return true;
}

void DllLibCurl::Unload()
{
  if (!IsLoaded())
    return;

  // close libcurl
  global_cleanup();

  DllDynamic::Unload();
}

DllLibCurl g_curlInterface;
