// XBMCTex.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "AnimatedGif.h"
#include "Bundler.h"
#include <stdio.h>
#include <algorithm>
#include "cmdlineargs.h"

extern "C" void SHA1(const BYTE* buf, DWORD len, BYTE hash[20]);

LPDIRECT3D8 pD3D;
LPDIRECT3DDEVICE8 pD3DDevice;

CBundler Bundler;

UINT UncompressedSize;
UINT CompressedSize;
UINT TotalSrcPixels;
UINT TotalDstPixels;

bool AllowLinear;
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
	printf("%4dx%-4d ", info.Width, info.Height);
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

void PrintAnimInfo(const CAnimatedGifSet& Anim)
{
	printf("%4dx%-4d (%5df)->", Anim.FrameWidth, Anim.FrameHeight, Anim.GetImageCount());
}

#define CheckHR(hr) if (FAILED(hr)) { printf("ERROR: %08x\n", hr); if (pCompSurf) pCompSurf->Release(); if (pDstSurf) pDstSurf->Release(); return false; }

bool GetFormatMSE(const D3DXIMAGE_INFO& info, LPDIRECT3DSURFACE8 pSrcSurf, D3DFORMAT fmt, double& CMSE, double& AMSE)
{
	LPDIRECT3DSURFACE8 pCompSurf = 0, pDstSurf = 0;
	HRESULT hr;

	// Compress
	int Width = PadPow2(info.Width), Height = PadPow2(info.Height);
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


struct XPRFile_t
{
	DWORD HeaderSize;
	DWORD* flags;
	struct AnimInfo_t {
		DWORD nLoops;
		DWORD RealSize;
	} *AnimInfo;
	D3DPalette* D3DPal;
	struct Texture_t {
		D3DTexture D3DTex;
		DWORD RealSize;
	} *Texture;
	char* Data;

	int nImages;
	char* OutputBuf;
	char* DataStart;
};
static XPRFile_t XPRFile;

enum XPR_FLAGS
{
	XPRFLAG_PALETTE = 0x00000001,
	XPRFLAG_ANIM =    0x00000002,
};

#undef CheckHR

void CommitXPR(const char* Filename)
{
	if (!XPRFile.nImages)
		return;

	const void* Buffers[2] = { XPRFile.OutputBuf, XPRFile.DataStart };
	DWORD Sizes[2] = { XPRFile.HeaderSize, XPRFile.Data - XPRFile.DataStart };
	if (!Bundler.AddFile(Filename, 2, Buffers, Sizes))
		printf("ERROR: Unable to compress data (out of memory?)\n");

	VirtualAlloc(XPRFile.OutputBuf, XPRFile.Data - XPRFile.OutputBuf, MEM_RESET, PAGE_NOACCESS);
}

void WriteXPRHeader(DWORD* pal, int nImages, DWORD nLoops = 0)
{
	// Set header pointers
	XPRFile.flags = (DWORD*)XPRFile.OutputBuf;
	void* next = XPRFile.flags + 1;
	if (nImages > 1)
	{
		XPRFile.AnimInfo = (XPRFile_t::AnimInfo_t*)next;
		next = XPRFile.AnimInfo + 1;
	}
	else
		XPRFile.AnimInfo = NULL;
	if (pal)
	{
		XPRFile.D3DPal = (D3DPalette*)next;
		next = XPRFile.D3DPal + 1;
	}
	else
		XPRFile.D3DPal = NULL;
	XPRFile.Texture = (XPRFile_t::Texture_t*)next;
	next = XPRFile.Texture + nImages;
	XPRFile.Data = (char*)next;
	XPRFile.nImages = 0;

	// commit memory for headers
	VirtualAlloc(XPRFile.OutputBuf, XPRFile.Data - XPRFile.OutputBuf, MEM_COMMIT, PAGE_READWRITE);

	// setup headers for xpr
	XPRFile.HeaderSize = ((XPRFile.Data - XPRFile.OutputBuf) + 127) & ~127;
	*XPRFile.flags = (nImages << 16);

	if (nImages > 1)
	{
		*XPRFile.flags |= XPRFLAG_ANIM;
		XPRFile.AnimInfo->nLoops = nLoops;
	}

	// align data to page
	XPRFile.Data = XPRFile.DataStart = (char*)(int(XPRFile.Data + 4095) & ~4095);

	if (pal)
	{
		// commit memory for palette
		VirtualAlloc(XPRFile.Data, 1024, MEM_COMMIT, PAGE_READWRITE);

		*XPRFile.flags |= XPRFLAG_PALETTE;
		XPRFile.D3DPal->Common = 1 | (3 << 16);
		XPRFile.D3DPal->Data = 0;
		XPRFile.D3DPal->Lock = 0;
		memcpy(XPRFile.Data, pal, 1024);
		XPRFile.Data += 1024;
	}
}

void AppendXPRImage(const D3DXIMAGE_INFO& info, LPDIRECT3DSURFACE8 pSrcSurf, XB_D3DFORMAT fmt)
{
	D3DSURFACE_DESC desc;
	pSrcSurf->GetDesc(&desc);

	HRESULT hr;
	UINT Pitch;
	UINT Size;

	if (fmt == XB_D3DFMT_DXT1 || fmt == XB_D3DFMT_DXT3 || fmt == XB_D3DFMT_DXT5)
	{
		if (fmt == XB_D3DFMT_DXT1)
			Pitch = desc.Width / 2;
		else
			Pitch = desc.Width;
		Size = ((Pitch * desc.Height) + 127) & ~127; // must be 128-byte aligned for any following images
		Pitch *= 4;

		VirtualAlloc(XPRFile.Data, Size, MEM_COMMIT, PAGE_READWRITE);

		D3DLOCKED_RECT slr;
		hr = pSrcSurf->LockRect(&slr, NULL, D3DLOCK_READONLY);
		if (FAILED(hr))
		{
			printf("ERROR: %08x\n", hr);
			return;
		}

    hr = CompressRect(XPRFile.Data, fmt, Pitch, desc.Width, desc.Height, slr.pBits, XB_D3DFMT_LIN_A8R8G8B8, slr.Pitch, 0.5f, 0);
    if (FAILED(hr))
		{
			printf("ERROR: %08x\n", hr);
			return;
		}

		pSrcSurf->UnlockRect();
	}
	else
	{
		UINT bpp = BytesPerPixelFromFormat(fmt);
		Pitch = desc.Width * bpp;
		Size = ((Pitch * desc.Height) + 127) & ~127; // must be 128-byte aligned for any following images

		VirtualAlloc(XPRFile.Data, Size, MEM_COMMIT, PAGE_READWRITE);

		D3DLOCKED_RECT slr;
		hr = pSrcSurf->LockRect(&slr, NULL, D3DLOCK_READONLY);
		if (FAILED(hr))
		{
			printf("ERROR: %08x\n", hr);
			return;
		}

		if (IsSwizzledFormat(fmt))
		{
			// Swizzle for xbox
			SwizzleRect(slr.pBits, 0, NULL, XPRFile.Data, desc.Width, desc.Height, NULL, bpp);
		}
		else
		{
			// copy
			BYTE* src = (BYTE*)slr.pBits;
			BYTE* dst = (BYTE*)XPRFile.Data;
			for (UINT y = 0; y < desc.Height; ++y)
			{
				memcpy(dst, src, desc.Width * bpp);
				src += slr.Pitch;
				dst += Pitch;
			}
		}

		pSrcSurf->UnlockRect();
	}

	SetTextureHeader(desc.Width, desc.Height, 1, 0, fmt, D3DPOOL_DEFAULT, 
		&XPRFile.Texture[XPRFile.nImages].D3DTex, XPRFile.Data - XPRFile.DataStart, Pitch);
	if (!(*XPRFile.flags & XPRFLAG_ANIM))
		XPRFile.Texture[XPRFile.nImages].RealSize = (info.Width & 0xffff) | ((info.Height & 0xffff) << 16);
	++XPRFile.nImages;

	XPRFile.Data += Size;
	CompressedSize += Size;
}

void AppendXPRImageLink(int iLinkedImage)
{
	memcpy(&XPRFile.Texture[XPRFile.nImages].D3DTex, &XPRFile.Texture[iLinkedImage].D3DTex, sizeof(D3DTexture));
	++XPRFile.nImages;
}

void WriteXPR(const char* Filename, const D3DXIMAGE_INFO& info, LPDIRECT3DSURFACE8 pSrcSurf, XB_D3DFORMAT fmt, DWORD* pal)
{
	WriteXPRHeader(pal, 1);
	AppendXPRImage(info, pSrcSurf, fmt);
	CommitXPR(Filename);
}

// Converts any fully transparent pixels to black so that the mse calcs work for dxt
void FixTransparency(LPDIRECT3DSURFACE8 pSrcSurf)
{
	D3DSURFACE_DESC desc;
	pSrcSurf->GetDesc(&desc);

	D3DLOCKED_RECT slr;
	if (FAILED(pSrcSurf->LockRect(&slr, NULL, 0)))
		return;

	DWORD* pix = (DWORD*)slr.pBits;
	for (UINT y = 0; y < desc.Width; ++y)
	{
		for (UINT x = 0; x < desc.Height; ++x)
		{
			if (!(*pix & 0xff000000))
				*pix = 0;
			++pix;
		}
	}

	pSrcSurf->UnlockRect();
}

#undef CheckHR
#define CheckHR(hr) if (FAILED(hr)) { printf("ERROR: %08x\n", hr); if (pDstSurf) pDstSurf->Release(); return false; }

// Converts to P8 format is colours <= 256
bool ConvertP8(LPDIRECT3DSURFACE8 pSrcSurf, LPDIRECT3DSURFACE8& pDstSurf, DWORD* pal, D3DXIMAGE_INFO &info)
{
	pDstSurf = 0;

	D3DSURFACE_DESC desc;
	pSrcSurf->GetDesc(&desc);

	// convert to p8
  UINT Width = PadPow2(desc.Width);
  UINT Height = PadPow2(desc.Height);
	HRESULT hr = pD3DDevice->CreateImageSurface(Width, Height, D3DFMT_A8R8G8B8, &pDstSurf);
	CheckHR(hr);

	D3DLOCKED_RECT slr, dlr;
	hr = pDstSurf->LockRect(&dlr, NULL, 0);
	CheckHR(hr);
	hr = pSrcSurf->LockRect(&slr, NULL, D3DLOCK_READONLY);
	CheckHR(hr);

	DWORD* src = (DWORD*)slr.pBits;
	BYTE* dst = (BYTE*)dlr.pBits;
	int n = 0, i;
	for (UINT y = 0; y < info.Height; ++y)
	{
		for (UINT x = 0; x < info.Width; ++x)
		{
			for (i = 0; i < n; ++i)
			{
				if (pal[i] == *src)
					break;
			}
			if (i == n)
			{
				if (n >= 256)
				{
					TRACE0(" Too many colours for P8\n");
					pSrcSurf->UnlockRect();
					pDstSurf->UnlockRect();
					pDstSurf->Release();
					return false;
				}
				pal[n++] = *src;
			}
			*dst++ = i;
			++src;
		}
		for (UINT x = info.Width; x < Width; ++x)
		{
			*dst++ = 0; // we don't care about the colour outside of our real image
			++src;
    }
	}
  for (UINT y = info.Height; y < Height; ++y)
  {
		for (UINT x = 0; x < Width; ++x)
		{
			*dst++ = 0; // we don't care about the colour outside of our real image
			++src;
    }
  }
	TRACE1(" Colours Used: %d\n", n);

	pDstSurf->UnlockRect();
	pSrcSurf->UnlockRect();

	return true;
}

#undef CheckHR
#define CheckHR(hr) if (FAILED(hr)) { printf("ERROR: %08x\n", hr); return; }

void ConvertFile(const char* Dir, const char* Filename, double MaxMSE)
{
	HRESULT hr;
	LPDIRECT3DSURFACE8 pSrcSurf = NULL;
	char OutFilename[52];
	if (Dir)
		_snprintf(OutFilename, 52, "%s\\%s", Dir, Filename);
	else
		_snprintf(OutFilename, 52, "%s", Filename);
	OutFilename[51] = 0;

	printf("%s: ", OutFilename);
	TRACE1("%s:\n", OutFilename);
	int n = strlen(OutFilename);
	if (n < 40)
		printf("%*c", 40-n, ' ');

	if (pSrcSurf)
		pSrcSurf->Release();
	pSrcSurf = NULL;

	// Load up the file
	D3DXIMAGE_INFO info;
	hr = D3DXGetImageInfoFromFile(Filename, &info);
	CheckHR(hr);

	PrintImageInfo(info);

	UINT Width = PadPow2(info.Width);
	UINT Height = PadPow2(info.Height);

	float Waste = 100.f * (float)(Width * Height - info.Width * info.Height) / (float)(Width * Height);

	UncompressedSize += Width * Height * 4;
	TotalSrcPixels += info.Width * info.Height;
	TotalDstPixels += Width * Height;

  // Special case for 256-colour files - just directly drop into a P8 xpr
	if (info.Format == D3DFMT_P8)
	{
		hr = pD3DDevice->CreateImageSurface(Width, Height, D3DFMT_A8R8G8B8, &pSrcSurf);
		CheckHR(hr);

		hr = D3DXLoadSurfaceFromFile(pSrcSurf, NULL, NULL, Filename, NULL, D3DX_FILTER_NONE, 0, NULL);
		CheckHR(hr);

		FixTransparency(pSrcSurf);

		if (Width * Height > 4096)
		{
			// DXT1 for P8s if lossless and more than 4k image
			LPDIRECT3DSURFACE8 pTempSurf;
			hr = pD3DDevice->CreateImageSurface(Width, Height, D3DFMT_A8R8G8B8, &pTempSurf);
			CheckHR(hr);

			hr = D3DXLoadSurfaceFromSurface(pTempSurf, NULL, NULL, pSrcSurf, NULL, NULL, D3DX_FILTER_NONE, 0);
			CheckHR(hr);

			double CMSE, AMSE;
			TRACE0(" Checking     DXT1: ");
			if (!GetFormatMSE(info, pTempSurf, D3DFMT_DXT1, CMSE, AMSE))
			{
				pTempSurf->Release();
				return;
			}
			TRACE2("CMSE=%05.2f, AMSE=%07.2f\n", CMSE, AMSE);
			if (CMSE <= 1e-6 && AMSE <= 1e-6)
			{
				printf("DXT1     %4dx%-4d (%5.2f%% waste)\n", Width, Height, Waste);
				TRACE0(" Selected Format: DXT1\n");
				WriteXPR(OutFilename, info, pTempSurf, XB_D3DFMT_DXT1, NULL);

				pTempSurf->Release();
				return;
			}
			pTempSurf->Release();
		}

		printf("P8       %4dx%-4d (%5.2f%% waste)\n", Width, Height, Waste);
		TRACE0(" Selected Format: P8\n");

		LPDIRECT3DSURFACE8 pTempSurf;
		DWORD pal[256];
		ConvertP8(pSrcSurf, pTempSurf, pal, info);

		WriteXPR(OutFilename, info, pTempSurf, XB_D3DFMT_P8, pal);
		pTempSurf->Release();
		return;
	}

  // test linear format versus non-linear format
  // Linear format requires 64 pixel aligned width, whereas
  // Non-linear format requires power of 2 width and height
  bool useLinearFormat(false);
  UINT linearWidth = (info.Width + 0x3f) & ~0x3f;
  if (AllowLinear && linearWidth * info.Height < Width * Height)
    useLinearFormat = true;

	hr = pD3DDevice->CreateImageSurface(Width, Height, D3DFMT_A8R8G8B8, &pSrcSurf);
	CheckHR(hr);
  
	hr = D3DXLoadSurfaceFromFile(pSrcSurf, NULL, NULL, Filename, NULL, D3DX_FILTER_NONE, 0, NULL);
	CheckHR(hr);

  // create the linear version as well
	LPDIRECT3DSURFACE8 pLinearSrcSurf = NULL;
  if (useLinearFormat)
  {
	  hr = pD3DDevice->CreateImageSurface(linearWidth, info.Height, D3DFMT_A8R8G8B8, &pLinearSrcSurf);
	  CheckHR(hr);
	  hr = D3DXLoadSurfaceFromFile(pLinearSrcSurf, NULL, NULL, Filename, NULL, D3DX_FILTER_NONE, 0, NULL);
	  CheckHR(hr);
  }


	// special case for small files - all textures are alloced on page granularity so just output uncompressed
	// dxt is crap on small files anyway
	if (Width * Height <= 1024)
	{
    if (useLinearFormat)
    {
      // correct sizing amounts
	    UncompressedSize -= Width * Height * 4;
      UncompressedSize += linearWidth * info.Height * 4;
	    TotalDstPixels -= Width * Height;
      TotalDstPixels += linearWidth * info.Height;

	    Waste = 100.f * (float)(linearWidth * info.Height - info.Width * info.Height) / (float)(linearWidth * info.Height);
		  printf("LIN_A8R8G8B8 %4dx%-4d (%5.2f%% waste)\n", linearWidth, info.Height, Waste);
		  TRACE0(" Selected Format: LIN_A8R8G8B8\n");
      WriteXPR(OutFilename, info, pLinearSrcSurf, XB_D3DFMT_LIN_A8R8G8B8, NULL);
    }
    else
    {
		  printf("A8R8G8B8 %4dx%-4d (%5.2f%% waste)\n", Width, Height, Waste);
		  TRACE0(" Selected Format: A8R8G8B8\n");
      WriteXPR(OutFilename, info, pSrcSurf, XB_D3DFMT_A8R8G8B8, NULL);
    }
		return;
	}

	FixTransparency(pSrcSurf);

	// Find the best format within specified tolerance
	double CMSE, AMSE[2];

	// DXT1 is the preferred format as it's smallest
	TRACE0(" Checking     DXT1: ");
	if (!GetFormatMSE(info, pSrcSurf, D3DFMT_DXT1, CMSE, AMSE[0]))
		return;
	TRACE2("CMSE=%05.2f, AMSE=%07.2f\n", CMSE, AMSE[0]);
	if (CMSE <= MaxMSE && AMSE[0] <= MaxMSE)
	{
		printf("DXT1     %4dx%-4d (%5.2f%% waste)\n", Width, Height, Waste);
		TRACE0(" Selected Format: DXT1\n");

		WriteXPR(OutFilename, info, pSrcSurf, XB_D3DFMT_DXT1, NULL);
		return;
	}

	// Use P8 is possible as it's lossless
	LPDIRECT3DSURFACE8 pTempSurf;
	DWORD pal[256];
	if (ConvertP8(pSrcSurf, pTempSurf, pal, info))
	{
		printf("P8       %4dx%-4d (%5.2f%% waste)\n", Width, Height, Waste);
		TRACE0(" Selected Format: P8\n");

		WriteXPR(OutFilename, info, pTempSurf, XB_D3DFMT_P8, pal);
		pTempSurf->Release();
		return;
	}

	// DXT3/5 are the same size so use whichever is better if good enough
	// CMSE will be equal for both
	TRACE0(" Checking     DXT3: ");
	if (!GetFormatMSE(info, pSrcSurf, D3DFMT_DXT3, CMSE, AMSE[0]))
		return;
	TRACE2("CMSE=%05.2f, AMSE=%07.2f\n", CMSE, AMSE[0]);

	TRACE0(" Checking     DXT5: ");
	if (!GetFormatMSE(info, pSrcSurf, D3DFMT_DXT5, CMSE, AMSE[1]))
		return;
	TRACE2("CMSE=%05.2f, AMSE=%07.2f\n", CMSE, AMSE[1]);

	if (AMSE[0] <= AMSE[1])
	{
		if (CMSE <= MaxMSE && AMSE[0] <= MaxMSE)
		{
			printf("DXT3     %4dx%-4d (%5.2f%% waste)\n", Width, Height, Waste);
			TRACE0(" Selected Format: DXT3\n");

			WriteXPR(OutFilename, info, pSrcSurf, XB_D3DFMT_DXT3, NULL);
			return;
		}
	}
	else
	{
		if (CMSE <= MaxMSE && AMSE[1] <= MaxMSE)
		{
			printf("DXT5     %4dx%-4d (%5.2f%% waste)\n", Width, Height, Waste);
			TRACE0(" Selected Format: DXT5\n");

			WriteXPR(OutFilename, info, pSrcSurf, XB_D3DFMT_DXT5, NULL);
			return;
		}
	}

	// No good compressed format so use uncompressed

	// A1R5G5B5 is worth a try I guess...
	TRACE0(" Checking A1R5G5B5: ");
	if (!GetFormatMSE(info, pSrcSurf, D3DFMT_A1R5G5B5, CMSE, AMSE[0]))
		return;
	TRACE2("CMSE=%05.2f, AMSE=%07.2f\n", CMSE, AMSE[0]);
	if (CMSE <= MaxMSE && AMSE[0] <= MaxMSE)
	{
		printf("A1R5G5B5 %4dx%-4d (%5.2f%% waste)\n", Width, Height, Waste);
		TRACE0(" Selected Format: A1R5G5B5\n");

		LPDIRECT3DSURFACE8 pTempSurf;
		hr = pD3DDevice->CreateImageSurface(Width, Height, D3DFMT_A1R5G5B5, &pTempSurf);
		CheckHR(hr);

		hr = D3DXLoadSurfaceFromSurface(pTempSurf, NULL, NULL, pSrcSurf, NULL, NULL, D3DX_FILTER_NONE, 0);
		CheckHR(hr);

		WriteXPR(OutFilename, info, pTempSurf, XB_D3DFMT_A1R5G5B5, NULL);

		pTempSurf->Release();
		return;
	}

	// Use A8R8G8B8
  if (useLinearFormat)
  {
    // correct sizing information
	  UncompressedSize -= Width * Height * 4;
    UncompressedSize += linearWidth * info.Height * 4;
	  TotalDstPixels -= Width * Height;
    TotalDstPixels += linearWidth * info.Height;
	  Waste = 100.f * (float)(linearWidth * info.Height - info.Width * info.Height) / (float)(linearWidth * info.Height);
		printf("LIN_A8R8G8B8 %4dx%-4d (%5.2f%% waste)\n", linearWidth, info.Height, Waste);
		TRACE0(" Selected Format: LIN_A8R8G8B8\n");
    WriteXPR(OutFilename, info, pLinearSrcSurf, XB_D3DFMT_LIN_A8R8G8B8, NULL);
  }
  else
  {
		printf("A8R8G8B8 %4dx%-4d (%5.2f%% waste)\n", Width, Height, Waste);
		TRACE0(" Selected Format: A8R8G8B8\n");
    WriteXPR(OutFilename, info, pSrcSurf, XB_D3DFMT_A8R8G8B8, NULL);
  }

	if (pSrcSurf)
		pSrcSurf->Release();
}

// only works for gifs or other 256-colour anims
void ConvertAnim(const char* Dir, const char* Filename, double MaxMSE)
{
	HRESULT hr;
	LPDIRECT3DSURFACE8 pSrcSurf = NULL;

	char OutFilename[52];
	if (Dir)
		_snprintf(OutFilename, 52, "%s\\%s", Dir, Filename);
	else
		_snprintf(OutFilename, 52, "%s", Filename);
	OutFilename[51] = 0;

	printf("%s: ", OutFilename);
	TRACE1("%s:\n", OutFilename);
	int n = strlen(OutFilename);
	if (n < 40)
		printf("%*c", 40-n, ' ');

	// Load up the file
	CAnimatedGifSet Anim;
	int nImages = Anim.LoadGIF(Filename);
	if (!nImages)
	{
		puts("ERROR: Unable to load gif (file corrupt?)");
		return;
	}
	if (nImages > 65535)
	{
		printf("ERROR: Too many frames in gif (%d > 65535)\n", nImages);
		return;
	}

	PrintAnimInfo(Anim);

	UINT Width = PadPow2(Anim.FrameWidth);
	UINT Height = PadPow2(Anim.FrameHeight);

	D3DXIMAGE_INFO info;
	info.Width = Anim.FrameWidth;
	info.Height = Anim.FrameHeight;
	info.MipLevels = 1;
	info.Depth = 0;
	info.ResourceType = D3DRTYPE_SURFACE;
	info.Format = D3DFMT_P8;
	info.ImageFileFormat = D3DXIFF_PNG;

	PALETTEENTRY pal[256];
	memcpy(pal, Anim.m_vecimg[0]->Palette, 256 * sizeof(PALETTEENTRY));
	for (int i = 0; i < 256; i++)
		pal[i].peFlags = 0xff; // alpha
	if (Anim.m_vecimg[0]->Transparency && Anim.m_vecimg[0]->Transparent >= 0)
		memset(&pal[Anim.m_vecimg[0]->Transparent], 0, sizeof(PALETTEENTRY));

	// setup xpr header
	WriteXPRHeader((DWORD*)pal, nImages);
	if (nImages > 1)
	{
		XPRFile.AnimInfo->RealSize = (info.Width & 0xffff) | ((info.Height & 0xffff) << 16);
		XPRFile.AnimInfo->nLoops = Anim.nLoops;
	}

	int nActualImages = 0;

	TotalSrcPixels += info.Width * info.Height * nImages;
	TotalDstPixels += Width * Height * nImages;
	float Waste = 100.f * (float)(Width * Height - info.Width * info.Height) / (float)(Width * Height);

	// alloc hash buffer
	BYTE (*HashBuf)[20] = new BYTE[nImages][20];

	for (int i = 0; i < nImages; ++i)
	{
		if (pSrcSurf)
			pSrcSurf->Release();
		pSrcSurf = NULL;

		printf("%3d%%\b\b\b\b", 100 * i / nImages);

		UncompressedSize += Width * Height;
		CAnimatedGif* pGif = Anim.m_vecimg[i];

		if (nImages > 1)
			XPRFile.Texture[i].RealSize = pGif->Delay;

		// generate sha1 hash
		SHA1((BYTE*)pGif->Raster, pGif->BytesPerRow * pGif->Height, HashBuf[i]);

		// duplicate scan
		int j;
		for (j = 0; j < i; ++j)
		{
			if (!memcmp(HashBuf[j], HashBuf[i], 20))
			{
				// duplicate image!
				TRACE2(" %03d: Duplicate of %03d\n", i, j);
				AppendXPRImageLink(j);
				break;
			}
		}
		if (j < i)
			continue;

		++nActualImages;

		// DXT1 for P8s if lossless
		hr = pD3DDevice->CreateImageSurface(Width, Height, D3DFMT_A8R8G8B8, &pSrcSurf);
		CheckHR(hr);

		D3DLOCKED_RECT slr;
		hr = pSrcSurf->LockRect(&slr, NULL, D3DLOCK_READONLY);
		CheckHR(hr);

		BYTE* src = (BYTE*)pGif->Raster;
		DWORD* dst = (DWORD*)slr.pBits;
		DWORD* dwPal = (DWORD*)pal;
		for (int y = 0; y < pGif->Height; ++y)
		{
			for (UINT x = 0; x < Width; ++x)
				*dst++ = dwPal[*src++];
		}
		memset(dst, 0, (Height - pGif->Height) * slr.Pitch);

		pSrcSurf->UnlockRect();

		double CMSE, AMSE;
		TRACE1(" %03d: Checking DXT1: ", i);
		if (!GetFormatMSE(info, pSrcSurf, D3DFMT_DXT1, CMSE, AMSE))
			return;
		TRACE2("CMSE=%05.2f, AMSE=%07.2f\n", CMSE, AMSE);
		
		if (CMSE <= 1e-6 && AMSE <= 1e-6)
		{
			TRACE1(" %03d: Selected Format: DXT1\n", i);
			AppendXPRImage(info, pSrcSurf, XB_D3DFMT_DXT1);
		}
		else
		{	
			pSrcSurf->Release();

			hr = pD3DDevice->CreateImageSurface(Width, Height, D3DFMT_P8, &pSrcSurf);
			CheckHR(hr);

			hr = pSrcSurf->LockRect(&slr, NULL, D3DLOCK_READONLY);
			CheckHR(hr);

			memcpy((BYTE*)slr.pBits, pGif->Raster, pGif->Height * slr.Pitch);
			memset((BYTE*)slr.pBits + pGif->Height * slr.Pitch, pGif->Transparent, (Height - pGif->Height) * slr.Pitch);

			pSrcSurf->UnlockRect();

			TRACE1(" %03d: Selected Format: P8\n", i);
			AppendXPRImage(info, pSrcSurf, XB_D3DFMT_P8);
		}
	}

	delete [] HashBuf;
	
	printf("(%5df) %4dx%-4d (%5.2f%% waste)\n", nActualImages, Width, Height, Waste);

	CommitXPR(OutFilename);
	if (pSrcSurf)
		pSrcSurf->Release();
}

// returns true for png, bmp, tga, jpg and dds files, otherwise returns false
bool IsGraphicsFile(char *strFileName)
{
	int n = (int)strlen(strFileName);
	if (n<4)
		return false;
	if (strnicmp(&strFileName[n-4], ".png", 4) &&
		strnicmp(&strFileName[n-4], ".bmp", 4) &&
		strnicmp(&strFileName[n-4], ".tga", 4) &&
		strnicmp(&strFileName[n-4], ".jpg", 4) &&
		strnicmp(&strFileName[n-4], ".dds", 4))
		return false;
	return true;
}

// returns true if it's a ".gif" otherwise returns false
bool IsGraphicsAnim(char *strFileName)
{
	int n = (int)strlen(strFileName);
	if (n<4 || strnicmp(&strFileName[n-4], ".gif", 4))
		return false;
	return true;
}

void ConvertDirectory(const char *strFullPath, char *strRelativePath, double MaxMSE)
{
	// Set our current directory
	if (strFullPath)
		SetCurrentDirectory(strFullPath);
	// Get our current pathname
	char strCurrentPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, strCurrentPath);

	// Now run through our directory, and find all subdirs
	WIN32_FIND_DATAA FindData;
	char Filename[4] = "*.*";
	HANDLE hFind = FindFirstFile(Filename, &FindData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			// Check if we've found a subdir
			if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// ignore any directory starting with a '.'
				if (strnicmp(FindData.cFileName,".",1))
				{
					char strNewFullPath[MAX_PATH];
					char strNewRelativePath[MAX_PATH];
					sprintf(strNewFullPath, "%s\\%s", strCurrentPath, FindData.cFileName);
					if (strRelativePath)
						sprintf(strNewRelativePath, "%s\\%s", strRelativePath, FindData.cFileName);
					else
						sprintf(strNewRelativePath, "%s", FindData.cFileName);
					// Recurse into the new directory
					ConvertDirectory(strNewFullPath, strNewRelativePath, MaxMSE);
					// Restore our current directory
					SetCurrentDirectory(strCurrentPath);
				}
			}
			else
			{	// just files - check if it's an allowed graphics file
				if (IsGraphicsFile(FindData.cFileName))
				{	// got a graphics file
					ConvertFile(strRelativePath,FindData.cFileName, MaxMSE);
				}
				if (IsGraphicsAnim(FindData.cFileName))
				{	// got a .gif anim
					ConvertAnim(strRelativePath,FindData.cFileName, MaxMSE);
				}
			}
		}
		while (FindNextFile(hFind, &FindData));
		FindClose(hFind);
	}
}

