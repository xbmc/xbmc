#include "stdafx.h"
#include "guicontrolfactory.h"
#include "localizestrings.h"
#include "guibuttoncontrol.h"
#include "guiRadiobuttoncontrol.h"
#include "guiSpinControl.h"
#include "guiListControl.h"
#include "guiImage.h"
#include "GUILabelControl.h"
#include "GUIFadeLabelControl.h"
#include "GUICheckMarkControl.h"
#include "GUIThumbnailPanel.h"
#include "GUIMButtonControl.h"
#include "GUIToggleButtonControl.h" 
#include "GUITextBox.h" 
#include "guiVideoControl.h"
#include "GUIProgressControl.h"
#include "GUISliderControl.h"
#include "GUISelectButtonControl.h"

CGUIControlFactory::CGUIControlFactory(void)
{
}

CGUIControlFactory::~CGUIControlFactory(void)
{
}

bool CGUIControlFactory::GetHex(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwHexValue)
{

	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	sscanf(pNode->FirstChild()->Value(),"%x", &dwHexValue );
	return true;
}
bool CGUIControlFactory::GetDWORD(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwDWORDValue)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	dwDWORDValue= atol(pNode->FirstChild()->Value());
	return true;
}
bool CGUIControlFactory::GetLong(const TiXmlNode* pRootNode, const char* strTag, long& lLongValue)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	lLongValue = atol(pNode->FirstChild()->Value());
	return true;
}
bool CGUIControlFactory::GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	iIntValue = atoi(pNode->FirstChild()->Value());
	return true;
}
bool CGUIControlFactory::GetBoolean(const TiXmlNode* pRootNode, const char* strTag, bool& bBoolValue)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	CStdString strEnabled=pNode->FirstChild()->Value();
	strEnabled.ToLower();
	if (strEnabled=="off" || strEnabled=="no"||strEnabled=="disabled") bBoolValue=false;
	else bBoolValue=true;
	return true;
}

bool CGUIControlFactory::GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	strStringValue=pNode->FirstChild()->Value();
	return true;
}
bool CGUIControlFactory::GetAlignment(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;

	CStdString strAlign=pNode->FirstChild()->Value();      
	if (strAlign=="right") dwAlignment=XBFONT_RIGHT;
	else if (strAlign=="center") dwAlignment=XBFONT_CENTER_X;
	else  dwAlignment=XBFONT_LEFT;
	return true;
}

