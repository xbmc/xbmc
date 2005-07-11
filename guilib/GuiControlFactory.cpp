#include "include.h"
#include "GUIControlFactory.h"
#include "LocalizeStrings.h"
#include "GUIButtoncontrol.h"
#include "GUIConditionalButtonControl.h"
#include "GUISpinButtonControl.h"
#include "GUIRadiobuttoncontrol.h"
#include "GUISpinControl.h"
#include "GUIRSSControl.h"
#include "GUIRAMControl.h"
#include "GUIConsoleControl.h"
#include "GUIListControl.h"
#include "GUIListControlEx.h"
#include "GUIImage.h"
#include "GUILabelControl.h"
#include "GUIEditControl.h"
#include "GUIFadeLabelControl.h"
#include "GUICheckMarkControl.h"
#include "GUIThumbnailPanel.h"
#include "GUIMButtonControl.h"
#include "GUIToggleButtonControl.h"
#include "GUITextBox.h"
#include "GUIVideoControl.h"
#include "GUIProgressControl.h"
#include "GUISliderControl.h"
#include "GUISelectButtonControl.h"
#include "GUIMoverControl.h"
#include "GUIResizeControl.h"
#include "GUIButtonScroller.h"
#include "GUISpinControlEx.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "GUIVisualisationControl.h"
#include "../xbmc/util.h"

CGUIControlFactory::CGUIControlFactory(void)
{}

CGUIControlFactory::~CGUIControlFactory(void)
{}

bool CGUIControlFactory::GetHex(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwHexValue)
{
  TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  sscanf(pNode->FirstChild()->Value(), "%x", &dwHexValue );
  return true;
}

bool CGUIControlFactory::GetDWORD(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwDWORDValue)
{
  TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  dwDWORDValue = atol(pNode->FirstChild()->Value());
  return true;
}

bool CGUIControlFactory::GetLong(const TiXmlNode* pRootNode, const char* strTag, long& lLongValue)
{
  TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  lLongValue = atol(pNode->FirstChild()->Value());
  return true;
}
bool CGUIControlFactory::GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue)
{
  TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  iIntValue = atoi(pNode->FirstChild()->Value());
  return true;
}

bool CGUIControlFactory::GetIntRange(const TiXmlNode* pRootNode, const char* strTag, int& iMinValue, int& iMaxValue, int& iIntervalValue)
{
  TiXmlNode* pNode = pRootNode->FirstChild(strTag);
  if (!pNode || !pNode->FirstChild()) return false;
  iMinValue = atoi(pNode->FirstChild()->Value());
  char* maxValue = strchr(pNode->FirstChild()->Value(), ',');
  if (maxValue)
  {
    maxValue++;
    iMaxValue = atoi(maxValue);

    char* intervalValue = strchr(maxValue, ',');
    if (intervalValue)
    {
      intervalValue++;
      iIntervalValue = atoi(intervalValue);
    }
  }

  return true;
}

bool CGUIControlFactory::GetFloatRange(const TiXmlNode* pRootNode, const char* strTag, float& fMinValue, float& fMaxValue, float& fIntervalValue)
{
  TiXmlNode* pNode = pRootNode->FirstChild(strTag);
  if (!pNode || !pNode->FirstChild()) return false;
  fMinValue = (float) atof(pNode->FirstChild()->Value());
  char* maxValue = strchr(pNode->FirstChild()->Value(), ',');
  if (maxValue)
  {
    maxValue++;
    fMaxValue = (float) atof(maxValue);

    char* intervalValue = strchr(maxValue, ',');
    if (intervalValue)
    {
      intervalValue++;
      fIntervalValue = (float) atoi(intervalValue);
    }
  }

  return true;
}

bool CGUIControlFactory::GetBoolean(const TiXmlNode* pRootNode, const char* strTag, bool& bBoolValue)
{
  TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;
  CStdString strEnabled = pNode->FirstChild()->Value();
  strEnabled.ToLower();
  if (strEnabled == "off" || strEnabled == "no" || strEnabled == "disabled" || strEnabled == "false")
    bBoolValue = false;
  else
  {
    bBoolValue = true;
    if (strEnabled != "on" && strEnabled != "yes" && strEnabled != "enabled" && strEnabled != "true")
      return false; // invalid bool switch - it's probably some other string.
  }
  return true;
}

bool CGUIControlFactory::GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue)
{
  TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode) return false;
  pNode = pNode->FirstChild();
  if (pNode != NULL) strStringValue = pNode->Value();
  else strStringValue = "";
  return true;
}

bool CGUIControlFactory::GetMultipleString(const TiXmlNode* pRootNode, const char* strTag, CStdStringArray& vecStringValue)
{
  TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode) return false;
  vecStringValue.clear();
  bool bFound = false;
  while (pNode)
  {
    TiXmlNode *pChild = pNode->FirstChild();
    if (pChild != NULL)
    {
      vecStringValue.push_back(pChild->Value());
      bFound = true;
    }
    pNode = pNode->NextSibling(strTag);
  }
  return bFound;
}

bool CGUIControlFactory::GetPath(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringPath)
{
  TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode) return false;
  strStringPath = pNode->FirstChild() ? pNode->FirstChild()->Value() : "";
  strStringPath.Replace('/', '\\');
  return true;
}

bool CGUIControlFactory::GetAlignment(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment)
{
  TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild()) return false;

  CStdString strAlign = pNode->FirstChild()->Value();
  if (strAlign == "right") dwAlignment = XBFONT_RIGHT;
  else if (strAlign == "center") dwAlignment = XBFONT_CENTER_X;
  else dwAlignment = XBFONT_LEFT;
  return true;
}

bool CGUIControlFactory::GetAlignmentY(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment)
{
  TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode || !pNode->FirstChild())
  {
    return false;
  }

  CStdString strAlign = pNode->FirstChild()->Value();

  dwAlignment = 0;
  if (strAlign == "center")
  {
    dwAlignment = XBFONT_CENTER_Y;
  }

  return true;
}


