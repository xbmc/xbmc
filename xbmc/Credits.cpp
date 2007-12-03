/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#define _WCTYPE_INLINE_DEFINED
#include <process.h>
#include "GUIFontManager.h"
#include "Credits.h"
#include "GUITextLayout.h"
#include "Application.h"
#include "lib/mikxbox/mikmod.h"
#include "lib/mikxbox/mikxbox.h"
#include "credits_res.h"
#include "lib/liblzo/LZO1X.H"
#include "SkinInfo.h"
#include "Util.h"
#include "GUIFont.h"
#else
#include "GuiFontXPR.h"
#endif

// Transition effects for text, must specific exactly one in and one out effect
enum CRED_EFFECTS
{
  EFF_IN_APPEAR = 0x0,      // appear on the screen instantly
  EFF_IN_FADE = 0x1,      // fade in over time
  EFF_IN_FLASH = 0x2,      // flash the screen white over time and appear (short dur recommended)
  EFF_IN_ASCEND = 0x3,      // ascend from the bottom of the screen
  EFF_IN_DESCEND = 0x4,      // descend from the top of the screen
  EFF_IN_LEFT = 0x5,      // slide in from the left
  EFF_IN_RIGHT = 0x6,      // slide in from the right

  EFF_OUT_APPEAR = 0x00,      // disappear from the screen instantly
  EFF_OUT_FADE = 0x10,      // fade out over time
  EFF_OUT_FLASH = 0x20,      // flash the screen white over time and disappear (short dur recommended)
  EFF_OUT_ASCEND = 0x30,      // ascend to the top of the screen
  EFF_OUT_DESCEND = 0x40,      // descend to the bottom of the screen
  EFF_OUT_LEFT = 0x50,      // slide out to the left
  EFF_OUT_RIGHT = 0x60,      // slide out to the right
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

  // Internal stuff - don't need to init
  LPDIRECT3DTEXTURE8 pTex; // Prerendered font texture
  float TextWidth, TextHeight;
};

// Module sync notes
// one row is (125*sngspd)/(bpm*50) seconds
// the module used is spd 6 (unless noted) so 6/50 = 0.12 seconds per row
// bpm compensation done in code