void Usage()
{
	puts("Usage:");
	puts("  -help            Show this screen.");
	puts("  -input <dir>     Input directory. Default: current dir");
	puts("  -output <dir>    Output directory/filename. Default: Textures.xpr");
	puts("  -quality <qual>  Quality setting (min, low, normal, high, max). Default: normal");
  puts("  -noprotect       XPR contents viewable at full quality in skin editor");
  puts("  -onlyswizzled    Only allow swizzled textures (faster rendering, larger memory use) rather than linear textures");
}

int main(int argc, char* argv[])
{
  int NoProtect = 0;
  AllowLinear = true;
  double MaxMSE = 4.0;

	CmdLineArgs args;

	if (args.size() == 1)
	{
		Usage();
		return 1;
	}

	const char* InputDir = NULL;
	const char* OutputFilename = "Textures.xpr";

	for (unsigned int i = 1; i < args.size(); ++i)
	{
		if (!stricmp(args[i], "-help") || !stricmp(args[i], "-h") || !stricmp(args[i], "-?"))
		{
			Usage();
			return 1;
		}
		else if (!stricmp(args[i], "-input") || !stricmp(args[i], "-i"))
		{
			InputDir = args[++i];
		}
		else if (!stricmp(args[i], "-output") || !stricmp(args[i], "-o"))
		{
			OutputFilename = args[++i];
		}
    else if (!stricmp(args[i], "-noprotect") || !stricmp(args[i], "-p"))
    {
      NoProtect = 1;
    }
    else if (!stricmp(args[i], "-onlyswizzled") || !stricmp(args[i], "-s"))
    {
      AllowLinear = false;
    }
    else if (!stricmp(args[i], "-quality") || !stricmp(args[i], "-q"))
		{
			++i;
			if (!stricmp(args[i], "min"))
			{
				MaxMSE = DBL_MAX;
			}
			else if (!stricmp(args[i], "low"))
			{
				MaxMSE = 20.0;
			}
			else if (!stricmp(args[i], "normal"))
			{
				MaxMSE = 4.0;
			}
			else if (!stricmp(args[i], "high"))
			{
				MaxMSE = 1.5;
			}
			else if (!stricmp(args[i], "max"))
			{
				MaxMSE = 0.0;
			}
			else
			{
				printf("Unrecognised quality setting: %s\n", args[i]);
			}
		}
		else
		{
			printf("Unrecognised command line flag: %s\n", args[i]);
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
	presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
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

	char HomeDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, HomeDir);

	XPRFile.OutputBuf = (char*)VirtualAlloc(0, 64 * 1024 * 1024, MEM_RESERVE, PAGE_NOACCESS);
	if (!XPRFile.OutputBuf)
	{
		printf("Memory allocation failure: %08x\n", GetLastError());
		pD3DDevice->Release();
		pD3D->Release();
		return 1;
	}

	Bundler.StartBundle();

	// Scan the input directory (or current dir if false) for media files
	ConvertDirectory(InputDir, NULL, MaxMSE);

	VirtualFree(XPRFile.OutputBuf, 0, MEM_RELEASE);

	pD3DDevice->Release();
	pD3D->Release();

	SetCurrentDirectory(HomeDir);
	DWORD attr = GetFileAttributes(OutputFilename);
	if (attr != -1 && (attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		SetCurrentDirectory(OutputFilename);
		OutputFilename = "Textures.xpr";
	}

	printf("\nWriting bundle: %s", OutputFilename);
  int BundleSize = Bundler.WriteBundle(OutputFilename, NoProtect);
	if (BundleSize == -1)
	{
		printf("\nERROR: %08x\n", GetLastError());
		return 1;
	}

	printf("\nUncompressed texture size: %6dkB\nCompressed texture size: %8dkB\nBundle size:             %8dkB\n\nWasted Pixels: %u/%u (%5.2f%%)\n",
		(UncompressedSize + 1023) / 1024, (((CompressedSize + 1023) / 1024) + 3) & ~3, (BundleSize + 1023) / 1024,
		TotalDstPixels - TotalSrcPixels, TotalDstPixels, 100.f * (float)(TotalDstPixels - TotalSrcPixels) / (float)TotalDstPixels);

	return 0;
}

