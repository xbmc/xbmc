// XBMCTex.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

LPDIRECT3D8 pD3D;
LPDIRECT3DDEVICE8 pD3DDevice;

UINT UncompressedSize;
UINT CompressedSize;

char HomeDir[MAX_PATH];
const char* InputDir;
const char* OutputDir;

UINT RoundPow2(UINT s)
{
	for (unsigned i = 0; i < 32; ++i)
	{
		if ((1u << i) >= s)
			return (1u << i);
	}
	return 0xffffffffu;
}

#pragma pack(push,1)
struct RGBCOLOUR
{
	BYTE b;
	BYTE g;
	BYTE r;
	BYTE a;
};
#pragma pack(pop)

void PrintImageInfo(D3DXIMAGE_INFO& info)
{
	printf("%4dx%-4d", info.Width, info.Height);
	switch (info.Format)
	{
	case D3DFMT_R8G8B8:
		fputs("  R8G8B8", stdout);
		break;
	case D3DFMT_A8R8G8B8:
		fputs("A8R8G8B8", stdout);
		break;
	case D3DFMT_X8R8G8B8:
		fputs("X8R8G8B8", stdout);
		break;
	case D3DFMT_R5G6B5:
		fputs("  R5G6B5", stdout);
		break;
	case D3DFMT_X1R5G5B5:
		fputs("X1R5G5B5", stdout);
		break;
	case D3DFMT_A1R5G5B5:
		fputs("A1R5G5B5", stdout);
		break;
	case D3DFMT_P8:
		fputs("      P8", stdout);
		break;
	case D3DFMT_DXT1:
		fputs("    DXT1", stdout);
		break;
	case D3DFMT_DXT2:
		fputs("    DXT2", stdout);
		break;
	case D3DFMT_DXT3:
		fputs("    DXT3", stdout);
		break;
	case D3DFMT_DXT4:
		fputs("    DXT4", stdout);
		break;
	case D3DFMT_DXT5:
		fputs("    DXT5", stdout);
		break;
	}
	fputs("->", stdout);
}

#define CheckHR(hr) if (FAILED(hr)) { printf("ERROR: %08x\n", hr); if (pCompSurf) pCompSurf->Release(); if (pDstSurf) pDstSurf->Release(); return false; }

bool GetFormatMSE(const D3DXIMAGE_INFO& info, LPDIRECT3DSURFACE8 pSrcSurf, D3DFORMAT fmt, double& CMSE, double& AMSE)
{
	LPDIRECT3DSURFACE8 pCompSurf = 0, pDstSurf = 0;
	HRESULT hr;

	// Compress
	int Width = RoundPow2(info.Width), Height = RoundPow2(info.Height);
	hr = pD3DDevice->CreateImageSurface(Width, Height, fmt, &pCompSurf);
	CheckHR(hr);

	hr = D3DXLoadSurfaceFromSurface(pCompSurf, NULL, NULL, pSrcSurf, NULL, NULL, D3DX_FILTER_NONE, 0);
	CheckHR(hr);

	// Decompress
	hr = pD3DDevice->CreateImageSurface(Width, Height, D3DFMT_A8R8G8B8, &pDstSurf);
	CheckHR(hr);

	hr = D3DXLoadSurfaceFromSurface(pDstSurf, NULL, NULL, pCompSurf, NULL, NULL, D3DX_FILTER_NONE, 0);
	CheckHR(hr);

	pCompSurf->Release(); pCompSurf = 0;

	// calculate mean square error
	D3DLOCKED_RECT slr, dlr;
	hr = pSrcSurf->LockRect(&slr, NULL, D3DLOCK_READONLY);
	CheckHR(hr);
	hr = pDstSurf->LockRect(&dlr, NULL, D3DLOCK_READONLY);
	CheckHR(hr);

	double CTSE = 0.0; // total colour square error
	double ATSE = 0.0; // total alpha square error

	RGBCOLOUR* src = (RGBCOLOUR*)slr.pBits;
	RGBCOLOUR* dst = (RGBCOLOUR*)dlr.pBits;
	for (UINT y = 0; y < info.Height; ++y)
	{
		for (UINT x = 0; x < info.Width; ++x)
		{
			CTSE += (src->b - dst->b) * (src->b - dst->b);
			CTSE += (src->g - dst->g) * (src->g - dst->g);
			CTSE += (src->r - dst->r) * (src->r - dst->r);
			ATSE += (src->a - dst->a) * (src->a - dst->a);
			++src; ++dst;
		}
		src += (slr.Pitch - info.Width*sizeof(RGBCOLOUR)) / sizeof(RGBCOLOUR);
		dst += (dlr.Pitch - info.Width*sizeof(RGBCOLOUR)) / sizeof(RGBCOLOUR);
	}
	CMSE = CTSE / double(info.Width * info.Height * 3);
	AMSE = ATSE / double(info.Width * info.Height);

	pSrcSurf->UnlockRect();
	pDstSurf->UnlockRect();
	pDstSurf->Release(); pDstSurf = 0;

	return true;
}

