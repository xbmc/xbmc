#include <list>
#include <map>
#include "GraphicContext.h"
#include "GUIFontManager.h"
#include "credits.h"
#include "Application.h"

// Transition effects for text, must specific exactly one in and one out effect
enum CRED_EFFECTS
{
	EFF_IN_APPEAR  = 0x0,     // appear on the screen instantly
	EFF_IN_FADE    = 0x1,     // fade in over time
	EFF_IN_FLASH   = 0x2,     // flash the screen white over time and appear (short dur recommended)
	EFF_IN_ASCEND  = 0x3,     // ascend from the bottom of the screen
	EFF_IN_DESCEND = 0x4,     // descend from the top of the screen
	EFF_IN_LEFT    = 0x5,     // slide in from the left
	EFF_IN_RIGHT   = 0x6,     // slide in from the right

	EFF_OUT_APPEAR  = 0x00,     // disappear from the screen instantly
	EFF_OUT_FADE    = 0x10,     // fade out over time
	EFF_OUT_FLASH   = 0x20,     // flash the screen white over time and disappear (short dur recommended)
	EFF_OUT_ASCEND  = 0x30,     // ascend to the top of the screen
	EFF_OUT_DESCEND = 0x40,     // descend to the bottom of the screen
	EFF_OUT_LEFT    = 0x50,     // slide out to the left
	EFF_OUT_RIGHT   = 0x60,     // slide out to the right
};
#define EFF_IN_MASK (0xf)
#define EFF_OUT_MASK (0xf0)

// One line of the credits
struct CreditLine_t
{
	short x, y;           // Position
	DWORD Time;           // Time to start transition in (in ms)
	DWORD Duration;       // Duration of display (excluding transitions) (in ms)
	WORD InDuration;      // In transition duration (in ms)
	WORD OutDuration;     // Out transition duration (in ms)
	BYTE Effects;         // Effects flags
	BYTE Font;            // Font size
	const wchar_t* Text;  // The text to display
};

