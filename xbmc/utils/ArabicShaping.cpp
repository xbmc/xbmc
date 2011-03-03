/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  A port of Mohammed Yousif's C Arabic shaping code
 *  Ported by Nibras Al-shaiba
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef _WIN32
#pragma comment(lib, "libfribidi.lib")
#endif

#include <stdlib.h>
#include "ArabicShaping.h"

#define NIL 0x0000

#define MAP_LENGTH 37
#define COMB_MAP_LENGTH 4
#define TRANS_CHARS_LENGTH 39

typedef struct
{
  FriBidiChar code;
  FriBidiChar mIsolated;
  FriBidiChar mInitial;
  FriBidiChar mMedial;
  FriBidiChar mFinal;
} CharRep;

typedef struct
{
  FriBidiChar code[2];
  FriBidiChar mIsolated;
  FriBidiChar mInitial;
  FriBidiChar mMedial;
  FriBidiChar mFinal;
} CombCharRep;

CharRep charsMap[MAP_LENGTH] =
{
  { 0x0621, 0xFE80, NIL, NIL, NIL }, /* HAMZA */
  { 0x0622, 0xFE81, NIL, NIL, 0xFE82 }, /* ALEF_MADDA */
  { 0x0623, 0xFE83, NIL, NIL, 0xFE84 }, /* ALEF_HAMZA_ABOVE */
  { 0x0624, 0xFE85, NIL, NIL, 0xFE86 }, /* WAW_HAMZA */
  { 0x0625, 0xFE87, NIL, NIL, 0xFE88 }, /* ALEF_HAMZA_BELOW */
  { 0x0626, 0xFE89, 0xFE8B, 0xFE8C, 0xFE8A }, /* YEH_HAMZA */
  { 0x0627, 0xFE8D, NIL, NIL, 0xFE8E }, /* ALEF */
  { 0x0628, 0xFE8F, 0xFE91, 0xFE92, 0xFE90 }, /* BEH */
  { 0x0629, 0xFE93, NIL, NIL, 0xFE94 }, /* TEH_MARBUTA */
  { 0x062A, 0xFE95, 0xFE97, 0xFE98, 0xFE96 }, /* TEH */
  { 0x062B, 0xFE99, 0xFE9B, 0xFE9C, 0xFE9A }, /* THEH */
  { 0x062C, 0xFE9D, 0xFE9F, 0xFEA0, 0xFE9E }, /* JEEM */
  { 0x062D, 0xFEA1, 0xFEA3, 0xFEA4, 0xFEA2 }, /* HAH */
  { 0x062E, 0xFEA5, 0xFEA7, 0xFEA8, 0xFEA6 }, /* KHAH */
  { 0x062F, 0xFEA9, NIL, NIL, 0xFEAA }, /* DAL */
  { 0x0630, 0xFEAB, NIL, NIL, 0xFEAC }, /* THAL */
  { 0x0631, 0xFEAD, NIL, NIL, 0xFEAE }, /* REH */
  { 0x0632, 0xFEAF, NIL, NIL, 0xFEB0 }, /* ZAIN */
  { 0x0633, 0xFEB1, 0xFEB3, 0xFEB4, 0xFEB2 }, /* SEEN */
  { 0x0634, 0xFEB5, 0xFEB7, 0xFEB8, 0xFEB6 }, /* SHEEN */
  { 0x0635, 0xFEB9, 0xFEBB, 0xFEBC, 0xFEBA }, /* SAD */
  { 0x0636, 0xFEBD, 0xFEBF, 0xFEC0, 0xFEBE }, /* DAD */
  { 0x0637, 0xFEC1, 0xFEC3, 0xFEC4, 0xFEC2 }, /* TAH */
  { 0x0638, 0xFEC5, 0xFEC7, 0xFEC8, 0xFEC6 }, /* ZAH */
  { 0x0639, 0xFEC9, 0xFECB, 0xFECC, 0xFECA }, /* AIN */
  { 0x063A, 0xFECD, 0xFECF, 0xFED0, 0xFECE }, /* GHAIN */
  { 0x0640, 0x0640, NIL, NIL, NIL }, /* TATWEEL */
  { 0x0641, 0xFED1, 0xFED3, 0xFED4, 0xFED2 }, /* FEH */
  { 0x0642, 0xFED5, 0xFED7, 0xFED8, 0xFED6 }, /* QAF */
  { 0x0643, 0xFED9, 0xFEDB, 0xFEDC, 0xFEDA }, /* KAF */
  { 0x0644, 0xFEDD, 0xFEDF, 0xFEE0, 0xFEDE }, /* LAM */
  { 0x0645, 0xFEE1, 0xFEE3, 0xFEE4, 0xFEE2 }, /* MEEM */
  { 0x0646, 0xFEE5, 0xFEE7, 0xFEE8, 0xFEE6 }, /* NOON */
  { 0x0647, 0xFEE9, 0xFEEB, 0xFEEC, 0xFEEA }, /* HEH */
  { 0x0648, 0xFEED, NIL, NIL, 0xFEEE }, /* WAW */
  //{ 0x0649, 0xFEEF, 0xFBE8, 0xFBE9, 0xFEF0 },    /* ALEF_MAKSURA */
  { 0x0649, 0xFEEF, NIL, NIL, 0xFEF0 }, /* ALEF_MAKSURA */
  { 0x064A, 0xFEF1, 0xFEF3, 0xFEF4, 0xFEF2 } /* YEH */
};

