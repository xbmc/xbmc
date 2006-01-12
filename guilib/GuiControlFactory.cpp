#include "include.h"
#include "GUIControlFactory.h"
#include "LocalizeStrings.h"
#include "GUIButtoncontrol.h"
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
#include "GUIVisualisationControl.h"
#include "GUISettingsSliderControl.h"
#include "GUIMultiImage.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "../xbmc/util.h"
#include "../xbmc/ButtonTranslator.h"
#include "XMLUtils.h"
#include "GUIFontManager.h"

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "SkinInfo.h"
#include "GUIConditionalButtonControl.h"
#endif

CGUIControlFactory::CGUIControlFactory(void)
{}

CGUIControlFactory::~CGUIControlFactory(void)
{}

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
  TiXmlNode* pNode = pRootNode->FirstChild(strTag);
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

bool CGUIControlFactory::GetConditionalVisibility(const TiXmlNode* control, int &condition, bool &allowHiddenFocus, bool &startHidden)
{
  TiXmlElement* node = control->FirstChildElement("visible");
  if (!node) return false;
  const char *hidden = node->Attribute("allowhiddenfocus");
  if (hidden && strcmpi(hidden, "true") == 0)
    allowHiddenFocus = true;
  if (g_SkinInfo.GetVersion() < 1.90)
  {
    const char *hidden = node->Attribute("start");
    if (hidden && strcmpi(hidden, "hidden") == 0)
      startHidden = true;
  }
  if (!node->NoChildren())
    condition = g_infoManager.TranslateString(node->FirstChild()->Value());
  return (condition != 0);
}

bool CGUIControlFactory::GetConditionalVisibility(const TiXmlNode *control, int &condition)
{
  bool allowHiddenFocus, startHidden;
  return GetConditionalVisibility(control, condition, allowHiddenFocus, startHidden);
}

bool CGUIControlFactory::GetAnimations(const TiXmlNode *control, vector<CAnimation> &animations, RESOLUTION res)
{
  if (g_SkinInfo.GetVersion() < 1.90)
  { // versions earlier than 1.90 used animations only on the visible condition
    TiXmlElement *node = control->FirstChildElement("visible");
    if (!node) return false;
    CVisibleEffect effect;
    effect.Create(node);
    // translate to animation effects
    {
      CAnimation anim;
      anim.type = ANIM_TYPE_VISIBLE;
      anim.effect = effect.m_type;
      anim.delay = effect.m_inDelay;
      anim.length = effect.m_inTime;
      anim.startX = effect.m_startX;
      anim.startY = effect.m_startY;
      // old effect acceleration was in fact deceleration on normal effects
      anim.acceleration = -effect.m_acceleration;
      anim.startAlpha = 0;
      anim.endAlpha = 100;
      animations.push_back(anim);
    }
    {
      CAnimation anim;
      anim.type = ANIM_TYPE_HIDDEN;
      anim.effect = effect.m_type;
      anim.delay = effect.m_outDelay;
      anim.length = effect.m_outTime;
      anim.endX = effect.m_startX;
      anim.endY = effect.m_startY;
      anim.acceleration = effect.m_acceleration;
      anim.startAlpha = 100;
      anim.endAlpha = 0;
      animations.push_back(anim);
    }
    return true;
  }
  else
  {
    TiXmlElement* node = control->FirstChildElement("animation");
    bool ret = false;
    while (node)
    {
      ret = true;
      if (node->FirstChild())
      {
        CAnimation anim;
        anim.Create(node, res);
        animations.push_back(anim);
        if (strcmpi(node->FirstChild()->Value(), "VisibleChange") == 0)
        { // add the hidden one as well
          CAnimation anim2;
          anim2.CreateReverse(anim);
          animations.push_back(anim2);
        }
      }
      node = node->NextSiblingElement("animation");
    }
    return ret;
  }
}

CGUIControl* CGUIControlFactory::Create(DWORD dwParentId, const TiXmlNode* pControlNode, CGUIControl* pReference, RESOLUTION res)
{
  CStdString strType;
  XMLUtils::GetString(pControlNode, "type", strType);

  int iPosX = 0, iPosY = 0;
  DWORD dwWidth = 0, dwHeight = 0;
  DWORD dwID = 0, left = 0, right = 0, up = 0, down = 0;
  DWORD dwColorDiffuse = 0xFFFFFFFF;
  bool bVisible = true;
  CStdString strTmp;
  vector<int> vecInfo;
  vector<wstring> vecLabel;
  wstring strLabel;
  int iUrlSet=0;
  CStdString strTextureFocus, strTextureNoFocus;
  CStdString strTextureAltFocus, strTextureAltNoFocus;
  int iToggleSelect;
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
  CStdString strTextureBg, strLeft, strRight, strMid, strMidFocus, strOverlay;
  CStdString strLeftFocus, strRightFocus;
  CStdString strTexture;
  DWORD dwColorKey = 0;
  CStdString strSuffix = "";

  int iControlOffsetX = 0;
  int iControlOffsetY = 0;

  DWORD dwitemWidth = 16, dwitemHeight = 16;
  DWORD dwSliderWidth = 150, dwSliderHeight = 16;
  DWORD textureWidthBig = 128;
  DWORD textureHeightBig = 128;
  DWORD itemWidthBig = 150;
  DWORD itemHeightBig = 150;
  DWORD dwDisposition = 0;

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
  bool allowHiddenFocus = false;
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  bool startHidden = false;
#endif
  vector<CAnimation> animations;

  bool bScrollLabel = false;
  bool bPulse = true;
  CStdString texturePath;
  DWORD timePerImage = 0;
  DWORD fadeTime = 0;
  bool randomized = false;
  bool loop = true;

  CLabelInfo labelInfo;
  CLabelInfo labelInfo2;
  DWORD dwTextColor3 = labelInfo.textColor;

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
      strLabel = ((CGUILabelControl*)pReference)->GetLabel();
      labelInfo = ((CGUILabelControl*)pReference)->GetLabelInfo();
      vecInfo = ((CGUILabelControl*)pReference)->GetInfo();
      bScrollLabel = ((CGUILabelControl*)pReference)->GetWidthControl();
    }
    else if (strType == "edit")
    {
      labelInfo = ((CGUIEditControl*)pReference)->GetLabelInfo();
      strLabel = ((CGUIEditControl*)pReference)->GetLabel();
    }
    else if (strType == "fadelabel")
    {
      labelInfo = ((CGUIFadeLabelControl*)pReference)->GetLabelInfo();
      vecInfo = ((CGUIFadeLabelControl*)pReference)->GetInfo();
      vecLabel = ((CGUIFadeLabelControl*)pReference)->GetLabel();
    }
    else if (strType == "rss")
    {
      labelInfo = ((CGUIRSSControl*)pReference)->GetLabelInfo();
      strRSSTags = ((CGUIRSSControl*)pReference)->GetTags();
      dwTextColor3 = ((CGUIRSSControl*)pReference)->GetChannelTextColor();
      labelInfo2.textColor = ((CGUIRSSControl*)pReference)->GetHeadlineTextColor();
    }
    else if (strType == "ram")
    {
      labelInfo = ((CGUIRAMControl*)pReference)->GetLabelInfo();
      labelInfo2 = ((CGUIRAMControl*)pReference)->GetTitleInfo();
      dwTextSpaceY = ((CGUIRAMControl*)pReference)->GetTextSpacing();

      ((CGUIRAMControl*)pReference)->GetThumbAttributes(dwThumbWidth, dwThumbHeight, dwThumbSpaceX, dwThumbSpaceY, strDefaultThumb);

    }
    else if (strType == "console")
    {
      labelInfo = ((CGUIConsoleControl*)pReference)->GetLabelInfo();
      labelInfo.textColor = ((CGUIConsoleControl*)pReference)->GetPenColor(0);
      labelInfo2.textColor = ((CGUIConsoleControl*)pReference)->GetPenColor(1);
      dwTextColor3 = ((CGUIConsoleControl*)pReference)->GetPenColor(2);
      labelInfo.selectedColor = ((CGUIConsoleControl*)pReference)->GetPenColor(3);
    }
    else if (strType == "button")
    {
      strTextureFocus = ((CGUIButtonControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUIButtonControl*)pReference)->GetTextureNoFocusName();
      strLabel = ((CGUIButtonControl*)pReference)->GetLabel();
      labelInfo = ((CGUIButtonControl*)pReference)->GetLabelInfo();
      iHyperLink = ((CGUIButtonControl*)pReference)->GetHyperLink();
      strExecuteAction = ((CGUIButtonControl*)pReference)->GetExecuteAction();
    }
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
    else if (g_SkinInfo.GetVersion() < 1.85 && strType == "conditionalbutton")
    {
      strTextureFocus = ((CGUIConditionalButtonControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUIConditionalButtonControl*)pReference)->GetTextureNoFocusName();
      labelInfo = ((CGUIConditionalButtonControl*)pReference)->GetLabelInfo();
      strLabel = ((CGUIConditionalButtonControl*)pReference)->GetLabel();
      iHyperLink = ((CGUIConditionalButtonControl*)pReference)->GetHyperLink();
      strExecuteAction = ((CGUIConditionalButtonControl*)pReference)->GetExecuteAction();
    }
