#include "XBVideoConfig.h"
#include "utils/log.h"

XBVideoConfig g_videoConfig;

XBVideoConfig::XBVideoConfig()
{
	bHasPAL = false;
	bHasNTSC = false;
	m_dwVideoFlags = XGetVideoFlags();
}

XBVideoConfig::~XBVideoConfig()
{
}

bool XBVideoConfig::HasPAL() const
{
	if (bHasPAL) return true;
	if (bHasNTSC) return false;	// has NTSC (or PAL60) but not PAL
	return (XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I) != 0;
}

bool XBVideoConfig::HasPAL60() const
{
	return (m_dwVideoFlags & XC_VIDEO_FLAGS_PAL_60Hz) != 0;
}

bool XBVideoConfig::HasWidescreen() const
{
	return (m_dwVideoFlags & XC_VIDEO_FLAGS_WIDESCREEN) != 0;
}

bool XBVideoConfig::Has480p() const
{
	return (m_dwVideoFlags & XC_VIDEO_FLAGS_HDTV_480p) != 0;
}

bool XBVideoConfig::Has720p() const
{
	return (m_dwVideoFlags & XC_VIDEO_FLAGS_HDTV_720p) != 0;
}

bool XBVideoConfig::Has1080i() const
{
	return (m_dwVideoFlags & XC_VIDEO_FLAGS_HDTV_1080i) != 0;
}

void XBVideoConfig::GetModes(LPDIRECT3D8 pD3D)
{
	bHasPAL = false;
	bHasNTSC = false;
	DWORD numModes = pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT);
  D3DDISPLAYMODE mode;
  CLog::Log(LOGINFO, "Available videomodes:");
  for( DWORD i=0; i<numModes; i++ )
  {
    pD3D->EnumAdapterModes( 0, i, &mode );

    // Skip modes we don't care about
    if( mode.Format != D3DFMT_LIN_A8R8G8B8 )
			continue;
    if( mode.Flags & D3DPRESENTFLAG_FIELD )
			continue;
    if( mode.Flags & D3DPRESENTFLAG_10X11PIXELASPECTRATIO )
			continue;
    if( mode.Flags & D3DPRESENTFLAG_EMULATE_REFRESH_RATE )
			continue;
		// ignore 640 wide modes
		if( mode.Width < 720)
			continue;

    // If we get here, we found an acceptable mode
		CLog::Log(LOGINFO, "Found mode: %ix%i at %iHz, %s", mode.Width, mode.Height, mode.RefreshRate, mode.Flags & D3DPRESENTFLAG_WIDESCREEN ? "Widescreen" : "");
		if (mode.Width = 720 && mode.Height == 576 && mode.RefreshRate == 50)
			bHasPAL = true;
		if (mode.Width = 720 && mode.Height == 480 && mode.RefreshRate == 60)
			bHasNTSC = true;
  }
}

RESOLUTION XBVideoConfig::GetSafeMode() const
{
	if (HasPAL()) return PAL_4x3;
	return NTSC_4x3;
}

//pre: XBVideoConfig::GetModes has been called before this function
RESOLUTION XBVideoConfig::GetInitialMode(LPDIRECT3D8 pD3D, D3DPRESENT_PARAMETERS *p3dParams)
{
  bool bHasPal = HasPAL();
	DWORD numModes = pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT);
  D3DDISPLAYMODE mode;
  for( DWORD i=0; i<numModes; i++ )
  {
    pD3D->EnumAdapterModes( 0, i, &mode );

    // Skip modes we don't care about
    if( mode.Format != D3DFMT_LIN_A8R8G8B8 )
			continue;
    if( mode.Flags & D3DPRESENTFLAG_FIELD )
			continue;
    if( mode.Flags & D3DPRESENTFLAG_10X11PIXELASPECTRATIO )
			continue;
    if( mode.Flags & D3DPRESENTFLAG_EMULATE_REFRESH_RATE )
			continue;
		// ignore 640 wide modes
		if( mode.Width < 720)
			continue;

    p3dParams->BackBufferWidth            = mode.Width;
    p3dParams->BackBufferHeight           = mode.Height;
    p3dParams->FullScreen_RefreshRateInHz = mode.RefreshRate;
    p3dParams->Flags                      = mode.Flags;
    if ((bHasPal) && ((mode.Height != 576) || (mode.RefreshRate != 50))) {
      continue;
    }
    //take the first available mode
    if (!HasWidescreen() && !(mode.Flags & D3DPRESENTFLAG_WIDESCREEN)){
      break;
    }
    if (HasWidescreen() && (mode.Flags & D3DPRESENTFLAG_WIDESCREEN)) {
      break;
    }
  }

  if (HasPAL()){
    if (HasWidescreen() && (p3dParams->Flags & D3DPRESENTFLAG_WIDESCREEN)) {
      return PAL_16x9;
    }
    else {
      return PAL_4x3;
    }
  }
  if (HasWidescreen() && (p3dParams->Flags & D3DPRESENTFLAG_WIDESCREEN)) {
    return NTSC_16x9;
  }
  else {
	  return NTSC_4x3;
  }
}