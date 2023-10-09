/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "TeletextDefines.h"
#include "guilib/GUITexture.h"
#include "utils/ColorUtils.h"

// stuff for freetype
#include <ft2build.h>

#ifdef TARGET_WINDOWS_STORE
#define generic GenericFromFreeTypeLibrary
#endif

#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_SMALL_BITMAPS_H

class CAction;

typedef enum /* object type */
{
  OBJ_PASSIVE,
  OBJ_ACTIVE,
  OBJ_ADAPTIVE
} tObjType;

class CTeletextDecoder
{
public:
  CTeletextDecoder();
  virtual ~CTeletextDecoder(void);

  bool NeedRendering() { return m_updateTexture; }
  void RenderingDone() { m_updateTexture = false; }
  UTILS::COLOR::Color* GetTextureBuffer()
  {
    return m_TextureBuffer + (m_RenderInfo.Width * m_YOffset);
  }
  int GetHeight() { return m_RenderInfo.Height; }
  int GetWidth() { return m_RenderInfo.Width; }

  /*!
   \brief Checks if the data in the decoder has changed
   \return true if the data in the decoder has changed, false otherwise
   */
  bool Changed();

  bool InitDecoder();
  void EndDecoder();
  void RenderPage();
  bool HandleAction(const CAction &action);

private:
  void PageInput(int Number);
  void GetNextPageOne(bool up);
  void GetNextSubPage(int offset);
  bool IsSubtitlePage(int pageNumber) const;
  void SwitchZoomMode();
  void SwitchTranspMode();
  void SwitchHintMode();
  void ColorKey(int target);
  void StartPageCatching();
  void StopPageCatching();
  void CatchNextPage(int firstlineinc, int inc);
  void RenderCatchedPage();
  void DoFlashing(int startrow);
  void DoRenderPage(int startrow, int national_subset_bak);
  void Decode_BTT();
  void Decode_ADIP();
  int TopText_GetNext(int startpage, int up, int findgroup);
  void Showlink(int column, int linkpage);
  void CreateLine25();
  void RenderCharFB(int Char, TextPageAttr_t *Attribute);
  void RenderCharBB(int Char, TextPageAttr_t *Attribute);
  void CopyBB2FB();
  void SetFontWidth(int newWidth);
  int GetCurFontWidth();
  void SetPosX(int column);
  void ClearBB(UTILS::COLOR::Color Color);
  void ClearFB(UTILS::COLOR::Color Color);
  void FillBorder(UTILS::COLOR::Color Color);
  void FillRect(
      UTILS::COLOR::Color* buffer, int xres, int x, int y, int w, int h, UTILS::COLOR::Color Color);
  void DrawVLine(
      UTILS::COLOR::Color* lfb, int xres, int x, int y, int l, UTILS::COLOR::Color color);
  void DrawHLine(
      UTILS::COLOR::Color* lfb, int xres, int x, int y, int l, UTILS::COLOR::Color color);
  void FillRectMosaicSeparated(UTILS::COLOR::Color* lfb,
                               int xres,
                               int x,
                               int y,
                               int w,
                               int h,
                               UTILS::COLOR::Color fgcolor,
                               UTILS::COLOR::Color bgcolor,
                               int set);
  void FillTrapez(UTILS::COLOR::Color* lfb,
                  int xres,
                  int x0,
                  int y0,
                  int l0,
                  int xoffset1,
                  int h,
                  int l1,
                  UTILS::COLOR::Color color);
  void FlipHorz(UTILS::COLOR::Color* lfb, int xres, int x, int y, int w, int h);
  void FlipVert(UTILS::COLOR::Color* lfb, int xres, int x, int y, int w, int h);
  int ShapeCoord(int param, int curfontwidth, int curfontheight);
  void DrawShape(UTILS::COLOR::Color* lfb,
                 int xres,
                 int x,
                 int y,
                 int shapenumber,
                 int curfontwidth,
                 int fontheight,
                 int curfontheight,
                 UTILS::COLOR::Color fgcolor,
                 UTILS::COLOR::Color bgcolor,
                 bool clear);
  void RenderDRCS(
      int xres,
      unsigned char* s, /* pointer to char data, parity undecoded */
      UTILS::COLOR::Color* d, /* pointer to frame buffer of top left pixel */
      unsigned char* ax, /* array[0..12] of x-offsets, array[0..10] of y-offsets for each pixel */
      UTILS::COLOR::Color fgcolor,
      UTILS::COLOR::Color bgcolor);
  void RenderCharIntern(TextRenderInfo_t* RenderInfo, int Char, TextPageAttr_t *Attribute, int zoom, int yoffset);
  int RenderChar(
      UTILS::COLOR::Color* buffer, // pointer to render buffer, min. fontheight*2*xres
      int xres, // length of 1 line in render buffer
      int Char, // character to render
      int*
          pPosX, // left border for rendering relative to *buffer, will be set to right border after rendering
      int PosY, // vertical position of char in *buffer
      TextPageAttr_t* Attribute, // Attributes of Char
      bool zoom, // 1= character will be rendered in double height
      int curfontwidth, // rendering width of character
      int curfontwidth2, // rendering width of next character (needed for doublewidth)
      int fontheight, // height of character
      bool transpmode, // 1= transparent display
      unsigned char* axdrcs, // width and height of DRCS-chars
      int Ascender);
  TextPageinfo_t* DecodePage(bool showl25,              // 1=decode Level2.5-graphics
                             unsigned char* PageChar,   // page buffer, min. 25*40
                             TextPageAttr_t *PageAtrb,  // attribute buffer, min 25*40
                             bool HintMode,             // 1=show hidden information
                             bool showflof);            // 1=decode FLOF-line
  void Eval_l25(unsigned char* page_char, TextPageAttr_t *PageAtrb, bool HintMode);
  void Eval_Object(int iONr, TextCachedPage_t *pstCachedPage,
                   unsigned char *pAPx, unsigned char *pAPy,
                   unsigned char *pAPx0, unsigned char *pAPy0,
                   tObjType ObjType, unsigned char* pagedata, unsigned char* page_char, TextPageAttr_t* PageAtrb);
  void Eval_NumberedObject(int p, int s, int packet, int triplet, int high,
                           unsigned char *pAPx, unsigned char *pAPy,
                           unsigned char *pAPx0, unsigned char *pAPy0, unsigned char* page_char, TextPageAttr_t* PageAtrb);
  int Eval_Triplet(int iOData, TextCachedPage_t *pstCachedPage,
                   unsigned char *pAPx, unsigned char *pAPy,
                   unsigned char *pAPx0, unsigned char *pAPy0,
                   unsigned char *drcssubp, unsigned char *gdrcssubp,
                   signed char *endcol, TextPageAttr_t *attrPassive, unsigned char* pagedata, unsigned char* page_char, TextPageAttr_t* PageAtrb);
  int iTripletNumber2Data(int iONr, TextCachedPage_t *pstCachedPage, unsigned char* pagedata);
  int SetNational(unsigned char sec);
  int NextHex(int i);
  void SetColors(const unsigned short *pcolormap, int offset, int number);
  UTILS::COLOR::Color GetColorRGB(enumTeletextColor ttc);