void WriteXPR(const char* Filename, const D3DXIMAGE_INFO& info, LPDIRECT3DSURFACE8 pSrcSurf, XB_D3DFORMAT fmt, PALETTEENTRY* pal)
{
	D3DSURFACE_DESC desc;
	pSrcSurf->GetDesc(&desc);

	HRESULT hr;
	UINT Pitch;
	UINT Size;
	void* buf = NULL;

	if (fmt == XB_D3DFMT_DXT1 || fmt == XB_D3DFMT_DXT3 || fmt == XB_D3DFMT_DXT5)
	{
		if (fmt == XB_D3DFMT_DXT1)
			Pitch = desc.Width / 2;
		else
			Pitch = desc.Width;
		Size = ((Pitch * desc.Height) + 63) & ~63; // must be 64-byte aligned for the palette afterwards
		buf = VirtualAlloc(0, Size, MEM_COMMIT, PAGE_READWRITE);
		Pitch *= 4;

		D3DLOCKED_RECT slr;
		hr = pSrcSurf->LockRect(&slr, NULL, D3DLOCK_READONLY);
		if (FAILED(hr))
		{
			printf("ERROR: %08x\n", hr);
			VirtualFree(buf, 0, MEM_RELEASE);
			return;
		}

		hr = XGCompressRect(buf, (D3DFORMAT)fmt, Pitch, desc.Width, desc.Height, slr.pBits, (D3DFORMAT)XB_D3DFMT_LIN_A8R8G8B8, slr.Pitch, 0.5f, 0);
		if (FAILED(hr))
		{
			printf("ERROR: %08x\n", hr);
			VirtualFree(buf, 0, MEM_RELEASE);
			return;
		}

		pSrcSurf->UnlockRect();
	}
	else
	{
		UINT bpp = XGBytesPerPixelFromFormat((D3DFORMAT)fmt);
		Pitch = desc.Width * bpp;
		Size = ((Pitch * desc.Height) + 63) & ~63; // must be 64-byte aligned for the palette afterwards
		buf = VirtualAlloc(0, Size, MEM_COMMIT, PAGE_READWRITE);

		D3DLOCKED_RECT slr;
		hr = pSrcSurf->LockRect(&slr, NULL, D3DLOCK_READONLY);
		if (FAILED(hr))
		{
			printf("ERROR: %08x\n", hr);
			VirtualFree(buf, 0, MEM_RELEASE);
			return;
		}

		if (XGIsSwizzledFormat((D3DFORMAT)fmt))
		{
			// Swizzle for xbox
			RECT rc = { 0, 0, info.Width, info.Height };
			XGSwizzleRect(slr.pBits, slr.Pitch, &rc, buf, desc.Width, desc.Height, (POINT*)&rc, bpp);
		}
		else
		{
			// copy
			BYTE* src = (BYTE*)slr.pBits;
			BYTE* dst = (BYTE*)buf;
			for (UINT y = 0; y < desc.Height; ++y)
			{
				memcpy(dst, src, desc.Width * bpp);
				src += slr.Pitch;
				dst += Pitch;
			}
		}

		pSrcSurf->UnlockRect();
	}

	// setup headers for xpr
	XPR_HEADER XPRHeader;
	XPRHeader.dwMagic = XPR_MAGIC_VALUE;
	XPRHeader.dwHeaderSize = sizeof(XPR_HEADER) + sizeof(D3DTexture) + 4;
	if (pal)
		XPRHeader.dwHeaderSize += sizeof(D3DPalette);
	XPRHeader.dwTotalSize = XPRHeader.dwHeaderSize + Size;
	if (pal)
		XPRHeader.dwTotalSize += 1024;
	D3DTexture D3DTex;
	XGSetTextureHeader(desc.Width, desc.Height, 1, 0, (D3DFORMAT)fmt, D3DPOOL_DEFAULT, (IDirect3DTexture8*)&D3DTex, 0, Pitch);
	D3DPalette D3DPal;
	D3DPal.Common = 1 | (3 << 16);
	D3DPal.Data = Size;
	D3DPal.Lock = 0;

	if (OutputDir)
	{
		SetCurrentDirectory(HomeDir);
		SetCurrentDirectory(OutputDir);
	}

	HANDLE hFile = CreateFile(Filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("ERROR: %08x\n", GetLastError());
		VirtualFree(buf, 0, MEM_RELEASE);
		return;
	}

	// write headers
	DWORD n;
	WriteFile(hFile, &XPRHeader, sizeof(XPR_HEADER), &n, NULL);
	WriteFile(hFile, &D3DTex, sizeof(D3DTexture), &n, NULL);
	if (pal)
		WriteFile(hFile, &D3DPal, sizeof(D3DPalette), &n, NULL);
	WORD RealSize[2] = { info.Width, info.Height };
	WriteFile(hFile, RealSize, 4, &n, NULL);

	// write texture
	WriteFile(hFile, buf, Size, &n, NULL);
	// write palette
	if (pal)
		WriteFile(hFile, pal, 1024, &n, NULL);

	VirtualFree(buf, 0, MEM_RELEASE);
	CloseHandle(hFile);

	if (OutputDir)
	{
		SetCurrentDirectory(HomeDir);
		if (InputDir)
			SetCurrentDirectory(InputDir);
	}

	CompressedSize += Size;
}