#endif
    else if (strType == "togglebutton")
    {
      strTextureAltFocus = ((CGUIToggleButtonControl*)pReference)->GetTextureAltFocusName();
      strTextureAltNoFocus = ((CGUIToggleButtonControl*)pReference)->GetTextureAltNoFocusName();
      strTextureFocus = ((CGUIToggleButtonControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUIToggleButtonControl*)pReference)->GetTextureNoFocusName();
      labelInfo = ((CGUIToggleButtonControl*)pReference)->GetLabelInfo();
      strLabel = ((CGUIToggleButtonControl*)pReference)->GetLabel();
      strExecuteAction = ((CGUIToggleButtonControl*)pReference)->GetExecuteAction();
      iHyperLink = ((CGUIToggleButtonControl*)pReference)->GetHyperLink();
      iToggleSelect = ((CGUIToggleButtonControl*)pReference)->GetToggleSelect();
    }
    else if (strType == "checkmark")
    {
      strTextureCheckMark = ((CGUICheckMarkControl*)pReference)->GetCheckMarkTextureName();
      strTextureCheckMarkNF = ((CGUICheckMarkControl*)pReference)->GetCheckMarkTextureNameNF();
      dwCheckWidth = ((CGUICheckMarkControl*)pReference)->GetCheckMarkWidth();
      dwCheckHeight = ((CGUICheckMarkControl*)pReference)->GetCheckMarkHeight();
      labelInfo = ((CGUICheckMarkControl*)pReference)->GetLabelInfo();
      strLabel = ((CGUICheckMarkControl*)pReference)->GetLabel();
    }
    else if (strType == "radiobutton")
    {
      strTextureRadioFocus = ((CGUIRadioButtonControl*)pReference)->GetTextureRadioFocusName();;
      strTextureRadioNoFocus = ((CGUIRadioButtonControl*)pReference)->GetTextureRadioNoFocusName();;
      strTextureFocus = ((CGUIRadioButtonControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUIRadioButtonControl*)pReference)->GetTextureNoFocusName();
      labelInfo = ((CGUIRadioButtonControl*)pReference)->GetLabelInfo();
      iHyperLink = ((CGUIRadioButtonControl*)pReference)->GetHyperLink();
    }
    else if (strType == "spincontrol")
    {
      labelInfo = ((CGUISpinControl*)pReference)->GetLabelInfo();
      strUp = ((CGUISpinControl*)pReference)->GetTextureUpName();
      strDown = ((CGUISpinControl*)pReference)->GetTextureDownName();
      strUpFocus = ((CGUISpinControl*)pReference)->GetTextureUpFocusName();
      strDownFocus = ((CGUISpinControl*)pReference)->GetTextureDownFocusName();
      iType = ((CGUISpinControl*)pReference)->GetType();
      dwWidth = ((CGUISpinControl*)pReference)->GetSpinWidth();
      dwHeight = ((CGUISpinControl*)pReference)->GetSpinHeight();
    }
    else if (strType == "slider")
    {
      strTextureBg = ((CGUISliderControl*)pReference)->GetBackGroundTextureName();
      strMid = ((CGUISliderControl*)pReference)->GetBackTextureMidName();
      iControlOffsetX = ((CGUISliderControl*)pReference)->GetControlOffsetX();
      iControlOffsetY = ((CGUISliderControl*)pReference)->GetControlOffsetY();
    }
    else if (strType == "sliderex")
    {
      dwSliderWidth = ((CGUISettingsSliderControl*)pReference)->GetSliderWidth();
      dwSliderHeight = ((CGUISettingsSliderControl*)pReference)->GetSliderHeight();
      strTextureBg = ((CGUISettingsSliderControl*)pReference)->GetBackGroundTextureName();
      strMid = ((CGUISettingsSliderControl*)pReference)->GetBackTextureMidName();
      iControlOffsetX = ((CGUISettingsSliderControl*)pReference)->GetControlOffsetX();
      iControlOffsetY = ((CGUISettingsSliderControl*)pReference)->GetControlOffsetY();
      strTextureFocus = ((CGUISettingsSliderControl*)pReference)->GetTextureFocusName();
      strTextureNoFocus = ((CGUISettingsSliderControl*)pReference)->GetTextureNoFocusName();
      labelInfo = ((CGUISettingsSliderControl*)pReference)->GetLabelInfo();
      strLabel = ((CGUISettingsSliderControl*)pReference)->GetLabel();
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
    else if (strType == "multiimage")
    {
      texturePath = ((CGUIMultiImage *)pReference)->GetTexturePath();
      bKeepAspectRatio = ((CGUIMultiImage *)pReference)->GetKeepAspectRatio();
      timePerImage =  ((CGUIMultiImage *)pReference)->GetTimePerImage();
      fadeTime =  ((CGUIMultiImage *)pReference)->GetFadeTime();
      loop = ((CGUIMultiImage *)pReference)->GetLoop();
      randomized = ((CGUIMultiImage *)pReference)->GetRandomized();
    }
    else if (strType == "listcontrol")
    {
      labelInfo = ((CGUIListControl*)pReference)->GetLabelInfo();
      labelInfo2 = ((CGUIListControl*)pReference)->GetLabelInfo2();
      dwSpinWidth = ((CGUIListControl*)pReference)->GetSpinWidth();
      dwSpinHeight = ((CGUIListControl*)pReference)->GetSpinHeight();
      strUp = ((CGUIListControl*)pReference)->GetTextureUpName();
      strDown = ((CGUIListControl*)pReference)->GetTextureDownName();
      strUpFocus = ((CGUIListControl*)pReference)->GetTextureUpFocusName();
      strDownFocus = ((CGUIListControl*)pReference)->GetTextureDownFocusName();
      dwSpinColor = ((CGUIListControl*)pReference)->GetSpinTextColor();
      iSpinPosX = ((CGUIListControl*)pReference)->GetSpinX();
      iSpinPosY = ((CGUIListControl*)pReference)->GetSpinY();
      strTextureNoFocus = ((CGUIListControl*)pReference)->GetButtonNoFocusName();
      strTextureFocus = ((CGUIListControl*)pReference)->GetButtonFocusName();
      strSuffix = ((CGUIListControl*)pReference)->GetSuffix();
      dwitemWidth = ((CGUIListControl*)pReference)->GetImageWidth();
      dwitemHeight = ((CGUIListControl*)pReference)->GetImageHeight();
      iTextureHeight = ((CGUIListControl*)pReference)->GetItemHeight();
      iSpace = ((CGUIListControl*)pReference)->GetSpace();
    }
    else if (strType == "listcontrolex")
    {
      labelInfo = ((CGUIListControlEx*)pReference)->GetLabelInfo();
      labelInfo2 = ((CGUIListControlEx*)pReference)->GetLabelInfo2();
      dwSpinWidth = ((CGUIListControlEx*)pReference)->GetSpinWidth();
      dwSpinHeight = ((CGUIListControlEx*)pReference)->GetSpinHeight();
      strUp = ((CGUIListControlEx*)pReference)->GetTextureUpName();
      strDown = ((CGUIListControlEx*)pReference)->GetTextureDownName();
      strUpFocus = ((CGUIListControlEx*)pReference)->GetTextureUpFocusName();
      strDownFocus = ((CGUIListControlEx*)pReference)->GetTextureDownFocusName();
      dwSpinColor = ((CGUIListControlEx*)pReference)->GetSpinTextColor();
      iSpinPosX = ((CGUIListControlEx*)pReference)->GetSpinX();
      iSpinPosY = ((CGUIListControlEx*)pReference)->GetSpinY();
      strTextureNoFocus = ((CGUIListControlEx*)pReference)->GetButtonNoFocusName();
      strTextureFocus = ((CGUIListControlEx*)pReference)->GetButtonFocusName();
      strSuffix = ((CGUIListControlEx*)pReference)->GetSuffix();
      dwitemWidth = ((CGUIListControlEx*)pReference)->GetImageWidth();
      dwitemHeight = ((CGUIListControlEx*)pReference)->GetImageHeight();
      iTextureHeight = ((CGUIListControlEx*)pReference)->GetItemHeight();
      iSpace = ((CGUIListControlEx*)pReference)->GetSpace();
    }
    else if (strType == "textbox")
    {
      labelInfo = ((CGUITextBox*)pReference)->GetLabelInfo();
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
      strImage = ((CGUIThumbnailPanel*)pReference)->GetNoFocusName();
      strImageFocus = ((CGUIThumbnailPanel*)pReference)->GetFocusName();
      dwitemWidth = ((CGUIThumbnailPanel*)pReference)->GetItemWidthLow();
      dwitemHeight = ((CGUIThumbnailPanel*)pReference)->GetItemHeightLow();
      dwSpinWidth = ((CGUIThumbnailPanel*)pReference)->GetSpinWidth();
      dwSpinHeight = ((CGUIThumbnailPanel*)pReference)->GetSpinHeight();
      strUp = ((CGUIThumbnailPanel*)pReference)->GetTextureUpName();
      strDown = ((CGUIThumbnailPanel*)pReference)->GetTextureDownName();
      strUpFocus = ((CGUIThumbnailPanel*)pReference)->GetTextureUpFocusName();
      strDownFocus = ((CGUIThumbnailPanel*)pReference)->GetTextureDownFocusName();
      dwSpinColor = ((CGUIThumbnailPanel*)pReference)->GetSpinTextColor();
      iSpinPosX = ((CGUIThumbnailPanel*)pReference)->GetSpinX();
      iSpinPosY = ((CGUIThumbnailPanel*)pReference)->GetSpinY();
      labelInfo = ((CGUIThumbnailPanel*)pReference)->GetLabelInfo();
      iTextureWidth = ((CGUIThumbnailPanel*)pReference)->GetTextureWidthLow();
      iTextureHeight = ((CGUIThumbnailPanel*)pReference)->GetTextureHeightLow();
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
      strLabel = ((CGUISelectButtonControl*)pReference)->GetLabel();
      labelInfo = ((CGUISelectButtonControl*)pReference)->GetLabelInfo();
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
      labelInfo = ((CGUIButtonScroller*)pReference)->GetLabelInfo();
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
      labelInfo = ((CGUISpinControlEx*)pReference)->GetLabelInfo();
      strUp = ((CGUISpinControlEx*)pReference)->GetTextureUpName();
      strDown = ((CGUISpinControlEx*)pReference)->GetTextureDownName();
      strUpFocus = ((CGUISpinControlEx*)pReference)->GetTextureUpFocusName();
      strDownFocus = ((CGUISpinControlEx*)pReference)->GetTextureDownFocusName();
      iType = ((CGUISpinControlEx*)pReference)->GetType();
      dwSpinWidth = ((CGUISpinControlEx*)pReference)->GetSpinWidth();
      dwSpinHeight = ((CGUISpinControlEx*)pReference)->GetSpinHeight();
      dwWidth = ((CGUISpinControlEx*)pReference)->GetWidth();
      dwHeight = ((CGUISpinControlEx*)pReference)->GetHeight();
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

  if (!XMLUtils::GetDWORD(pControlNode, "id", dwID))
  {
    return NULL; // NO id????
  }

  if (g_SkinInfo.GetVersion() < 1.85)
  {
    if (XMLUtils::GetInt(pControlNode, "posX", iPosX)) g_graphicsContext.ScaleXCoord(iPosX, res);
    if (XMLUtils::GetInt(pControlNode, "posY", iPosY)) g_graphicsContext.ScaleYCoord(iPosY, res);

    if (XMLUtils::GetDWORD(pControlNode, "width", dwWidth)) g_graphicsContext.ScaleXCoord(dwWidth, res);
    if (XMLUtils::GetDWORD(pControlNode, "height", dwHeight)) g_graphicsContext.ScaleYCoord(dwHeight, res);
    if (XMLUtils::GetDWORD(pControlNode, "textSpaceY", dwTextSpaceY)) g_graphicsContext.ScaleYCoord(dwTextSpaceY, res);

    if (XMLUtils::GetInt(pControlNode, "controlOffsetX", iControlOffsetX)) g_graphicsContext.ScaleXCoord(iControlOffsetX, res);
    if (XMLUtils::GetInt(pControlNode, "controlOffsetY", iControlOffsetY)) g_graphicsContext.ScaleYCoord(iControlOffsetY, res);

    if (XMLUtils::GetDWORD(pControlNode, "gfxThumbWidth", dwThumbWidth)) g_graphicsContext.ScaleXCoord(dwThumbWidth, res);
    if (XMLUtils::GetDWORD(pControlNode, "gfxThumbHeight", dwThumbHeight)) g_graphicsContext.ScaleYCoord(dwThumbHeight, res);
    if (XMLUtils::GetDWORD(pControlNode, "gfxThumbSpaceX", dwThumbSpaceX)) g_graphicsContext.ScaleXCoord(dwThumbSpaceX, res);
    if (XMLUtils::GetDWORD(pControlNode, "gfxThumbSpaceY", dwThumbSpaceY)) g_graphicsContext.ScaleYCoord(dwThumbSpaceY, res);
    XMLUtils::GetString(pControlNode, "gfxThumbDefault", strDefaultThumb);

    if (!XMLUtils::GetDWORD(pControlNode, "onup" , up ))
    {
      up = dwID - 1;
    }
    if (!XMLUtils::GetDWORD(pControlNode, "ondown" , down))
    {
      down = dwID + 1;
    }
    if (!XMLUtils::GetDWORD(pControlNode, "onleft" , left ))
    {
      left = dwID;
    }
    if (!XMLUtils::GetDWORD(pControlNode, "onright", right))
    {
      right = dwID;
    }

    XMLUtils::GetHex(pControlNode, "colordiffuse", dwColorDiffuse);
    
    GetConditionalVisibility(pControlNode, iVisibleCondition, allowHiddenFocus, startHidden);
    GetAnimations(pControlNode, animations, res);

    XMLUtils::GetHex(pControlNode, "disabledcolor", labelInfo.disabledColor);
    XMLUtils::GetHex(pControlNode, "textcolor", labelInfo.textColor);
    XMLUtils::GetHex(pControlNode, "shadowcolor", labelInfo.shadowColor);
    XMLUtils::GetHex(pControlNode, "selectedColor", labelInfo.selectedColor);
    labelInfo2.selectedColor = labelInfo.selectedColor;
    if (XMLUtils::GetInt(pControlNode, "textOffsetX", labelInfo.offsetX)) g_graphicsContext.ScaleXCoord(labelInfo.offsetX, res);
    if (XMLUtils::GetInt(pControlNode, "textOffsetY", labelInfo.offsetY)) g_graphicsContext.ScaleYCoord(labelInfo.offsetY, res);
    if (XMLUtils::GetInt(pControlNode, "textXOff", labelInfo.offsetX)) g_graphicsContext.ScaleXCoord(labelInfo.offsetX, res);
    if (XMLUtils::GetInt(pControlNode, "textYOff", labelInfo.offsetY)) g_graphicsContext.ScaleYCoord(labelInfo.offsetY, res);
    if (XMLUtils::GetInt(pControlNode, "textXOff2", labelInfo2.offsetX)) g_graphicsContext.ScaleXCoord(labelInfo2.offsetX, res);
    if (XMLUtils::GetInt(pControlNode, "textYOff2", labelInfo2.offsetY)) g_graphicsContext.ScaleYCoord(labelInfo2.offsetY, res);
    CStdString strFont;
    if (XMLUtils::GetString(pControlNode, "font", strFont))
      labelInfo.font = g_fontManager.GetFont(strFont);

    DWORD align = 0;
    if (GetAlignment(pControlNode, "align", align))
      labelInfo.align = align | (labelInfo.align & 4);
    DWORD alignY = 0;
    if (GetAlignmentY(pControlNode, "alignY", alignY))
      labelInfo.align = alignY | (labelInfo.align & 3);
    if (XMLUtils::GetString(pControlNode, "font2", strFont))
      labelInfo2.font = g_fontManager.GetFont(strFont);
    if (!labelInfo2.font) labelInfo2.font = labelInfo.font;
    XMLUtils::GetHex(pControlNode, "selectedColor2", labelInfo2.selectedColor);
    XMLUtils::GetHex(pControlNode, "textcolor2", labelInfo2.textColor);

    CStdString strWindow;
    XMLUtils::GetString(pControlNode, "hyperlink", strWindow);
    iHyperLink = WINDOW_INVALID;
    if (!strWindow.IsEmpty())
      iHyperLink = g_buttonTranslator.TranslateWindowString(strWindow.c_str());

    XMLUtils::GetString(pControlNode, "script", strExecuteAction); // left in for backwards compatibility.
    XMLUtils::GetString(pControlNode, "execute", strExecuteAction);

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
    GetPath(pControlNode, "textureFocus", strTextureFocus);
    GetPath(pControlNode, "textureNoFocus", strTextureNoFocus);
    GetPath(pControlNode, "AltTextureFocus", strTextureAltFocus);
    GetPath(pControlNode, "AltTextureNoFocus", strTextureAltNoFocus);
    CStdString strToggleSelect;
    XMLUtils::GetString(pControlNode, "UseAltTexture", strToggleSelect);
    iToggleSelect = g_infoManager.TranslateString(strToggleSelect);
    XMLUtils::GetDWORD(pControlNode, "bitmaps", dwItems);

    XMLUtils::GetBoolean(pControlNode, "hasPath", bHasPath);

    GetPath(pControlNode, "textureUp", strUp);
    GetPath(pControlNode, "textureDown", strDown);
    GetPath(pControlNode, "textureUpFocus", strUpFocus);
    GetPath(pControlNode, "textureDownFocus", strDownFocus);

    GetPath(pControlNode, "textureLeft", strLeft);
    GetPath(pControlNode, "textureRight", strRight);
    GetPath(pControlNode, "textureLeftFocus", strLeftFocus);
    GetPath(pControlNode, "textureRightFocus", strRightFocus);

    XMLUtils::GetHex(pControlNode, "spinColor", dwSpinColor);
    if (XMLUtils::GetDWORD(pControlNode, "spinWidth", dwSpinWidth)) g_graphicsContext.ScaleXCoord(dwSpinWidth, res);
    if (XMLUtils::GetDWORD(pControlNode, "spinHeight", dwSpinHeight)) g_graphicsContext.ScaleYCoord(dwSpinHeight, res);
    if (XMLUtils::GetInt(pControlNode, "spinPosX", iSpinPosX))
    { // pre 1.85 skins had absolute spin positions
      g_graphicsContext.ScaleXCoord(iSpinPosX, res);
      iSpinPosX -= iPosX;
    }
    if (XMLUtils::GetInt(pControlNode, "spinPosY", iSpinPosY))
    { // pre 1.85 skins had absolute spin positions
      g_graphicsContext.ScaleYCoord(iSpinPosY, res);
      iSpinPosY -= iPosY;
    }

    if (XMLUtils::GetDWORD(pControlNode, "MarkWidth", dwCheckWidth)) g_graphicsContext.ScaleXCoord(dwCheckWidth, res);
    if (XMLUtils::GetDWORD(pControlNode, "MarkHeight", dwCheckHeight)) g_graphicsContext.ScaleYCoord(dwCheckHeight, res);
    if (XMLUtils::GetDWORD(pControlNode, "sliderWidth", dwSliderWidth)) g_graphicsContext.ScaleXCoord(dwSliderWidth, res);
    if (XMLUtils::GetDWORD(pControlNode, "sliderHeight", dwSliderHeight)) g_graphicsContext.ScaleYCoord(dwSliderHeight, res);
    GetPath(pControlNode, "textureCheckmark", strTextureCheckMark);
    GetPath(pControlNode, "textureCheckmarkNoFocus", strTextureCheckMarkNF);
    GetPath(pControlNode, "textureRadioFocus", strTextureRadioFocus);
    GetPath(pControlNode, "textureRadioNoFocus", strTextureRadioNoFocus);

    GetPath(pControlNode, "textureSliderBar", strTextureBg);
    GetPath(pControlNode, "textureSliderNib", strMid);
    GetPath(pControlNode, "textureSliderNibFocus", strMidFocus);
    XMLUtils::GetDWORD(pControlNode, "disposition", dwDisposition);

    XMLUtils::GetString(pControlNode, "title", strTitle);
    XMLUtils::GetString(pControlNode, "tagset", strRSSTags);
    XMLUtils::GetHex(pControlNode, "headlinecolor", dwTextColor3);
    XMLUtils::GetHex(pControlNode, "titlecolor", labelInfo2.textColor);

    if (XMLUtils::GetString(pControlNode, "subtype", strSubType))
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

    XMLUtils::GetBoolean(pControlNode, "reverse", bReverse);

    GetPath(pControlNode, "texturebg", strTextureBg);
    GetPath(pControlNode, "lefttexture", strLeft);
    GetPath(pControlNode, "midtexture", strMid);
    GetPath(pControlNode, "righttexture", strRight);
    GetPath(pControlNode, "overlaytexture", strOverlay);
    GetPath(pControlNode, "texture", strTexture);
    XMLUtils::GetHex(pControlNode, "colorkey", dwColorKey);

    XMLUtils::GetString(pControlNode, "suffix", strSuffix);

    if (XMLUtils::GetDWORD(pControlNode, "itemWidth", dwitemWidth)) g_graphicsContext.ScaleXCoord(dwitemWidth, res);
    if (XMLUtils::GetDWORD(pControlNode, "itemHeight", dwitemHeight)) g_graphicsContext.ScaleYCoord(dwitemHeight, res);
    if (XMLUtils::GetInt(pControlNode, "spaceBetweenItems", iSpace)) g_graphicsContext.ScaleYCoord(iSpace, res);

    GetPath(pControlNode, "imageFolder", strImage);
    GetPath(pControlNode, "imageFolderFocus", strImageFocus);
    if (XMLUtils::GetInt(pControlNode, "textureWidth", iTextureWidth)) g_graphicsContext.ScaleXCoord(iTextureWidth, res);
    if (XMLUtils::GetInt(pControlNode, "textureHeight", iTextureHeight)) g_graphicsContext.ScaleYCoord(iTextureHeight, res);

    if (XMLUtils::GetInt(pControlNode, "thumbWidth", iThumbWidth)) g_graphicsContext.ScaleXCoord(iThumbWidth, res);
    if (XMLUtils::GetInt(pControlNode, "thumbHeight", iThumbHeight)) g_graphicsContext.ScaleYCoord(iThumbHeight, res);
    if (XMLUtils::GetInt(pControlNode, "thumbPosX", iThumbXPos)) g_graphicsContext.ScaleXCoord(iThumbXPos, res);
    if (XMLUtils::GetInt(pControlNode, "thumbPosY", iThumbYPos)) g_graphicsContext.ScaleYCoord(iThumbYPos, res);

    GetAlignment(pControlNode, "thumbAlign", dwThumbAlign);

    if (XMLUtils::GetInt(pControlNode, "thumbWidthBig", iThumbWidthBig)) g_graphicsContext.ScaleXCoord(iThumbWidthBig, res);
    if (XMLUtils::GetInt(pControlNode, "thumbHeightBig", iThumbHeightBig)) g_graphicsContext.ScaleYCoord(iThumbHeightBig, res);
    if (XMLUtils::GetInt(pControlNode, "thumbPosXBig", iThumbXPosBig)) g_graphicsContext.ScaleXCoord(iThumbXPosBig, res);
    if (XMLUtils::GetInt(pControlNode, "thumbPosYBig", iThumbYPosBig)) g_graphicsContext.ScaleYCoord(iThumbYPosBig, res);

    if (XMLUtils::GetDWORD(pControlNode, "textureWidthBig", textureWidthBig)) g_graphicsContext.ScaleXCoord(textureWidthBig, res);
    if (XMLUtils::GetDWORD(pControlNode, "textureHeightBig", textureHeightBig)) g_graphicsContext.ScaleYCoord(textureHeightBig, res);
    if (XMLUtils::GetDWORD(pControlNode, "itemWidthBig", itemWidthBig)) g_graphicsContext.ScaleXCoord(itemWidthBig, res);
    if (XMLUtils::GetDWORD(pControlNode, "itemHeightBig", itemHeightBig)) g_graphicsContext.ScaleYCoord(itemHeightBig, res);
    XMLUtils::GetDWORD(pControlNode, "buddycontrolid", dwBuddyControlID);

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

    XMLUtils::GetInt(pControlNode,"urlset",iUrlSet);

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
    if ( XMLUtils::GetString(pControlNode, "orientation", strTmp) )
    {
      if (strTmp.ToLower() == "horizontal")
        bHorizontal = true;
    }
    if (XMLUtils::GetInt(pControlNode, "buttongap", iButtonGap))
    {
      if (bHorizontal)
        g_graphicsContext.ScaleXCoord(iButtonGap, res);
      else
        g_graphicsContext.ScaleYCoord(iButtonGap, res);
    }
    XMLUtils::GetInt(pControlNode, "numbuttons", iNumSlots);
    XMLUtils::GetInt(pControlNode, "movement", iMovementRange);
    XMLUtils::GetInt(pControlNode, "defaultbutton", iDefaultSlot);
    XMLUtils::GetInt(pControlNode, "alpha", iAlpha);
    XMLUtils::GetBoolean(pControlNode, "wraparound", bWrapAround);
    XMLUtils::GetBoolean(pControlNode, "smoothscrolling", bSmoothScrolling);
    XMLUtils::GetBoolean(pControlNode, "keepaspectratio", bKeepAspectRatio);
    XMLUtils::GetBoolean(pControlNode, "scroll", bScrollLabel);
    XMLUtils::GetBoolean(pControlNode,"pulseonselect", bPulse);

    GetPath(pControlNode,"imagepath", texturePath);
    XMLUtils::GetDWORD(pControlNode,"timeperimage", timePerImage);
    XMLUtils::GetDWORD(pControlNode,"fadetime", fadeTime);
    XMLUtils::GetBoolean(pControlNode, "randomize", randomized);
    XMLUtils::GetBoolean(pControlNode, "loop", loop);
  }
  else
  {
    if (XMLUtils::GetInt(pControlNode, "posx", iPosX)) g_graphicsContext.ScaleXCoord(iPosX, res);
    if (XMLUtils::GetInt(pControlNode, "posy", iPosY)) g_graphicsContext.ScaleYCoord(iPosY, res);

    if (XMLUtils::GetDWORD(pControlNode, "width", dwWidth)) g_graphicsContext.ScaleXCoord(dwWidth, res);
    if (XMLUtils::GetDWORD(pControlNode, "height", dwHeight)) g_graphicsContext.ScaleYCoord(dwHeight, res);
    if (XMLUtils::GetDWORD(pControlNode, "textspacey", dwTextSpaceY)) g_graphicsContext.ScaleYCoord(dwTextSpaceY, res);

    if (XMLUtils::GetInt(pControlNode, "controloffsetx", iControlOffsetX)) g_graphicsContext.ScaleXCoord(iControlOffsetX, res);
    if (XMLUtils::GetInt(pControlNode, "controloffsety", iControlOffsetY)) g_graphicsContext.ScaleYCoord(iControlOffsetY, res);

    if (XMLUtils::GetDWORD(pControlNode, "gfxthumbwidth", dwThumbWidth)) g_graphicsContext.ScaleXCoord(dwThumbWidth, res);
    if (XMLUtils::GetDWORD(pControlNode, "gfxthumbheight", dwThumbHeight)) g_graphicsContext.ScaleYCoord(dwThumbHeight, res);
    if (XMLUtils::GetDWORD(pControlNode, "gfxthumbspacex", dwThumbSpaceX)) g_graphicsContext.ScaleXCoord(dwThumbSpaceX, res);
    if (XMLUtils::GetDWORD(pControlNode, "gfxthumbspacey", dwThumbSpaceY)) g_graphicsContext.ScaleYCoord(dwThumbSpaceY, res);
    XMLUtils::GetString(pControlNode, "gfxthumbdefault", strDefaultThumb);

    if (!XMLUtils::GetDWORD(pControlNode, "onup" , up ))
    {
      up = dwID - 1;
    }
    if (!XMLUtils::GetDWORD(pControlNode, "ondown" , down))
    {
      down = dwID + 1;
    }
    if (!XMLUtils::GetDWORD(pControlNode, "onleft" , left ))
    {
      left = dwID;
    }
    if (!XMLUtils::GetDWORD(pControlNode, "onright", right))
    {
      right = dwID;
    }

    XMLUtils::GetHex(pControlNode, "colordiffuse", dwColorDiffuse);
    
    GetConditionalVisibility(pControlNode, iVisibleCondition, allowHiddenFocus, startHidden);
    GetAnimations(pControlNode, animations, res);

    XMLUtils::GetHex(pControlNode, "textcolor", labelInfo.textColor);
    XMLUtils::GetHex(pControlNode, "disabledcolor", labelInfo.disabledColor);
    XMLUtils::GetHex(pControlNode, "shadowcolor", labelInfo.shadowColor);
    XMLUtils::GetHex(pControlNode, "selectedcolor", labelInfo.selectedColor);
    if (XMLUtils::GetInt(pControlNode, "textoffsetx", labelInfo.offsetX)) g_graphicsContext.ScaleXCoord(labelInfo.offsetX, res);
    if (XMLUtils::GetInt(pControlNode, "textoffsety", labelInfo.offsetY)) g_graphicsContext.ScaleYCoord(labelInfo.offsetY, res);
    if (XMLUtils::GetInt(pControlNode, "textxoff", labelInfo.offsetX)) g_graphicsContext.ScaleXCoord(labelInfo.offsetX, res);
    if (XMLUtils::GetInt(pControlNode, "textyoff", labelInfo.offsetY)) g_graphicsContext.ScaleYCoord(labelInfo.offsetY, res);
    if (XMLUtils::GetInt(pControlNode, "textxoff2", labelInfo2.offsetX)) g_graphicsContext.ScaleXCoord(labelInfo2.offsetX, res);
    if (XMLUtils::GetInt(pControlNode, "textyoff2", labelInfo2.offsetY)) g_graphicsContext.ScaleYCoord(labelInfo2.offsetY, res);
    int angle = 0;
    if (XMLUtils::GetInt(pControlNode, "angle", angle)) labelInfo.angle = CAngle(angle);
    CStdString strFont;
    if (XMLUtils::GetString(pControlNode, "font", strFont))
      labelInfo.font = g_fontManager.GetFont(strFont);
    DWORD align = 0;
    if (GetAlignment(pControlNode, "align", align))
      labelInfo.align = align | (labelInfo.align & 4);
    DWORD alignY = 0;
    if (GetAlignmentY(pControlNode, "aligny", alignY))
      labelInfo.align = alignY | (labelInfo.align & 3);
    labelInfo2.selectedColor = labelInfo.selectedColor;
    XMLUtils::GetHex(pControlNode, "selectedcolor2", labelInfo2.selectedColor);
    XMLUtils::GetHex(pControlNode, "textcolor2", labelInfo2.textColor);
    if (XMLUtils::GetString(pControlNode, "font2", strFont))
      labelInfo2.font = g_fontManager.GetFont(strFont);
    if (!labelInfo2.font) labelInfo2.font = labelInfo.font;
    CStdString strWindow;
    XMLUtils::GetString(pControlNode, "hyperlink", strWindow);
    iHyperLink = WINDOW_INVALID;
    if (!strWindow.IsEmpty())
      iHyperLink = g_buttonTranslator.TranslateWindowString(strWindow.c_str());

    XMLUtils::GetString(pControlNode, "script", strExecuteAction); // left in for backwards compatibility.
    XMLUtils::GetString(pControlNode, "execute", strExecuteAction);

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
    GetPath(pControlNode, "texturefocus", strTextureFocus);
    GetPath(pControlNode, "texturenofocus", strTextureNoFocus);
    GetPath(pControlNode, "alttexturefocus", strTextureAltFocus);
    GetPath(pControlNode, "alttexturenofocus", strTextureAltNoFocus);
    CStdString strToggleSelect;
    XMLUtils::GetString(pControlNode, "usealttexture", strToggleSelect);
    iToggleSelect = g_infoManager.TranslateString(strToggleSelect);
    XMLUtils::GetDWORD(pControlNode, "bitmaps", dwItems);

    XMLUtils::GetBoolean(pControlNode, "haspath", bHasPath);

    GetPath(pControlNode, "textureup", strUp);
    GetPath(pControlNode, "texturedown", strDown);
    GetPath(pControlNode, "textureupfocus", strUpFocus);
    GetPath(pControlNode, "texturedownfocus", strDownFocus);

    GetPath(pControlNode, "textureleft", strLeft);
    GetPath(pControlNode, "textureright", strRight);
    GetPath(pControlNode, "textureleftfocus", strLeftFocus);
    GetPath(pControlNode, "texturerightfocus", strRightFocus);

    XMLUtils::GetHex(pControlNode, "spincolor", dwSpinColor);
    if (XMLUtils::GetDWORD(pControlNode, "spinwidth", dwSpinWidth)) g_graphicsContext.ScaleXCoord(dwSpinWidth, res);
    if (XMLUtils::GetDWORD(pControlNode, "spinheight", dwSpinHeight)) g_graphicsContext.ScaleYCoord(dwSpinHeight, res);
    if (XMLUtils::GetInt(pControlNode, "spinposx", iSpinPosX)) g_graphicsContext.ScaleXCoord(iSpinPosX, res);
    if (XMLUtils::GetInt(pControlNode, "spinposy", iSpinPosY)) g_graphicsContext.ScaleYCoord(iSpinPosY, res);

    if (XMLUtils::GetDWORD(pControlNode, "markwidth", dwCheckWidth)) g_graphicsContext.ScaleXCoord(dwCheckWidth, res);
    if (XMLUtils::GetDWORD(pControlNode, "markheight", dwCheckHeight)) g_graphicsContext.ScaleYCoord(dwCheckHeight, res);
    if (XMLUtils::GetDWORD(pControlNode, "sliderwidth", dwSliderWidth)) g_graphicsContext.ScaleXCoord(dwSliderWidth, res);
    if (XMLUtils::GetDWORD(pControlNode, "sliderheight", dwSliderHeight)) g_graphicsContext.ScaleYCoord(dwSliderHeight, res);
    GetPath(pControlNode, "texturecheckmark", strTextureCheckMark);
    GetPath(pControlNode, "texturecheckmarknofocus", strTextureCheckMarkNF);
    GetPath(pControlNode, "textureradiofocus", strTextureRadioFocus);
    GetPath(pControlNode, "textureradionofocus", strTextureRadioNoFocus);

    GetPath(pControlNode, "texturesliderbar", strTextureBg);
    GetPath(pControlNode, "textureslidernib", strMid);
    GetPath(pControlNode, "textureslidernibfocus", strMidFocus);
    XMLUtils::GetDWORD(pControlNode, "disposition", dwDisposition);

    XMLUtils::GetString(pControlNode, "title", strTitle);
    XMLUtils::GetString(pControlNode, "tagset", strRSSTags);
    XMLUtils::GetHex(pControlNode, "headlinecolor", dwTextColor3);
    XMLUtils::GetHex(pControlNode, "titlecolor", labelInfo2.textColor);

    if (XMLUtils::GetString(pControlNode, "subtype", strSubType))
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

    XMLUtils::GetBoolean(pControlNode, "reverse", bReverse);

    GetPath(pControlNode, "texturebg", strTextureBg);
    GetPath(pControlNode, "lefttexture", strLeft);
    GetPath(pControlNode, "midtexture", strMid);
    GetPath(pControlNode, "righttexture", strRight);
    GetPath(pControlNode, "overlaytexture", strOverlay);
    GetPath(pControlNode, "texture", strTexture);
    XMLUtils::GetHex(pControlNode, "colorkey", dwColorKey);

    XMLUtils::GetString(pControlNode, "suffix", strSuffix);

    if (XMLUtils::GetDWORD(pControlNode, "itemwidth", dwitemWidth)) g_graphicsContext.ScaleXCoord(dwitemWidth, res);
    if (XMLUtils::GetDWORD(pControlNode, "itemheight", dwitemHeight)) g_graphicsContext.ScaleYCoord(dwitemHeight, res);
    if (XMLUtils::GetInt(pControlNode, "spacebetweenitems", iSpace)) g_graphicsContext.ScaleYCoord(iSpace, res);

    GetPath(pControlNode, "imagefolder", strImage);
    GetPath(pControlNode, "imagefolderfocus", strImageFocus);
    if (XMLUtils::GetInt(pControlNode, "texturewidth", iTextureWidth)) g_graphicsContext.ScaleXCoord(iTextureWidth, res);
    if (XMLUtils::GetInt(pControlNode, "textureheight", iTextureHeight)) g_graphicsContext.ScaleYCoord(iTextureHeight, res);

    if (XMLUtils::GetInt(pControlNode, "thumbwidth", iThumbWidth)) g_graphicsContext.ScaleXCoord(iThumbWidth, res);
    if (XMLUtils::GetInt(pControlNode, "thumbheight", iThumbHeight)) g_graphicsContext.ScaleYCoord(iThumbHeight, res);
    if (XMLUtils::GetInt(pControlNode, "thumbposx", iThumbXPos)) g_graphicsContext.ScaleXCoord(iThumbXPos, res);
    if (XMLUtils::GetInt(pControlNode, "thumbposy", iThumbYPos)) g_graphicsContext.ScaleYCoord(iThumbYPos, res);

    GetAlignment(pControlNode, "thumbalign", dwThumbAlign);

    if (XMLUtils::GetInt(pControlNode, "thumbwidthbig", iThumbWidthBig)) g_graphicsContext.ScaleXCoord(iThumbWidthBig, res);
    if (XMLUtils::GetInt(pControlNode, "thumbheightbig", iThumbHeightBig)) g_graphicsContext.ScaleYCoord(iThumbHeightBig, res);
    if (XMLUtils::GetInt(pControlNode, "thumbposxbig", iThumbXPosBig)) g_graphicsContext.ScaleXCoord(iThumbXPosBig, res);
    if (XMLUtils::GetInt(pControlNode, "thumbposybig", iThumbYPosBig)) g_graphicsContext.ScaleYCoord(iThumbYPosBig, res);

    if (XMLUtils::GetDWORD(pControlNode, "texturewidthbig", textureWidthBig)) g_graphicsContext.ScaleXCoord(textureWidthBig, res);
    if (XMLUtils::GetDWORD(pControlNode, "textureheightbig", textureHeightBig)) g_graphicsContext.ScaleYCoord(textureHeightBig, res);
    if (XMLUtils::GetDWORD(pControlNode, "itemwidthbig", itemWidthBig)) g_graphicsContext.ScaleXCoord(itemWidthBig, res);
    if (XMLUtils::GetDWORD(pControlNode, "itemheightbig", itemHeightBig)) g_graphicsContext.ScaleYCoord(itemHeightBig, res);

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

    XMLUtils::GetInt(pControlNode,"urlset",iUrlSet);

    // stuff for button scroller
    if ( XMLUtils::GetString(pControlNode, "orientation", strTmp) )
    {
      if (strTmp.ToLower() == "horizontal")
        bHorizontal = true;
    }
    if (XMLUtils::GetInt(pControlNode, "buttongap", iButtonGap))
    {
      if (bHorizontal)
        g_graphicsContext.ScaleXCoord(iButtonGap, res);
      else
        g_graphicsContext.ScaleYCoord(iButtonGap, res);
    }
    XMLUtils::GetInt(pControlNode, "numbuttons", iNumSlots);
    XMLUtils::GetInt(pControlNode, "movement", iMovementRange);
    XMLUtils::GetInt(pControlNode, "defaultbutton", iDefaultSlot);
    XMLUtils::GetInt(pControlNode, "alpha", iAlpha);
    XMLUtils::GetBoolean(pControlNode, "wraparound", bWrapAround);
    XMLUtils::GetBoolean(pControlNode, "smoothscrolling", bSmoothScrolling);
    XMLUtils::GetBoolean(pControlNode, "keepaspectratio", bKeepAspectRatio);
    XMLUtils::GetBoolean(pControlNode, "scroll", bScrollLabel);
    XMLUtils::GetBoolean(pControlNode,"pulseonselect", bPulse);

    GetPath(pControlNode,"imagepath", texturePath);
    XMLUtils::GetDWORD(pControlNode,"timeperimage", timePerImage);
    XMLUtils::GetDWORD(pControlNode,"fadetime", fadeTime);
    XMLUtils::GetBoolean(pControlNode, "randomize", randomized);
    XMLUtils::GetBoolean(pControlNode, "loop", loop);
  }
  /////////////////////////////////////////////////////////////////////////////
  // Instantiate a new control using the properties gathered above
  //

  if (strType == "label")
  {
    CGUILabelControl* pControl = new CGUILabelControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strLabel, labelInfo, bHasPath);

    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetInfo(vecInfo);
    pControl->SetWidthControl(bScrollLabel);
    return pControl;
  }
  else if (strType == "edit")
  {
    CGUIEditControl* pControl = new CGUIEditControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      labelInfo, strLabel);

    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    return pControl;
  }
  else if (strType == "videowindow")
  {
    CGUIVideoControl* pControl = new CGUIVideoControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    return pControl;
  }
  else if (strType == "fadelabel")
  {
    CGUIFadeLabelControl* pControl = new CGUIFadeLabelControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      labelInfo);

    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetLabel(vecLabel);
    pControl->SetInfo(vecInfo);
    return pControl;
  }
  else if (strType == "rss")
  {
    CGUIRSSControl* pControl = new CGUIRSSControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      labelInfo, labelInfo2.textColor, dwTextColor3, strRSSTags);

    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
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
      labelInfo, labelInfo2);

    pControl->SetTextSpacing(dwTextSpaceY);
    pControl->SetThumbAttributes(dwThumbWidth, dwThumbHeight, dwThumbSpaceX, dwThumbSpaceY, strDefaultThumb);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "console")
  {
    CGUIConsoleControl* pControl = new CGUIConsoleControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      labelInfo, labelInfo.textColor, labelInfo2.textColor, dwTextColor3, labelInfo.selectedColor);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetNavigation(up, down, left, right);
    return pControl;
  }
  else if (strType == "button")
  {
    CGUIButtonControl* pControl = new CGUIButtonControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureFocus, strTextureNoFocus,
      labelInfo);

    pControl->SetText(strLabel);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetHyperLink(iHyperLink);
    pControl->SetExecuteAction(strExecuteAction);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  else if (g_SkinInfo.GetVersion() < 1.85 && strType == "conditionalbutton")
  {
    CGUIConditionalButtonControl* pControl = new CGUIConditionalButtonControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureFocus, strTextureNoFocus,
      labelInfo);

    pControl->SetText(strLabel);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetHyperLink(iHyperLink);
    pControl->SetExecuteAction(strExecuteAction);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