// The credits - these should be sorted by Time
// x, y are percentage distance across the screen of the center point
// Time is delay since last credit
//    x,   y,   Time,    Dur,  InD, OutD,                        Effects, Font, Text
CreditLine_t Credits[] = {
	// Intro
	{  50,  30,    200,  10000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"XBOX" },
	{  50,  45,    400,  10000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"MEDIA" },
	{  50,  60,    400,  10000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"CENTER" },

	// Devs
	{  50,  25,  12000,  16500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Developers" },
	{  50,  35,    500,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Bobbin007" },
	{  50,  43,    100,   7400,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Butcher" },
	{  50,  51,    100,   7300,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Forza" },
	{  50,  59,    100,   7200,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Frodo" },
	
	{  50,  35,   8200,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"JCMarshall" },
	{  50,  43,      0,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"jwnmulder" },
	{  50,  51,      0,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"monkeyhappy" },
	{  50,  59,      0,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Tslayer" },

	// Project Management
	{  50,  25,   9500,   5000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Project Management" },
	{  50,  35,    500,   4500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Gamester17" },

	// Testers
	{  50,  25,   6500,  16500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Testers" },
	{  50,  35,    500,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"DDay" },
	{  50,  43,    100,   7400,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"[XC]D-Ice" },
	{  50,  51,    100,   7300,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"MrMario64" },
	{  50,  59,    100,   7200,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Obstler" },
	{  50,  67,    100,   7100,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Pike" },

	{  50,  35,   8100,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Poing" },
	{  50,  43,      0,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"RUNTiME" },
	{  50,  51,      0,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Shadow_Mx" },
	{  50,  59,      0,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"xAD" },
	
	// Support
	{  50,  25,   9500,   5000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Tech Support" },
	{  50,  35,    500,   4500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Hullebulle" },

	// Notable patches
	{  50,  25,   6500,   7000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Patches" },
	{  50,  35,    500,   6500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Q-Silver" },
	{  50,  43,    100,   6400,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"WiSo" },

	// Stream server
	{  50,  25,   8500,   8000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Stream Server" },
	{  50,  35,    500,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"[XC]D-Ice" },
	{  50,  43,    100,   7400,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"PuhPuh" },
	{  50,  51,    100,   7300,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Pope-X" },

	// Website Hosts
	{  50,  25,   9500,   8000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Wesite Hosting" },
	{  50,  35,    500,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"HulleBulle" },
	{  50,  43,    100,   7400,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"MrX" },
	{  50,  51,    100,   7300,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Team-XBMC" },
	{  50,  59,    100,   7200,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"xAD" },
	
	// Special Thanks
	{  50,  25,   9500,   8000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Special Thanks to" },
	{  50,  35,    500,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"The Joker of Team Avalaunch" },
	{  50,  43,    100,   7400,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Team SmartXX" },
	{  50,  51,    100,   7300,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   14, L"Team Xecuter" },

	// blank line as a pause
	{   0,   0,  11500,      0,    0,    0, EFF_IN_APPEAR |EFF_OUT_APPEAR ,   14, L"" },
};

#define NUM_CREDITS (sizeof(Credits) / sizeof(Credits[0]))

void RunCredits()
{
	using std::map;
	using std::list;

	g_graphicsContext.Lock(); // exclusive access
	LPDIRECT3DDEVICE8 pD3DDevice = g_graphicsContext.Get3DDevice();
	
	// Fade down display
	D3DGAMMARAMP StartRamp, Ramp;
	pD3DDevice->GetGammaRamp(&StartRamp);
	for (int i = 49; i; --i)
	{
		for (int j = 0; j < 256; ++j)
		{
			Ramp.blue[j] = StartRamp.blue[j] * i / 50;
			Ramp.green[j] = StartRamp.green[j] * i / 50;
			Ramp.red[j] = StartRamp.red[j] * i / 50;
		}
		pD3DDevice->BlockUntilVerticalBlank();
		pD3DDevice->SetGammaRamp(D3DSGR_IMMEDIATE, &Ramp);
	}
	pD3DDevice->Clear(0, 0, D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
	pD3DDevice->Present(0, 0, 0, 0);

	DWORD Time = 0;
	map<int, CGUIFont*> Fonts;
	for (int i = 0; i < NUM_CREDITS; ++i)
	{
		// map fonts
		if (Fonts.find(Credits[i].Font) == Fonts.end())
		{
			CStdString strFont;
			CGUIFont* pFont = NULL;
			// Find closest font size
			for (int j = 0; j < Credits[i].Font; ++j)
			{
				strFont.Fmt("font%d", Credits[i].Font + j);
				pFont = g_fontManager.GetFont(strFont);
				if (pFont)
					break;
				if (j)
				{
					strFont.Fmt("font%d", Credits[i].Font - j);
					pFont = g_fontManager.GetFont(strFont);
					if (pFont)
						break;
				}
			}
			if (!pFont)
				pFont = g_fontManager.GetFont("font13"); // should have this!
			Fonts.insert(std::pair<int, CGUIFont*>(Credits[i].Font, pFont));
		}

		// validate credits
		if ((Credits[i].Effects & EFF_IN_MASK) == EFF_IN_APPEAR)
		{
			Credits[i].Duration += Credits[i].InDuration;
			Credits[i].InDuration = 0;
		}
		if ((Credits[i].Effects & EFF_OUT_MASK) == EFF_OUT_APPEAR)
		{
			Credits[i].Duration += Credits[i].OutDuration;
			Credits[i].OutDuration = 0;
		}

		Credits[i].x = Credits[i].x * g_graphicsContext.GetWidth() / 100;
		Credits[i].y = Credits[i].y * g_graphicsContext.GetHeight() / 100;

		Credits[i].Time += Time;
		Time = Credits[i].Time;
	}

	// music?

	// Start credits loop
	for (;;)
	{
		// restore gamma
		pD3DDevice->SetGammaRamp(D3DSGR_IMMEDIATE, &StartRamp);

		DWORD StartTime = timeGetTime();

		list<CreditLine_t*> ActiveList;
		int NextCredit = 0;

		// Do render loop
		while (NextCredit < NUM_CREDITS || !ActiveList.empty())
		{
			Time = timeGetTime() - StartTime;

			pD3DDevice->Clear(0, 0, D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

			// Activate new credits
			while (NextCredit < NUM_CREDITS && Credits[NextCredit].Time <= Time)
			{
				ActiveList.push_back(&Credits[NextCredit]);
				++NextCredit;
			}

			DWORD Gamma = 0;

			// Render active credits
			for (list<CreditLine_t*>::iterator iCredit = ActiveList.begin(); iCredit != ActiveList.end(); ++iCredit)
			{
				CreditLine_t* pCredit = *iCredit;

				// check for retirement
				while (Time > pCredit->Time + pCredit->InDuration + pCredit->OutDuration + pCredit->Duration)
				{
					iCredit = ActiveList.erase(iCredit);
					if (iCredit == ActiveList.end())
						break;
					pCredit = *iCredit;
				}
				if (iCredit == ActiveList.end())
					break;

				if (pCredit->Text[0])
				{
					// Render
					float x;
					float y;
					DWORD alpha;

					CGUIFont* pFont = Fonts.find(pCredit->Font)->second;

					if (Time < pCredit->Time + pCredit->InDuration)
					{
						#define INPROPORTION (Time - pCredit->Time) / pCredit->InDuration
						switch (pCredit->Effects & EFF_IN_MASK)
						{
						case EFF_IN_FADE:
							x = pCredit->x;
							y = pCredit->y;
							alpha = 255 * INPROPORTION;
							break;

						case EFF_IN_FLASH:
							{
								x = pCredit->x;
								y = pCredit->y;
								alpha = 0;
								DWORD g = 255 * INPROPORTION;
								if (g > Gamma)
									Gamma = g;
							}
							break;

						case EFF_IN_ASCEND:
							{
								x = pCredit->x;
								float d = (float)(g_graphicsContext.GetHeight() - pCredit->y);
								y = pCredit->y + d - (d * INPROPORTION);
								alpha = 255;
							}
							break;

						case EFF_IN_DESCEND:
							{
								x = pCredit->x;
								float w, h;
								pFont->GetTextExtent(pCredit->Text, wcslen(pCredit->Text), &w, &h);
								float d = pCredit->y + h;
								y = -h + (d * INPROPORTION);
								alpha = 255;
							}
							break;

						case EFF_IN_LEFT:
							break;

						case EFF_IN_RIGHT:
							break;
						}
					}
					else if (Time > pCredit->Time + pCredit->InDuration + pCredit->Duration)
					{
						#define OUTPROPORTION (Time - (pCredit->Time + pCredit->InDuration + pCredit->Duration)) / pCredit->OutDuration
						switch (pCredit->Effects & EFF_OUT_MASK)
						{
						case EFF_OUT_FADE:
							x = pCredit->x;
							y = pCredit->y;
							alpha = 255 - (255 * OUTPROPORTION);
							break;

						case EFF_OUT_FLASH:
							{
								x = pCredit->x;
								y = pCredit->y;
								alpha = 0;
								DWORD g = 255 * OUTPROPORTION;
								if (g > Gamma)
									Gamma = g;
							}
							break;

						case EFF_OUT_ASCEND:
							{
								x = pCredit->x;
								float w, h;
								pFont->GetTextExtent(pCredit->Text, wcslen(pCredit->Text), &w, &h);
								float d = pCredit->y + h;
								y = -h + (d - d * OUTPROPORTION);
								alpha = 255;
							}
							break;

						case EFF_OUT_DESCEND:
							{
								x = pCredit->x;
								float d = (float)(g_graphicsContext.GetHeight() - pCredit->y);
								y = pCredit->y + (d * OUTPROPORTION);
								alpha = 255;
							}
							break;

						case EFF_OUT_LEFT:
							break;

						case EFF_OUT_RIGHT:
							break;
						}
					}
					else // not transitioning
					{
						x = pCredit->x;
						y = pCredit->y;
						alpha = 0xff;
					}

					if (alpha)
						pFont->DrawText(x, y, 0xffffff | (alpha << 24), pCredit->Text, XBFONT_CENTER_X | XBFONT_CENTER_Y);
				}
			}

			if (Gamma)
			{
				for (int j = 0; j < 256; ++j)
				{
#define clamp(x) (x) > 255 ? 255 : (BYTE)((x) & 0xff)
					Ramp.blue[j] = clamp(StartRamp.blue[j] + Gamma);
					Ramp.green[j] = clamp(StartRamp.green[j] + Gamma);
					Ramp.red[j] = clamp(StartRamp.red[j] + Gamma);
				}
				pD3DDevice->SetGammaRamp(0, &Ramp);
			}
			else
				pD3DDevice->SetGammaRamp(0, &StartRamp);

			// check for keypress
			g_application.ReadInput();
			if (g_application.m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_A])
			{
				pD3DDevice->Clear(0, 0, D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
				pD3DDevice->SetGammaRamp(0, &StartRamp);
				pD3DDevice->Present(0, 0, 0, 0);
				g_graphicsContext.Unlock();
				return;
			}

			// present scene
			pD3DDevice->BlockUntilVerticalBlank();
			pD3DDevice->Present(0, 0, 0, 0);
		}
	}
}
