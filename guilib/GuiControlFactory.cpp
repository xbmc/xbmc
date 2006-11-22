#include "include.h"
#include "GUIControlFactory.h"
#include "LocalizeStrings.h"
#include "GUIButtoncontrol.h"
#include "GUIRadiobuttoncontrol.h"
#include "GUISpinControl.h"
#include "GUIRSSControl.h"
#include "GUIRAMControl.h"
#include "GUIConsoleControl.h"
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
#include "GUIControlGroup.h"
#include "GUIControlGroupList.h"
#include "GUIScrollBarControl.h"
#include "GUIListContainer.h"
#include "GUIWrappingListContainer.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/util.h"
#include "../xbmc/ButtonTranslator.h"
#include "XMLUtils.h"
#include "GUIFontManager.h"

#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
#include "SkinInfo.h"
#endif

CGUIControlFactory::CGUIControlFactory(void)
{}

CGUIControlFactory::~CGUIControlFactory(void)
{}

bool CGUIControlFactory::GetIntRange(const TiXmlNode* pRootNode, const char* strTag, int& iMinValue, int& iMaxValue, int& iIntervalValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
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
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
  if (!pNode || !pNode->FirstChild()) return false;
  fMinValue = (float)atof(pNode->FirstChild()->Value());
  char* maxValue = strchr(pNode->FirstChild()->Value(), ',');
  if (maxValue)
  {
    maxValue++;
    fMaxValue = (float)atof(maxValue);

    char* intervalValue = strchr(maxValue, ',');
    if (intervalValue)
    {
      intervalValue++;
      fIntervalValue = (float)atof(intervalValue);
    }
  }

  return true;
}

bool CGUIControlFactory::GetMultipleString(const TiXmlNode* pRootNode, const char* strTag, vector<CStdString>& vecStringValue)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode) return false;
  vecStringValue.clear();
  bool bFound = false;
  while (pNode)
  {
    const TiXmlNode *pChild = pNode->FirstChild();
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
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode) return false;
  strStringPath = pNode->FirstChild() ? pNode->FirstChild()->Value() : "";
  strStringPath.Replace('/', '\\');
  return true;
}

bool CGUIControlFactory::GetTexture(const TiXmlNode* pRootNode, const char* strTag, CImage &image)
{
  const TiXmlElement* pNode = pRootNode->FirstChildElement(strTag);
  if (!pNode) return false;
  const char *border = pNode->Attribute("border");
  if (border)
  {
    // format is border="left,right,top,bottom"
    CStdStringArray borders;
    StringUtils::SplitString(border, ",", borders);
    if (borders.size() == 1)
    {
      float left = (float)atof(borders[0].c_str());
      image.border.left = left;
      image.border.top = left;
      image.border.right = left;
      image.border.bottom = left;
    }
    else if (borders.size() == 4)
    {
      image.border.left = (float)atof(borders[0].c_str());
      image.border.top = (float)atof(borders[1].c_str());
      image.border.right = (float)atof(borders[2].c_str());
      image.border.bottom = (float)atof(borders[3].c_str());
    }
  }
  image.file = pNode->FirstChild() ? pNode->FirstChild()->Value() : "";
  image.file.Replace('/', '\\');
  return true;
}

bool CGUIControlFactory::GetAlignment(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
  if (!pNode || !pNode->FirstChild()) return false;

  CStdString strAlign = pNode->FirstChild()->Value();
  if (strAlign == "right") dwAlignment = XBFONT_RIGHT;
  else if (strAlign == "center") dwAlignment = XBFONT_CENTER_X;
  else dwAlignment = XBFONT_LEFT;
  return true;
}

bool CGUIControlFactory::GetAlignmentY(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag );
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

bool CGUIControlFactory::GetConditionalVisibility(const TiXmlNode* control, int &condition, bool &allowHiddenFocus)
{
  const TiXmlElement* node = control->FirstChildElement("visible");
  if (!node) return false;
  allowHiddenFocus = false;
  vector<CStdString> conditions;
  while (node)
  {
    const char *hidden = node->Attribute("allowhiddenfocus");
    if (hidden && strcmpi(hidden, "true") == 0)
      allowHiddenFocus = true;
    // add to our condition string
    if (!node->NoChildren())
      conditions.push_back(node->FirstChild()->Value());
    node = node->NextSiblingElement("visible");
  }
  if (!conditions.size())
    return false;
  if (conditions.size() == 1)
    condition = g_infoManager.TranslateString(conditions[0]);
  else
  { // multiple conditions should be anded together
    CStdString conditionString = "[";
    for (unsigned int i = 0; i < conditions.size() - 1; i++)
      conditionString += conditions[i] + "] + [";
    conditionString += conditions[conditions.size() - 1] + "]";
    condition = g_infoManager.TranslateString(conditionString);
  }
  return (condition != 0);
}

bool CGUIControlFactory::GetConditionalVisibility(const TiXmlNode *control, int &condition)
{
  bool allowHiddenFocus;
  return GetConditionalVisibility(control, condition, allowHiddenFocus);
}

