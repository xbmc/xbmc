#pragma once

class XBVideoConfig
{
public:
	XBVideoConfig();
	~XBVideoConfig();

	bool HasPAL() const;
	bool HasPAL60() const;
	bool HasWidescreen() const;
	bool Has480p() const;
	bool Has720p() const;
	bool Has1080i() const;
	void GetModes(LPDIRECT3D8 pD3D);
	RESOLUTION GetSafeMode() const;
	RESOLUTION GetBestMode() const;
	bool IsValidResolution(RESOLUTION res) const;
  RESOLUTION GetInitialMode(LPDIRECT3D8 pD3D, D3DPRESENT_PARAMETERS *p3dParams);
	void PrintInfo() const;

private:
	bool bHasPAL;
	bool bHasNTSC;
	DWORD m_dwVideoFlags;
};

extern XBVideoConfig g_videoConfig;