// The credits - these should be sorted by Time
// x, y are percentage distance across the screen of the center point
// Time is delay since last credit
//    x,   y,   Time,    Dur,  InD, OutD,                        Effects, Font, Text
CreditLine_t Credits[] =
  {
    // Intro  fadein 32 rows, on 80 rows, fadeout 16 rows
    { 50, 30, 0, 9600, 3840, 1920, EFF_IN_FADE | EFF_OUT_FADE , 80, L"XBOX" },
    { 50, 50, 0, 9600, 3840, 1920, EFF_IN_FADE | EFF_OUT_FADE , 80, L"MEDIA" },
    { 50, 70, 0, 9600, 3840, 1920, EFF_IN_FADE | EFF_OUT_FADE , 80, L"CENTER" },

    // Lead dev (32 beats)
    { 50, 25, 15360, 13200, 240, 1920, EFF_IN_FADE | EFF_OUT_FADE , 42, L"Project Founders" },
    { 50, 40, 720, 12480, 240, 1920, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Frodo (yamp, xbmc)" },
    { 50, 50, 0, 12480, 240, 1920, EFF_IN_FADE | EFF_OUT_FADE , 24, L"RUNTiME (xbplayer)" },
    { 50, 60, 0, 12480, 240, 1920, EFF_IN_FADE | EFF_OUT_FADE , 24, L"d7o3g4q (xbmp)" },
    { 50, 70, 0, 12480, 240, 1920, EFF_IN_FADE | EFF_OUT_FADE , 24, L" " },
    { 50, 80, 0, 12480, 240, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"We wish you well with your future projects" },

    // Devs  (16 beats per group, 32 beats total
    { 50, 20, 14640, 13380, 60, 1920, EFF_IN_FLASH | EFF_OUT_FADE , 42, L"Developers" },
    { 50, 32, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 24, L"J Marshall" },
    { 50, 42, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 24, L"Darkie" },
    { 50, 52, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 24, L"elupus" },
    { 50, 62, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 24, L"Bobbin007" },
    { 50, 72, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 24, L"spiff" },
    { 50, 82, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 24, L"chadoe" },
    //  crossfade 3.5, on 45, fadeout 16
    { 50, 32, 7620, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Kraqh3d" },
    { 50, 42, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 24, L"GeminiServer" },
    { 50, 52, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Nad" },
    { 50, 62, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 24, L"MrC" },
    { 50, 72, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 24, L"c0diq" },
    { 50, 82, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Ysbox" },

    // Project management (16 beats)
    { 50, 30, 7740, 6360, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 42, L"Project Managers" },
    { 50, 45, 0, 6360, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 24, L"Gamester17" },
    { 50, 55, 0, 6360, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 24, L"Pike" },

    // Tech support (8 beats per group - 16 total)
    { 50, 25, 6960+420, 7200, 420, 420, EFF_IN_FADE | EFF_OUT_FADE , 42, L"Technical Support" },
    { 50, 40, 0, 3840, 420, 420, EFF_IN_FADE | EFF_OUT_FADE , 24, L"DDay (a.k.a. JayDee on Xbox-Scene.com)" },
    { 50, 50, 0, 3840, 420, 420, EFF_IN_FADE | EFF_OUT_FADE , 24, L"AlTheKill (IRC)" },
    { 50, 60, 0, 3840, 420, 420, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Cocoliso (IRC)" },
    { 50, 70, 0, 3840, 420, 420, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Jezz_X (IRC)" },

    { 50, 40, 3840, 3360, 420, 420, EFF_IN_FADE | EFF_OUT_FADE , 24, L"MattAAron (IRC)" },
    { 50, 50, 0, 3360, 420, 420, EFF_IN_FADE | EFF_OUT_FADE , 24, L"No1CaNTeL (IRC)" },
    { 50, 60, 0, 3360, 420, 420, EFF_IN_FADE | EFF_OUT_FADE , 24, L"xLoial (IRC)" },

    // Testers (8 beats per group - 24 total + 8 beats blank)
    { 50, 25, 4200, 7560 + 7560/2, 60, 840, EFF_IN_FLASH | EFF_OUT_FADE , 42, L"Testers" },
    { 50, 40, 0, 7560/2, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 24, L"Caldor" },
    { 50, 50, 0, 7560/2, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 24, L"C-Quel" },
    { 50, 60, 0, 7560/2, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 24, L"Donno" },

    { 50, 40, 7560/2, 7560/2, 60, 420, EFF_IN_FADE | EFF_OUT_FADE , 24, L"ModHack" },
    { 50, 50, 0, 7560/2, 60, 420, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Nuka1195" },
    { 50, 60, 0, 7560/2, 60, 420, EFF_IN_FADE | EFF_OUT_FADE , 24, L"sCAPe" },

    { 50, 40, 7560/2, 7560/2, 60, 840, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Smokehead" },
    { 50, 50, 0, 7560/2, 60, 840, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Sollie" },
    { 50, 60, 0, 7560/2, 60, 840, EFF_IN_FADE | EFF_OUT_FADE , 24, L"TeknoJuce" },

    // Note the pause of 8 beats here

    // Visualisations (16 beats)
    { 50, 22, 7680, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 42, L"Visualisations" },
    { 50, 35, 0, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Dinomight" },
    { 50, 45, 0, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"MrC" },
    { 50, 55, 0, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"mynci" },
    { 50, 65, 0, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"nmrs" },
    { 50, 75, 0, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"questor (a.k.a. fused)" },

    // Screensavers (16 beats)
    { 50, 25, 7680, 6400, 360, 660, EFF_IN_FADE | EFF_OUT_FADE , 42, L"Screensavers" },
    { 50, 40, 0, 6400, 360, 660, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Asteron" },	
    { 50, 50, 0, 6400, 360, 660, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Jme" },
    { 50, 60, 0, 6400, 360, 660, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Siw" },
    { 50, 70, 0, 6400, 360, 660, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Sylfan" },

    // Stream Servers (16 beats)
    { 50, 25, 7680, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 42, L"Stream Servers, Clients" },
    { 50, 40, 0, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"c0diq" },
    { 50, 50, 0, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"puh-puh" },
    { 50, 60, 0, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"[XC]D-Ice" },

    // Patches (16 beats)
    { 50, 22, 7680, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 42, L"Great Patch Submitters" },
    { 50, 35, 0, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Lossol93" },
    { 50, 45, 0, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"markeen" },
    { 50, 55, 0, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"soepy" },
    { 50, 65, 0, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"stgcolin" },
    { 50, 75, 0, 6400, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"WiSO" },

    // Translators (8 beats per group, 32 total)
    { 50, 22, 7680, 7320+7320, 360, 960, EFF_IN_FADE | EFF_OUT_FADE , 42, L"Translators" },
    { 50, 35, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"gamepc (Chinese (Simple))" },
    { 50, 45, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"omenpica (Chinese (Traditional))" },
    { 50, 55, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"nightspirit (Dutch)" },
    { 50, 65, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"console-ombouw (Dutch)" },
    { 50, 75, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"tijmengozer (Dutch)" },
    { 50, 85, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"mrnice (Dutch)" },

    { 50, 35, 7320/2, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Modhack (French)" },
    { 50, 45, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Bobbin007 (German)" },
    { 50, 55, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"sCAPe (German)" },
    { 50, 65, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Deezle (German)" },
    { 50, 75, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"ceomr (German (Austrian))" },
    { 50, 85, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"vgs (Hebrew)" },

    { 50, 35, 7320/2, 7320/2, 360, 960, EFF_IN_FADE | EFF_OUT_FADE , 24, L"yuvalt (Hebrew)" },
    { 50, 45, 0, 7320/2, 360, 960, EFF_IN_FADE | EFF_OUT_FADE , 24, L"ookladek (Hebrew)" },
    { 50, 55, 0, 7320/2, 360, 960, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Team XboxKlub (Hungarian)" },
    { 50, 65, 0, 7320/2, 360, 960, EFF_IN_FADE | EFF_OUT_FADE , 24, L"djoens (Indonesian)" },
    { 50, 75, 0, 7320/2, 360, 960, EFF_IN_FADE | EFF_OUT_FADE , 24, L"napek (Polish)" },
    { 50, 85, 0, 7320/2, 360, 960, EFF_IN_FADE | EFF_OUT_FADE , 24, L"smuto (Polish)" },

    { 50, 35, 7320/2, 7320/2, 360, 960, EFF_IN_FADE | EFF_OUT_FADE , 24, L"mvbm (Portuguese (Brazil))" },
    { 50, 45, 0, 7320/2, 360, 960, EFF_IN_FADE | EFF_OUT_FADE , 24, L"que_ (Russian)" },
    { 50, 55, 0, 7320/2, 360, 960, EFF_IN_FADE | EFF_OUT_FADE , 24, L"xsiluro (Slovenian)" },
    { 50, 65, 0, 7320/2, 360, 960, EFF_IN_FADE | EFF_OUT_FADE , 24, L"jose_t (Spanish)" },
    { 50, 75, 0, 7320/2, 360, 960, EFF_IN_FADE | EFF_OUT_FADE , 24, L"blittan (Swedish)" },

    // note pause here of 8 beats as more translators pop up

    // Skin credits (16 beats)
    { 50, 22, 7980, 7320, 60, 360, EFF_IN_FLASH | EFF_OUT_FADE , 42, L"$SKINTITLE" },
    { 50, 35, 0, 7320, 60, 360, EFF_IN_FLASH | EFF_OUT_FADE , 20, NULL },  // skin names go in these 5
    { 50, 45, 0, 7320, 60, 360, EFF_IN_FLASH | EFF_OUT_FADE , 20, NULL },
    { 50, 55, 0, 7320, 60, 360, EFF_IN_FLASH | EFF_OUT_FADE , 20, NULL },
    { 50, 65, 0, 7320, 60, 360, EFF_IN_FLASH | EFF_OUT_FADE , 20, NULL },
    { 50, 75, 0, 7320, 60, 360, EFF_IN_FLASH | EFF_OUT_FADE , 20, NULL },

    // Online manual (16 beats)
    { 50, 22, 7680, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 42, L"Online Manual (WIKI)" },
    { 50, 35, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Dankula" },
    { 50, 45, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"nimbles" },
    { 50, 55, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Sig Olafsson" },
    { 50, 65, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"No Good" },
    { 50, 75, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Loto_Bak" },

    // Webhosting (16 beats)
    { 50, 22, 7680, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 42, L"Web Hosting" },
    { 50, 35, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Xbox-Scene" },
    { 50, 45, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"MrX" },
    { 50, 55, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"EnderW" },
    { 50, 65, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"SourceForge.net" },
    { 50, 75, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Hullebulle (a.k.a. Nuendo)" },
    { 50, 85, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"xAD (a.k.a. xantal)" },

    // Sponsors (16 beats)
    { 50, 22, 7680, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 42, L"Sponsors" },
    { 50, 40, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"None currently" },
    { 50, 50, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Contact us if you want to sponsor XBMC" },

    // Ex team members (8 beats per group, 32 total)
    { 50, 22, 7680, 7320+7320/2+360, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 42, L"Retired Team Members" },
    { 50, 35, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Butcher" },
    { 50, 45, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Chokemaniac" },
    { 50, 55, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Forza" },
    { 50, 65, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Guybrush" },
    { 50, 75, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Hullebulle (a.k.a. Nuendo)" },

    { 50, 35, 7320/2, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"monkeyhappy" },
    { 50, 45, 0, 7320/2,360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Obstler" },
    { 50, 55, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Poing" },
    { 50, 65, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Shadow_Mx" },

    { 50, 35, 7320/2+360, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"Tslayer" },
    { 50, 45, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"xAD (a.k.a. xantal)" },
    { 50, 55, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"yuvalt" },
    { 50, 65, 0, 7320/2, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 24, L"[XC]D-Ice" },

//    // empty block, can be used for more credits if needed
 //   { 0, 0, 7800, 15360, 0, 0, EFF_IN_APPEAR | EFF_OUT_APPEAR , 24, NULL },

    // All stuff after this just scrolls

    // Code credits
    { 50, 50, 7200, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Code Credits" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Adplug" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"BiosChecker" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"CxImage" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"D.U.M.B." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"dosfs" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Drempels" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"FFmpeg" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"FileZilla (FTP-Server)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"FreeType (freetype.org)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"FriBiDi" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"G-Force" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"GensAPU" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"GoAhead (Web-Server)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"in_cube" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"liba52" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libCDIO" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libcurl" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libdaap" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libdca/libdts (VideoLan.org)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libdvdcss" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libdvdnav/dvdread" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libfaad2 (AudioCoding.com)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libid3tag" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libLame" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libmad" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libmpcdec" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libmpeg2" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libOggVorbis" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libSpeex" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"libvorbis" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"LZO" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Matroska" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"MikMod" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Milkdrop" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"MPlayer" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"MXM" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"NoseFart" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Platinum (UPnP-client)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Python" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Samba/libsmb (SMB/CIFS-client)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Sidplay2" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"SNESAPU" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"SQLite" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"St-Sound Library" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"UnRAR (rarlab.com)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"xiph" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Xored Trainer Engine" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"XviD" },

    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"TEAM THANKS:" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Team-XBMC would like to send"},
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"a special thanks out to:" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Xbox-Scene.com" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Dankula (for the manual labour)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Bizzeh (wiki and forums setup)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"MrC (great visualizations)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"freakware.com (some SmartXx v3)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Modchip makers for LCD code" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"modplug (credits music)" },

    // JOKE section;-)

    // section gamester17
    { 50, 50, 8000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Gamester17" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Special thanks to all in" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Team-XBMC who stay active." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to Frodo, RUNTiME and d7o3g4g" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"for starting XBMP." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to all all new developers" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"who submit code patches." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to everyone who has ever" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"contributed to XBMC." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Remember, real men don't make backups" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"but they cry often!" },

    // section pike
    { 50, 50, 3600, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Pike" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"XBMC has come a long way since 1.1.0" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Enjoy our vision of pure excellence" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Special thanks to all in team" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"who remain active." },

    // section JMarshall
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"J Marshall" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to everyone in the XBMC team" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"for the awesome work" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"My beautiful wife Keren" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"for supporting me while I spend all my free" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"time coding and playing with my toys" },

    // section dday
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"DDay" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Special thanks to:" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Liza, for putting up with me." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Liza, for understanding that it" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"sometimes is normal to have five" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"xbox's hooked up at home." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Liza, for giving me Emil" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Pike, for doing a great job" },

    // section bobbin007
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Bobbin007" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Mr. Lucas for the Arts," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Pott's beer for there great plopp bottles," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"My girlfriend," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Lidl for selling the greatest" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"frozen salami pizza ever" },

    // section elupus
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"elupus" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanx to everybody in the XBMC team," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"with special thanx to frodo for giving me" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"cvs access and in that something else" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"to do instead of sleeping at nights." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanx a bunch!" },

    // section spiff
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"spiff" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to the world for beer!" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Appreciation to the team for the product" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"\"If you make something fool proof," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"a better fool will be invented.\"" },

    // section chadoe
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"chadoe" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to whoever invented the internet" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"thus making this kind of team work possible" },

    // section geminiserver
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"GeminiServer" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to everybody involved in this Project" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"To our little angel Helin J. " },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"without her crying in the nights, i'd never" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"have found time for this project!" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"To Andrew Huang (bunnie)," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"for his initial work to run unsigned code!" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Special Thanks to M$ for the XBOX!" },

    // section tslayer
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Tslayer" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Special thanks to everybody involved" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"with XBMC, especially Frodo." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Without this amazing program," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"I would never have bought an XBOX" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"and would never have had a chance" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"to become part of this amazing team." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks all!" },

    // section nad
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Nad" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"To my Mum who passed away April 2006 -" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"we miss you." },

    // section mrc
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"MrC" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to all of Team-XBMC for" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"their hard work. Thanks to Pike" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"for his cool chats and support." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"And thanks to Mumbles for his" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Vortex textures." },

    // section c0diq
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"c0diq" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks Bunnie, you're the man" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to K., M., & L." },

    // section Donno
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Donno" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks TeknoJuce for making" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"#xbmc worthwhile. Thanks to" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Tazta for intro to XBMC n Mods." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks Pepsi for the Max" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks MString for the XBOX" },

    // section C-Quel
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"C-Quel" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to Spiff for putting up with my" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"lame ass code skills!" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks MS for the superb piece of h/w," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"shame about your s/w." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks Gamester for reminding me" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"to buy some tissues..." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thank you God for my fingers or i'd be" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"stuck typing this!" },

    // section sollie
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"sollie" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to all XBMC-devs for creating" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"this great application!!" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Special thanks to my girl and son for" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"not throwing away my xbox" },

    // section modhack
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Modhack" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to my job for giving me" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"free time to play with XBMC" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to gueux.be community" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"for the french support" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks Mathias for your mighty skin!" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Big thanks to Alexsolex for the script MyCine" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to Thibaud, Greg (Mgt1275)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"and Mathieu (Sergent M@B)" },

    // section scape
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"sCAPe" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Special thanks to everyone in Team-XBMC" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"for creating the best Media-Center ever" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"and for being part of the team!" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to my wonderful wife for having" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"so much patience with me," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"while playing with XBMC." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Also many greetings to the German" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"XBOX and XBMC-Scene" },

    // section teknojuce
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"TeknoJuce" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to our great XBMC team/family" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"for the years of pure bliss!" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Special thanks to my Swede brother Pike," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"play me them ch0oNZ!" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"DOS4GW, Spiff, Nad, Elupus, Tslayer," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Kraqh3d, DDay, Chokemaniac, Darkie" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"my apprentice Donno and JMarshall" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L" - the gawd among men." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"Thanks to our entire userbase in #XBMC," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 22, L"<3 U's okplzthx!" },

    // can duplicate the lines below as many times as required for more credits
    // {  50,  50,   4000,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   36, L"" },
    // {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"" },

    // Leave this as is - it tells the music to fade
    { 0, 0, 10000, 5000, 0, 0, EFF_IN_APPEAR | EFF_OUT_APPEAR , 22, NULL },
  };


#define NUM_CREDITS (sizeof(Credits) / sizeof(Credits[0]))

unsigned __stdcall CreditsMusicThread(void* pParam);
static BOOL s_bStopPlaying;
static BOOL s_bFadeMusic;
static HANDLE s_hMusicStarted;

// Logo rendering stuff
static DWORD s_dwVertexDecl[] =
  {
    D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),        // position
    D3DVSD_REG( 1, D3DVSDT_NORMPACKED3 ),   // normal
    D3DVSD_REG( 2, D3DVSDT_FLOAT2 ),        // tex co-ords
    D3DVSD_REG( 3, D3DVSDT_NORMPACKED3 ),   // tangent
    D3DVSD_REG( 4, D3DVSDT_NORMPACKED3 ),   // binormal
    D3DVSD_END()
  };

static IDirect3DVertexBuffer8* pVBuffer;   // vertex and index buffers
static IDirect3DIndexBuffer8* pIBuffer;
static DWORD s_dwVShader, s_dwPShader; // shaders
static IDirect3DCubeTexture8* pSpecEnvMap;   // texture for specular environment map
static IDirect3DTexture8* pFrontTex;   // specular environment map front surface base
static IDirect3DTexture8* pNormalMap;   // the bump map
static DWORD NumFaces, NumVerts;
static D3DXMATRIX matWorld, matVP, matWVP; // world, view-proj, world-view-proj matrices
static D3DXVECTOR3 vEye;    // camera
static D3DXVECTOR4 ambient(0.1f, 0.1f, 0.1f, 0);
static D3DXVECTOR4 colour(1.0f, 1.0f, 1.0f, 0);
static char* ResourceHeader;
static void* ResourceData;
static int SkinOffset;

LPDIRECT3DTEXTURE8 CreateCreditsTexture(CGUIFont *font, const wchar_t *text);
void GetCreditsTextExtent(CGUIFont *font, const wchar_t *text, float &width, float &height);

static map<int, CGUIFont*> Fonts;

static HRESULT InitLogo()
{
  DWORD n;

  // Open XPR
  HANDLE hFile = CreateFile("q:\\credits\\credits.xpr", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if (hFile == INVALID_HANDLE_VALUE)
    return E_FAIL;

  // Get header
  XPR_HEADER XPRHeader;
  if (!ReadFile(hFile, &XPRHeader, sizeof(XPR_HEADER), &n, 0) || n < sizeof(XPR_HEADER))
  {
    CloseHandle(hFile);
    return E_FAIL;
  }

  if (XPRHeader.dwMagic != XPR_MAGIC_VALUE)
  {
    CloseHandle(hFile);
    return E_FAIL;
  }

  // Load header data (includes shaders)
  DWORD Size = XPRHeader.dwHeaderSize - sizeof(XPR_HEADER);
  ResourceHeader = (char*)malloc(Size);
  if (!ResourceHeader)
  {
    CloseHandle(hFile);
    return E_OUTOFMEMORY;
  }
  ZeroMemory(ResourceHeader, Size);
  if (!ReadFile(hFile, ResourceHeader, Size, &n, 0) || n < Size)
  {
    CloseHandle(hFile);
    return E_FAIL;
  }

  // create shaders (8 bytes of header)
  D3DDevice::CreateVertexShader(s_dwVertexDecl, (const DWORD*)(ResourceHeader + credits_VShader_OFFSET + 8), &s_dwVShader, 0);
  D3DDevice::CreatePixelShader((const D3DPIXELSHADERDEF*)(ResourceHeader + credits_PShader_OFFSET + 12), &s_dwPShader);

  // Get info
  DWORD* MeshInfo = (DWORD*)(ResourceHeader + credits_MeshInfo_OFFSET + 8);
  NumFaces = MeshInfo[0];
  NumVerts = MeshInfo[1];

  // load resource data
  Size = XPRHeader.dwTotalSize - XPRHeader.dwHeaderSize;
  DWORD* PackedData = (DWORD*)malloc(Size);
  if (!PackedData)
  {
    CloseHandle(hFile);
    return E_OUTOFMEMORY;
  }
  if (!ReadFile(hFile, PackedData, Size, &n, 0) || n < Size)
  {
    free(PackedData);
    CloseHandle(hFile);
    return E_FAIL;
  }
  CloseHandle(hFile);

  ResourceData = XPhysicalAlloc(*PackedData, MAXULONG_PTR, 128, PAGE_READWRITE);
  if (!ResourceData)
  {
    free(PackedData);
    return E_OUTOFMEMORY;
  }

  if (lzo_init() != LZO_E_OK)
  {
    free(PackedData);
    return E_FAIL;
  }

  // unpack resource data
  lzo_uint s = *PackedData;
  if (lzo1x_decompress((lzo_byte*)(PackedData + 1), Size - 4, (lzo_byte*)ResourceData, &s, NULL) != LZO_E_OK || s != *PackedData)
  {
    free(PackedData);
    return E_FAIL;
  }

  free(PackedData);

  // enable write combine here so the decompress is fast
  XPhysicalProtect(ResourceData, s, PAGE_READWRITE | PAGE_WRITECOMBINE);

  // Register resources
  pVBuffer = (LPDIRECT3DVERTEXBUFFER8)(ResourceHeader + credits_XBMCVBuffer_OFFSET);
  pVBuffer->Register(ResourceData);

  pIBuffer = (LPDIRECT3DINDEXBUFFER8)(ResourceHeader + credits_XBMCIBuffer_OFFSET);
  WORD* IdxBuf = new WORD[NumFaces * 3]; // needs to be in cached memory
  memcpy(IdxBuf, (char*)ResourceData + pIBuffer->Data, NumFaces * 3 * sizeof(WORD));
  pIBuffer->Data = (DWORD)IdxBuf;

  pNormalMap = (LPDIRECT3DTEXTURE8)(ResourceHeader + credits_NormMap_OFFSET);
  pNormalMap->Register(ResourceData);

  pSpecEnvMap = (LPDIRECT3DCUBETEXTURE8)(ResourceHeader + credits_SpecEnvMap_OFFSET);
  pSpecEnvMap->Register(ResourceData);

  // make copy of front texture
  D3DSURFACE_DESC desc;
  pSpecEnvMap->GetLevelDesc(0, &desc);
  D3DDevice::CreateTexture(desc.Width, desc.Height, 1, 0, desc.Format, 0, &pFrontTex);
  LPDIRECT3DSURFACE8 pSrcSurf, pDstSurf;
  pSpecEnvMap->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_Z, 0, &pSrcSurf);
  pFrontTex->GetSurfaceLevel(0, &pDstSurf);
  D3DXLoadSurfaceFromSurface(pDstSurf, NULL, NULL, pSrcSurf, NULL, NULL, D3DX_FILTER_NONE, 0);
  pSrcSurf->Release();
  pDstSurf->Release();

  D3DXMATRIX matView, matProj;
  // Set view matrix
  if (g_graphicsContext.IsWidescreen())
    vEye = D3DXVECTOR3(0.0f, 32.0f, -160.0f);
  else
    vEye = D3DXVECTOR3(0.0f, 40.0f, -200.0f);
  D3DXVECTOR3 vAt(0.0f, 0.0f, 0.f);
  D3DXVECTOR3 vRight(1, 0, 0);
  D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);

  D3DXVECTOR3 vLook = vAt - vEye;
  D3DXVec3Normalize(&vLook, &vLook);
  D3DXVec3Cross(&vUp, &vLook, &vRight);

  D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);

  // setup projection matrix
  if (g_graphicsContext.IsWidescreen())
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 16.0f / 9.0f, 0.1f, 1000.f);
  else
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 4.0f / 3.0f, 0.1f, 1000.f);

  D3DXMatrixMultiply(&matVP, &matView, &matProj);

  // setup world matrix
  D3DXMatrixIdentity(&matWorld);

  D3DXMATRIX mat;
  D3DXMatrixTranspose(&matWVP, D3DXMatrixMultiply(&mat, &matWorld, &matVP));

  D3DDevice::SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  D3DDevice::SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

  return S_OK;
}

static void RenderLogo(float fElapsedTime)
{
  D3DXMATRIX mat;

  // Rotate the mesh
  if (fElapsedTime > 0.0f)
  {
    if (fElapsedTime > 0.1f)
      fElapsedTime = 0.1f;
    D3DXMATRIX matRotate;
    FLOAT fXRotate = -fElapsedTime * D3DX_PI * 0.08f;
    D3DXMatrixRotationX(&matRotate, fXRotate);
    D3DXMatrixMultiply(&matWorld, &matWorld, &matRotate);

    D3DXMatrixTranspose(&matWVP, D3DXMatrixMultiply(&mat, &matWorld, &matVP));
  }

  DWORD TextureState[2][5];
  for (int i = 0; i < 2; ++i)
  {
    D3DDevice::GetTextureStageState(i ? 3 : 0, D3DTSS_MAGFILTER, &TextureState[i][0]);
    D3DDevice::GetTextureStageState(i ? 3 : 0, D3DTSS_MINFILTER, &TextureState[i][1]);
    D3DDevice::GetTextureStageState(i ? 3 : 0, D3DTSS_ADDRESSU, &TextureState[i][2]);
    D3DDevice::GetTextureStageState(i ? 3 : 0, D3DTSS_ADDRESSV, &TextureState[i][3]);
    D3DDevice::GetTextureStageState(i ? 3 : 0, D3DTSS_ADDRESSW, &TextureState[i][4]);
  }

  // normal map (no filter!)
  D3DDevice::SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
  D3DDevice::SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
  D3DDevice::SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
  D3DDevice::SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);

  // env map
  D3DDevice::SetTextureStageState(3, D3DTSS_MAGFILTER, D3DTEXF_GAUSSIANCUBIC);
  D3DDevice::SetTextureStageState(3, D3DTSS_MINFILTER, D3DTEXF_GAUSSIANCUBIC);
  D3DDevice::SetTextureStageState(3, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
  D3DDevice::SetTextureStageState(3, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
  D3DDevice::SetTextureStageState(3, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP);

  // Set vertex shader
  D3DDevice::SetVertexShader(s_dwVShader);

  // Set WVP matrix as constants 0-3
  D3DDevice::SetVertexShaderConstantFast(0, (float*)matWVP, 4);
  // Set world-view matrix as constants 4-7
  D3DXMatrixTranspose(&mat, &matWorld);
  D3DDevice::SetVertexShaderConstantFast(4, (float*)mat, 4);
  // Set camera position as c8
  D3DDevice::SetVertexShaderConstantFast(8, (float*)vEye, 1);

  // Set pixel shader
  D3DDevice::SetPixelShader(s_dwPShader);

  // ambient in c0
  D3DDevice::SetPixelShaderConstant(0, &ambient, 1);
  // colour in c1
  D3DDevice::SetPixelShaderConstant(1, &colour, 1);

  D3DDevice::SetStreamSource(0, pVBuffer, 32);
  D3DDevice::SetIndices(pIBuffer, 0);

  D3DDevice::SetTexture(0, pNormalMap);
  D3DDevice::SetTexture(3, pSpecEnvMap);

  // Render geometry
  D3DDevice::DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, NumVerts, 0, NumFaces);

  D3DDevice::SetTexture(0, NULL);
  D3DDevice::SetTexture(3, NULL);
  D3DDevice::SetPixelShader(0);
  D3DDevice::SetVertexShader(0);
  D3DDevice::SetStreamSource(0, NULL, 0);
  D3DDevice::SetIndices(NULL, 0);

  for (int i = 0; i < 2; ++i)
  {
    D3DDevice::SetTextureStageState(i ? 3 : 0, D3DTSS_MAGFILTER, TextureState[i][0]);
    D3DDevice::SetTextureStageState(i ? 3 : 0, D3DTSS_MINFILTER, TextureState[i][1]);
    D3DDevice::SetTextureStageState(i ? 3 : 0, D3DTSS_ADDRESSU, TextureState[i][2]);
    D3DDevice::SetTextureStageState(i ? 3 : 0, D3DTSS_ADDRESSV, TextureState[i][3]);
    D3DDevice::SetTextureStageState(i ? 3 : 0, D3DTSS_ADDRESSW, TextureState[i][4]);
  }
}

static void CleanupLogo()
{

  if (pFrontTex)
    pFrontTex->Release();
  pFrontTex = 0;

  if (s_dwVShader)
    D3DDevice::DeleteVertexShader(s_dwVShader);
  s_dwVShader = 0;

  if (s_dwPShader)
    D3DDevice::DeletePixelShader(s_dwPShader);
  s_dwPShader = 0;

  if (pIBuffer && pIBuffer->Data)
  {
    delete [] (WORD*)pIBuffer->Data;
    pIBuffer->Data = 0;
    pIBuffer = 0;
  }
  pVBuffer = 0;
  pNormalMap = 0;
  pSpecEnvMap = 0;

  if (ResourceHeader)
    free(ResourceHeader);
  ResourceHeader = 0;

  if (ResourceData)
    XPhysicalFree(ResourceData);
  ResourceData = 0;
}

static void RenderCredits(const list<CreditLine_t*>& ActiveList, DWORD& Gamma, DWORD Time, float Scale = 1.0f)
{
  D3DVIEWPORT8 vp;
  D3DDevice::GetViewport(&vp);

  float XScale = (float)vp.Width / g_graphicsContext.GetWidth();
  float YScale = (float)vp.Height / g_graphicsContext.GetHeight();

  // Set scissor region - 12% top / bottom
  D3DRECT rs = { vp.X, vp.Y + vp.Height * 12 / 100, vp.X + vp.Width, vp.Y + vp.Height * 88 / 100 };
  D3DDevice::SetScissors(1, FALSE, &rs);

  DWORD m_dwSavedState[7];
  D3DDevice::GetRenderState(D3DRS_ALPHABLENDENABLE, &m_dwSavedState[0]);
  D3DDevice::GetRenderState(D3DRS_SRCBLEND, &m_dwSavedState[1]);
  D3DDevice::GetRenderState(D3DRS_DESTBLEND, &m_dwSavedState[2]);
  D3DDevice::GetRenderState(D3DRS_ZENABLE, &m_dwSavedState[3]);
  D3DDevice::GetRenderState(D3DRS_STENCILENABLE, &m_dwSavedState[4]);
  D3DDevice::GetTextureStageState(0, D3DTSS_MINFILTER, &m_dwSavedState[5]);
  D3DDevice::GetTextureStageState(0, D3DTSS_MAGFILTER, &m_dwSavedState[6]);

  // Set the necessary render states
  D3DDevice::SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);
  D3DDevice::SetPixelShader(0);
  D3DDevice::SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  D3DDevice::SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  D3DDevice::SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  D3DDevice::SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
  D3DDevice::SetRenderState(D3DRS_STENCILENABLE, FALSE);
  D3DDevice::SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
  D3DDevice::SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);

  // set FFP settings
  D3DDevice::SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  D3DDevice::SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT);
  D3DDevice::SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
  D3DDevice::SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  D3DDevice::SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
  D3DDevice::SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
  D3DDevice::SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
  D3DDevice::SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

  // Render active credits
  for (list<CreditLine_t*>::const_iterator iCredit = ActiveList.begin(); iCredit != ActiveList.end(); ++iCredit)
  {
    CreditLine_t* pCredit = *iCredit;

    if (pCredit->Text)
    {
      // Render
      float x;
      float y;
      DWORD alpha;

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
            if (Time + 20 >= pCredit->Time + pCredit->InDuration)
              g = 255; // make sure last frame is 255
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
            y = (float)pCredit->y * INPROPORTION;
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
            if (Time + 20 >= pCredit->Time + pCredit->InDuration + pCredit->OutDuration + pCredit->Duration)
              g = 255; // make sure last frame is 255
            if (g > Gamma)
              Gamma = g;
          }
          break;

        case EFF_OUT_ASCEND:
          {
            x = pCredit->x;
            y = (float)pCredit->y - ((float)pCredit->y * OUTPROPORTION);
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
      {
        x *= XScale;
        y *= YScale;
        float w = pCredit->TextWidth / 2;
        float h = pCredit->TextHeight / 2;
        w *= Scale;
        h *= Scale;

        if (Scale > 0.5f)
        {
          if (w > g_graphicsContext.GetWidth() * 0.45f)
            w = g_graphicsContext.GetWidth() * 0.45f;
        }

        D3DDevice::SetTexture(0, pCredit->pTex);
        D3DDevice::SetRenderState(D3DRS_TEXTUREFACTOR, alpha << 24);

        D3DDevice::Begin(D3DPT_QUADLIST);
        D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, 0.0f, 0.0f);
        D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, x - w, y - h, 0.0f, 0.0f);
        D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, pCredit->TextWidth, 0.0f);
        D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, x + w, y - h, 0.0f, 0.0f);
        D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, pCredit->TextWidth, pCredit->TextHeight);
        D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, x + w, y + h, 0.0f, 0.0f);
        D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, 0.0f, pCredit->TextHeight);
        D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, x - w, y + h, 0.0f, 0.0f);
        D3DDevice::End();

        D3DDevice::SetTexture(0, 0);
      }
    }
  }

  // Reset scissor region
  D3DDevice::SetScissors(0, FALSE, NULL);

  D3DDevice::SetRenderState(D3DRS_ALPHABLENDENABLE, m_dwSavedState[0]);
  D3DDevice::SetRenderState(D3DRS_SRCBLEND, m_dwSavedState[1]);
  D3DDevice::SetRenderState(D3DRS_DESTBLEND, m_dwSavedState[2]);
  D3DDevice::SetRenderState(D3DRS_ZENABLE, m_dwSavedState[3]);
  D3DDevice::SetRenderState(D3DRS_STENCILENABLE, m_dwSavedState[4]);
  D3DDevice::SetTextureStageState(0, D3DTSS_MINFILTER, m_dwSavedState[5]);
  D3DDevice::SetTextureStageState(0, D3DTSS_MAGFILTER, m_dwSavedState[6]);
}

void RunCredits()
{
  using std::map;
  using std::list;

  // Pause any media
  bool NeedUnpause = false;
  if (g_application.IsPlaying() && !g_application.m_pPlayer->IsPaused())
  {
    g_application.m_pPlayer->Pause();
    NeedUnpause = true;
  }

  g_graphicsContext.Lock(); // exclusive access

  // Fade down display
  D3DGAMMARAMP StartRamp, Ramp;
  D3DDevice::GetGammaRamp(&StartRamp);
  for (int i = 49; i; --i)
  {
    for (int j = 0; j < 256; ++j)
    {
      Ramp.blue[j] = StartRamp.blue[j] * i / 50;
      Ramp.green[j] = StartRamp.green[j] * i / 50;
      Ramp.red[j] = StartRamp.red[j] * i / 50;
    }
    D3DDevice::BlockUntilVerticalBlank();
    D3DDevice::SetGammaRamp(D3DSGR_IMMEDIATE, &Ramp);
  }

  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  g_graphicsContext.SetVideoResolution(res, TRUE);
  float fFrameTime;
  if (res == PAL_4x3 || res == PAL_16x9)
    fFrameTime = 1.0f / 50.0f;
  else
    fFrameTime = 1.0f / 59.94f;

  D3DDevice::Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
  D3DDevice::Present(0, 0, 0, 0);

  MEMORYSTATUS stat;
  GlobalMemoryStatus(&stat);
  if (stat.dwAvailPhys < 10*1024*1024)
  {
    CLog::Log(LOGERROR, "Not enough memory to display credits");
    D3DDevice::SetGammaRamp(D3DSGR_IMMEDIATE, &StartRamp);
    g_graphicsContext.SetVideoResolution(res, TRUE);
    g_graphicsContext.Unlock();
    if (NeedUnpause)
      g_application.m_pPlayer->Pause();
    return ;
  }

  if (FAILED(InitLogo()))
  {
    CLog::Log(LOGERROR, "Unable to load credits logo");
    D3DDevice::SetGammaRamp(D3DSGR_IMMEDIATE, &StartRamp);
    CleanupLogo();
    g_graphicsContext.SetVideoResolution(res, TRUE);
    g_graphicsContext.Unlock();
    if (NeedUnpause)
      g_application.m_pPlayer->Pause();
    return ;
  }

  static bool FixedCredits = false;

  DWORD Time = 0;
  for (int i = 0; i < NUM_CREDITS; ++i)
  {
    // map fonts
    if (Fonts.find(Credits[i].Font) == Fonts.end())
    {
      // first try loading it
      CStdString fontPath = "Q:\\media\\Fonts\\Arial.ttf";
      CStdString strFont;
      strFont.Fmt("__credits%d__", Credits[i].Font);
      CGUIFont *font = g_fontManager.LoadTTF(strFont, fontPath, 0xFFdadada, 0, Credits[i].Font, FONT_STYLE_BOLD);
      Fonts.insert(std::pair<int, CGUIFont*>(Credits[i].Font, font));
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

      // scaling for 124 bpm
      Credits[i].Time = 125 * Credits[i].Time / 124;
      Credits[i].Duration = 125 * Credits[i].Duration / 124;
      Credits[i].InDuration = 125 * Credits[i].InDuration / 124;
      Credits[i].OutDuration = 125 * Credits[i].OutDuration / 124;

      if (Credits[i].Text && !wcsicmp(Credits[i].Text, L"$SKINTITLE"))
        SkinOffset = i;

      // calculate text size (allows ttf to scroll smoothly, as initial rendertime
      // can be quite long)
      if (Credits[i].Text)
      {
        CGUIFont* pFont = Fonts.find(Credits[i].Font)->second;
        GetCreditsTextExtent(pFont, Credits[i].Text, Credits[i].TextWidth, Credits[i].TextHeight);
      }
    }
  }

  // Load the skin credits from <skindir>\skin.xml
  for (int i = 0; i < 6; i++)
    Credits[SkinOffset + i].Text = g_SkinInfo.GetCreditsLine(i);

  FixedCredits = true;

  s_hMusicStarted = CreateEvent(0, TRUE, FALSE, 0);
  s_bStopPlaying = false;
  HANDLE hMusicThread = (HANDLE)_beginthreadex(0, 0, CreditsMusicThread, "q:\\credits\\credits.mod", 0, NULL);
  WaitForSingleObject(s_hMusicStarted, INFINITE);
  CloseHandle(s_hMusicStarted);

#ifdef _DEBUG
  LARGE_INTEGER freq, start, end, start2, end2;
  __int64 rendertime = 0;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&start2);
#endif // _DEBUG

  // Start credits loop
  for (;;)
  {
    // restore gamma
    D3DDevice::SetGammaRamp(D3DSGR_IMMEDIATE, &StartRamp);

    DWORD StartTime = timeGetTime();
    DWORD LastTime = StartTime;
    DWORD LastCreditTime = 0;

    if (WaitForSingleObject(hMusicThread, 0) == WAIT_TIMEOUT)
      LastTime = 0;

    list<CreditLine_t*> ActiveList;
    int NextCredit = 0;

    // Do render loop
    int n = 0;
    while (NextCredit < NUM_CREDITS || !ActiveList.empty())
    {
#ifdef _DEBUG
      QueryPerformanceCounter(&start);
#endif // _DEBUG

      if (WaitForSingleObject(hMusicThread, 0) == WAIT_TIMEOUT)
        Time = (DWORD)mikxboxGetPTS();
      else
        Time = timeGetTime() - StartTime;

      if (Time < LastTime)
      {
        // Wrapped!
        NextCredit = 0;
        ActiveList.clear();
      }
      LastTime = Time;

      // fade out if at the end
      if (NextCredit == NUM_CREDITS && ActiveList.size() == 1)
        s_bFadeMusic = true;

      // Activate new credits
      while (NextCredit < NUM_CREDITS && Credits[NextCredit].Time <= Time)
      {
        if (Credits[NextCredit].Text)
        {
          CGUIFont* pFont = Fonts.find(Credits[NextCredit].Font)->second;
          Credits[NextCredit].pTex = CreateCreditsTexture(pFont, Credits[NextCredit].Text);
          GetCreditsTextExtent(pFont, Credits[NextCredit].Text, Credits[NextCredit].TextWidth, Credits[NextCredit].TextHeight);
        }
        ActiveList.push_back(&Credits[NextCredit]);
        LastCreditTime = Credits[NextCredit].Time;
        ++NextCredit;
      }
      // CreateTexture zaps the viewport
      D3DVIEWPORT8 vpBackBuffer = { 0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight(), 0.0f, 1.0f };
      D3DDevice::SetViewport(&vpBackBuffer);

      // check for retirement
      for (list<CreditLine_t*>::iterator iCredit = ActiveList.begin(); iCredit != ActiveList.end(); ++iCredit)
      {
        CreditLine_t* pCredit = *iCredit;

        while (Time > pCredit->Time + pCredit->InDuration + pCredit->OutDuration + pCredit->Duration)
        {
          if ((*iCredit)->pTex)
            (*iCredit)->pTex->Release();
          (*iCredit)->pTex = NULL;
          iCredit = ActiveList.erase(iCredit);
          if (iCredit == ActiveList.end())
            break;
          pCredit = *iCredit;
        }
        if (iCredit == ActiveList.end())
          break;
      }

      DWORD Gamma = 0;

      // render cubemap
      LPDIRECT3DSURFACE8 pRenderSurf, pOldRT, pOldZS;
      pSpecEnvMap->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_Z, 0, &pRenderSurf);
      D3DDevice::GetRenderTarget(&pOldRT);
      D3DDevice::GetDepthStencilSurface(&pOldZS);
      D3DDevice::SetRenderTarget(pRenderSurf, NULL);

      D3DSURFACE_DESC desc;
      pSpecEnvMap->GetLevelDesc(0, &desc);

      D3DDevice::SetTexture(0, pFrontTex);
      D3DDevice::SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);

      D3DDevice::SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
      D3DDevice::SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT);
      D3DDevice::SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
      D3DDevice::SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
      D3DDevice::SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
      D3DDevice::SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

      D3DDevice::Begin(D3DPT_QUADLIST);
      D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, 0.0f, 0.0f);
      D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, 0.0f, 0.0f, 0.0f, 0.0f);
      D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, 1.0f, 0.0f);
      D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, (float)desc.Width, 0.0f, 0.0f, 0.0f);
      D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, 1.0f, 1.0f);
      D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, (float)desc.Width, (float)desc.Height, 0.0f, 0.0f);
      D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, 0.0f, 1.0f);
      D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, 0.0f, (float)desc.Height, 0.0f, 0.0f);
      D3DDevice::End();

      D3DDevice::SetTexture(0, 0);

      RenderCredits(ActiveList, Gamma, Time, 0.4f);

      D3DDevice::SetRenderTarget(pOldRT, pOldZS);
      pOldRT->Release();
      if (pOldZS)
        pOldZS->Release();
      pRenderSurf->Release();

      D3DDevice::Clear(0, 0, D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

      // Background
      RenderLogo(fFrameTime);

      Gamma = 0;

      RenderCredits(ActiveList, Gamma, Time, g_graphicsContext.GetHeight() / 480.0f);

      if (Gamma)
      {
        for (int j = 0; j < 256; ++j)
        {
#define bclamp(x) (x) > 255 ? 255 : (BYTE)((x) & 0xff)
          Ramp.blue[j] = bclamp(StartRamp.blue[j] + Gamma);
          Ramp.green[j] = bclamp(StartRamp.green[j] + Gamma);
          Ramp.red[j] = bclamp(StartRamp.red[j] + Gamma);
        }
        D3DDevice::SetGammaRamp(0, &Ramp);
      }
      else
        D3DDevice::SetGammaRamp(0, &StartRamp);

      // check for keypress
      g_application.ReadInput();
      if (g_application.m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_A] > XINPUT_GAMEPAD_MAX_CROSSTALK)
      {
        // output timing to log, rounded to 60ms
        DWORD roundedTime = Time - (Time % 60);
        CLog::Log(LOGDEBUG, __FUNCTION__" time since last credit: %i (%i)", roundedTime - LastCreditTime, roundedTime);
      }

      if (g_application.m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_B] > XINPUT_GAMEPAD_MAX_CROSSTALK ||
          g_application.m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_BACK ||
          g_application.m_DefaultIR_Remote.wButtons == XINPUT_IR_REMOTE_BACK ||
          g_application.m_DefaultIR_Remote.wButtons == XINPUT_IR_REMOTE_MENU ||
          g_Mouse.bClick[MOUSE_LEFT_BUTTON] || g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
      {
        s_bStopPlaying = true;
        CloseHandle(hMusicThread);

        // Unload fonts
        for (map<int, CGUIFont*>::iterator iFont = Fonts.begin(); iFont != Fonts.end(); ++iFont)
        {
          CGUIFont *font = iFont->second;
          CStdString fontName = font->GetFontName();
          g_fontManager.Unload(fontName);
        }
        Fonts.clear();

        CleanupLogo();

        // cleanup our activelist
        for (list<CreditLine_t*>::iterator iCredit = ActiveList.begin(); iCredit != ActiveList.end(); ++iCredit)
        {
          CreditLine_t* pCredit = *iCredit;
          if (pCredit->pTex)
            pCredit->pTex->Release();
          pCredit->pTex = NULL;
        }
        ActiveList.clear();

        // wait for button release
        do
        {
          Sleep(1);
          g_application.ReadInput();
        }
        while (g_application.m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_B] > XINPUT_GAMEPAD_MAX_CROSSTALK ||
               g_application.m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_BACK ||
               g_application.m_DefaultIR_Remote.wButtons == XINPUT_IR_REMOTE_BACK ||
               g_application.m_DefaultIR_Remote.wButtons == XINPUT_IR_REMOTE_MENU);

        // clear screen and exit to gui
        g_graphicsContext.SetVideoResolution(res, TRUE);
        D3DDevice::Clear(0, 0, D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
        D3DDevice::SetGammaRamp(0, &StartRamp);
        D3DDevice::Present(0, 0, 0, 0);
        g_graphicsContext.Unlock();

        if (NeedUnpause)
          g_application.m_pPlayer->Pause();

        return ;
      }

#ifdef _DEBUG
      static char FPS[80];
      QueryPerformanceCounter(&end);
      rendertime += end.QuadPart - start.QuadPart;
      if (++n == 50)
      {
        QueryPerformanceCounter(&end2);

        MEMORYSTATUS stat;
        GlobalMemoryStatus(&stat);
        float f = float(end2.QuadPart - start2.QuadPart);
        sprintf(FPS, "Render: %.2f fps (%.2f%%)\nFreeMem: %.1f/%uMB", 50.0f * freq.QuadPart / f, 100.f * rendertime / f,
                 float(stat.dwAvailPhys) / (1024.0f*1024.0f), stat.dwTotalPhys / (1024*1024));

        rendertime = 0;
        n = 0;
        QueryPerformanceCounter(&start2);
      }      
      CGUITextLayout::DrawText(g_fontManager.GetFont("font13"), 50, 30, 0xffffffff, 0, FPS, 0);
#endif

      // present scene
      D3DDevice::BlockUntilVerticalBlank();
      D3DDevice::Present(0, 0, 0, 0);
    }
  }
}

