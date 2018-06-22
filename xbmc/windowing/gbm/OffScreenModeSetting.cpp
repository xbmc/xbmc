#include "OffScreenModeSetting.h"
#include "utils/log.h"

bool COffScreenModeSetting::InitDrm()
{
  if (!CDRMUtils::OpenDrm(false))
  {
    return false;
  }

  CLog::Log(LOGDEBUG, "COffScreenModeSetting::%s - initialized offscreen DRM", __FUNCTION__);
  return true;
}

void COffScreenModeSetting::DestroyDrm()
{
  close(m_fd);
  m_fd = -1;
}

std::vector<RESOLUTION_INFO> COffScreenModeSetting::GetModes()
{
    std::vector<RESOLUTION_INFO> resolutions;
    resolutions.push_back(GetCurrentMode());
    return resolutions;
}

RESOLUTION_INFO COffScreenModeSetting::GetCurrentMode()
{
  RESOLUTION_INFO res;
  res.iScreenWidth = DISPLAY_WIDTH;
  res.iWidth = DISPLAY_WIDTH;
  res.iScreenHeight = DISPLAY_HEIGHT;
  res.iHeight = DISPLAY_HEIGHT;
  res.fRefreshRate = DISPLAY_REFRESH;
  res.iSubtitles = static_cast<int>(0.965 * res.iHeight);
  res.fPixelRatio = 1.0f;
  res.bFullScreen = true;
  res.strId = "0";

  return res;
}
