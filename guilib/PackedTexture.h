
HRESULT LoadPackedTexture(LPDIRECT3DDEVICE8 pDevice, const char* szFilename, D3DXIMAGE_INFO* pInfo, LPDIRECT3DTEXTURE8* ppTexture,
													LPDIRECT3DPALETTE8* ppPalette);

int LoadPackedAnim(LPDIRECT3DDEVICE8 pDevice, const char* szFilename, D3DXIMAGE_INFO* pInfo, LPDIRECT3DTEXTURE8** ppTextures,
									 LPDIRECT3DPALETTE8* ppPalette, int& nLoops, int** ppDelays);