  static FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface);

  std::string         m_teletextFont;     /* Path to teletext font */
  int                 m_YOffset;          /* Swap position for Front buffer and Back buffer */
  UTILS::COLOR::Color* m_TextureBuffer; /* Texture buffer to hold generated data */
  bool                m_updateTexture;    /* Update the texture if set */
  char                prevHeaderPage;     /* Needed for texture update if header is changed */
  char                prevTimeSec;        /* Needed for Time string update */

  int                 m_CatchRow;         /* for page catching */
  int                 m_CatchCol;         /*  "   "       "    */
  int                 m_CatchedPage;      /*  "   "       "    */
  int                 m_PCOldRow;         /*  "   "       "    */
  int                 m_PCOldCol;         /*  "   "       "    */

  FT_Library          m_Library;          /* FreeType 2 data */
  FTC_Manager         m_Manager;          /*  "       "   "  */
  FTC_SBitCache       m_Cache;            /*  "       "   "  */
  FTC_SBit            m_sBit;             /*  "       "   "  */
  FT_Face             m_Face;             /*  "       "   "  */
  /*! An opaque handle to a cache node object. Each cache node is reference-counted. */
  FTC_Node m_anode;
  FTC_ImageTypeRec    m_TypeTTF;          /*  "       "   "  */
  int                 m_Ascender;         /*  "       "   "  */

  int                 m_TempPage;         /* Temporary page number for number input */
  int                 m_LastPage;         /* Last selected Page */
  std::shared_ptr<TextCacheStruct_t>  m_txtCache;         /* Text cache generated by the VideoPlayer if Teletext present */
  TextRenderInfo_t    m_RenderInfo;       /* Rendering information of displayed Teletext page */
};