CGUIControl* CGUIControlFactory::Create(DWORD dwParentId, const TiXmlNode* pControlNode, CGUIControl* pReference, RESOLUTION res)
{
  CStdString strType;
  GetString(pControlNode, "type", strType);

  int iPosX = 0, iPosY = 0;
  DWORD dwWidth = 0, dwHeight = 0;
  DWORD dwID = 0, left = 0, right = 0, up = 0, down = 0;
  DWORD dwColorDiffuse = 0xFFFFFFFF;
  bool bVisible = true;
  CStdString strFont = "";
  CStdString strTmp;
  vector<int> vecInfo;
  vector<wstring> vecLabel;
  wstring strLabel;
  DWORD dwTextColor = 0xFFFFFFFF;
  int iUrlSet=0;
  DWORD dwAlign = XBFONT_LEFT;
  DWORD dwAlignY = 0;
  CStdString strTextureFocus, strTextureNoFocus, strTextureUpFocus, strTextureDownFocus;
  CStdString strTextureAltFocus, strTextureAltNoFocus;
  int iToggleSelect;
  DWORD dwDisabledColor = 0xffffffff;;
  int iHyperLink = WINDOW_INVALID;
  DWORD dwItems;
  CStdString strUp, strDown;
  CStdString strUpFocus, strDownFocus;
  DWORD dwSpinColor = 0xffffffff;
  DWORD dwSpinWidth = 16;
  DWORD dwSpinHeight = 16;
  int iSpinPosX, iSpinPosY;
  CStdString strTextureCheckMark;
  CStdString strTextureCheckMarkNF;
  DWORD dwCheckWidth, dwCheckHeight;
  CStdString strTextureRadioFocus, strTextureRadioNoFocus;
  CStdString strSubType;
  int iType = SPIN_CONTROL_TYPE_TEXT;
  int iMin = 0;
  int iMax = 100;
  int iInterval = 1;
  float fMin = 0.0f;
  float fMax = 1.0f;
  float fInterval = 0.1f;
  bool bReverse = false;
  bool bShadow;
  CStdString strTextureBg, strLeft, strRight, strMid, strMidFocus, strOverlay;
  CStdString strLeftFocus, strRightFocus;
  CStdString strTexture;
  DWORD dwColorKey = 0;
  DWORD dwSelectedColor;
  CStdString strButton, strButtonFocus;
  CStdString strSuffix = "";
  CStdString strFont2 = "";

  long lTextOffsetX = 10;
  long lTextOffsetY = 2;
  int iControlOffsetX = 0;
  int iControlOffsetY = 0;

  int iTextXOff = 0;
  int iTextYOff = 0;
  int iTextXOff2 = 0;
  int iTextYOff2 = 0;
  DWORD dwitemWidth = 16, dwitemHeight = 16;
  DWORD textureWidthBig = 128;
  DWORD textureHeightBig = 128;
  DWORD itemWidthBig = 150;
  DWORD itemHeightBig = 150;
  DWORD dwDisposition = 0;
  DWORD dwTextColor2 = dwTextColor;
  DWORD dwTextColor3 = dwTextColor;
  DWORD dwSelectedColor2;
  int iSpace = 2;
  int iTextureHeight = 30;
  CStdString strImage, strImageFocus;
  int iTextureWidth = 80;
  bool bHasPath = false;
  CStdString strExecuteAction = "";
  CStdString strTitle = "";
  CStdString strRSSTags = "";

  DWORD dwThumbAlign = 0;
  DWORD dwThumbWidth = 80;
  DWORD dwThumbHeight = 128;
  DWORD dwThumbSpaceX = 6;
  DWORD dwThumbSpaceY = 25;
  DWORD dwTextSpaceY = 12;
  CStdString strDefaultThumb;

  int iThumbXPos = 4;
  int iThumbYPos = 10;
  int iThumbWidth = 64;
  int iThumbHeight = 64;

  int iThumbXPosBig = 14;
  int iThumbYPosBig = 14;
  int iThumbWidthBig = 100;
  int iThumbHeightBig = 100;
  DWORD dwBuddyControlID = 0;
  int iNumSlots = 7;
  int iButtonGap = 5;
  int iDefaultSlot = 2;
  int iMovementRange = 2;
  bool bHorizontal = false;
  int iAlpha = 0;
  bool bWrapAround = true;
  bool bSmoothScrolling = true;
  bool bKeepAspectRatio = false;
  int iVisibleCondition = 0;
  bool bScrollLabel = false;
  bool bPulse = true;

  /////////////////////////////////////////////////////////////////////////////
  // Read default properties from reference controls
  //

  if (pReference)
  {
    bVisible = pReference->IsVisible();
    dwColorDiffuse = pReference->GetColourDiffuse();
    iPosX = pReference->GetXPosition();
    iPosY = pReference->GetYPosition();
    dwWidth = pReference->GetWidth();
    dwHeight = pReference->GetHeight();
    bPulse = pReference->GetPulseOnSelect();
    iVisibleCondition = pReference->GetVisibleCondition();
    if (strType == "label")
    {
      strFont = ((CGUILabelControl*)pReference)->GetFontName();
      strLabel = ((CGUILabelControl*)pReference)->GetLabel();
      dwTextColor = ((CGUILabelControl*)pReference)->GetTextColor();
      dwAlign = ((CGUILabelControl*)pReference)->m_dwTextAlign;
      dwDisabledColor = ((CGUILabelControl*)pReference)->GetDisabledColor();
      vecInfo = ((CGUILabelControl*)pReference)->GetInfo();
      bScrollLabel = ((CGUILabelControl*)pReference)->GetWidthControl();
    }
    else if (strType == "edit")
    {
      strFont = ((CGUIEditControl*)pReference)->GetFontName();
      strLabel = ((CGUIEditControl*)pReference)->GetLabel();
      dwTextColor = ((CGUIEditControl*)pReference)->GetTextColor();
      dwAlign = ((CGUIEditControl*)pReference)->m_dwTextAlign;
      dwDisabledColor = ((CGUIEditControl*)pReference)->GetDisabledColor();
    }
    else if (strType == "fadelabel")
    {
      strFont = ((CGUIFadeLabelControl*)pReference)->GetFontName();
      dwTextColor = ((CGUIFadeLabelControl*)pReference)->GetTextColor();
      dwAlign = ((CGUIFadeLabelControl*)pReference)->GetAlignment();
      vecInfo = ((CGUIFadeLabelControl*)pReference)->GetInfo();
      vecLabel = ((CGUIFadeLabelControl*)pReference)->GetLabel();
    }
    else if (strType == "rss")
    {
      strFont = ((CGUIRSSControl*)pReference)->GetFontName();
      strRSSTags = ((CGUIRSSControl*)pReference)->GetTags();
      dwTextColor3 = ((CGUIRSSControl*)pReference)->GetChannelTextColor();
      dwTextColor2 = ((CGUIRSSControl*)pReference)->GetHeadlineTextColor();
      dwTextColor = ((CGUIRSSControl*)pReference)->GetNormalTextColor();
    }
    else if (strType == "ram")
    {
      strFont = ((CGUIRAMControl*)pReference)->GetFontName();
      strFont2 = ((CGUIRAMControl*)pReference)->GetFont2Name();
      dwTextColor3 = ((CGUIRAMControl*)pReference)->GetTitleTextColor();
      dwTextColor = ((CGUIRAMControl*)pReference)->GetNormalTextColor();
      dwSelectedColor = ((CGUIRAMControl*)pReference)->GetSelectedTextColor();
      lTextOffsetX = ((CGUIRAMControl*)pReference)->GetTextOffsetX();
      lTextOffsetY = ((CGUIRAMControl*)pReference)->GetTextOffsetY();
      dwTextSpaceY = ((CGUIRAMControl*)pReference)->GetTextSpacing();

      ((CGUIRAMControl*)pReference)->GetThumbAttributes(dwThumbWidth, dwThumbHeight, dwThumbSpaceX, dwThumbSpaceY, strDefaultThumb);

    }
    else if (strType == "console")
    {
      strFont = ((CGUIConsoleControl*)pReference)->GetFontName();
      dwTextColor = ((CGUIConsoleControl*)pReference)->GetPenColor(0);
      dwTextColor2 = ((CGUIConsoleControl*)pReference)->GetPenColor(1);
      dwTextColor3 = ((CGUIConsoleControl*)pReference)->GetPenColor(2);
      dwSelectedColor = ((CGUIConsoleControl*)pReference)->GetPenColor(3);
    }
    else if (strType == "button")
    {
      strTextureFocus = ((CGUIButtonControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUIButtonControl*)pReference)->GetTextureNoFocusName();
      strFont = ((CGUIButtonControl*)pReference)->GetFontName();
      strLabel = ((CGUIButtonControl*)pReference)->GetLabel();
      dwTextColor = ((CGUIButtonControl*)pReference)->GetTextColor();
      dwAlign = ((CGUIButtonControl*)pReference)->GetTextAlign() & 0x00000003;
      dwAlignY = ((CGUIButtonControl*)pReference)->GetTextAlign() & 0x00000004;
      dwDisabledColor = ((CGUIButtonControl*)pReference)->GetDisabledColor() ;
      iHyperLink = ((CGUIButtonControl*)pReference)->GetHyperLink();
      strExecuteAction = ((CGUIButtonControl*)pReference)->GetExecuteAction();
      lTextOffsetX = ((CGUIButtonControl*)pReference)->GetTextOffsetX();
      lTextOffsetY = ((CGUIButtonControl*)pReference)->GetTextOffsetY();
    }
    else if (strType == "conditionalbutton")
    {
      strTextureFocus = ((CGUIConditionalButtonControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUIConditionalButtonControl*)pReference)->GetTextureNoFocusName();
      strFont = ((CGUIConditionalButtonControl*)pReference)->GetFontName();
      strLabel = ((CGUIConditionalButtonControl*)pReference)->GetLabel();
      dwTextColor = ((CGUIConditionalButtonControl*)pReference)->GetTextColor();
      dwAlign = ((CGUIConditionalButtonControl*)pReference)->GetTextAlign() & 0x00000003;
      dwAlignY = ((CGUIConditionalButtonControl*)pReference)->GetTextAlign() & 0x00000004;
      dwDisabledColor = ((CGUIConditionalButtonControl*)pReference)->GetDisabledColor() ;
      iHyperLink = ((CGUIConditionalButtonControl*)pReference)->GetHyperLink();
      strExecuteAction = ((CGUIConditionalButtonControl*)pReference)->GetExecuteAction();
      lTextOffsetX = ((CGUIConditionalButtonControl*)pReference)->GetTextOffsetX();
      lTextOffsetY = ((CGUIConditionalButtonControl*)pReference)->GetTextOffsetY();
    }
    else if (strType == "spinbutton")
    {
      iType = ((CGUISpinButtonControl*)pReference)->GetType();
      strTextureDownFocus = ((CGUISpinButtonControl*)pReference)->GetTextureDownFocusName();
      strTextureUpFocus = ((CGUISpinButtonControl*)pReference)->GetTextureUpFocusName();
      strTextureFocus = ((CGUISpinButtonControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUISpinButtonControl*)pReference)->GetTextureNoFocusName();
      strFont = ((CGUISpinButtonControl*)pReference)->GetFontName();
      dwTextColor = ((CGUISpinButtonControl*)pReference)->GetTextColor();
      dwDisabledColor = ((CGUISpinButtonControl*)pReference)->GetDisabledColor() ;
    }
    else if (strType == "togglebutton")
    {
      strTextureAltFocus = ((CGUIToggleButtonControl*)pReference)->GetTextureAltFocusName();
      strTextureAltNoFocus = ((CGUIToggleButtonControl*)pReference)->GetTextureAltNoFocusName();
      strTextureFocus = ((CGUIToggleButtonControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUIToggleButtonControl*)pReference)->GetTextureNoFocusName();
      strFont = ((CGUIToggleButtonControl*)pReference)->GetFontName();
      strLabel = ((CGUIToggleButtonControl*)pReference)->GetLabel();
      strExecuteAction = ((CGUIToggleButtonControl*)pReference)->GetExecuteAction();
      dwTextColor = ((CGUIToggleButtonControl*)pReference)->GetTextColor();
      dwDisabledColor = ((CGUIToggleButtonControl*)pReference)->GetDisabledColor();
      dwAlign = ((CGUIToggleButtonControl*)pReference)->GetTextAlign() & 0x00000003;
      dwAlignY = ((CGUIToggleButtonControl*)pReference)->GetTextAlign() & 0x00000004;
      iHyperLink = ((CGUIToggleButtonControl*)pReference)->GetHyperLink();
      lTextOffsetX = ((CGUIToggleButtonControl*)pReference)->GetTextOffsetX();
      lTextOffsetY = ((CGUIToggleButtonControl*)pReference)->GetTextOffsetY();
      iToggleSelect = ((CGUIToggleButtonControl*)pReference)->GetToggleSelect();
    }
    else if (strType == "buttonM")
    {
      dwItems = ((CGUIMButtonControl*)pReference)->GetItems();
      strTextureFocus = ((CGUIMButtonControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUIMButtonControl*)pReference)->GetTextureNoFocusName();
      strFont = ((CGUIMButtonControl*)pReference)->GetFontName();
      strLabel = ((CGUIMButtonControl*)pReference)->GetLabel();
      dwTextColor = ((CGUIMButtonControl*)pReference)->GetTextColor();
      dwDisabledColor = ((CGUIMButtonControl*)pReference)->GetDisabledColor() ;
      iHyperLink = ((CGUIMButtonControl*)pReference)->GetHyperLink();
      lTextOffsetX = ((CGUIMButtonControl*)pReference)->GetTextOffsetX();
      lTextOffsetY = ((CGUIMButtonControl*)pReference)->GetTextOffsetY();
    }
    else if (strType == "checkmark")
    {
      bShadow = ((CGUICheckMarkControl*)pReference)->GetShadow();
      strTextureCheckMark = ((CGUICheckMarkControl*)pReference)->GetCheckMarkTextureName();
      strTextureCheckMarkNF = ((CGUICheckMarkControl*)pReference)->GetCheckMarkTextureNameNF();
      dwCheckWidth = ((CGUICheckMarkControl*)pReference)->GetCheckMarkWidth();
      dwCheckHeight = ((CGUICheckMarkControl*)pReference)->GetCheckMarkHeight();
      dwAlign = ((CGUICheckMarkControl*)pReference)->GetAlignment();
      strFont = ((CGUICheckMarkControl*)pReference)->GetFontName();
      strLabel = ((CGUICheckMarkControl*)pReference)->GetLabel();
      dwTextColor = ((CGUICheckMarkControl*)pReference)->GetTextColor();
      dwDisabledColor = ((CGUICheckMarkControl*)pReference)->GetDisabledColor();
    }
    else if (strType == "radiobutton")
    {
      strTextureRadioFocus = ((CGUIRadioButtonControl*)pReference)->GetTextureRadioFocusName();;
      strTextureRadioNoFocus = ((CGUIRadioButtonControl*)pReference)->GetTextureRadioNoFocusName();;
      strTextureFocus = ((CGUIRadioButtonControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUIRadioButtonControl*)pReference)->GetTextureNoFocusName();
      strFont = ((CGUIRadioButtonControl*)pReference)->GetFontName();
      strLabel = ((CGUIRadioButtonControl*)pReference)->GetLabel();
      dwTextColor = ((CGUIRadioButtonControl*)pReference)->GetTextColor();
      dwAlign = ((CGUIRadioButtonControl*)pReference)->GetTextAlign() & 0x00000003;
      dwAlignY = ((CGUIRadioButtonControl*)pReference)->GetTextAlign() & 0x00000004;
      dwDisabledColor = ((CGUIRadioButtonControl*)pReference)->GetDisabledColor() ;
      iHyperLink = ((CGUIRadioButtonControl*)pReference)->GetHyperLink();
      lTextOffsetX = ((CGUIRadioButtonControl*)pReference)->GetTextOffsetX();
      lTextOffsetY = ((CGUIRadioButtonControl*)pReference)->GetTextOffsetY();
    }
    else if (strType == "spincontrol")
    {
      strFont = ((CGUISpinControl*)pReference)->GetFontName();
      dwTextColor = ((CGUISpinControl*)pReference)->GetTextColor();
      dwAlign = ((CGUISpinControl*)pReference)->GetAlignment();
      dwAlignY = ((CGUISpinControl*)pReference)->GetAlignmentY();
      strUp = ((CGUISpinControl*)pReference)->GetTextureUpName();
      strDown = ((CGUISpinControl*)pReference)->GetTextureDownName();
      strUpFocus = ((CGUISpinControl*)pReference)->GetTextureUpFocusName();
      strDownFocus = ((CGUISpinControl*)pReference)->GetTextureDownFocusName();
      iType = ((CGUISpinControl*)pReference)->GetType();
      dwWidth = ((CGUISpinControl*)pReference)->GetSpinWidth();
      dwHeight = ((CGUISpinControl*)pReference)->GetSpinHeight();
      dwDisabledColor = ((CGUISpinControl*)pReference)->GetDisabledColor() ;
      lTextOffsetX = ((CGUISpinControl*)pReference)->GetTextOffsetX();
      lTextOffsetY = ((CGUISpinControl*)pReference)->GetTextOffsetY();
    }
    else if (strType == "slider")
    {
      strTextureBg = ((CGUISliderControl*)pReference)->GetBackGroundTextureName();
      strMid = ((CGUISliderControl*)pReference)->GetBackTextureMidName();
      iControlOffsetX = ((CGUISliderControl*)pReference)->GetControlOffsetX();
      iControlOffsetY = ((CGUISliderControl*)pReference)->GetControlOffsetY();
    }
    else if (strType == "progress")
    {
      strTextureBg = ((CGUIProgressControl*)pReference)->GetBackGroundTextureName();
      strLeft = ((CGUIProgressControl*)pReference)->GetBackTextureLeftName();
      strMid = ((CGUIProgressControl*)pReference)->GetBackTextureMidName();
      strRight = ((CGUIProgressControl*)pReference)->GetBackTextureRightName();
      strOverlay = ((CGUIProgressControl*)pReference)->GetBackTextureOverlayName();
    }
    else if (strType == "image")
    {
      strTexture = ((CGUIImage *)pReference)->GetFileName();
      dwColorKey = ((CGUIImage *)pReference)->GetColorKey();
      bKeepAspectRatio = ((CGUIImage *)pReference)->GetKeepAspectRatio();
      vecInfo.push_back(((CGUIImage *)pReference)->GetInfo());
    }
    else if (strType == "listcontrol")
    {
      strFont = ((CGUIListControl*)pReference)->GetFontName();
      dwSpinWidth = ((CGUIListControl*)pReference)->GetSpinWidth();
      dwSpinHeight = ((CGUIListControl*)pReference)->GetSpinHeight();
      strUp = ((CGUIListControl*)pReference)->GetTextureUpName();
      strDown = ((CGUIListControl*)pReference)->GetTextureDownName();
      strUpFocus = ((CGUIListControl*)pReference)->GetTextureUpFocusName();
      strDownFocus = ((CGUIListControl*)pReference)->GetTextureDownFocusName();
      dwSpinColor = ((CGUIListControl*)pReference)->GetSpinTextColor();
      iSpinPosX = ((CGUIListControl*)pReference)->GetSpinX();
      iSpinPosY = ((CGUIListControl*)pReference)->GetSpinY();
      dwTextColor = ((CGUIListControl*)pReference)->GetTextColor();
      dwSelectedColor = ((CGUIListControl*)pReference)->GetSelectedColor();
      strButton = ((CGUIListControl*)pReference)->GetButtonNoFocusName();
      strButtonFocus = ((CGUIListControl*)pReference)->GetButtonFocusName();
      strSuffix = ((CGUIListControl*)pReference)->GetSuffix();
      iTextXOff = ((CGUIListControl*)pReference)->GetTextOffsetX();
      iTextYOff = ((CGUIListControl*)pReference)->GetTextOffsetY();
      iTextXOff2 = ((CGUIListControl*)pReference)->GetTextOffsetX2();
      iTextYOff2 = ((CGUIListControl*)pReference)->GetTextOffsetY2();
      dwAlignY = ((CGUIListControl*)pReference)->GetAlignmentY();
      dwitemWidth = ((CGUIListControl*)pReference)->GetImageWidth();
      dwitemHeight = ((CGUIListControl*)pReference)->GetImageHeight();
      iTextureHeight = ((CGUIListControl*)pReference)->GetItemHeight();
      iSpace = ((CGUIListControl*)pReference)->GetSpace();
      dwTextColor2 = ((CGUIListControl*)pReference)->GetTextColor2();
      dwSelectedColor2 = ((CGUIListControl*)pReference)->GetSelectedColor2();
      strFont2 = ((CGUIListControl*)pReference)->GetFontName2();
      lTextOffsetX = ((CGUIListControl*)pReference)->GetButtonTextOffsetX();
      lTextOffsetY = ((CGUIListControl*)pReference)->GetButtonTextOffsetY();
    }
    else if (strType == "listcontrolex")
    {
      strFont = ((CGUIListControlEx*)pReference)->GetFontName();
      dwSpinWidth = ((CGUIListControlEx*)pReference)->GetSpinWidth();
      dwSpinHeight = ((CGUIListControlEx*)pReference)->GetSpinHeight();
      strUp = ((CGUIListControlEx*)pReference)->GetTextureUpName();
      strDown = ((CGUIListControlEx*)pReference)->GetTextureDownName();
      strUpFocus = ((CGUIListControlEx*)pReference)->GetTextureUpFocusName();
      strDownFocus = ((CGUIListControlEx*)pReference)->GetTextureDownFocusName();
      dwSpinColor = ((CGUIListControlEx*)pReference)->GetSpinTextColor();
      iSpinPosX = ((CGUIListControlEx*)pReference)->GetSpinX();
      iSpinPosY = ((CGUIListControlEx*)pReference)->GetSpinY();
      dwTextColor = ((CGUIListControlEx*)pReference)->GetTextColor();
      dwSelectedColor = ((CGUIListControlEx*)pReference)->GetSelectedColor();
      strButton = ((CGUIListControlEx*)pReference)->GetButtonNoFocusName();
      strButtonFocus = ((CGUIListControlEx*)pReference)->GetButtonFocusName();
      strSuffix = ((CGUIListControlEx*)pReference)->GetSuffix();
      dwitemWidth = ((CGUIListControlEx*)pReference)->GetImageWidth();
      dwitemHeight = ((CGUIListControlEx*)pReference)->GetImageHeight();
      iTextureHeight = ((CGUIListControlEx*)pReference)->GetItemHeight();
      iSpace = ((CGUIListControlEx*)pReference)->GetSpace();
      dwTextColor2 = ((CGUIListControlEx*)pReference)->GetTextColor2();
      dwSelectedColor2 = ((CGUIListControlEx*)pReference)->GetSelectedColor2();
      strFont2 = ((CGUIListControlEx*)pReference)->GetFontName2();
      lTextOffsetX = ((CGUIListControlEx*)pReference)->GetTextOffsetX();
      lTextOffsetY = ((CGUIListControlEx*)pReference)->GetTextOffsetY();
      dwAlignY = ((CGUIListControlEx*)pReference)->GetAlignmentY();
    }
    else if (strType == "textbox")
    {
      strFont = ((CGUITextBox*)pReference)->GetFontName();
      dwTextColor = ((CGUITextBox*)pReference)->GetTextColor();
      dwSpinWidth = ((CGUITextBox*)pReference)->GetSpinWidth();
      dwSpinHeight = ((CGUITextBox*)pReference)->GetSpinHeight();
      strUp = ((CGUITextBox*)pReference)->GetTextureUpName();
      strDown = ((CGUITextBox*)pReference)->GetTextureDownName();
      strUpFocus = ((CGUITextBox*)pReference)->GetTextureUpFocusName();
      strDownFocus = ((CGUITextBox*)pReference)->GetTextureDownFocusName();
      dwSpinColor = ((CGUITextBox*)pReference)->GetSpinTextColor();
      iSpinPosX = ((CGUITextBox*)pReference)->GetSpinX();
      iSpinPosY = ((CGUITextBox*)pReference)->GetSpinY();
      dwSpinWidth = ((CGUITextBox*)pReference)->GetSpinWidth();
      dwSpinHeight = ((CGUITextBox*)pReference)->GetSpinHeight();
    }
    else if (strType == "thumbnailpanel")
    {
      textureWidthBig = ((CGUIThumbnailPanel*)pReference)->GetTextureWidthBig();
      textureHeightBig = ((CGUIThumbnailPanel*)pReference)->GetTextureHeightBig();
      itemWidthBig = ((CGUIThumbnailPanel*)pReference)->GetItemWidthBig();
      itemHeightBig = ((CGUIThumbnailPanel*)pReference)->GetItemHeightBig();
      strFont = ((CGUIThumbnailPanel*)pReference)->GetFontName();
      strImage = ((CGUIThumbnailPanel*)pReference)->GetNoFocusName();
      strImageFocus = ((CGUIThumbnailPanel*)pReference)->GetFocusName();
      dwitemWidth = ((CGUIThumbnailPanel*)pReference)->GetItemWidth();
      dwitemHeight = ((CGUIThumbnailPanel*)pReference)->GetItemHeight();
      dwSpinWidth = ((CGUIThumbnailPanel*)pReference)->GetSpinWidth();
      dwSpinHeight = ((CGUIThumbnailPanel*)pReference)->GetSpinHeight();
      strUp = ((CGUIThumbnailPanel*)pReference)->GetTextureUpName();
      strDown = ((CGUIThumbnailPanel*)pReference)->GetTextureDownName();
      strUpFocus = ((CGUIThumbnailPanel*)pReference)->GetTextureUpFocusName();
      strDownFocus = ((CGUIThumbnailPanel*)pReference)->GetTextureDownFocusName();
      dwSpinColor = ((CGUIThumbnailPanel*)pReference)->GetSpinTextColor();
      iSpinPosX = ((CGUIThumbnailPanel*)pReference)->GetSpinX();
      iSpinPosY = ((CGUIThumbnailPanel*)pReference)->GetSpinY();
      dwTextColor = ((CGUIThumbnailPanel*)pReference)->GetTextColor();
      dwSelectedColor = ((CGUIThumbnailPanel*)pReference)->GetSelectedColor();
      iTextureWidth = ((CGUIThumbnailPanel*)pReference)->GetTextureWidth();
      iTextureHeight = ((CGUIThumbnailPanel*)pReference)->GetTextureHeight();
      strSuffix = ((CGUIThumbnailPanel*)pReference)->GetSuffix();
      dwThumbAlign = ((CGUIThumbnailPanel*)pReference)->GetThumbAlign();
      ((CGUIThumbnailPanel*)pReference)->GetThumbDimensions(iThumbXPos, iThumbYPos, iThumbWidth, iThumbHeight);
      ((CGUIThumbnailPanel*)pReference)->GetThumbDimensionsBig(iThumbXPosBig, iThumbYPosBig, iThumbWidthBig, iThumbHeightBig);
    }
    else if (strType == "selectbutton")
    {
      strTextureBg = ((CGUISelectButtonControl*)pReference)->GetTextureBackground();
      strLeft = ((CGUISelectButtonControl*)pReference)->GetTextureLeft();
      strLeftFocus = ((CGUISelectButtonControl*)pReference)->GetTextureLeftFocus();
      strRight = ((CGUISelectButtonControl*)pReference)->GetTextureRight();
      strRightFocus = ((CGUISelectButtonControl*)pReference)->GetTextureRightFocus();
      strTextureFocus = ((CGUISelectButtonControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUISelectButtonControl*)pReference)->GetTextureNoFocusName();
      strFont = ((CGUISelectButtonControl*)pReference)->GetFontName();
      strLabel = ((CGUISelectButtonControl*)pReference)->GetLabel();
      dwTextColor = ((CGUISelectButtonControl*)pReference)->GetTextColor();
      dwAlign = ((CGUISelectButtonControl*)pReference)->GetTextAlign() & 0x00000003;
      dwAlignY = ((CGUISelectButtonControl*)pReference)->GetTextAlign() & 0x00000004;
      dwDisabledColor = ((CGUISelectButtonControl*)pReference)->GetDisabledColor() ;
      lTextOffsetX = ((CGUISelectButtonControl*)pReference)->GetTextOffsetX();
      lTextOffsetY = ((CGUISelectButtonControl*)pReference)->GetTextOffsetY();
    }
    else if (strType == "mover")
    {
      strTextureFocus = ((CGUIMoverControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUIMoverControl*)pReference)->GetTextureNoFocusName();
    }
    else if (strType == "resize")
    {
      strTextureFocus = ((CGUIResizeControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUIResizeControl*)pReference)->GetTextureNoFocusName();
    }
    else if (strType == "buttonscroller")
    {
      strTextureFocus = ((CGUIButtonScroller*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUIButtonScroller*)pReference)->GetTextureNoFocusName();
      strFont = ((CGUIButtonScroller*)pReference)->GetFontName();
      dwTextColor = ((CGUIButtonScroller*)pReference)->GetTextColor();
      dwAlign = ((CGUIButtonScroller*)pReference)->GetTextAlign() & 0x00000003;
      dwAlignY = ((CGUIButtonScroller*)pReference)->GetTextAlign() & 0x00000004;
      lTextOffsetX = ((CGUIButtonScroller*)pReference)->GetTextOffsetX();
      lTextOffsetY = ((CGUIButtonScroller*)pReference)->GetTextOffsetY();
      iNumSlots = ((CGUIButtonScroller*)pReference)->GetNumSlots();
      iButtonGap = ((CGUIButtonScroller*)pReference)->GetButtonGap();
      iDefaultSlot = ((CGUIButtonScroller*)pReference)->GetDefaultSlot();
      iMovementRange = ((CGUIButtonScroller*)pReference)->GetMovementRange();
      bHorizontal = ((CGUIButtonScroller*)pReference)->GetHorizontal();
      iAlpha = ((CGUIButtonScroller*)pReference)->GetAlpha();
      bWrapAround = ((CGUIButtonScroller*)pReference)->GetWrapAround();
      bSmoothScrolling = ((CGUIButtonScroller*)pReference)->GetSmoothScrolling();
    }
    else if (strType == "spincontrolex")
    {
      strFont = ((CGUISpinControlEx*)pReference)->GetFontName();
      dwTextColor = ((CGUISpinControlEx*)pReference)->GetTextColor();
      dwAlignY = ((CGUISpinControlEx*)pReference)->GetAlignment();
      strUp = ((CGUISpinControlEx*)pReference)->GetTextureUpName();
      strDown = ((CGUISpinControlEx*)pReference)->GetTextureDownName();
      strUpFocus = ((CGUISpinControlEx*)pReference)->GetTextureUpFocusName();
      strDownFocus = ((CGUISpinControlEx*)pReference)->GetTextureDownFocusName();
      iType = ((CGUISpinControlEx*)pReference)->GetType();
      dwSpinWidth = ((CGUISpinControlEx*)pReference)->GetSpinWidth();
      dwSpinHeight = ((CGUISpinControlEx*)pReference)->GetSpinHeight();
      dwWidth = ((CGUISpinControlEx*)pReference)->GetWidth();
      dwHeight = ((CGUISpinControlEx*)pReference)->GetHeight();
      dwDisabledColor = ((CGUISpinControlEx*)pReference)->GetDisabledColor();
      lTextOffsetX = ((CGUISpinControlEx*)pReference)->GetTextOffsetX();
      lTextOffsetY = ((CGUISpinControlEx*)pReference)->GetTextOffsetY();

      strTextureFocus = ((CGUISpinControlEx*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUISpinControlEx*)pReference)->GetTextureNoFocusName();
      strLabel = ((CGUISpinControlEx*)pReference)->GetLabel();
    }
    else if (strType == "visualisation")
    { // nothing to do here
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // Read control properties from XML
  //

  if (!GetDWORD(pControlNode, "id", dwID))
  {
    return NULL; // NO id????
  }

  if (GetInt(pControlNode, "posX", iPosX)) g_graphicsContext.ScaleXCoord(iPosX, res);
  if (GetInt(pControlNode, "posY", iPosY)) g_graphicsContext.ScaleYCoord(iPosY, res);
  if (GetDWORD(pControlNode, "width", dwWidth)) g_graphicsContext.ScaleXCoord(dwWidth, res);
  if (GetDWORD(pControlNode, "height", dwHeight)) g_graphicsContext.ScaleYCoord(dwHeight, res);
  if (GetLong(pControlNode, "textOffsetX", lTextOffsetX)) g_graphicsContext.ScaleXCoord(lTextOffsetX, res);
  if (GetLong(pControlNode, "textOffsetY", lTextOffsetY)) g_graphicsContext.ScaleYCoord(lTextOffsetY, res);
  if (GetDWORD(pControlNode, "textSpaceY", dwTextSpaceY)) g_graphicsContext.ScaleYCoord(dwTextSpaceY, res);

  if (GetInt(pControlNode, "controlOffsetX", iControlOffsetX)) g_graphicsContext.ScaleXCoord(iControlOffsetX, res);
  if (GetInt(pControlNode, "controlOffsetY", iControlOffsetY)) g_graphicsContext.ScaleYCoord(iControlOffsetY, res);

  if (GetDWORD(pControlNode, "gfxThumbWidth", dwThumbWidth)) g_graphicsContext.ScaleXCoord(dwThumbWidth, res);
  if (GetDWORD(pControlNode, "gfxThumbHeight", dwThumbHeight)) g_graphicsContext.ScaleYCoord(dwThumbHeight, res);
  if (GetDWORD(pControlNode, "gfxThumbSpaceX", dwThumbSpaceX)) g_graphicsContext.ScaleXCoord(dwThumbSpaceX, res);
  if (GetDWORD(pControlNode, "gfxThumbSpaceY", dwThumbSpaceY)) g_graphicsContext.ScaleYCoord(dwThumbSpaceY, res);
  GetString(pControlNode, "gfxThumbDefault", strDefaultThumb);

  if (!GetDWORD(pControlNode, "onup" , up ))
  {
    up = dwID - 1;
  }
  if (!GetDWORD(pControlNode, "ondown" , down))
  {
    down = dwID + 1;
  }
  if (!GetDWORD(pControlNode, "onleft" , left ))
  {
    left = dwID;
  }
  if (!GetDWORD(pControlNode, "onright", right))
  {
    right = dwID;
  }

  GetHex(pControlNode, "colordiffuse", dwColorDiffuse);
  if (!GetBoolean(pControlNode, "visible", bVisible))
  { // try a conditional visibility
    CStdString strVisible;
    GetString(pControlNode, "visible", strVisible);
    int iVisible = g_infoManager.TranslateString(strVisible);
    if (iVisible != 0)
      iVisibleCondition = iVisible;
  }
  GetString(pControlNode, "font", strFont);
  GetAlignment(pControlNode, "align", dwAlign);
  GetAlignmentY(pControlNode, "alignY", dwAlignY);
  GetInt(pControlNode, "hyperlink", iHyperLink);
  // windows are referenced from WINDOW_HOME
  if (iHyperLink != WINDOW_INVALID) iHyperLink += WINDOW_HOME;

  GetString(pControlNode, "script", strExecuteAction); // left in for backwards compatibility.
  GetString(pControlNode, "execute", strExecuteAction);

  CStdStringArray strVecInfo;
  if (GetMultipleString(pControlNode, "info", strVecInfo))
  {
    vecInfo.clear();
    for (unsigned int i = 0; i < strVecInfo.size(); i++)
    {
      int info = g_infoManager.TranslateString(strVecInfo[i]);
      if (info)
        vecInfo.push_back(info);
    }
  }
  GetHex(pControlNode, "disabledcolor", dwDisabledColor);
  GetPath(pControlNode, "textureDownFocus", strTextureDownFocus);
  GetPath(pControlNode, "textureUpFocus", strTextureUpFocus);
  GetPath(pControlNode, "textureFocus", strTextureFocus);
  GetPath(pControlNode, "textureNoFocus", strTextureNoFocus);
  GetPath(pControlNode, "AltTextureFocus", strTextureAltFocus);
  GetPath(pControlNode, "AltTextureNoFocus", strTextureAltNoFocus);
  CStdString strToggleSelect;
  GetString(pControlNode, "UseAltTexture", strToggleSelect);
  iToggleSelect = g_infoManager.TranslateString(strToggleSelect);
  GetDWORD(pControlNode, "bitmaps", dwItems);
  GetHex(pControlNode, "textcolor", dwTextColor);

  GetBoolean(pControlNode, "hasPath", bHasPath);
  GetBoolean(pControlNode, "shadow", bShadow);

  GetPath(pControlNode, "textureUp", strUp);
  GetPath(pControlNode, "textureDown", strDown);
  GetPath(pControlNode, "textureUpFocus", strUpFocus);
  GetPath(pControlNode, "textureDownFocus", strDownFocus);

  GetPath(pControlNode, "textureLeft", strLeft);
  GetPath(pControlNode, "textureRight", strRight);
  GetPath(pControlNode, "textureLeftFocus", strLeftFocus);
  GetPath(pControlNode, "textureRightFocus", strRightFocus);

  GetHex(pControlNode, "spinColor", dwSpinColor);
  if (GetDWORD(pControlNode, "spinWidth", dwSpinWidth)) g_graphicsContext.ScaleXCoord(dwSpinWidth, res);
  if (GetDWORD(pControlNode, "spinHeight", dwSpinHeight)) g_graphicsContext.ScaleYCoord(dwSpinHeight, res);
  if (GetInt(pControlNode, "spinPosX", iSpinPosX)) g_graphicsContext.ScaleXCoord(iSpinPosX, res);
  if (GetInt(pControlNode, "spinPosY", iSpinPosY)) g_graphicsContext.ScaleYCoord(iSpinPosY, res);

  if (GetDWORD(pControlNode, "MarkWidth", dwCheckWidth)) g_graphicsContext.ScaleXCoord(dwCheckWidth, res);
  if (GetDWORD(pControlNode, "MarkHeight", dwCheckHeight)) g_graphicsContext.ScaleYCoord(dwCheckHeight, res);
  GetPath(pControlNode, "textureCheckmark", strTextureCheckMark);
  GetPath(pControlNode, "textureCheckmarkNoFocus", strTextureCheckMarkNF);
  GetPath(pControlNode, "textureRadioFocus", strTextureRadioFocus);
  GetPath(pControlNode, "textureRadioNoFocus", strTextureRadioNoFocus);

  GetPath(pControlNode, "textureSliderBar", strTextureBg);
  GetPath(pControlNode, "textureSliderNib", strMid);
  GetPath(pControlNode, "textureSliderNibFocus", strMidFocus);
  GetDWORD(pControlNode, "disposition", dwDisposition);

  GetString(pControlNode, "title", strTitle);
  GetString(pControlNode, "tagset", strRSSTags);
  GetHex(pControlNode, "headlinecolor", dwTextColor2);
  GetHex(pControlNode, "titlecolor", dwTextColor3);

  if (GetString(pControlNode, "subtype", strSubType))
  {
    strSubType.ToLower();

    if ( strSubType == "int")
    {
      iType = SPIN_CONTROL_TYPE_INT;
    }
    else if ( strSubType == "float")
    {
      iType = SPIN_CONTROL_TYPE_FLOAT;
    }
    else
    {
      iType = SPIN_CONTROL_TYPE_TEXT;
    }
  }

  if (!GetIntRange(pControlNode, "range", iMin, iMax, iInterval))
  {
    GetFloatRange(pControlNode, "range", fMin, fMax, fInterval);
  }

  GetBoolean(pControlNode, "reverse", bReverse);

  GetPath(pControlNode, "texturebg", strTextureBg);
  GetPath(pControlNode, "lefttexture", strLeft);
  GetPath(pControlNode, "midtexture", strMid);
  GetPath(pControlNode, "righttexture", strRight);
  GetPath(pControlNode, "overlaytexture", strOverlay);
  GetPath(pControlNode, "texture", strTexture);
  GetHex(pControlNode, "colorkey", dwColorKey);

  GetHex(pControlNode, "selectedColor", dwSelectedColor);
  dwSelectedColor2 = dwSelectedColor;

  GetPath(pControlNode, "textureNoFocus", strButton);
  GetPath(pControlNode, "textureFocus", strButtonFocus);
  GetString(pControlNode, "suffix", strSuffix);
  if (GetInt(pControlNode, "textXOff", iTextXOff)) g_graphicsContext.ScaleXCoord(iTextXOff, res);
  if (GetInt(pControlNode, "textYOff", iTextYOff)) g_graphicsContext.ScaleYCoord(iTextYOff, res);
  if (GetInt(pControlNode, "textXOff2", iTextXOff2)) g_graphicsContext.ScaleXCoord(iTextXOff2, res);
  if (GetInt(pControlNode, "textYOff2", iTextYOff2)) g_graphicsContext.ScaleYCoord(iTextYOff2, res);

  if (GetDWORD(pControlNode, "itemWidth", dwitemWidth)) g_graphicsContext.ScaleXCoord(dwitemWidth, res);
  if (GetDWORD(pControlNode, "itemHeight", dwitemHeight)) g_graphicsContext.ScaleYCoord(dwitemHeight, res);
  if (GetInt(pControlNode, "spaceBetweenItems", iSpace)) g_graphicsContext.ScaleYCoord(iSpace, res);

  GetHex(pControlNode, "selectedColor2", dwSelectedColor2);
  GetHex(pControlNode, "textcolor2", dwTextColor2);
  GetString(pControlNode, "font2", strFont2);
  GetHex(pControlNode, "selectedColor", dwSelectedColor);

  GetPath(pControlNode, "imageFolder", strImage);
  GetPath(pControlNode, "imageFolderFocus", strImageFocus);
  if (GetInt(pControlNode, "textureWidth", iTextureWidth)) g_graphicsContext.ScaleXCoord(iTextureWidth, res);
  if (GetInt(pControlNode, "textureHeight", iTextureHeight)) g_graphicsContext.ScaleYCoord(iTextureHeight, res);

  if (GetInt(pControlNode, "thumbWidth", iThumbWidth)) g_graphicsContext.ScaleXCoord(iThumbWidth, res);
  if (GetInt(pControlNode, "thumbHeight", iThumbHeight)) g_graphicsContext.ScaleYCoord(iThumbHeight, res);
  if (GetInt(pControlNode, "thumbPosX", iThumbXPos)) g_graphicsContext.ScaleXCoord(iThumbXPos, res);
  if (GetInt(pControlNode, "thumbPosY", iThumbYPos)) g_graphicsContext.ScaleYCoord(iThumbYPos, res);

  GetAlignment(pControlNode, "thumbAlign", dwThumbAlign);

  if (GetInt(pControlNode, "thumbWidthBig", iThumbWidthBig)) g_graphicsContext.ScaleXCoord(iThumbWidthBig, res);
  if (GetInt(pControlNode, "thumbHeightBig", iThumbHeightBig)) g_graphicsContext.ScaleYCoord(iThumbHeightBig, res);
  if (GetInt(pControlNode, "thumbPosXBig", iThumbXPosBig)) g_graphicsContext.ScaleXCoord(iThumbXPosBig, res);
  if (GetInt(pControlNode, "thumbPosYBig", iThumbYPosBig)) g_graphicsContext.ScaleYCoord(iThumbYPosBig, res);

  if (GetDWORD(pControlNode, "textureWidthBig", textureWidthBig)) g_graphicsContext.ScaleXCoord(textureWidthBig, res);
  if (GetDWORD(pControlNode, "textureHeightBig", textureHeightBig)) g_graphicsContext.ScaleYCoord(textureHeightBig, res);
  if (GetDWORD(pControlNode, "itemWidthBig", itemWidthBig)) g_graphicsContext.ScaleXCoord(itemWidthBig, res);
  if (GetDWORD(pControlNode, "itemHeightBig", itemHeightBig)) g_graphicsContext.ScaleYCoord(itemHeightBig, res);
  GetDWORD(pControlNode, "buddycontrolid", dwBuddyControlID);

  CStdStringArray strVecLabel;
  if (GetMultipleString(pControlNode, "label", strVecLabel))
  {
    vecLabel.clear();
    for (unsigned int i = 0; i < strVecLabel.size(); i++)
    {
      strTmp = strVecLabel[i];
      if (strTmp.size() > 0)
      {
        if (strTmp[0] != '-')
        {
          if (CUtil::IsNaturalNumber(strTmp))
          {
            DWORD dwLabelID = atol(strTmp);
            strLabel = g_localizeStrings.Get(dwLabelID);
          }
          else
          {
            WCHAR wszTmp[256];
            swprintf(wszTmp, L"%S", strTmp.c_str());
            strLabel = wszTmp;
          }
          vecLabel.push_back(strLabel);
        }
        else
          strLabel = L"";
      }
    }
  }

  GetInt(pControlNode,"urlset",iUrlSet);

  /*  CStdStringArray strVecUrl;
  if (GetMultipleString(pControlNode, "feed", strVecUrl))
  {
    vecUrls.clear();
    for (unsigned int i = 0; i < strVecUrl.size(); i++)
    {
      strTmp = strVecUrl[i];
      if (strTmp.size() > 0)
      {
        WCHAR wszTmp[1024];
        swprintf(wszTmp, L"%S", strTmp.c_str());
        wstring tempUrl = wszTmp;
        vecUrls.push_back(tempUrl);
      }
    }
  }*/

  // stuff for button scroller
  if ( GetString(pControlNode, "orientation", strTmp) )
  {
    if (strTmp.ToLower() == "horizontal")
      bHorizontal = true;
  }
  if (GetInt(pControlNode, "buttongap", iButtonGap))
  {
    if (bHorizontal)
      g_graphicsContext.ScaleXCoord(iButtonGap, res);
    else
      g_graphicsContext.ScaleYCoord(iButtonGap, res);
  }
  GetInt(pControlNode, "numbuttons", iNumSlots);
  GetInt(pControlNode, "movement", iMovementRange);
  GetInt(pControlNode, "defaultbutton", iDefaultSlot);
  GetInt(pControlNode, "alpha", iAlpha);
  GetBoolean(pControlNode, "wraparound", bWrapAround);
  GetBoolean(pControlNode, "smoothscrolling", bSmoothScrolling);
  GetBoolean(pControlNode, "keepaspectratio", bKeepAspectRatio);
  GetBoolean(pControlNode, "scroll", bScrollLabel);
  GetBoolean(pControlNode,"pulseonselect", bPulse);

  /////////////////////////////////////////////////////////////////////////////
  // Instantiate a new control using the properties gathered above
  //

  if (strType == "label")
  {
    CGUILabelControl* pControl = new CGUILabelControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strFont, strLabel, dwTextColor, dwDisabledColor,
      dwAlign, bHasPath);

    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetInfo(vecInfo);
    pControl->SetWidthControl(bScrollLabel);
    return pControl;
  }
  else if (strType == "edit")
  {
    CGUIEditControl* pControl = new CGUIEditControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strFont, strLabel, dwTextColor, dwDisabledColor);

    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    return pControl;
  }
  else if (strType == "videowindow")
  {
    CGUIVideoControl* pControl = new CGUIVideoControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight);
    pControl->SetVisibleCondition(iVisibleCondition);
    return pControl;
  }
  else if (strType == "fadelabel")
  {
    CGUIFadeLabelControl* pControl = new CGUIFadeLabelControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strFont, dwTextColor, (dwAlign|dwAlignY));

    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetLabel(vecLabel);
    pControl->SetInfo(vecInfo);
    return pControl;
  }
  else if (strType == "spinbutton")
  {
    CGUISpinButtonControl* pControl = new CGUISpinButtonControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureNoFocus, strTextureFocus,
      strTextureUpFocus, strTextureDownFocus,
      strFont, dwTextColor, (int)dwDisposition);

    pControl->SetDisabledColor(dwDisabledColor);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "rss")
  {
    CGUIRSSControl* pControl = new CGUIRSSControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strFont, dwTextColor3, dwTextColor2, dwTextColor, strRSSTags);

    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    std::map<int, std::pair<std::vector<int>,std::vector<wstring> > >::iterator iter=g_settings.m_mapRssUrls.find(iUrlSet);
    if (iter != g_settings.m_mapRssUrls.end())
    {
      pControl->SetUrls(iter->second.second);
      pControl->SetIntervals(iter->second.first);
    }
    else
      CLog::Log(LOGERROR,"invalid rss url set referenced in skin");
    return pControl;
  }
  else if (strType == "ram")
  {
    CGUIRAMControl* pControl = new CGUIRAMControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strFont, strFont2, dwTextColor3, dwTextColor,
      dwSelectedColor, lTextOffsetX, lTextOffsetY);

    pControl->SetTextSpacing(dwTextSpaceY);
    pControl->SetThumbAttributes(dwThumbWidth, dwThumbHeight, dwThumbSpaceX, dwThumbSpaceY, strDefaultThumb);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "console")
  {
    CGUIConsoleControl* pControl = new CGUIConsoleControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strFont, dwTextColor, dwTextColor2, dwTextColor3, dwSelectedColor);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetNavigation(up, down, left, right);
    return pControl;
  }
  else if (strType == "button")
  {
    CGUIButtonControl* pControl = new CGUIButtonControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureFocus, strTextureNoFocus,
      lTextOffsetX, lTextOffsetY, (dwAlign | dwAlignY));

    pControl->SetLabel(strFont, strLabel, dwTextColor);
    pControl->SetDisabledColor(dwDisabledColor);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetHyperLink(iHyperLink);
    pControl->SetExecuteAction(strExecuteAction);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "conditionalbutton")
  {
    CGUIConditionalButtonControl* pControl = new CGUIConditionalButtonControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureFocus, strTextureNoFocus,
      lTextOffsetX, lTextOffsetY, (dwAlign | dwAlignY));

    pControl->SetLabel(strFont, strLabel, dwTextColor);
    pControl->SetDisabledColor(dwDisabledColor);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetHyperLink(iHyperLink);
    pControl->SetExecuteAction(strExecuteAction);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "togglebutton")
  {
    CGUIToggleButtonControl* pControl = new CGUIToggleButtonControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureFocus, strTextureNoFocus,
      strTextureAltFocus, strTextureAltNoFocus, lTextOffsetX, lTextOffsetY, (dwAlign | dwAlignY));

    pControl->SetLabel(strFont, strLabel, dwTextColor);
    pControl->SetDisabledColor(dwDisabledColor);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetHyperLink(iHyperLink);
    pControl->SetExecuteAction(strExecuteAction);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetToggleSelect(iToggleSelect);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "buttonM")
  {
    CGUIMButtonControl* pControl = new CGUIMButtonControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      dwItems, strTextureFocus, strTextureNoFocus,
      lTextOffsetX, lTextOffsetY);

    pControl->SetLabel(strFont, strLabel, dwTextColor);
    pControl->SetDisabledColor(dwDisabledColor);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetHyperLink(iHyperLink);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "checkmark")
  {
    CGUICheckMarkControl* pControl = new CGUICheckMarkControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureCheckMark, strTextureCheckMarkNF,
      dwCheckWidth, dwCheckHeight, dwAlign);

    pControl->SetLabel(strFont, strLabel, dwTextColor);
    pControl->SetDisabledColor(dwDisabledColor);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetShadow(bShadow);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "radiobutton")
  {
    CGUIRadioButtonControl* pControl = new CGUIRadioButtonControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureFocus, strTextureNoFocus,
      lTextOffsetX, lTextOffsetY, (dwAlign | dwAlignY),
      strTextureRadioFocus, strTextureRadioNoFocus);

    pControl->SetLabel(strFont, strLabel, dwTextColor);
    pControl->SetDisabledColor(dwDisabledColor);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetHyperLink(iHyperLink);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "spincontrol")
  {
    CGUISpinControl* pControl = new CGUISpinControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strUp, strDown, strUpFocus, strDownFocus,
      strFont, dwTextColor, iType, dwAlign);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetReverse(bReverse);
    pControl->SetBuddyControlID(dwBuddyControlID);
    pControl->SetDisabledColor(dwDisabledColor);
    pControl->SetTextOffsetX(lTextOffsetX);
    pControl->SetTextOffsetY(lTextOffsetY);
    pControl->SetAlignmentY(dwAlignY);
    pControl->SetPulseOnSelect(bPulse);

    if (iType == SPIN_CONTROL_TYPE_INT)
    {
      pControl->SetRange(iMin, iMax);
    }
    else if (iType == SPIN_CONTROL_TYPE_FLOAT)
    {
      pControl->SetFloatRange(fMin, fMax);
      pControl->SetFloatInterval(fInterval);
    }

    return pControl;
  }
  else if (strType == "slider")
  {
    CGUISliderControl* pControl = new CGUISliderControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureBg, strMid, strMidFocus, iType);

    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetInfo(vecInfo.size() ? vecInfo[0] : 0);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetControlOffsetX(iControlOffsetX);
    pControl->SetControlOffsetY(iControlOffsetY);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "progress")
  {
    CGUIProgressControl* pControl = new CGUIProgressControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureBg, strLeft, strMid, strRight, strOverlay);

    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetInfo(vecInfo.size() ? vecInfo[0] : 0);
    return pControl;
  }
  else if (strType == "image")
  {
    CGUIImage* pControl = new CGUIImage(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight, strTexture, dwColorKey);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetKeepAspectRatio(bKeepAspectRatio);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetInfo(vecInfo.size() ? vecInfo[0] : 0);
    return pControl;
  }
  else if (strType == "listcontrol")
  {
    CGUIListControl* pControl = new CGUIListControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strFont,
      dwSpinWidth, dwSpinHeight,
      strUp, strDown,
      strUpFocus, strDownFocus,
      dwSpinColor, iSpinPosX, iSpinPosY,
      strFont, dwTextColor, dwSelectedColor,
      strButton, strButtonFocus,
      lTextOffsetX, lTextOffsetY);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetScrollySuffix(strSuffix);
    pControl->SetTextOffsets(iTextXOff, iTextYOff, iTextXOff2, iTextYOff2);
    pControl->SetAlignmentY(dwAlignY);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetImageDimensions(dwitemWidth, dwitemHeight);
    pControl->SetItemHeight(iTextureHeight);
    pControl->SetSpace(iSpace);
    pControl->SetColors2(dwTextColor2, dwSelectedColor2);
    pControl->SetFont2( strFont2 );
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "listcontrolex")
  {
    CGUIListControlEx* pControl = new CGUIListControlEx(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strFont,
      dwSpinWidth, dwSpinHeight,
      strUp, strDown,
      strUpFocus, strDownFocus,
      dwSpinColor, iSpinPosX, iSpinPosY,
      strFont, dwTextColor, dwSelectedColor,
      strButton, strButtonFocus,
      lTextOffsetX, lTextOffsetY, dwAlignY);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetScrollySuffix(strSuffix);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetImageDimensions(dwitemWidth, dwitemHeight);
    pControl->SetItemHeight(iTextureHeight);
    pControl->SetSpace(iSpace);
    pControl->SetColors2(dwTextColor2, dwSelectedColor2);
    pControl->SetFont2( strFont2 );
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "textbox")
  {
    CGUITextBox* pControl = new CGUITextBox(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strFont,
      dwSpinWidth, dwSpinHeight,
      strUp, strDown,
      strUpFocus, strDownFocus,
      dwSpinColor, iSpinPosX, iSpinPosY,
      strFont, dwTextColor);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "thumbnailpanel")
  {
    CGUIThumbnailPanel* pControl = new CGUIThumbnailPanel(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strFont,
      strImage, strImageFocus,
      dwitemWidth, dwitemHeight,
      dwSpinWidth, dwSpinHeight,
      strUp, strDown,
      strUpFocus, strDownFocus,
      dwSpinColor, iSpinPosX, iSpinPosY,
      strFont, dwTextColor, dwSelectedColor);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetScrollySuffix(strSuffix);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetTextureDimensions(iTextureWidth, iTextureHeight);
    pControl->SetThumbDimensions(iThumbXPos, iThumbYPos, iThumbWidth, iThumbHeight);
    pControl->SetTextureWidthBig(textureWidthBig);
    pControl->SetTextureHeightBig(textureHeightBig);
    pControl->SetItemWidthBig(itemWidthBig);
    pControl->SetItemHeightBig(itemHeightBig);
    pControl->SetTextureWidthLow(iTextureWidth);
    pControl->SetTextureHeightLow(iTextureHeight);
    pControl->SetItemWidthLow(dwitemWidth);
    pControl->SetItemHeightLow(dwitemHeight);
    pControl->SetThumbDimensionsLow(iThumbXPos, iThumbYPos, iThumbWidth, iThumbHeight);
    pControl->SetThumbDimensionsBig(iThumbXPosBig, iThumbYPosBig, iThumbWidthBig, iThumbHeightBig);
    pControl->SetThumbAlign(dwThumbAlign);
    pControl->SetPulseOnSelect(bPulse);

    return pControl;
  }
  else if (strType == "selectbutton")
  {
    CGUISelectButtonControl* pControl = new CGUISelectButtonControl(
      dwParentId, dwID, iPosX, iPosY,
      dwWidth, dwHeight, strTextureFocus, strTextureNoFocus,
      lTextOffsetX, lTextOffsetY, (dwAlign | dwAlignY),
      strTextureBg, strLeft, strLeftFocus, strRight, strRightFocus);

    pControl->SetLabel(strFont, strLabel, dwTextColor);
    pControl->SetDisabledColor(dwDisabledColor);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "mover")
  {
    CGUIMoverControl* pControl = new CGUIMoverControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureFocus, strTextureNoFocus);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "resize")
  {
    CGUIResizeControl* pControl = new CGUIResizeControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureFocus, strTextureNoFocus);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "buttonscroller")
  {
    CGUIButtonScroller* pControl = new CGUIButtonScroller(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight, iButtonGap, iNumSlots, iDefaultSlot,
      iMovementRange, bHorizontal, iAlpha, bWrapAround, bSmoothScrolling,
      strTextureFocus, strTextureNoFocus, lTextOffsetX, lTextOffsetY, dwAlign | dwAlignY);
    pControl->SetFont(strFont, dwTextColor);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "spincontrolex")
  {
    CGUISpinControlEx* pControl = new CGUISpinControlEx(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight, dwSpinWidth, dwSpinHeight,
      strTextureFocus, strTextureNoFocus, strUp, strDown, strUpFocus, strDownFocus,
      strFont, dwTextColor, lTextOffsetX, lTextOffsetY, dwAlignY, iType);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition);
    pControl->SetReverse(bReverse);
    pControl->SetDisabledColor(dwDisabledColor);
    pControl->SetLabel(strLabel);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "visualisation")
  {
    CGUIVisualisationControl* pControl = new CGUIVisualisationControl(dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight);
    pControl->SetVisibleCondition(iVisibleCondition);
    return pControl;
  }
  return NULL;
}