CombCharRep combCharsMap[COMB_MAP_LENGTH] =
{
  { { 0x0644, 0x0622 }, 0xFEF5, NIL, NIL, 0xFEF6 }, /* LAM_ALEF_MADDA */
  { { 0x0644, 0x0623 }, 0xFEF7, NIL, NIL, 0xFEF8 }, /* LAM_ALEF_HAMZA_ABOVE */
  { { 0x0644, 0x0625 }, 0xFEF9, NIL, NIL, 0xFEFA }, /* LAM_ALEF_HAMZA_BELOW */
  { { 0x0644, 0x0627 }, 0xFEFB, NIL, NIL, 0xFEFC } /* LAM_ALEF */
};

FriBidiChar transChars[TRANS_CHARS_LENGTH] =
{
  0x0610, /* ARABIC SIGN SALLALLAHOU ALAYHE WASSALLAM */
  0x0612, /* ARABIC SIGN ALAYHE ASSALLAM */
  0x0613, /* ARABIC SIGN RADI ALLAHOU ANHU */
  0x0614, /* ARABIC SIGN TAKHALLUS */
  0x0615, /* ARABIC SMALL HIGH TAH */
  0x064B, /* ARABIC FATHATAN */
  0x064C, /* ARABIC DAMMATAN */
  0x064D, /* ARABIC KASRATAN */
  0x064E, /* ARABIC FATHA */
  0x064F, /* ARABIC DAMMA */
  0x0650, /* ARABIC KASRA */
  0x0651, /* ARABIC SHADDA */
  0x0652, /* ARABIC SUKUN */
  0x0653, /* ARABIC MADDAH ABOVE */
  0x0654, /* ARABIC HAMZA ABOVE */
  0x0655, /* ARABIC HAMZA BELOW */
  0x0656, /* ARABIC SUBSCRIPT ALEF */
  0x0657, /* ARABIC INVERTED DAMMA */
  0x0658, /* ARABIC MARK NOON GHUNNA */
  0x0670, /* ARABIC LETTER SUPERSCRIPT ALEF */
  0x06D6, /* ARABIC SMALL HIGH LIGATURE SAD WITH LAM WITH ALEF MAKSURA */
  0x06D7, /* ARABIC SMALL HIGH LIGATURE QAF WITH LAM WITH ALEF MAKSURA */
  0x06D8, /* ARABIC SMALL HIGH MEEM INITIAL FORM */
  0x06D9, /* ARABIC SMALL HIGH LAM ALEF */
  0x06DA, /* ARABIC SMALL HIGH JEEM */
  0x06DB, /* ARABIC SMALL HIGH THREE DOTS */
  0x06DC, /* ARABIC SMALL HIGH SEEN */
  0x06DF, /* ARABIC SMALL HIGH ROUNDED ZERO */
  0x06E0, /* ARABIC SMALL HIGH UPRIGHT RECTANGULAR ZERO */
  0x06E1, /* ARABIC SMALL HIGH DOTLESS HEAD OF KHAH */
  0x06E2, /* ARABIC SMALL HIGH MEEM ISOLATED FORM */
  0x06E3, /* ARABIC SMALL LOW SEEN */
  0x06E4, /* ARABIC SMALL HIGH MADDA */
  0x06E7, /* ARABIC SMALL HIGH YEH */
  0x06E8, /* ARABIC SMALL HIGH NOON */
  0x06EA, /* ARABIC EMPTY CENTRE LOW STOP */
  0x06EB, /* ARABIC EMPTY CENTRE HIGH STOP */
  0x06EC, /* ARABIC ROUNDED HIGH STOP WITH FILLED CENTRE */
  0x06ED /* ARABIC SMALL LOW MEEM */
};