bool CGUIControlFactory::GetAnimations(const TiXmlNode *control, vector<CAnimation> &animations)
{
  const TiXmlElement* node = control->FirstChildElement("animation");
  bool ret = false;
  if (node)
    animations.clear();
  while (node)
  {
    ret = true;
    if (node->FirstChild())
    {
      CAnimation anim;
      anim.Create(node);
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

CStdString CGUIControlFactory::GetType(const TiXmlElement *pControlNode)
{
  CStdString type;
  const char *szType = pControlNode->Attribute("type");
  if (szType)
    type = szType;
  else  // backward compatibility - not desired
    XMLUtils::GetString(pControlNode, "type", type);
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  // check if we are a <controlgroup>
  if (strcmpi(pControlNode->Value(), "controlgroup") == 0)
    type = "group";
#endif
  return type;
}

CGUIControl* CGUIControlFactory::Create(DWORD dwParentId, CGUIControl *group, TiXmlElement* pControlNode)
{
  // resolve any <include> tag's in this control
  g_SkinInfo.ResolveIncludes(pControlNode);

  // get the control type
  CStdString strType = GetType(pControlNode);

  // resolve again with strType set so that <default> tags are added
  g_SkinInfo.ResolveIncludes(pControlNode, strType);

  int id = 0;
  float posX = 0, posY = 0;
  float width = 0, height = 0;

  DWORD left = 0, right = 0, up = 0, down = 0;
  DWORD pageControl = 0;
  DWORD dwColorDiffuse = 0xFFFFFFFF;
  DWORD defaultControl = 0;
  CStdString strTmp;
  vector<int> vecInfo;
  vector<string> vecLabel;
  string strLabel;
  int iUrlSet=0;
  int iToggleSelect;

  DWORD dwItems;
  float spinWidth = 16;
  float spinHeight = 16;
  float spinPosX, spinPosY;
  float checkWidth, checkHeight;
  CStdString strSubType;
  int iType = SPIN_CONTROL_TYPE_TEXT;
  int iMin = 0;
  int iMax = 100;
  int iInterval = 1;
  float fMin = 0.0f;
  float fMax = 1.0f;
  float fInterval = 0.1f;
  bool bReverse = false;
  CImage textureBackground, textureLeft, textureRight, textureMid, textureOverlay;
  CImage textureNib, textureNibFocus, textureBar, textureBarFocus;
  CImage textureLeftFocus, textureRightFocus;
  CImage textureUp, textureDown;
  CImage textureUpFocus, textureDownFocus;
  CImage texture;
  CImage textureCheckMark, textureCheckMarkNF;
  CImage textureFocus, textureNoFocus;
  CImage textureAltFocus, textureAltNoFocus;
  CImage textureRadioFocus, textureRadioNoFocus;
  CImage imageNoFocus, imageFocus;
  DWORD dwColorKey = 0;
  CStdString strSuffix = "";

  float controlOffsetX = 0;
  float controlOffsetY = 0;

  float itemWidth = 16, itemHeight = 16;
  float sliderWidth = 150, sliderHeight = 16;
  float textureWidthBig = 128;
  float textureHeightBig = 128;
  float textureHeight = 30;
  float textureWidth = 80;
  float itemWidthBig = 150;
  float itemHeightBig = 150;
  DWORD dwDisposition = 0;

  float spaceBetweenItems = 2;
  bool bHasPath = false;
  vector<CStdString> clickActions;
  vector<CStdString> focusActions;
  CStdString strTitle = "";
  CStdString strRSSTags = "";

  DWORD dwThumbAlign = 0;
#ifdef HAS_RAM_CONTROL
  float thumbSpaceX = 6;
  float thumbSpaceY = 25;
  float textSpaceY = 12;
  CStdString strDefaultThumb;
#endif

  float thumbXPos = 4;
  float thumbYPos = 10;
  float thumbWidth = 64;
  float thumbHeight = 64;

  float thumbXPosBig = 14;
  float thumbYPosBig = 14;
  float thumbWidthBig = 100;
  float thumbHeightBig = 100;
  DWORD dwBuddyControlID = 0;
  int iNumSlots = 7;
  float buttonGap = 5;
  int iDefaultSlot = 2;
  int iMovementRange = 2;
  bool bHorizontal = false;
  int iAlpha = 0;
  bool bWrapAround = true;
  bool bSmoothScrolling = true;
  CGUIImage::GUIIMAGE_ASPECT_RATIO aspectRatio = CGUIImage::ASPECT_RATIO_STRETCH;
  if (strType == "thumbnailpanel")  // default for thumbpanel is keep
    aspectRatio = CGUIImage::ASPECT_RATIO_KEEP;

  int iVisibleCondition = 0;
  bool allowHiddenFocus = false;

  vector<CAnimation> animations;

  bool bScrollLabel = false;
  bool bPulse = true;
  CStdString texturePath;
  DWORD timePerImage = 0;
  DWORD fadeTime = 0;
  bool randomized = false;
  bool loop = true;
  bool wrapMultiLine = false;
  CGUIThumbnailPanel::LABEL_STATE labelState = CGUIThumbnailPanel::SHOW_ALL;
  ORIENTATION orientation = VERTICAL;
  bool showOnePage = true;

  CLabelInfo labelInfo;
  CLabelInfo labelInfo2;
  CLabelInfo spinInfo;

  DWORD dwTextColor3 = labelInfo.textColor;

  float radioWidth = 0;
  float radioHeight = 0;
  float radioPosX = 0;
  float radioPosY = 0;

  CStdString altLabel;

  int focusPosition = 0;
  int scrollTime = 200;

  /////////////////////////////////////////////////////////////////////////////
  // Read control properties from XML
  //

#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  // check if we are a <controlgroup>
  if (strcmpi(pControlNode->Value(), "controlgroup") == 0)
  {
    if (pControlNode->Attribute("id", &id))
      id += 9000;       // offset at 9000 for old controlgroups
                        // NOTE: An old control group with no id means that it can't be focused
                        //       Which isn't too good :(
                        //       We check for this in OnWindowLoaded()
  }
  else
#endif
  if (!pControlNode->Attribute("id", &id))
    XMLUtils::GetInt(pControlNode, "id", id);       // backward compatibility - not desired
  // TODO: Perhaps we should check here whether id is valid for focusable controls
  // such as buttons etc.  For labels/fadelabels/images it does not matter

  XMLUtils::GetFloat(pControlNode, "posx", posX);
  XMLUtils::GetFloat(pControlNode, "posy", posY);
  // Convert these from relative coords
  CStdString pos;
  XMLUtils::GetString(pControlNode, "posx", pos);
  if (pos.Right(1) == "r")
  {
    if (group && group->GetWidth())
      posX = group->GetXPosition() + group->GetWidth() - posX;
    else
      posX = g_graphicsContext.GetWidth() - posX;
  }
  else if (group)
    posX += group->GetXPosition();
  XMLUtils::GetString(pControlNode, "posy", pos);
  if (pos.Right(1) == "r")
  {
    if (group && group->GetHeight())
      posY = group->GetYPosition() + group->GetHeight() - posY;
    else
      posY = g_graphicsContext.GetHeight() - posY;
  }
  else if (group)
    posY += group->GetYPosition();

  XMLUtils::GetFloat(pControlNode, "width", width);
  XMLUtils::GetFloat(pControlNode, "height", height);
#ifdef HAS_RAM_CONTROL
  XMLUtils::GetFloat(pControlNode, "textspacey", textSpaceY);
  XMLUtils::GetFloat(pControlNode, "gfxthumbwidth", thumbWidth);
  XMLUtils::GetFloat(pControlNode, "gfxthumbheight", thumbHeight);
  XMLUtils::GetFloat(pControlNode, "gfxthumbspacex", thumbSpaceX);
  XMLUtils::GetFloat(pControlNode, "gfxthumbspacey", thumbSpaceY);
  XMLUtils::GetString(pControlNode, "gfxthumbdefault", strDefaultThumb);
#endif
  XMLUtils::GetFloat(pControlNode, "controloffsetx", controlOffsetX);
  XMLUtils::GetFloat(pControlNode, "controloffsety", controlOffsetY);

  if (!XMLUtils::GetDWORD(pControlNode, "onup" , up ))
  {
    up = id - 1;
  }
  if (!XMLUtils::GetDWORD(pControlNode, "ondown" , down))
  {
    down = id + 1;
  }
  if (!XMLUtils::GetDWORD(pControlNode, "onleft" , left ))
  {
    left = id;
  }
  if (!XMLUtils::GetDWORD(pControlNode, "onright", right))
  {
    right = id;
  }

  XMLUtils::GetDWORD(pControlNode, "defaultcontrol", defaultControl);
  XMLUtils::GetDWORD(pControlNode, "pagecontrol", pageControl);

  XMLUtils::GetHex(pControlNode, "colordiffuse", dwColorDiffuse);
  
  GetConditionalVisibility(pControlNode, iVisibleCondition, allowHiddenFocus);
  GetAnimations(pControlNode, animations);

  XMLUtils::GetHex(pControlNode, "textcolor", labelInfo.textColor);
  XMLUtils::GetHex(pControlNode, "focusedcolor", labelInfo.focusedColor);
  XMLUtils::GetHex(pControlNode, "disabledcolor", labelInfo.disabledColor);
  XMLUtils::GetHex(pControlNode, "shadowcolor", labelInfo.shadowColor);
  XMLUtils::GetHex(pControlNode, "selectedcolor", labelInfo.selectedColor);
  XMLUtils::GetFloat(pControlNode, "textoffsetx", labelInfo.offsetX);
  XMLUtils::GetFloat(pControlNode, "textoffsety", labelInfo.offsetY);
  XMLUtils::GetFloat(pControlNode, "textxoff", labelInfo.offsetX);
  XMLUtils::GetFloat(pControlNode, "textyoff", labelInfo.offsetY);
  XMLUtils::GetFloat(pControlNode, "textxoff2", labelInfo2.offsetX);
  XMLUtils::GetFloat(pControlNode, "textyoff2", labelInfo2.offsetY);
  int angle = 0;  // use the negative angle to compensate for our vertically flipped cartesian plane
  if (XMLUtils::GetInt(pControlNode, "angle", angle)) labelInfo.angle = CAngle(-angle);
  CStdString strFont;
  if (XMLUtils::GetString(pControlNode, "font", strFont))
    labelInfo.font = g_fontManager.GetFont(strFont);
  GetAlignment(pControlNode, "align", labelInfo.align);
  DWORD alignY = 0;
  if (GetAlignmentY(pControlNode, "aligny", alignY))
    labelInfo.align |= alignY;
  if (XMLUtils::GetFloat(pControlNode, "textwidth", labelInfo.width))
    labelInfo.align |= XBFONT_TRUNCATED;
  labelInfo2.selectedColor = labelInfo.selectedColor;
  XMLUtils::GetHex(pControlNode, "selectedcolor2", labelInfo2.selectedColor);
  XMLUtils::GetHex(pControlNode, "textcolor2", labelInfo2.textColor);
  XMLUtils::GetHex(pControlNode, "focusedcolor2", labelInfo2.focusedColor);
  labelInfo2.font = labelInfo.font;
  if (XMLUtils::GetString(pControlNode, "font2", strFont))
    labelInfo2.font = g_fontManager.GetFont(strFont);

  GetMultipleString(pControlNode, "onclick", clickActions);
  GetMultipleString(pControlNode, "onfocus", focusActions);

  vector<CStdString> strVecInfo;
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
  GetTexture(pControlNode, "texturefocus", textureFocus);
  GetTexture(pControlNode, "texturenofocus", textureNoFocus);
  GetTexture(pControlNode, "alttexturefocus", textureAltFocus);
  GetTexture(pControlNode, "alttexturenofocus", textureAltNoFocus);
  CStdString strToggleSelect;
  XMLUtils::GetString(pControlNode, "usealttexture", strToggleSelect);
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  if (g_SkinInfo.GetVersion() < 2.1 && strToggleSelect.IsEmpty() && strType == "togglebutton")
  { // swap them over
    CImage temp = textureFocus;
    textureFocus = textureAltFocus;
    textureAltFocus = temp;
    temp = textureNoFocus;
    textureNoFocus = textureAltNoFocus;
    textureAltNoFocus = temp;
  }
#endif
  iToggleSelect = g_infoManager.TranslateString(strToggleSelect);
  XMLUtils::GetString(pControlNode, "selected", strToggleSelect);
  iToggleSelect = g_infoManager.TranslateString(strToggleSelect);
  XMLUtils::GetDWORD(pControlNode, "bitmaps", dwItems);

  XMLUtils::GetBoolean(pControlNode, "haspath", bHasPath);

  GetTexture(pControlNode, "textureup", textureUp);
  GetTexture(pControlNode, "texturedown", textureDown);
  GetTexture(pControlNode, "textureupfocus", textureUpFocus);
  GetTexture(pControlNode, "texturedownfocus", textureDownFocus);

  GetTexture(pControlNode, "textureleft", textureLeft);
  GetTexture(pControlNode, "textureright", textureRight);
  GetTexture(pControlNode, "textureleftfocus", textureLeftFocus);
  GetTexture(pControlNode, "texturerightfocus", textureRightFocus);

  XMLUtils::GetHex(pControlNode, "spincolor", spinInfo.textColor);
  if (XMLUtils::GetString(pControlNode, "spinfont", strFont))
    spinInfo.font = g_fontManager.GetFont(strFont);
  if (!spinInfo.font) spinInfo.font = labelInfo.font;

  XMLUtils::GetFloat(pControlNode, "spinwidth", spinWidth);
  XMLUtils::GetFloat(pControlNode, "spinheight", spinHeight);
  XMLUtils::GetFloat(pControlNode, "spinposx", spinPosX);
  XMLUtils::GetFloat(pControlNode, "spinposy", spinPosY);

  XMLUtils::GetFloat(pControlNode, "markwidth", checkWidth);
  XMLUtils::GetFloat(pControlNode, "markheight", checkHeight);
  XMLUtils::GetFloat(pControlNode, "sliderwidth", sliderWidth);
  XMLUtils::GetFloat(pControlNode, "sliderheight", sliderHeight);
  GetTexture(pControlNode, "texturecheckmark", textureCheckMark);
  GetTexture(pControlNode, "texturecheckmarknofocus", textureCheckMarkNF);
  GetTexture(pControlNode, "textureradiofocus", textureRadioFocus);
  GetTexture(pControlNode, "textureradionofocus", textureRadioNoFocus);

  GetTexture(pControlNode, "texturesliderbackground", textureBackground);
  GetTexture(pControlNode, "texturesliderbar", textureBar);
  GetTexture(pControlNode, "texturesliderbarfocus", textureBarFocus);
  GetTexture(pControlNode, "textureslidernib", textureNib);
  GetTexture(pControlNode, "textureslidernibfocus", textureNibFocus);
  XMLUtils::GetDWORD(pControlNode, "disposition", dwDisposition);

  XMLUtils::GetString(pControlNode, "title", strTitle);
  XMLUtils::GetString(pControlNode, "tagset", strRSSTags);
  XMLUtils::GetHex(pControlNode, "headlinecolor", labelInfo2.textColor);
  XMLUtils::GetHex(pControlNode, "titlecolor", dwTextColor3);

  if (XMLUtils::GetString(pControlNode, "subtype", strSubType))
  {
    strSubType.ToLower();

    if ( strSubType == "int")
      iType = SPIN_CONTROL_TYPE_INT;
    else if ( strSubType == "page")
      iType = SPIN_CONTROL_TYPE_PAGE;
    else if ( strSubType == "float")
      iType = SPIN_CONTROL_TYPE_FLOAT;
    else
      iType = SPIN_CONTROL_TYPE_TEXT;
  }

  if (!GetIntRange(pControlNode, "range", iMin, iMax, iInterval))
  {
    GetFloatRange(pControlNode, "range", fMin, fMax, fInterval);
  }

  XMLUtils::GetBoolean(pControlNode, "reverse", bReverse);

  CStdString hideLabels;
  if (XMLUtils::GetString(pControlNode, "hidelabels", hideLabels))
  {
    if (hideLabels.Equals("all"))
      labelState = CGUIThumbnailPanel::HIDE_ALL;
    else if (hideLabels.Equals("files"))
      labelState = CGUIThumbnailPanel::HIDE_FILES;
    else if (hideLabels.Equals("folders"))
      labelState = CGUIThumbnailPanel::HIDE_FOLDERS;
    else
      labelState = CGUIThumbnailPanel::SHOW_ALL;
  }

  GetTexture(pControlNode, "texturebg", textureBackground);
  GetTexture(pControlNode, "lefttexture", textureLeft);
  GetTexture(pControlNode, "midtexture", textureMid);
  GetTexture(pControlNode, "righttexture", textureRight);
  GetTexture(pControlNode, "overlaytexture", textureOverlay);
  GetTexture(pControlNode, "texture", texture);
  XMLUtils::GetHex(pControlNode, "colorkey", dwColorKey);

  XMLUtils::GetString(pControlNode, "suffix", strSuffix);

  XMLUtils::GetFloat(pControlNode, "itemwidth", itemWidth);
  XMLUtils::GetFloat(pControlNode, "itemheight", itemHeight);
  XMLUtils::GetFloat(pControlNode, "spacebetweenitems", spaceBetweenItems);

  GetTexture(pControlNode, "imagefolder", imageNoFocus);
  GetTexture(pControlNode, "imagefolderfocus", imageFocus);
  XMLUtils::GetFloat(pControlNode, "texturewidth", textureWidth);
  XMLUtils::GetFloat(pControlNode, "textureheight", textureHeight);

  XMLUtils::GetFloat(pControlNode, "thumbwidth", thumbWidth);
  XMLUtils::GetFloat(pControlNode, "thumbheight", thumbHeight);
  XMLUtils::GetFloat(pControlNode, "thumbposx", thumbXPos);
  XMLUtils::GetFloat(pControlNode, "thumbposy", thumbYPos);

  GetAlignment(pControlNode, "thumbalign", dwThumbAlign);

  XMLUtils::GetFloat(pControlNode, "thumbwidthbig", thumbWidthBig);
  XMLUtils::GetFloat(pControlNode, "thumbheightbig", thumbHeightBig);
  XMLUtils::GetFloat(pControlNode, "thumbposxbig", thumbXPosBig);
  XMLUtils::GetFloat(pControlNode, "thumbposybig", thumbYPosBig);

  XMLUtils::GetFloat(pControlNode, "texturewidthbig", textureWidthBig);
  XMLUtils::GetFloat(pControlNode, "textureheightbig", textureHeightBig);
  XMLUtils::GetFloat(pControlNode, "itemwidthbig", itemWidthBig);
  XMLUtils::GetFloat(pControlNode, "itemheightbig", itemHeightBig);

  int labelNumber = 0;
  if (XMLUtils::GetInt(pControlNode, "number", labelNumber))
  {
    CStdString label;
    label.Format("%i", labelNumber);
    strLabel = label;
  }
  vector<CStdString> strVecLabel;
  if (GetMultipleString(pControlNode, "label", strVecLabel))
  {
    CStdString label;
    vecLabel.clear();
    for (unsigned int i = 0; i < strVecLabel.size(); i++)
    {
      label = strVecLabel[i];
      if (label.size() > 0)
      {
        if (label[0] != '-')
        {
          if (StringUtils::IsNaturalNumber(label))
          {
            DWORD dwLabelID = atol(label.c_str());
            label = g_localizeStrings.Get(dwLabelID);
          }
          else
          { // TODO: UTF-8: What if the xml is encoded as UTF-8 already?
            CStdString utf8String;
            g_charsetConverter.stringCharsetToUtf8(label, utf8String);
            label = utf8String;
          }
          vecLabel.push_back(label);
        }
      }
      if (i == 0 && vecLabel.size())
        strLabel = vecLabel[0];
    }
  }
  if (XMLUtils::GetString(pControlNode, "altlabel", altLabel))
  {
    if (StringUtils::IsNaturalNumber(altLabel))
      altLabel = g_localizeStrings.Get(atoi(altLabel.c_str()));
    else
    { // TODO: UTF-8: What if the xml is encoded as UTF-8 already?
      g_charsetConverter.stringCharsetToUtf8(altLabel);
    }
  }

  XMLUtils::GetBoolean(pControlNode, "wrapmultiline", wrapMultiLine);
  XMLUtils::GetInt(pControlNode,"urlset",iUrlSet);

  // stuff for button scroller
  if ( XMLUtils::GetString(pControlNode, "orientation", strTmp) )
  {
    if (strTmp.ToLower() == "horizontal")
    {
      bHorizontal = true;
      orientation = HORIZONTAL;
    }
  }
  XMLUtils::GetFloat(pControlNode, "buttongap", buttonGap);
  XMLUtils::GetFloat(pControlNode, "itemgap", buttonGap);
  XMLUtils::GetInt(pControlNode, "numbuttons", iNumSlots);
  XMLUtils::GetInt(pControlNode, "movement", iMovementRange);
  XMLUtils::GetInt(pControlNode, "defaultbutton", iDefaultSlot);
  XMLUtils::GetInt(pControlNode, "alpha", iAlpha);
  XMLUtils::GetBoolean(pControlNode, "wraparound", bWrapAround);
  XMLUtils::GetBoolean(pControlNode, "smoothscrolling", bSmoothScrolling);
  bool keepAR;
  if (XMLUtils::GetBoolean(pControlNode, "keepaspectratio", keepAR))
    aspectRatio = CGUIImage::ASPECT_RATIO_KEEP;
  CStdString aspect;
  if (XMLUtils::GetString(pControlNode, "aspectratio", aspect))
  {
    if (aspect.CompareNoCase("keep") == 0) aspectRatio = CGUIImage::ASPECT_RATIO_KEEP;
    else if (aspect.CompareNoCase("scale") == 0) aspectRatio = CGUIImage::ASPECT_RATIO_SCALE;
    else if (aspect.CompareNoCase("center") == 0) aspectRatio = CGUIImage::ASPECT_RATIO_CENTER;
  }
  XMLUtils::GetBoolean(pControlNode, "scroll", bScrollLabel);
  XMLUtils::GetBoolean(pControlNode,"pulseonselect", bPulse);

  GetPath(pControlNode,"imagepath", texturePath);
  XMLUtils::GetDWORD(pControlNode,"timeperimage", timePerImage);
  XMLUtils::GetDWORD(pControlNode,"fadetime", fadeTime);
  XMLUtils::GetBoolean(pControlNode, "randomize", randomized);
  XMLUtils::GetBoolean(pControlNode, "loop", loop);

  XMLUtils::GetFloat(pControlNode, "radiowidth", radioWidth);
  XMLUtils::GetFloat(pControlNode, "radioheight", radioHeight);
  XMLUtils::GetFloat(pControlNode, "radioposx", radioPosX);
  XMLUtils::GetFloat(pControlNode, "radioposy", radioPosY);

  XMLUtils::GetBoolean(pControlNode, "showonepage", showOnePage);
  XMLUtils::GetInt(pControlNode, "focusposition", focusPosition);
  XMLUtils::GetInt(pControlNode, "scrolltime", scrollTime);

  if (strType == "group" || strType == "grouplist")
  {
    if (!width)
    {
      if (group)
        width = max(group->GetWidth() + group->GetXPosition() - posX, 0);
      else
        width = max(g_graphicsContext.GetWidth() - posX, 0);
    }
    if (!height)
    {
      if (group)
        height = max(group->GetHeight() + group->GetYPosition() - posY, 0);
      else
        height = max(g_graphicsContext.GetHeight() - posY, 0);
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // Instantiate a new control using the properties gathered above
  //

  if (strType == "group")
  {
    CGUIControlGroup* pControl = new CGUIControlGroup(
      dwParentId, id, posX, posY, width, height);

    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
    if (g_SkinInfo.GetVersion() < 2.1)
      pControl->SetNavigation(id, id, id, id);
    else
#endif
    pControl->SetNavigation(up, down, left, right);
    pControl->SetDefaultControl(defaultControl);
    return pControl;
  }
  else if (strType == "grouplist")
  {
    CGUIControlGroupList* pControl = new CGUIControlGroupList(
      dwParentId, id, posX, posY, width, height, buttonGap, pageControl, orientation);

    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetNavigation(up, down, left, right);
    return pControl;
  }
  else if (strType == "label")
  {
    CGUILabelControl* pControl = new CGUILabelControl(
      dwParentId, id, posX, posY, width, height,
      strLabel, labelInfo, bHasPath);

    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetInfo(vecInfo.size() ? vecInfo[0] : 0);
    pControl->SetWidthControl(bScrollLabel);
    pControl->SetWrapMultiLine(wrapMultiLine);
    return pControl;
  }
  else if (strType == "edit")
  {
    CGUIEditControl* pControl = new CGUIEditControl(
      dwParentId, id, posX, posY, width, height,
      labelInfo, strLabel);

    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    return pControl;
  }
  else if (strType == "videowindow")
  {
    CGUIVideoControl* pControl = new CGUIVideoControl(
      dwParentId, id, posX, posY, width, height);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    return pControl;
  }
  else if (strType == "fadelabel")
  {
    CGUIFadeLabelControl* pControl = new CGUIFadeLabelControl(
      dwParentId, id, posX, posY, width, height,
      labelInfo);

    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetLabel(vecLabel);
    pControl->SetInfo(vecInfo);
    return pControl;
  }
  else if (strType == "rss")
  {
    CGUIRSSControl* pControl = new CGUIRSSControl(
      dwParentId, id, posX, posY, width, height,
      labelInfo, dwTextColor3, labelInfo2.textColor, strRSSTags);

    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    std::map<int, std::pair<std::vector<int>,std::vector<string> > >::iterator iter=g_settings.m_mapRssUrls.find(iUrlSet);
    if (iter != g_settings.m_mapRssUrls.end())
    {
      pControl->SetUrls(iter->second.second);
      pControl->SetIntervals(iter->second.first);
    }
    else
      CLog::Log(LOGERROR,"invalid rss url set referenced in skin");
    return pControl;
  }
#ifdef HAS_RAM_CONTROL
  else if (strType == "ram")
  {
    CGUIRAMControl* pControl = new CGUIRAMControl(
      dwParentId, id, posX, posY, width, height,
      labelInfo, labelInfo2);

    pControl->SetTextSpacing(textSpaceY);
    pControl->SetThumbAttributes(thumbWidth, thumbHeight, thumbSpaceX, thumbSpaceY, strDefaultThumb);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
#endif
  else if (strType == "console")
  {
    CGUIConsoleControl* pControl = new CGUIConsoleControl(
      dwParentId, id, posX, posY, width, height,
      labelInfo, labelInfo.textColor, labelInfo2.textColor, dwTextColor3, labelInfo.selectedColor);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetNavigation(up, down, left, right);
    return pControl;
  }
  else if (strType == "button")
  {
    CGUIButtonControl* pControl = new CGUIButtonControl(
      dwParentId, id, posX, posY, width, height,
      textureFocus, textureNoFocus,
      labelInfo);

    pControl->SetLabel(strLabel);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetClickActions(clickActions);
    pControl->SetFocusActions(focusActions);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "togglebutton")
  {
    CGUIToggleButtonControl* pControl = new CGUIToggleButtonControl(
      dwParentId, id, posX, posY, width, height,
      textureFocus, textureNoFocus,
      textureAltFocus, textureAltNoFocus, labelInfo);

    pControl->SetLabel(strLabel);
    pControl->SetAltLabel(altLabel);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetClickActions(clickActions);
    pControl->SetFocusActions(focusActions);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetToggleSelect(iToggleSelect);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "checkmark")
  {
    CGUICheckMarkControl* pControl = new CGUICheckMarkControl(
      dwParentId, id, posX, posY, width, height,
      textureCheckMark, textureCheckMarkNF,
      checkWidth, checkHeight, labelInfo);

    pControl->SetLabel(strLabel);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "radiobutton")
  {
    CGUIRadioButtonControl* pControl = new CGUIRadioButtonControl(
      dwParentId, id, posX, posY, width, height,
      textureFocus, textureNoFocus,
      labelInfo,
      textureRadioFocus, textureRadioNoFocus);

    pControl->SetLabel(strLabel);
    pControl->SetRadioDimensions(radioPosX, radioPosY, radioWidth, radioHeight);
    pControl->SetToggleSelect(iToggleSelect);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetClickActions(clickActions);
    pControl->SetFocusActions(focusActions);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "spincontrol")
  {
    CGUISpinControl* pControl = new CGUISpinControl(
      dwParentId, id, posX, posY, width, height,
      textureUp, textureDown, textureUpFocus, textureDownFocus,
      labelInfo, iType);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetReverse(bReverse);
    pControl->SetPulseOnSelect(bPulse);

    if (iType == SPIN_CONTROL_TYPE_INT)
    {
      pControl->SetRange(iMin, iMax);
    }
    else if (iType == SPIN_CONTROL_TYPE_PAGE)
    {
      pControl->SetRange(iMin, iMax);
      pControl->SetShowRange(true);
      pControl->SetReverse(false);
      pControl->SetShowOnePage(showOnePage);
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
      dwParentId, id, posX, posY, width, height,
      textureBar, textureNib, textureNibFocus, SPIN_CONTROL_TYPE_TEXT);

    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetInfo(vecInfo.size() ? vecInfo[0] : 0);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetControlOffsetX(controlOffsetX);
    pControl->SetControlOffsetY(controlOffsetY);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "sliderex")
  {
    labelInfo.align |= XBFONT_CENTER_Y;    // always center text vertically
    CGUISettingsSliderControl* pControl = new CGUISettingsSliderControl(
      dwParentId, id, posX, posY, width, height, sliderWidth, sliderHeight, textureFocus, textureNoFocus,
      textureBar, textureNib, textureNibFocus, labelInfo, SPIN_CONTROL_TYPE_TEXT);

    pControl->SetText(strLabel);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetInfo(vecInfo.size() ? vecInfo[0] : 0);
    pControl->SetNavigation(up, down, left, right);
    return pControl;
  }
  else if (strType == "scrollbar")
  {
    CGUIScrollBar* pControl = new CGUIScrollBar(
      dwParentId, id, posX, posY, width, height,
      textureBackground, textureBar, textureBarFocus, textureNib, textureNibFocus, orientation, showOnePage);

    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "progress")
  {
    CGUIProgressControl* pControl = new CGUIProgressControl(
      dwParentId, id, posX, posY, width, height,
      textureBackground, textureLeft, textureMid, textureRight, textureOverlay);

    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetInfo(vecInfo.size() ? vecInfo[0] : 0);
    return pControl;
  }
  else if (strType == "image")
  {
    CGUIImage* pControl = new CGUIImage(
      dwParentId, id, posX, posY, width, height, texture, dwColorKey);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetAspectRatio(aspectRatio);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetInfo(vecInfo.size() ? vecInfo[0] : 0);
    return pControl;
  }
  else if (strType == "multiimage")
  {
    CGUIMultiImage* pControl = new CGUIMultiImage(
      dwParentId, id, posX, posY, width, height, texturePath, timePerImage, fadeTime, randomized, loop);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetAspectRatio(aspectRatio);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetInfo(vecInfo.size() ? vecInfo[0] : 0);
    return pControl;
  }
  else if (strType == "list")
  {
    CGUIListContainer* pControl = new CGUIListContainer(dwParentId, id, posX, posY, width, height, orientation, scrollTime);
    pControl->LoadLayout(pControlNode);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetPulseOnSelect(bPulse);
    pControl->SetPageControl(pageControl);
    return pControl;
  }
  else if (strType == "wraplist")
  {
    CGUIWrappingListContainer* pControl = new CGUIWrappingListContainer(dwParentId, id, posX, posY, width, height, orientation, scrollTime, focusPosition);
    pControl->LoadLayout(pControlNode);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetPulseOnSelect(bPulse);
    pControl->SetPageControl(pageControl);
    return pControl;
  }
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  else if (strType == "listcontrol")
  {
    // create the spin control
    CGUISpinControl *pSpin = new CGUISpinControl(dwParentId, id + 5000, posX + spinPosX, posY + spinPosY, spinWidth, spinHeight,
      textureUp, textureDown, textureUpFocus, textureDownFocus, spinInfo, SPIN_CONTROL_TYPE_PAGE);
    // spincontrol should be visible when our list is
    CStdString spinVis;
    spinVis.Format("control.isvisible(%i)", id);
    pSpin->SetVisibleCondition(g_infoManager.TranslateString(spinVis), false);
    pSpin->SetAnimations(animations);
    pSpin->SetParentControl(group);
    pSpin->SetNavigation(id, down, id, right);
    pSpin->SetSpinAlign(XBFONT_CENTER_Y | XBFONT_RIGHT, 0);

    labelInfo2.align |= XBFONT_RIGHT;
    if (labelInfo.align & XBFONT_CENTER_Y)
      labelInfo2.align |= XBFONT_CENTER_Y;
    CGUIListContainer* pControl = new CGUIListContainer(dwParentId, id, posX, posY, width, height - spinHeight - 5,
      labelInfo, labelInfo2, textureNoFocus, textureFocus, textureHeight, itemWidth, itemHeight, spaceBetweenItems, pSpin);

    pControl->SetPageControl(id + 5000);
    pControl->SetNavigation(up, down, left, id + 5000);

    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);

    // do we have a match for this?
    // pControl->SetScrollySuffix(strSuffix);
    // pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
#endif
  else if (strType == "listcontrolex")
  {
    CGUIListControlEx* pControl = new CGUIListControlEx(
      dwParentId, id, posX, posY, width, height,
      spinWidth, spinHeight,
      textureUp, textureDown,
      textureUpFocus, textureDownFocus,
      spinInfo, spinPosX, spinPosY,
      labelInfo, labelInfo2,
      textureNoFocus, textureFocus);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetScrollySuffix(strSuffix);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetImageDimensions(itemWidth, itemHeight);
    pControl->SetItemHeight(textureHeight);
    pControl->SetSpaceBetweenItems(spaceBetweenItems);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "textbox")
  {
    CGUITextBox* pControl = new CGUITextBox(
      dwParentId, id, posX, posY, width, height,
      spinWidth, spinHeight,
      textureUp, textureDown,
      textureUpFocus, textureDownFocus,
      spinInfo, spinPosX, spinPosY,
      labelInfo);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetPulseOnSelect(bPulse);
    pControl->SetPageControl(pageControl);
    return pControl;
  }
  else if (strType == "thumbnailpanel")
  {
    CGUIThumbnailPanel* pControl = new CGUIThumbnailPanel(
      dwParentId, id, posX, posY, width, height,
      imageNoFocus, imageFocus,
      spinWidth, spinHeight,
      textureUp, textureDown,
      textureUpFocus, textureDownFocus,
      spinInfo, spinPosX, spinPosY,
      labelInfo);

    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetScrollySuffix(strSuffix);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetThumbDimensions(thumbXPos, thumbYPos, thumbWidth, thumbHeight);
    pControl->SetTextureWidthBig(textureWidthBig);
    pControl->SetTextureHeightBig(textureHeightBig);
    pControl->SetItemWidthBig(itemWidthBig);
    pControl->SetItemHeightBig(itemHeightBig);
    pControl->SetTextureWidthLow(textureWidth);
    pControl->SetTextureHeightLow(textureHeight);
    pControl->SetItemWidthLow(itemWidth);
    pControl->SetItemHeightLow(itemHeight);
    pControl->SetThumbDimensionsLow(thumbXPos, thumbYPos, thumbWidth, thumbHeight);
    pControl->SetThumbDimensionsBig(thumbXPosBig, thumbYPosBig, thumbWidthBig, thumbHeightBig);
    pControl->SetThumbAlign(dwThumbAlign);
    pControl->SetAspectRatio(aspectRatio);
    pControl->SetPulseOnSelect(bPulse);
    pControl->SetLabelState(labelState);
    pControl->SetPageControl(pageControl);
    return pControl;
  }
  else if (strType == "selectbutton")
  {
    CGUISelectButtonControl* pControl = new CGUISelectButtonControl(
      dwParentId, id, posX, posY,
      width, height, textureFocus, textureNoFocus,
      labelInfo,
      textureBackground, textureLeft, textureLeftFocus, textureRight, textureRightFocus);

    pControl->SetLabel(strLabel);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "mover")
  {
    CGUIMoverControl* pControl = new CGUIMoverControl(
      dwParentId, id, posX, posY, width, height,
      textureFocus, textureNoFocus);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "resize")
  {
    CGUIResizeControl* pControl = new CGUIResizeControl(
      dwParentId, id, posX, posY, width, height,
      textureFocus, textureNoFocus);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "buttonscroller")
  {
    CGUIButtonScroller* pControl = new CGUIButtonScroller(
      dwParentId, id, posX, posY, width, height, buttonGap, iNumSlots, iDefaultSlot,
      iMovementRange, bHorizontal, iAlpha, bWrapAround, bSmoothScrolling,
      textureFocus, textureNoFocus, labelInfo);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetPulseOnSelect(bPulse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->LoadButtons(pControlNode);
    return pControl;
  }
  else if (strType == "spincontrolex")
  {
    CGUISpinControlEx* pControl = new CGUISpinControlEx(
      dwParentId, id, posX, posY, width, height, spinWidth, spinHeight,
      labelInfo, textureFocus, textureNoFocus, textureUp, textureDown, textureUpFocus, textureDownFocus,
      labelInfo, iType);

    pControl->SetText(strLabel);
    pControl->SetNavigation(up, down, left, right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    pControl->SetReverse(bReverse);
    pControl->SetText(strLabel);
    pControl->SetPulseOnSelect(bPulse);
    return pControl;
  }
  else if (strType == "visualisation")
  {
    CGUIVisualisationControl* pControl = new CGUIVisualisationControl(dwParentId, id, posX, posY, width, height);
    pControl->SetVisibleCondition(iVisibleCondition, allowHiddenFocus);
    pControl->SetAnimations(animations);
    pControl->SetParentControl(group);
    return pControl;
  }
  return NULL;
}

void CGUIControlFactory::ScaleElement(TiXmlElement *element, RESOLUTION fileRes, RESOLUTION destRes)
{
  if (element->FirstChild())
  {
    const char *value = element->FirstChild()->Value();
    if (value)
    {
      float v = (float)atof(value);
      CStdString name = element->Value();
      if (name == "posx" ||
          name == "width" ||
          name == "gfxthumbwidth" ||
          name == "gfxthumbspacex" ||
          name == "controloffsetx" ||
          name == "textoffsetx" ||
          name == "textxoff" ||
          name == "textxoff2" ||
          name == "textwidth" ||
          name == "spinwidth" ||
          name == "spinposx" ||
          name == "markwidth" ||
          name == "sliderwidth" ||
          name == "itemwidth" ||
          name == "texturewidth" ||
          name == "thumbwidth" ||
          name == "thumbposx" ||
          name == "thumbwidthbig" ||
          name == "thumbposxbig" ||
          name == "texturewidthbig" ||
          name == "itemwidthbig" ||
          name == "radiowidth" ||
          name == "radioposx")
      {
        // scale
        v *= (float)g_settings.m_ResInfo[destRes].iWidth / g_settings.m_ResInfo[fileRes].iWidth;
        CStdString floatValue;
        floatValue.Format("%f", v);
        element->FirstChild()->SetValue(floatValue);
      }
      else if (name == "posy" ||
          name == "height" ||
          name == "textspacey" ||
          name == "gfxthumbheight" ||
          name == "gfxthumbspacey" ||
          name == "controloffsety" ||
          name == "textoffsety" ||
          name == "textyoff" ||
          name == "textyoff2" ||
          name == "spinheight" ||
          name == "spinposy" ||
          name == "markheight" ||
          name == "sliderheight" ||
          name == "spacebetweenitems" ||
          name == "textureheight" ||
          name == "thumbheight" ||
          name == "thumbposy" ||
          name == "thumbheightbig" ||
          name == "thumbposybig" ||
          name == "textureheightbig" ||
          name == "itemheightbig" ||
          name == "buttongap" ||  // should really depend on orientation
          name == "radioheight" ||
          name == "radioposy")
      {
        // scale
        v *= (float)g_settings.m_ResInfo[destRes].iHeight / g_settings.m_ResInfo[fileRes].iHeight;
        CStdString floatValue;
        floatValue.Format("%f", v);
        element->FirstChild()->SetValue(floatValue);
      }
    }
  }
}