CGUIControl* CGUIControlFactory::Create(DWORD dwParentId,const TiXmlNode* pControlNode, CGUIControl* pReference, bool bLoadReferences)
{
	CStdString strType;
	GetString(pControlNode,"type",strType);

  DWORD  			dwPosX=0,dwPosY=0;
  DWORD  			dwWidth=0, dwHeight=0;
  DWORD  			dwID=0,left=0,right=0,up=0,down=0;
	DWORD	 			dwColorDiffuse=0xFFFFFFFF;
	bool   			bVisible=true;
	wstring			strLabel=L"";
	CStdString  strFont="";
	CStdString  strTmp;
	DWORD     	dwTextColor=0xFFFFFFFF;
	DWORD				dwAlign=XBFONT_LEFT;
	CStdString  strTextureFocus,strTextureNoFocus;
	CStdString	strTextureAltFocus,strTextureAltNoFocus;
	DWORD				dwDisabledColor=0xffffffff;;
	int					iHyperLink=-1;
	DWORD				dwItems;
	CStdString  strUp,strDown;
	CStdString  strUpFocus,strDownFocus;
	DWORD				dwSpinColor=0xffffffff;
	DWORD				dwSpinWidth,dwSpinHeight,dwSpinPosX,dwSpinPosY;
	CStdString  strTextureCheckMark;
	CStdString  strTextureCheckMarkNF;
	DWORD				dwCheckWidth, dwCheckHeight;
	CStdString	strTextureRadioFocus,strTextureRadioNoFocus;
	CStdString	strSubType;
	int					iType=SPIN_CONTROL_TYPE_TEXT;
	bool				bReverse=false;
  bool        bShadow;
	CStdString	strTextureBg, strLeft,strRight,strMid,strMidFocus;
	CStdString	strLeftFocus, strRightFocus;
	CStdString	strTexture;
	DWORD 			dwColorKey=0xffffffff;
	DWORD 			dwSelectedColor;
	CStdString 	strButton,strButtonFocus;
	CStdString 	strSuffix="";
	CStdString 	strFont2="";
	int 				iTextXOff=0;
	int 				iTextYOff=0;
	int 				iTextXOff2=0;
	int 				iTextYOff2=0;
	DWORD			 	dwitemWidth=16, dwitemHeight=16;
  DWORD       textureWidthBig=128;
  DWORD       textureHeightBig=128;
  DWORD       itemWidthBig=150;
  DWORD       itemHeightBig=150;
	DWORD			 	dwTextColor2=dwTextColor;
	DWORD			 	dwSelectedColor2;
	int        	iSpace=2;
	int				 	iTextureHeight=30;
	CStdString 	strImage,strImageFocus;
	int				 	iTextureWidth=80;
	bool				bHasPath=false;
	CStdString	strScriptAction="";
  int         iThumbXPos=4;
  int         iThumbYPos=10;
  int         iThumbWidth=64;
  int         iThumbHeight=64;

  int         iThumbXPosBig=14;
  int         iThumbYPosBig=14;
  int         iThumbWidthBig=100;
  int         iThumbHeightBig=100;
  DWORD		dwBuddyControlID=0;
	
	// get defaults from reference control
	if (pReference)
	{
		bVisible			= pReference->IsVisible();
		dwColorDiffuse= pReference->GetColourDiffuse();
		dwPosX				= pReference->GetXPosition();
		dwPosY				= pReference->GetYPosition();
		dwWidth				= pReference->GetWidth();
		dwHeight			= pReference->GetHeight();
		if (strType=="label")
		{
			strFont			= ((CGUILabelControl*)pReference)->GetFontName();
			strLabel		= ((CGUILabelControl*)pReference)->GetLabel();
			dwTextColor = ((CGUILabelControl*)pReference)->GetTextColor();
			dwAlign			= ((CGUILabelControl*)pReference)->GetAlignment();
			dwDisabledColor = ((CGUILabelControl*)pReference)->GetDisabledColor();
		}
		if (strType=="fadelabel")
		{
			strFont			= ((CGUIFadeLabelControl*)pReference)->GetFontName();
			dwTextColor = ((CGUIFadeLabelControl*)pReference)->GetTextColor();
			dwAlign			= ((CGUIFadeLabelControl*)pReference)->GetAlignment();
		}
		if (strType=="button")
		{
			strTextureFocus	 = ((CGUIButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus= ((CGUIButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont					 = ((CGUIButtonControl*)pReference)->GetFontName();
			strLabel				 = ((CGUIButtonControl*)pReference)->GetLabel();
			dwTextColor			 = ((CGUIButtonControl*)pReference)->GetTextColor();
			dwDisabledColor  = ((CGUIButtonControl*)pReference)->GetDisabledColor() ;
			iHyperLink			 = ((CGUIButtonControl*)pReference)->GetHyperLink();
			strScriptAction	 = ((CGUIButtonControl*)pReference)->GetScriptAction();
		}
		if (strType=="togglebutton")
		{
			strTextureAltFocus  = ((CGUIToggleButtonControl*)pReference)->GetTexutureAltFocusName();
			strTextureAltNoFocus= ((CGUIToggleButtonControl*)pReference)->GetTexutureAltNoFocusName();
			strTextureFocus			= ((CGUIToggleButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus		= ((CGUIToggleButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont							= ((CGUIToggleButtonControl*)pReference)->GetFontName();
			strLabel						= ((CGUIToggleButtonControl*)pReference)->GetLabel();
			dwTextColor					= ((CGUIToggleButtonControl*)pReference)->GetTextColor();
			dwDisabledColor			= ((CGUIToggleButtonControl*)pReference)->GetDisabledColor() ;
			iHyperLink					= ((CGUIToggleButtonControl*)pReference)->GetHyperLink();
		}
		if (strType=="buttonM")
		{
			dwItems							= ((CGUIMButtonControl*)pReference)->GetItems();
			strTextureFocus			= ((CGUIMButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus		= ((CGUIMButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont							= ((CGUIMButtonControl*)pReference)->GetFontName();
			strLabel						= ((CGUIMButtonControl*)pReference)->GetLabel();
			dwTextColor					= ((CGUIMButtonControl*)pReference)->GetTextColor();
			dwDisabledColor			= ((CGUIMButtonControl*)pReference)->GetDisabledColor() ;
			iHyperLink					= ((CGUIMButtonControl*)pReference)->GetHyperLink();
		}
		if (strType=="checkmark")
		{
      bShadow             = ((CGUICheckMarkControl*)pReference)->GetShadow();
			strTextureCheckMark	= ((CGUICheckMarkControl*)pReference)->GetCheckMarkTextureName();
			strTextureCheckMarkNF	= ((CGUICheckMarkControl*)pReference)->GetCheckMarkTextureNameNF();
			dwCheckWidth				= ((CGUICheckMarkControl*)pReference)->GetCheckMarkWidth();
			dwCheckHeight				= ((CGUICheckMarkControl*)pReference)->GetCheckMarkHeight();
			dwAlign							= ((CGUICheckMarkControl*)pReference)->GetAlignment();
			strFont							= ((CGUICheckMarkControl*)pReference)->GetFontName();
			strLabel						= ((CGUICheckMarkControl*)pReference)->GetLabel();
			dwTextColor					= ((CGUICheckMarkControl*)pReference)->GetTextColor();
			dwDisabledColor			= ((CGUICheckMarkControl*)pReference)->GetDisabledColor();
		}
		if (strType=="radiobutton")
		{
			strTextureRadioFocus	= ((CGUIRadioButtonControl*)pReference)->GetTexutureRadioFocusName();;
			strTextureRadioNoFocus= ((CGUIRadioButtonControl*)pReference)->GetTexutureRadioNoFocusName();;
			strTextureFocus				= ((CGUIRadioButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus			= ((CGUIRadioButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont								= ((CGUIRadioButtonControl*)pReference)->GetFontName();
			strLabel							= ((CGUIRadioButtonControl*)pReference)->GetLabel();
			dwTextColor						= ((CGUIRadioButtonControl*)pReference)->GetTextColor();
			dwDisabledColor				= ((CGUIRadioButtonControl*)pReference)->GetDisabledColor() ;
			iHyperLink						= ((CGUIRadioButtonControl*)pReference)->GetHyperLink();
		}
		if (strType=="spincontrol")
		{
			strFont								= ((CGUISpinControl*)pReference)->GetFontName();
			dwTextColor						= ((CGUISpinControl*)pReference)->GetTextColor();
			dwAlign							  = ((CGUISpinControl*)pReference)->GetAlignment();
			strUp									= ((CGUISpinControl*)pReference)->GetTexutureUpName();
			strDown								= ((CGUISpinControl*)pReference)->GetTexutureDownName();
			strUpFocus						= ((CGUISpinControl*)pReference)->GetTexutureUpFocusName();
			strDownFocus					= ((CGUISpinControl*)pReference)->GetTexutureDownFocusName();
			iType									= ((CGUISpinControl*)pReference)->GetType();
			dwWidth								= ((CGUISpinControl*)pReference)->GetSpinWidth();
			dwHeight							= ((CGUISpinControl*)pReference)->GetSpinHeight();
			dwDisabledColor				= ((CGUISpinControl*)pReference)->GetDisabledColor() ;
		}
		if (strType=="slider")
		{
			strTextureBg	= ((CGUISliderControl*)pReference)->GetBackGroundTextureName();
			strMid				= ((CGUISliderControl*)pReference)->GetBackTextureMidName();
		}
		if (strType=="progress")
		{
			strTextureBg	= ((CGUIProgressControl*)pReference)->GetBackGroundTextureName();
			strLeft				= ((CGUIProgressControl*)pReference)->GetBackTextureLeftName();
			strMid				= ((CGUIProgressControl*)pReference)->GetBackTextureMidName();
			strRight			= ((CGUIProgressControl*)pReference)->GetBackTextureRightName();
		}
		if (strType=="image")
		{
			strTexture		= ((CGUIImage *)pReference)->GetFileName();
			dwColorKey		= ((CGUIImage *)pReference)->GetColorKey();
		}
		if (strType=="listcontrol")
		{
			strFont					=	((CGUIListControl*)pReference)->GetFontName();
			dwSpinWidth			=	((CGUIListControl*)pReference)->GetSpinWidth();
			dwSpinHeight		=	((CGUIListControl*)pReference)->GetSpinHeight();
			strUp						=	((CGUIListControl*)pReference)->GetTexutureUpName();
			strDown					=	((CGUIListControl*)pReference)->GetTexutureDownName();
			strUpFocus			=	((CGUIListControl*)pReference)->GetTexutureUpFocusName();
			strDownFocus		=	((CGUIListControl*)pReference)->GetTexutureDownFocusName();
			dwSpinColor			=	((CGUIListControl*)pReference)->GetSpinTextColor();
			dwSpinPosX			=	((CGUIListControl*)pReference)->GetSpinX();
			dwSpinPosY			=	((CGUIListControl*)pReference)->GetSpinY();
			dwTextColor			=	((CGUIListControl*)pReference)->GetTextColor();
			dwSelectedColor = ((CGUIListControl*)pReference)->GetSelectedColor();
			strButton				=	((CGUIListControl*)pReference)->GetButtonNoFocusName();
			strButtonFocus	=	((CGUIListControl*)pReference)->GetButtonFocusName();
			strSuffix				=	((CGUIListControl*)pReference)->GetSuffix();
			iTextXOff				=	((CGUIListControl*)pReference)->GetTextOffsetX();
			iTextYOff				=	((CGUIListControl*)pReference)->GetTextOffsetY();
			iTextXOff2			=	((CGUIListControl*)pReference)->GetTextOffsetX2();
			iTextYOff2			=	((CGUIListControl*)pReference)->GetTextOffsetY2();
			dwitemWidth			=	((CGUIListControl*)pReference)->GetImageWidth();
			dwitemHeight		=	((CGUIListControl*)pReference)->GetImageHeight();
			iTextureHeight	=	((CGUIListControl*)pReference)->GetItemHeight();
			iSpace					=	((CGUIListControl*)pReference)->GetSpace();
			dwTextColor2		=	((CGUIListControl*)pReference)->GetTextColor2(); 
			dwSelectedColor2=	((CGUIListControl*)pReference)->GetSelectedColor2();
			strFont2				=	((CGUIListControl*)pReference)->GetFontName2();
		}
		if (strType=="textbox")
		{
				strFont				= ((CGUITextBox*)pReference)->GetFontName();
				dwTextColor		= ((CGUITextBox*)pReference)->GetTextColor();
				dwSpinWidth		= ((CGUITextBox*)pReference)->GetSpinWidth();
				dwSpinHeight	= ((CGUITextBox*)pReference)->GetSpinHeight();
				strUp					= ((CGUITextBox*)pReference)->GetTexutureUpName();
				strDown				= ((CGUITextBox*)pReference)->GetTexutureDownName();
				strUpFocus		= ((CGUITextBox*)pReference)->GetTexutureUpFocusName();
				strDownFocus	= ((CGUITextBox*)pReference)->GetTexutureDownFocusName();
				dwSpinColor		= ((CGUITextBox*)pReference)->GetSpinTextColor();
				dwSpinPosX		= ((CGUITextBox*)pReference)->GetSpinX();
				dwSpinPosY		= ((CGUITextBox*)pReference)->GetSpinY();
				dwSpinWidth			=	((CGUITextBox*)pReference)->GetSpinWidth();
				dwSpinHeight		=	((CGUITextBox*)pReference)->GetSpinHeight();
		}
		if (strType=="thumbnailpanel")
		{
      
      textureWidthBig = ((CGUIThumbnailPanel*)pReference)->GetTextureWidthBig();
      textureHeightBig= ((CGUIThumbnailPanel*)pReference)->GetTextureHeightBig();
      itemWidthBig    = ((CGUIThumbnailPanel*)pReference)->GetItemWidthBig();
      itemHeightBig   = ((CGUIThumbnailPanel*)pReference)->GetItemHeightBig();

			strFont					= ((CGUIThumbnailPanel*)pReference)->GetFontName();
			strImage				= ((CGUIThumbnailPanel*)pReference)->GetNoFocusName();
			strImageFocus		= ((CGUIThumbnailPanel*)pReference)->GetFocusName();
			dwitemWidth			= ((CGUIThumbnailPanel*)pReference)->GetItemWidth();
			dwitemHeight		= ((CGUIThumbnailPanel*)pReference)->GetItemHeight();
			dwSpinWidth			= ((CGUIThumbnailPanel*)pReference)->GetSpinWidth();
			dwSpinHeight		= ((CGUIThumbnailPanel*)pReference)->GetSpinHeight();
			strUp						= ((CGUIThumbnailPanel*)pReference)->GetTexutureUpName();
			strDown					= ((CGUIThumbnailPanel*)pReference)->GetTexutureDownName();
			strUpFocus			= ((CGUIThumbnailPanel*)pReference)->GetTexutureUpFocusName();
			strDownFocus		= ((CGUIThumbnailPanel*)pReference)->GetTexutureDownFocusName();
			dwSpinColor			= ((CGUIThumbnailPanel*)pReference)->GetSpinTextColor();
			dwSpinPosX			= ((CGUIThumbnailPanel*)pReference)->GetSpinX();
			dwSpinPosY			= ((CGUIThumbnailPanel*)pReference)->GetSpinY();
			dwTextColor			= ((CGUIThumbnailPanel*)pReference)->GetTextColor();
			dwSelectedColor = ((CGUIThumbnailPanel*)pReference)->GetSelectedColor();
			iTextureWidth		= ((CGUIThumbnailPanel*)pReference)->GetTextureWidth();
			iTextureHeight	= ((CGUIThumbnailPanel*)pReference)->GetTextureHeight();
      ((CGUIThumbnailPanel*)pReference)->GetThumbDimensions(iThumbXPos, iThumbYPos,iThumbWidth, iThumbHeight);
      ((CGUIThumbnailPanel*)pReference)->GetThumbDimensionsBig(iThumbXPosBig, iThumbYPosBig,iThumbWidthBig, iThumbHeightBig);
      
		}
		if (strType=="selectbutton")
		{
			strTextureBg  	 = ((CGUISelectButtonControl*)pReference)->GetTextureBackground();
			strLeft          = ((CGUISelectButtonControl*)pReference)->GetTextureLeft();
			strLeftFocus     = ((CGUISelectButtonControl*)pReference)->GetTextureLeftFocus();
			strRight         = ((CGUISelectButtonControl*)pReference)->GetTextureRight();
			strRightFocus    = ((CGUISelectButtonControl*)pReference)->GetTextureRightFocus();
			strTextureFocus	 = ((CGUISelectButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus= ((CGUISelectButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont					 = ((CGUISelectButtonControl*)pReference)->GetFontName();
			strLabel				 = ((CGUISelectButtonControl*)pReference)->GetLabel();
			dwTextColor			 = ((CGUISelectButtonControl*)pReference)->GetTextColor();
			dwDisabledColor  = ((CGUISelectButtonControl*)pReference)->GetDisabledColor() ;
		}
	}


  if (!GetDWORD(pControlNode, "id", dwID)) return NULL; // NO id????
  
	GetDWORD(pControlNode, "posX", dwPosX);
	GetDWORD(pControlNode, "posY", dwPosY);
	GetDWORD(pControlNode, "width", dwWidth);
	GetDWORD(pControlNode, "height", dwHeight);

	if (!GetDWORD(pControlNode, "onup"   , up  )) up    = dwID-1;
	if (!GetDWORD(pControlNode, "ondown" , down)) down  = dwID+1;
	if (!GetDWORD(pControlNode, "onleft" , left  )) left  = dwID;
	if (!GetDWORD(pControlNode, "onright", right)) right = dwID;

  GetHex(pControlNode, "colordiffuse", dwColorDiffuse);
 	GetBoolean(pControlNode,"visible",bVisible);
	GetString(pControlNode,"font", strFont);
	GetAlignment(pControlNode,"align", dwAlign);
	GetInt(pControlNode,"hyperlink",iHyperLink);
	GetString(pControlNode,"script", strScriptAction);
	GetHex(pControlNode,"disabledcolor",dwDisabledColor);
	GetString(pControlNode,"textureFocus",strTextureFocus);
	GetString(pControlNode,"textureNoFocus",strTextureNoFocus);
	GetString(pControlNode,"AltTextureFocus",strTextureAltFocus);
	GetString(pControlNode,"AltTextureNoFocus",strTextureAltNoFocus);
	GetDWORD(pControlNode,"bitmaps", dwItems);
	GetHex(pControlNode, "textcolor", dwTextColor);

 	GetBoolean(pControlNode,"hasPath",bHasPath);
 	GetBoolean(pControlNode,"shadow",bShadow);

	GetString(pControlNode,"textureUp",strUp);
	GetString(pControlNode,"textureDown",strDown);
	GetString(pControlNode,"textureUpFocus",strUpFocus);
	GetString(pControlNode,"textureDownFocus",strDownFocus);

	GetString(pControlNode,"textureLeft",strLeft);
	GetString(pControlNode,"textureRight",strRight);
	GetString(pControlNode,"textureLeftFocus",strLeftFocus);
	GetString(pControlNode,"textureRightFocus",strRightFocus);

	GetHex(pControlNode,"spinColor",dwSpinColor);
	GetDWORD(pControlNode,"spinWidth",dwSpinWidth);
	GetDWORD(pControlNode,"spinHeight",dwSpinHeight);
	GetDWORD(pControlNode,"spinPosX",dwSpinPosX);
	GetDWORD(pControlNode,"spinPosY",dwSpinPosY);      

	GetDWORD(pControlNode,"MarkWidth",dwCheckWidth);
	GetDWORD(pControlNode,"MarkHeight",dwCheckHeight);
	GetString(pControlNode,"textureCheckmark",strTextureCheckMark);
  GetString(pControlNode,"textureCheckmarkNoFocus",strTextureCheckMarkNF);
	GetString(pControlNode,"textureRadioFocus",strTextureRadioFocus);
	GetString(pControlNode,"textureRadioNoFocus",strTextureRadioNoFocus);

	GetString(pControlNode,"textureSliderBar", strTextureBg);
	GetString(pControlNode,"textureSliderNib", strMid);
	GetString(pControlNode,"textureSliderNibFocus", strMidFocus);

	if (GetString(pControlNode,"subtype",strSubType))
	{
		strSubType.ToLower();
		if ( strSubType=="int") iType=SPIN_CONTROL_TYPE_INT;
		else if ( strSubType=="float") iType=SPIN_CONTROL_TYPE_FLOAT;
		else iType=SPIN_CONTROL_TYPE_TEXT;
	}
	GetBoolean(pControlNode,"reverse",bReverse);

	GetString(pControlNode,"texturebg",strTextureBg);
	GetString(pControlNode,"lefttexture",strLeft);
	GetString(pControlNode,"midtexture",strMid);
	GetString(pControlNode,"righttexture",strRight);
	GetString(pControlNode,"texture",strTexture);
	GetHex(pControlNode,"colorkey",dwColorKey);

	GetHex  (pControlNode,"selectedColor",dwSelectedColor);
	dwSelectedColor2=dwSelectedColor;

	GetString(pControlNode,"textureNoFocus",strButton);
	GetString(pControlNode,"textureFocus",strButtonFocus);
	GetString(pControlNode,"suffix",strSuffix);
	GetInt(pControlNode,"textXOff",iTextXOff);
	GetInt(pControlNode,"textYOff",iTextYOff);
	GetInt(pControlNode,"textXOff2",iTextXOff2);
	GetInt(pControlNode,"textYOff2",iTextYOff2);

	GetDWORD(pControlNode,"itemWidth",dwitemWidth);
	GetDWORD(pControlNode,"itemHeight",dwitemHeight);
	GetInt(pControlNode,"spaceBetweenItems",iSpace);

	GetHex(pControlNode,"selectedColor2",dwSelectedColor2);
	GetHex(pControlNode,"textcolor2",dwTextColor2);
	GetString(pControlNode,"font2",strFont2);
	GetHex(pControlNode,"selectedColor",dwSelectedColor);

	GetString(pControlNode,"imageFolder",strImage);
	GetString(pControlNode,"imageFolderFocus",strImageFocus);
	GetInt(pControlNode,"textureWidth",iTextureWidth);
	GetInt(pControlNode,"textureHeight",iTextureHeight);

  
	GetInt(pControlNode,"thumbWidth",iThumbWidth);
	GetInt(pControlNode,"thumbHeight",iThumbHeight);
	GetInt(pControlNode,"thumbPosX",iThumbXPos);
	GetInt(pControlNode,"thumbPosY",iThumbYPos);

  
	GetInt(pControlNode,"thumbWidthBig",iThumbWidthBig);
	GetInt(pControlNode,"thumbHeightBig",iThumbHeightBig);
	GetInt(pControlNode,"thumbPosXBig",iThumbXPosBig);
	GetInt(pControlNode,"thumbPosYBig",iThumbYPosBig);

  GetDWORD(pControlNode,"textureWidthBig",textureWidthBig);
	GetDWORD(pControlNode,"textureHeightBig",textureHeightBig);
  GetDWORD(pControlNode,"itemWidthBig",itemWidthBig);
	GetDWORD(pControlNode,"itemHeightBig",itemHeightBig);
	GetDWORD(pControlNode,"buddycontrolid",dwBuddyControlID);
	

	if ( GetString(pControlNode, "label", strTmp))
	{
		if (strTmp.size() > 0)
		{
			if (strTmp[0] != '-') 
			{
				DWORD dwLabelID=atol(strTmp);
				strLabel=g_localizeStrings.Get(dwLabelID);
			}
		}
	}

  if (strType=="label")
  {
			if (!bLoadReferences) if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
      CGUILabelControl* pControl = new CGUILabelControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strFont,strLabel,dwTextColor,dwDisabledColor,dwAlign, bHasPath);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }

  if (strType=="videowindow")
  {
		if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
    CGUIVideoControl* pControl = new CGUIVideoControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight);
    return pControl;
  }

	if (strType=="fadelabel")
  {
			if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
      CGUIFadeLabelControl* pControl = new CGUIFadeLabelControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strFont,dwTextColor,dwAlign);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }

  if (strType=="button")
	{
			if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
      CGUIButtonControl* pControl = new CGUIButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
			pControl->SetHyperLink(iHyperLink);
			pControl->SetScriptAction(strScriptAction);
      pControl->SetVisible(bVisible);
      return pControl;
  }
  
  if (strType=="togglebutton")
  {
			if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
      CGUIToggleButtonControl* pControl = new CGUIToggleButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus,strTextureAltFocus,strTextureAltNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
			pControl->SetHyperLink(iHyperLink);
      pControl->SetVisible(bVisible);
      return pControl;
  }

  if (strType=="buttonM")
  {
			if (!bLoadReferences) g_graphicsContext.ScalePosToScreenResolution(dwPosX,dwPosY);
      CGUIMButtonControl* pControl = new CGUIMButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,dwItems,strTextureFocus,strTextureNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
			pControl->SetHyperLink(iHyperLink);
      pControl->SetVisible(bVisible);
      return pControl;
  }
  
  if (strType=="checkmark")
  {
			if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
			CGUICheckMarkControl* pControl = new CGUICheckMarkControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureCheckMark,strTextureCheckMarkNF,dwCheckWidth,dwCheckHeight,dwAlign);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      pControl->SetShadow(bShadow);
      return pControl;
  }


  if (strType=="radiobutton")
  {
			if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
      CGUIRadioButtonControl* pControl = new CGUIRadioButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus, strTextureRadioFocus,strTextureRadioNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
			pControl->SetHyperLink(iHyperLink);
      pControl->SetVisible(bVisible);
      return pControl;
  }
 

  if (strType=="spincontrol")
  {
			if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);

      CGUISpinControl* pControl = new CGUISpinControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strUp,strDown,strUpFocus,strDownFocus,strFont,dwTextColor,iType,dwAlign);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
	  pControl->SetReverse(bReverse);
	  pControl->SetBuddyControlID(dwBuddyControlID);
	  pControl->SetDisabledColor(dwDisabledColor);
      return pControl;
  }

  if (strType=="slider")
  {
			if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
	  
      CGUISliderControl* pControl= new CGUISliderControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,strTextureBg,strMid,strMidFocus,iType);
      pControl->SetVisible(bVisible);
	  pControl->SetNavigation(up,down,left,right);
      return pControl;
  } 
  if (strType=="progress")
  {
			if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
      CGUIProgressControl* pControl= new CGUIProgressControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureBg,strLeft,strMid,strRight);
      pControl->SetVisible(bVisible);
      return pControl;
  } 

  if (strType=="image")
  {
			if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
			CGUIImage* pControl = new CGUIImage(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTexture,dwColorKey);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }
 if (strType=="listcontrol")
 {
			if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
      CGUIListControl* pControl = new CGUIListControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
                                                      strFont,
                                                      dwSpinWidth,dwSpinHeight,
                                                      strUp,strDown,
                                                      strUpFocus,strDownFocus,
                                                      dwSpinColor,dwSpinPosX,dwSpinPosY,
                                                      strFont,dwTextColor,dwSelectedColor,
                                                      strButton,strButtonFocus);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetScrollySuffix(strSuffix);
			pControl->SetTextOffsets(iTextXOff,iTextYOff, iTextXOff2,iTextYOff2);
      pControl->SetVisible(bVisible);
			pControl->SetImageDimensions(dwitemWidth, dwitemHeight);
			pControl->SetItemHeight(iTextureHeight);
			pControl->SetSpace(iSpace);
			pControl->SetColors2(dwTextColor2, dwSelectedColor2);
			pControl->SetFont2( strFont2 );
      return pControl;
  }

 if (strType=="textbox")
 {
			if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
      
      CGUITextBox* pControl = new CGUITextBox(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
                                                      strFont,
                                                      dwSpinWidth,dwSpinHeight,
                                                      strUp,strDown,
                                                      strUpFocus,strDownFocus,
                                                      dwSpinColor,dwSpinPosX,dwSpinPosY,
                                                      strFont,dwTextColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }

 if (strType=="thumbnailpanel")
 {
			if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
      CGUIThumbnailPanel* pControl = new CGUIThumbnailPanel(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
                                                      strFont,
                                                      strImage,strImageFocus,
                                                      dwitemWidth,dwitemHeight,
                                                      dwSpinWidth,dwSpinHeight,
                                                      strUp,strDown,
                                                      strUpFocus,strDownFocus,
                                                      dwSpinColor,dwSpinPosX,dwSpinPosY,
                                                      strFont,dwTextColor,dwSelectedColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
			pControl->SetTextureDimensions(iTextureWidth,iTextureHeight);
      pControl->SetThumbDimensions(iThumbXPos, iThumbYPos,iThumbWidth, iThumbHeight);

      pControl->SetTextureWidthBig(textureWidthBig);
      pControl->SetTextureHeightBig(textureHeightBig);
      pControl->SetItemWidthBig(itemWidthBig);
      pControl->SetItemHeightBig(itemHeightBig);

      pControl->SetTextureWidthLow(iTextureWidth);
      pControl->SetTextureHeightLow(iTextureHeight);
      pControl->SetItemWidthLow(dwitemWidth);
      pControl->SetItemHeightLow(dwitemHeight);
      pControl->SetThumbDimensionsLow(iThumbXPos, iThumbYPos,iThumbWidth, iThumbHeight);
      pControl->SetThumbDimensionsBig(iThumbXPosBig, iThumbYPosBig,iThumbWidthBig, iThumbHeightBig);

      return pControl;
  }

	if (strType=="selectbutton")
	{
		if (!bLoadReferences) g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight);
		CGUISelectButtonControl* pControl = new CGUISelectButtonControl(dwParentId,dwID,dwPosX,dwPosY,
																																		dwWidth, dwHeight,
																																		strTextureFocus,strTextureNoFocus, 
																																		strTextureBg, 
																																		strLeft, strLeftFocus, 
																																		strRight, strRightFocus);
		pControl->SetLabel(strFont,strLabel,dwTextColor);
    pControl->SetDisabledColor(dwDisabledColor);
    pControl->SetNavigation(up,down,left,right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);

    return pControl;
	}
  return NULL;
}