unsigned __stdcall CreditsMusicThread(void* pParam)
{
  CSectionLoader::Load("MOD_RX");
  CSectionLoader::Load("MOD_RW");

  if (!mikxboxInit())
  {
    SetEvent(s_hMusicStarted);
    return 1;
  }
  int MusicVol = 128;
  mikxboxSetMusicVolume((MusicVol * g_application.GetVolume()) / 100);

  MODULE* pModule = Mod_Player_Load((char*)pParam, 127, 0);

  if (!pModule)
  {
    SetEvent(s_hMusicStarted);
    return 1;
  }

  // set to loop and jump to the end section
  // pModule->loop = 1;
  // pModule->sngpos = 18;
  s_bFadeMusic = false;

  Mod_Player_Start(pModule);

  SetEvent(s_hMusicStarted);

  do
  {
    while (!s_bStopPlaying && MusicVol > 0 && Mod_Player_Active())
    {
      MikMod_Update();
      if (s_bFadeMusic)
      {
        --MusicVol;
        mikxboxSetMusicVolume((MusicVol * g_application.GetVolume()) / 100);
      }
      else
      {
        // check for end of pattern at position 12
        if (pModule->sngpos == 12 &&
            pModule->patpos == pModule->numrow - 1 &&
            !pModule->vbtick)
        {
          // jump back to posiiton 9
          pModule->patpos = 0;
          pModule->posjmp = 2;
          pModule->sngpos = 9;
        }
      }
    }

    if (!s_bStopPlaying)
    {
      Mod_Player_Stop();
      Mod_Player_Free(pModule);
      pModule = Mod_Player_Load((char*)pParam, 127, 0);
      //   pModule->loop = 1;
      //   pModule->sngpos = 18;
      s_bFadeMusic = false;
      MusicVol = 128;
      mikxboxSetMusicVolume((MusicVol * g_application.GetVolume()) / 100);
      Mod_Player_Start(pModule);
    }
  }
  while (!s_bStopPlaying);

  Mod_Player_Free(pModule);

  mikxboxExit();

  CSectionLoader::Unload("MOD_RX");
  CSectionLoader::Unload("MOD_RW");

  return 0;
}