static fribidi_boolean CharacterMapContains(FriBidiChar c)
{
  for (int i = 0; i < MAP_LENGTH; i++)
  {
    if (charsMap[i].code == c)
      return true;
  }

  return false;
}

static CharRep GetCharRep(FriBidiChar c)
{
  for (int i = 0; i < MAP_LENGTH; i++)
  {
    if (charsMap[i].code == c)
      return charsMap[i];
  }

  CharRep nilRep =  { NIL, NIL, NIL, NIL };
  return nilRep;
}

static CombCharRep GetCombCharRep(FriBidiChar c1, FriBidiChar c2)
{
  int i = 0;
  for (i = 0; i < COMB_MAP_LENGTH; i++)
  {
    if (combCharsMap[i].code[0] == c1 && combCharsMap[i].code[1] == c2)
      return combCharsMap[i];
  }
  CombCharRep combNilRep =
  {
  { NIL, NIL }, NIL, NIL, NIL };
  return combNilRep;
}

static fribidi_boolean IsTransparent(FriBidiChar c)
{
  int i = 0;
  for (i = 0; i < TRANS_CHARS_LENGTH; i++)
  {
    if (transChars[i] == c)
      return true;
  }
  return false;
}

FriBidiChar* shape_arabic(FriBidiChar * str, int len)
{
  CharRep crep = { 0 };
  CombCharRep combcrep;
  FriBidiChar * shaped =
      (FriBidiChar *) malloc(sizeof(FriBidiChar) * (len + 1));
  int writeCount = 0;
  for (int i = 0; i < len; i++)
  {
    FriBidiChar current = str[i];
    if (CharacterMapContains(current))
    {
      FriBidiChar prev = NIL;
      FriBidiChar next = NIL;
      int prevID = i - 1;
      int nextID = i + 1;

      /*
       Transparent characters have no effect in the shaping process.
       So, ignore all the transparent characters that are BEFORE the
       current character.
       */
      for (; prevID >= 0; prevID--)
        if (!IsTransparent(str[prevID]))
          break;

      if ((prevID < 0) || !CharacterMapContains(prev = str[prevID]) || (!((crep
          = GetCharRep(prev)).mInitial != NIL) && !(crep.mMedial != NIL)))
        prev = NIL;

      /*
       Transparent characters have no effect in the shaping process.
       So, ignore all the transparent characters that are AFTER the
       current character.
       */
      for (; nextID < len; nextID++)
        if (!IsTransparent(str[nextID]))
          break;

      if ((nextID >= len) || !CharacterMapContains(next = str[nextID])
          || (!((crep = GetCharRep(next)).mMedial != NIL) && !((crep
              = GetCharRep(next)).mFinal != NIL) && (next != 0x0640)))
        next = NIL;

      /* Combinations */
      if (current == 0x0644 && next != NIL && (next == 0x0622 || next == 0x0623
          || next == 0x0625 || next == 0x0627))
      {
        combcrep = GetCombCharRep(current, next);
        if (prev != NIL)
        {
          shaped[writeCount++] = combcrep.mFinal;
        }
        else
        {
          shaped[writeCount++] = combcrep.mIsolated;
        }
        i++;
        continue;
      }

      crep = GetCharRep(current);

      /* Medial */
      if (prev != NIL && next != NIL && crep.mMedial != NIL)
      {
        shaped[writeCount++] = crep.mMedial;
        continue;
        /* Final */
      }
      else if (prev != NIL && crep.mFinal != NIL)
      {
        shaped[writeCount++] = crep.mFinal;
        continue;
        /* Initial */
      }
      else if (next != NIL && crep.mInitial != NIL)
      {
        shaped[writeCount++] = crep.mInitial;
        continue;
      }
      /* Isolated */
      shaped[writeCount++] = crep.mIsolated;
    }
    else
    {
      shaped[writeCount++] = current;
    }
  }
  shaped[writeCount] = NIL;
  return shaped;
}