#undef CheckHR
#define CheckHR(hr) if (FAILED(hr)) { printf("ERROR: %08x\n", hr); continue; }

void ConvertFiles(const char* Filename, double MaxMSE)
{
	HRESULT hr;

	WIN32_FIND_DATAA FindData;
	HANDLE hFind = FindFirstFile(Filename, &FindData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		LPDIRECT3DSURFACE8 pSrcSurf = NULL;
		do {
			if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				printf("%s: ", FindData.cFileName);
				TRACE1("%s:\n", FindData.cFileName);

				int n = (int)strlen(FindData.cFileName);
				char* OutputFile = (char*)_alloca(n + 5);
				memcpy(OutputFile, FindData.cFileName, n);
				strcpy(OutputFile + n, ".xpr");
				if (n < 40)
					printf("%*c", 40 - n, ' ');

				if (pSrcSurf)
					pSrcSurf->Release();
				pSrcSurf = NULL;

				// Load up the file
				D3DXIMAGE_INFO info;
				D3DXGetImageInfoFromFile(FindData.cFileName, &info);

				PrintImageInfo(info);

				UINT Width = RoundPow2(info.Width);
				UINT Height = RoundPow2(info.Height);

				UncompressedSize += Width * Height * 4;

				// Special case for 256-colour files - just directly drop into a P8 xpr
				if (info.Format == D3DFMT_P8)
				{
					hr = pD3DDevice->CreateImageSurface(Width, Height, D3DFMT_P8, &pSrcSurf);
					CheckHR(hr);

					PALETTEENTRY pal[256];
					hr = D3DXLoadSurfaceFromFile(pSrcSurf, pal, NULL, FindData.cFileName, NULL, D3DX_FILTER_NONE, 0, NULL);
					CheckHR(hr);

					printf("P8       %4dx%-4d\n", Width, Height);
					TRACE0(" Selected Format: P8\n");

					WriteXPR(OutputFile, info, pSrcSurf, XB_D3DFMT_P8, pal);
					continue;
				}

				hr = pD3DDevice->CreateImageSurface(Width, Height, D3DFMT_A8R8G8B8, &pSrcSurf);
				CheckHR(hr);

				hr = D3DXLoadSurfaceFromFile(pSrcSurf, NULL, NULL, FindData.cFileName, NULL, D3DX_FILTER_NONE, 0, NULL);
				CheckHR(hr);

				// special case for small files - all textures are alloced on page granularity so just output uncompressed
				// dxt is crap on small files anyway
				if (Width * Height <= 1024)
				{
					printf("A8R8G8B8 %4dx%-4d\n", Width, Height);
					TRACE0(" Selected Format: A8R8G8B8\n");

					WriteXPR(OutputFile, info, pSrcSurf, XB_D3DFMT_A8R8G8B8, NULL);
					continue;
				}

				// Find the best format within specified tolerance
				double CMSE, AMSE[2];

				// DXT1 is the preferred format as it's smallest
				TRACE0(" Checking     DXT1: ");
				if (!GetFormatMSE(info, pSrcSurf, D3DFMT_DXT1, CMSE, AMSE[0]))
					continue;
				TRACE2("CMSE=%05.2f, AMSE=%07.2f\n", CMSE, AMSE[0]);
				if (CMSE <= MaxMSE && AMSE[0] <= MaxMSE)
				{
					printf("DXT1     %4dx%-4d\n", Width, Height);
					TRACE0(" Selected Format: DXT1\n");

					WriteXPR(OutputFile, info, pSrcSurf, XB_D3DFMT_DXT1, NULL);
					continue;
				}

				// DXT3/5 are the same size so use whichever is better if good enough
				// CMSE will be equal for both
				TRACE0(" Checking     DXT3: ");
				if (!GetFormatMSE(info, pSrcSurf, D3DFMT_DXT3, CMSE, AMSE[0]))
					continue;
				TRACE2("CMSE=%05.2f, AMSE=%07.2f\n", CMSE, AMSE[0]);

				TRACE0(" Checking     DXT5: ");
				if (!GetFormatMSE(info, pSrcSurf, D3DFMT_DXT5, CMSE, AMSE[1]))
					continue;
				TRACE2("CMSE=%05.2f, AMSE=%07.2f\n", CMSE, AMSE[1]);

				if (AMSE[0] <= AMSE[1])
				{
					if (CMSE <= MaxMSE && AMSE[0] <= MaxMSE)
					{
						printf("DXT3     %4dx%-4d\n", Width, Height);
						TRACE0(" Selected Format: DXT3\n");

						WriteXPR(OutputFile, info, pSrcSurf, XB_D3DFMT_DXT3, NULL);
						continue;
					}
				}
				else
				{
					if (CMSE <= MaxMSE && AMSE[1] <= MaxMSE)
					{
						printf("DXT5     %4dx%-4d\n", Width, Height);
						TRACE0(" Selected Format: DXT5\n");

						WriteXPR(OutputFile, info, pSrcSurf, XB_D3DFMT_DXT5, NULL);
						continue;
					}
				}

				// No good compressed format so use uncompressed

				// A1R5G5B5 is worth a try I guess...
				TRACE0(" Checking A1R5G5B5: ");
				if (!GetFormatMSE(info, pSrcSurf, D3DFMT_A1R5G5B5, CMSE, AMSE[0]))
					continue;
				TRACE2("CMSE=%05.2f, AMSE=%07.2f\n", CMSE, AMSE[0]);
				if (CMSE <= MaxMSE && AMSE[0] <= MaxMSE)
				{
					printf("A1R5G5B5 %4dx%-4d\n", Width, Height);
					TRACE0(" Selected Format: A1R5G5B5\n");

					LPDIRECT3DSURFACE8 pTempSurf;
					hr = pD3DDevice->CreateImageSurface(Width, Height, D3DFMT_A1R5G5B5, &pTempSurf);
					CheckHR(hr);

					hr = D3DXLoadSurfaceFromSurface(pTempSurf, NULL, NULL, pSrcSurf, NULL, NULL, D3DX_FILTER_NONE, 0);
					CheckHR(hr);

					WriteXPR(OutputFile, info, pTempSurf, XB_D3DFMT_A1R5G5B5, NULL);

					pTempSurf->Release();
					continue;
				}

				// Use A8R8G8B8
				printf("A8R8G8B8 %4dx%-4d\n", Width, Height);
				TRACE0(" Selected Format: A8R8G8B8\n");

				WriteXPR(OutputFile, info, pSrcSurf, XB_D3DFMT_A8R8G8B8, NULL);
			}
		} while (FindNextFile(hFind, &FindData));
		FindClose(hFind);

		if (pSrcSurf)
			pSrcSurf->Release();
	}
}