void GetCreditsTextExtent(CGUIFont *font, const wchar_t *text, float &width, float &height)
{
  // this is really hacky, but the credits need a redesign at some point anyway :p
  if (!font) return;
  CStdString utf8;
  g_charsetConverter.utf16toUTF8(text, utf8);
  CGUITextLayout layout(font, false);
  layout.Update(utf8);
  layout.GetTextExtent(width, height);
}

LPDIRECT3DTEXTURE8 CreateCreditsTexture(CGUIFont *font, const wchar_t *text)
{
  CStdString utf8;
  g_charsetConverter.utf16toUTF8(text, utf8);
  CGUITextLayout layout(font, false);
  layout.Update(utf8);

  // grab the text extents
  float width, height;
  layout.GetTextExtent(width, height);
  // create a texture of this size
  LPDIRECT3DTEXTURE8 texture = NULL;
  OutputDebugString("Creating texture\n");
  if (D3D_OK == D3DDevice::CreateTexture((UINT)width, (UINT)height, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &texture))
  {
    // grab the surface level
    D3DSurface *oldSurface, *newSurface;
    texture->GetSurfaceLevel(0, &newSurface);
    D3DDevice::GetRenderTarget(&oldSurface);
    D3DDevice::SetRenderTarget(newSurface, NULL);
    D3DXMATRIX mtxWorld, mtxView, mtxProjection, flipY, translate;
    D3DXMatrixIdentity(&mtxWorld);
    D3DDevice::SetTransform(D3DTS_WORLD, &mtxWorld);
    D3DXMatrixIdentity(&mtxView);
    D3DXMatrixScaling(&flipY, 1.0f, -1.0f, 1.0f);
    D3DXMatrixTranslation(&translate, -0.5f*width, -0.5f*height, 2.0f);
    D3DXMatrixMultiply(&mtxView, &translate, &flipY);
    D3DDevice::SetTransform(D3DTS_VIEW, &mtxView);
    D3DXMatrixPerspectiveLH(&mtxProjection, width*0.5f, height*0.5f, 1.0f, 100.0f);
    D3DDevice::SetTransform(D3DTS_PROJECTION, &mtxProjection);
    // render text into it
    D3DDevice::Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
    layout.Render(0, 0, 0, 0xffdadada, 0, 0, 0);
    D3DDevice::SetRenderTarget(oldSurface, NULL);
    newSurface->Release();
    oldSurface->Release();
  }
  return texture;
}

