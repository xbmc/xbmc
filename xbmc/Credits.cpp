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
	CreditLine_t Credits[] = 
{

	// Intro
	{  50,  25,    400,  10000,  100,  800, EFF_IN_FLASH  |EFF_OUT_FADE   ,   78, L"XBOX" },
	{  50,  45,      0,  10000,  100,  800, EFF_IN_FLASH  |EFF_OUT_FADE   ,   78, L"MEDIA" },
	{  50,  65,      0,  10000,  100,  800, EFF_IN_FLASH  |EFF_OUT_FADE   ,   78, L"CENTER" },

	// Lead dev
	{  50,  22,  11500,   5000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Lead Developer" },
	{  50,  35,    500,   4500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Frodo" },

	// Devs
	{  50,  22,   6500,  16500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Developers" },
	{  50,  35,    500,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Butcher" },
	{  50,  45,    100,   7400,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Forza" },
	{  50,  55,    100,   7300,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Bobbin007" },
	{  50,  65,    100,   7200,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"JCMarshall" },

	{  50,  35,   8200,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"jwnmulder" },
	{  50,  45,      0,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"monkeyhappy" },
	{  50,  55,      0,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Tslayer" },

	// Project Management
	{  50,  22,   9500,   5000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Project Management" },
	{  50,  35,    500,   4500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Gamester17" },

	// Testers
	{  50,  22,   6500,  16500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Testers" },
	{  50,  35,    500,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"DDay" },
	{  50,  45,    100,   7400,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"[XC]D-Ice" },
	{  50,  55,    100,   7300,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"MrMario64" },
	{  50,  65,    100,   7200,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Obstler" },
	{  50,  75,    100,   7100,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Pike" },

	{  50,  35,   8100,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Poing" },
	{  50,  45,      0,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"RUNTiME" },
	{  50,  55,      0,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Shadow_Mx" },
	{  50,  65,      0,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"xAD" },

	// Support
	{  50,  22,   9500,   5000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Tech Support" },
	{  50,  35,    500,   4500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Hullebulle" },

	// Notable patches
	{  50,  22,   6500,   7000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Patches" },
	{  50,  35,    500,   6500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Q-Silver" },
	{  50,  45,    100,   6400,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"WiSo" },

	// Stream server
	{  50,  22,   8500,   8000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Stream Server" },
	{  50,  35,    500,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"[XC]D-Ice" },
	{  50,  45,    100,   7400,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"PuhPuh" },
	{  50,  55,    100,   7300,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Pope-X" },

	// Website Hosts
	{  50,  22,   9500,   8000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Website Hosting" },
	{  50,  35,    500,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"HulleBulle" },
	{  50,  45,    100,   7400,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"MrX" },
	{  50,  55,    100,   7300,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Team-XBMC" },
	{  50,  65,    100,   7200,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"xAD (ASP Interface Site Upload)" },

	// Special Thanks
	{  50,  22,   9500,   8000,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Special Thanks to" },
	{  50,  35,    500,   7500,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"The Joker of Team Avalaunch" },
	{  50,  45,    100,   7400,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Team SmartXX" },
	{  50,  55,    100,   7300,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Team Xecuter" },
	{  50,  65,    100,   7300,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Team Ozxodus" },
	{  50,  75,    100,   7300,  800,  800, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"bestplayer.de" },

  // JOKE section;-)

  // joke section Frodo
	{  50,  22,   9500,   8000,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   36, L"Frodo" },
	{  50,  35,    500,   7500,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"Special thanks to my dear friends" },
	{  50,  45,    100,   7400,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"Gandalf, Legolas, Gimli , Aragorn, Boromir" },
	{  50,  55,    100,   7300,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"Merry, Pippin and offcourse Samwise" },

	// joke section mario64
	{  50,  22,   9500,   8000,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   36, L"Mario64" },
  {  50,  35,    500,   7500,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"Special thanks to:" },
	{  50,  45,    100,   7400,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"The coyote for never catching the road runner" },
	{  50,  55,    100,   7300,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"The pizza delivery guy" },
	{  50,  65,    100,   7300,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"Amstel & Heineken" },
	{  50,  75,    100,   7300,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"Rob Hubbard for his great sid tunes" },


  // joke section dday
	{  50,  22,   9500,   8000,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   36, L"DDay" },
  {  50,  35,    500,   7500,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"Special thanks to:" },
	{  50,  45,    100,   7400,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"Frodo for not kicking my skull" },
	{  50,  55,    100,   7300,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"god for pizza, beer and women" },
	{  50,  65,    100,   7300,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"Douglas Adams for making me smile each summer" },
	{  50,  75,    100,   7300,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"hitchhikers guide to the galaxy" },

    // joke section [XC]D-Ice
	{  50,  22,   9500,   8000,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   36, L"[XC]D-Ice" },
  {  50,  35,    500,   7500,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"Special thanks to:" },
	{  50,  45,    100,   7400,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"Heineken for getting me VERY drunk when coding" },
	{  50,  55,    100,   7300,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"and Pepsi for curing my hangovers" },
	{  50,  65,    100,   7300,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"my dad for paying for the ADSL line" },
	{  50,  75,    100,   7300,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"my school for giving me a laptop for free" },

  // joke section xAD
	{  50,  22,   9500,   8000,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   36, L"xAD" },
  {  50,  35,    500,   7500,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"Special thanks to:" },
	{  50,  45,    100,   7400,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"Butcher for making me happy (Sid player)" },
	{  50,  55,    100,   7300,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"My wife for moral support" },
	{  50,  65,    100,   7300,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"My friend 'Fox/nIGHTFALL' for the ASP help" },
	{  50,  75,    100,   7300,  800,  800, EFF_IN_ASCEND |EFF_OUT_FADE   ,   20, L"My little computer room" },

  // blank line as a pause
	{   0,   0,  11500,      0,    0,    0, EFF_IN_APPEAR |EFF_OUT_APPEAR ,   20, L"" },
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

	static bool FixedCredits = false;

	DWORD Time = 0;
	map<int, CGUIFont*> Fonts;
	for (int i = 0; i < NUM_CREDITS; ++i)
	{
		// map fonts
		if (Fonts.find(Credits[i].Font) == Fonts.end())
		{
			CStdString strFont;
			strFont.Fmt("creditsfont%d", Credits[i].Font);
			// first try loading it
			CStdString strFilename;
			strFilename.Fmt("credits-font%d.xpr", Credits[i].Font);
			CGUIFont* pFont = g_fontManager.Load(strFont, strFilename);
			if (!pFont)
			{
				// Find closest skin font size
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
			}
			Fonts.insert(std::pair<int, CGUIFont*>(Credits[i].Font, pFont));
		}

		// validate credits
		if (!FixedCredits)
		{
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
	}

	FixedCredits = true;

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
			if (g_application.m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_B] ||
				g_application.m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_BACK ||
				g_application.m_DefaultIR_Remote.wButtons == XINPUT_IR_REMOTE_BACK ||
				g_application.m_DefaultIR_Remote.wButtons == XINPUT_IR_REMOTE_MENU)
			{
				// Unload fonts
				for (map<int, CGUIFont*>::iterator iFont = Fonts.begin(); iFont != Fonts.end(); ++iFont)
				{
					CStdString strFont;
					strFont.Fmt("creditsfont%d", iFont->first);
					g_fontManager.Unload(strFont);
				}

				// clear screen and exit to gui
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