int main(int argc, char* argv[])
{
	double MaxMSE = 4.0;
	for (int i = 1; i < argc; ++i)
	{
		if (!stricmp(argv[i], "-help") || !stricmp(argv[i], "-h") || !stricmp(argv[i], "-?"))
		{
			puts("Usage:");
			puts("  -help            Show this screen.");
			puts("  -input <dir>     Input directory. Default: current dir");
			puts("  -output <dir>    Output directory. Default: current dir");
			puts("  -quality <qual>  Quality setting (min, low, normal, high, max). Default: normal");
			return 1;
		}
		else if (!stricmp(argv[i], "-input") || !stricmp(argv[i], "-i"))
		{
			InputDir = argv[++i];
		}
		else if (!stricmp(argv[i], "-output") || !stricmp(argv[i], "-o"))
		{
			OutputDir = argv[++i];
		}
		else if (!stricmp(argv[i], "-quality") || !stricmp(argv[i], "-q"))
		{
			++i;
			if (!stricmp(argv[i], "min"))
			{
				MaxMSE = DBL_MAX;
			}
			else if (!stricmp(argv[i], "low"))
			{
				MaxMSE = 8.0;
			}
			else if (!stricmp(argv[i], "normal"))
			{
				MaxMSE = 4.0;
			}
			else if (!stricmp(argv[i], "high"))
			{
				MaxMSE = 2.0;
			}
			else if (!stricmp(argv[i], "max"))
			{
				MaxMSE = 0.0;
			}
			else
			{
				printf("Unrecognised quality setting: %s\n", argv[i]);
			}
		}
		else
		{
			printf("Unrecognised command line flag: %s\n", argv[i]);
		}
	}

	// Initialize DirectDraw
	pD3D = Direct3DCreate8(D3D_SDK_VERSION);
	if (pD3D == NULL)
	{
		puts("Cannot init D3D");
		return 1;
	}

	HRESULT hr;
	D3DDISPLAYMODE dispMode;
	D3DPRESENT_PARAMETERS presentParams;

	pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dispMode);

	ZeroMemory(&presentParams, sizeof(presentParams));
	presentParams.Windowed = TRUE;
	presentParams.hDeviceWindow = GetConsoleWindow();
	presentParams.SwapEffect = D3DSWAPEFFECT_COPY_VSYNC;
	presentParams.BackBufferWidth = 8;
	presentParams.BackBufferHeight = 8;
	presentParams.BackBufferFormat = dispMode.Format;

	hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParams, &pD3DDevice);
	if (FAILED(hr))
	{
		printf("Cannot init D3D device: %08x\n", hr);
		pD3D->Release();
		return 1;
	}

	GetCurrentDirectory(MAX_PATH, HomeDir);

	if (OutputDir)
		CreateDirectory(OutputDir, NULL);

	if (InputDir)
		SetCurrentDirectory(InputDir);

	ConvertFiles("*.png", MaxMSE);
	ConvertFiles("*.bmp", MaxMSE);
	ConvertFiles("*.tga", MaxMSE);
	ConvertFiles("*.jpg", MaxMSE);
	ConvertFiles("*.dds", MaxMSE);

	pD3DDevice->Release();
	pD3D->Release();

	printf("\nUncompressed texture size: %6dkB\nCompressed texture size: %8dkB\n",
		(UncompressedSize + 1023) / 1024, (((CompressedSize + 1023) / 1024) + 3) & ~3);

	return 0;
}