#endif
  else if (strType == "togglebutton")
  {
    CGUIToggleButtonControl* pControl = new CGUIToggleButtonControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureFocus, strTextureNoFocus,
      strTextureAltFocus, strTextureAltNoFocus, labelInfo);

    pControl->SetText(strLabel);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetHyperLink(iHyperLink);
    pControl->SetExecuteAction(strExecuteAction);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetToggleSelect(iToggleSelect);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "checkmark")
  {
    CGUICheckMarkControl* pControl = new CGUICheckMarkControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureCheckMark, strTextureCheckMarkNF,
      dwCheckWidth, dwCheckHeight, labelInfo);

    pControl->SetText(strLabel);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "radiobutton")
  {
    CGUIRadioButtonControl* pControl = new CGUIRadioButtonControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureFocus, strTextureNoFocus,
      labelInfo,
      strTextureRadioFocus, strTextureRadioNoFocus);

    pControl->SetText(strLabel);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetHyperLink(iHyperLink);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "spincontrol")
  {
    CGUISpinControl* pControl = new CGUISpinControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strUp, strDown, strUpFocus, strDownFocus,
      labelInfo, labelInfo.textColor, iType);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetReverse(bReverse);
    if (g_SkinInfo.GetVersion() < 1.85)
      pControl->SetBuddyControlID(dwBuddyControlID);
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
      strTextureBg, strMid, strMidFocus, (g_SkinInfo.GetVersion() < 1.85) ? iType : SPIN_CONTROL_TYPE_TEXT);

    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetInfo(vecInfo.size() ? vecInfo[0] : 0);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetControlOffsetX(iControlOffsetX);
    pControl->SetControlOffsetY(iControlOffsetY);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "sliderex")
  {
    labelInfo.align |= XBFONT_CENTER_Y;    // always center text vertically
    CGUISettingsSliderControl* pControl = new CGUISettingsSliderControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight, dwSliderWidth, dwSliderHeight, strTextureFocus, strTextureNoFocus,
      strTextureBg, strMid, strMidFocus, labelInfo, (g_SkinInfo.GetVersion() < 1.85) ? iType : SPIN_CONTROL_TYPE_TEXT);

    pControl->SetText(strLabel);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetInfo(vecInfo.size() ? vecInfo[0] : 0);
    pControl->SetNavigation(up, down, left, right);
    return pControl;
  }
  else if (strType == "progress")
  {
    CGUIProgressControl* pControl = new CGUIProgressControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strTextureBg, strLeft, strMid, strRight, strOverlay);

    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
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
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetInfo(vecInfo.size() ? vecInfo[0] : 0);
    return pControl;
  }
  else if (strType == "multiimage")
  {
    CGUIMultiImage* pControl = new CGUIMultiImage(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight, texturePath, timePerImage, fadeTime, randomized, loop);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetKeepAspectRatio(bKeepAspectRatio);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    return pControl;
  }
  else if (strType == "listcontrol")
  {
    CGUIListControl* pControl = new CGUIListControl(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      dwSpinWidth, dwSpinHeight,
      strUp, strDown,
      strUpFocus, strDownFocus,
      dwSpinColor, iSpinPosX, iSpinPosY,
      labelInfo, labelInfo2,
      strTextureNoFocus, strTextureFocus);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetScrollySuffix(strSuffix);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetImageDimensions(dwitemWidth, dwitemHeight);
    pControl->SetItemHeight(iTextureHeight);
    pControl->SetSpace(iSpace);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "listcontrolex")
  {
    CGUIListControlEx* pControl = new CGUIListControlEx(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      dwSpinWidth, dwSpinHeight,
      strUp, strDown,
      strUpFocus, strDownFocus,
      dwSpinColor, iSpinPosX, iSpinPosY,
      labelInfo, labelInfo2,
      strTextureNoFocus, strTextureFocus);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetScrollySuffix(strSuffix);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetImageDimensions(dwitemWidth, dwitemHeight);
    pControl->SetItemHeight(iTextureHeight);
    pControl->SetSpace(iSpace);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "textbox")
  {
    CGUITextBox* pControl = new CGUITextBox(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      dwSpinWidth, dwSpinHeight,
      strUp, strDown,
      strUpFocus, strDownFocus,
      dwSpinColor, iSpinPosX, iSpinPosY,
      labelInfo);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "thumbnailpanel")
  {
    CGUIThumbnailPanel* pControl = new CGUIThumbnailPanel(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight,
      strImage, strImageFocus,
      dwSpinWidth, dwSpinHeight,
      strUp, strDown,
      strUpFocus, strDownFocus,
      dwSpinColor, iSpinPosX, iSpinPosY,
      labelInfo);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetScrollySuffix(strSuffix);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
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
      labelInfo,
      strTextureBg, strLeft, strLeftFocus, strRight, strRightFocus);

    pControl->SetText(strLabel);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
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
      strTextureFocus, strTextureNoFocus, labelInfo);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetPulseOnSelect(bPulse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->LoadButtons(pControlNode);
    return pControl;
  }
  else if (strType == "spincontrolex")
  {
    CGUISpinControlEx* pControl = new CGUISpinControlEx(
      dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight, dwSpinWidth, dwSpinHeight,
      dwSpinColor, strTextureFocus, strTextureNoFocus, strUp, strDown, strUpFocus, strDownFocus,
      labelInfo, iType);

    pControl->SetText(strLabel);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    pControl->SetReverse(bReverse);
    pControl->SetText(strLabel);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "visualisation")
  {
    CGUIVisualisationControl* pControl = new CGUIVisualisationControl(dwParentId, dwID, iPosX, iPosY, dwWidth, dwHeight);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus, startHidden);
    pControl->SetAnimations(animations);
    return pControl;
  }
  return NULL;
}
