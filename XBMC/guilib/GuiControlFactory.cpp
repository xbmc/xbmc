#include "stdafx.h"
#include "guicontrolfactory.h"
#include "localizestrings.h"
#include "guiButtoncontrol.h"
#include "guiSpinButtonControl.h"
#include "guiRadiobuttoncontrol.h"
#include "guiSpinControl.h"
#include "guiRSSControl.h"
#include "guiRAMControl.h"
#include "guiListControl.h"
#include "guiListControlEx.h"
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

bool CGUIControlFactory::GetPath(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringPath)
{
	TiXmlNode* pNode=pRootNode->FirstChild(strTag );
	if (!pNode) return false;
	strStringPath=pNode->FirstChild()->Value();
    strStringPath.Replace('/','\\');
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

CGUIControl* CGUIControlFactory::Create(DWORD dwParentId,const TiXmlNode* pControlNode, CGUIControl* pReference, RESOLUTION res)
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
	CStdString  strTextureFocus,strTextureNoFocus,strTextureUpFocus,strTextureDownFocus;
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
	DWORD dwTextOffsetX = 10;
	DWORD dwTextOffsetY = 2;
	int 				iTextXOff=0;
	int 				iTextYOff=0;
	int 				iTextXOff2=0;
	int 				iTextYOff2=0;
	DWORD			 	dwitemWidth=16, dwitemHeight=16;
  DWORD       textureWidthBig=128;
  DWORD       textureHeightBig=128;
  DWORD       itemWidthBig=150;
  DWORD       itemHeightBig=150;
  DWORD			dwDisposition=0;
	DWORD			 	dwTextColor2=dwTextColor;
	DWORD				dwTextColor3=dwTextColor;
	DWORD			 	dwSelectedColor2;
	int        	iSpace=2;
	int				 	iTextureHeight=30;
	CStdString 	strImage,strImageFocus;
	int				 	iTextureWidth=80;
	bool				bHasPath=false;
	CStdString	strScriptAction="";
	CStdString	strRSSUrl="";
	CStdString  strTitle="";

	DWORD dwThumbWidth = 80;
	DWORD dwThumbHeight = 128;
	DWORD dwThumbSpaceX = 6;
	DWORD dwThumbSpaceY	= 25;
	DWORD dwTextSpaceY = 12;
	CStdString strDefaultThumb;

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
		if (strType=="rss")
		{
			strFont			= ((CGUIRSSControl*)pReference)->GetFontName();
			strRSSUrl		= ((CGUIRSSControl*)pReference)->GetUrl();
			dwTextColor3	= ((CGUIRSSControl*)pReference)->GetChannelTextColor();
			dwTextColor2	= ((CGUIRSSControl*)pReference)->GetHeadlineTextColor();
			dwTextColor		= ((CGUIRSSControl*)pReference)->GetNormalTextColor();
		}
		if (strType=="ram")
		{
			strFont			= ((CGUIRAMControl*)pReference)->GetFontName();
			strFont2		= ((CGUIRAMControl*)pReference)->GetFont2Name();
			dwTextColor3	= ((CGUIRAMControl*)pReference)->GetTitleTextColor();
			dwTextColor		= ((CGUIRAMControl*)pReference)->GetNormalTextColor();
			dwSelectedColor	= ((CGUIRAMControl*)pReference)->GetSelectedTextColor();
			dwTextOffsetX	= ((CGUIRAMControl*)pReference)->GetTextOffsetX();
			dwTextOffsetY	= ((CGUIRAMControl*)pReference)->GetTextOffsetY();
			dwTextSpaceY	= ((CGUIRAMControl*)pReference)->GetTextSpacing();

			((CGUIRAMControl*)pReference)->GetThumbAttributes(dwThumbWidth,dwThumbHeight,dwThumbSpaceX,dwThumbSpaceY,strDefaultThumb);

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
			dwTextOffsetX	= ((CGUIButtonControl*)pReference)->GetTextOffsetX();
			dwTextOffsetY	= ((CGUIButtonControl*)pReference)->GetTextOffsetY();
		}
		if (strType=="spinbutton")
		{
			iType				= ((CGUISpinButtonControl*)pReference)->GetType();
			strTextureDownFocus	= ((CGUISpinButtonControl*)pReference)->GetTexutureDownFocusName();
			strTextureUpFocus	= ((CGUISpinButtonControl*)pReference)->GetTexutureUpFocusName();
			strTextureFocus		= ((CGUISpinButtonControl*)pReference)->GetTexutureFocusName();
			strTextureNoFocus	= ((CGUISpinButtonControl*)pReference)->GetTexutureNoFocusName();
			strFont				= ((CGUISpinButtonControl*)pReference)->GetFontName();
			dwTextColor			= ((CGUISpinButtonControl*)pReference)->GetTextColor();
			dwDisabledColor		= ((CGUISpinButtonControl*)pReference)->GetDisabledColor() ;
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
			dwTextOffsetX	= ((CGUIMButtonControl*)pReference)->GetTextOffsetX();
			dwTextOffsetY	= ((CGUIMButtonControl*)pReference)->GetTextOffsetY();
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
			dwTextOffsetX	= ((CGUIRadioButtonControl*)pReference)->GetTextOffsetX();
			dwTextOffsetY	= ((CGUIRadioButtonControl*)pReference)->GetTextOffsetY();
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
			dwTextOffsetX	= ((CGUISpinControl*)pReference)->GetTextOffsetX();
			dwTextOffsetY	= ((CGUISpinControl*)pReference)->GetTextOffsetY();
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
			dwTextOffsetX			=	((CGUIListControl*)pReference)->GetButtonTextOffsetX();
			dwTextOffsetY			=	((CGUIListControl*)pReference)->GetButtonTextOffsetY();
		}
		if (strType=="listcontrolex")
		{
			strFont					=	((CGUIListControlEx*)pReference)->GetFontName();
			dwSpinWidth			=	((CGUIListControlEx*)pReference)->GetSpinWidth();
			dwSpinHeight		=	((CGUIListControlEx*)pReference)->GetSpinHeight();
			strUp						=	((CGUIListControlEx*)pReference)->GetTexutureUpName();
			strDown					=	((CGUIListControlEx*)pReference)->GetTexutureDownName();
			strUpFocus			=	((CGUIListControlEx*)pReference)->GetTexutureUpFocusName();
			strDownFocus		=	((CGUIListControlEx*)pReference)->GetTexutureDownFocusName();
			dwSpinColor			=	((CGUIListControlEx*)pReference)->GetSpinTextColor();
			dwSpinPosX			=	((CGUIListControlEx*)pReference)->GetSpinX();
			dwSpinPosY			=	((CGUIListControlEx*)pReference)->GetSpinY();
			dwTextColor			=	((CGUIListControlEx*)pReference)->GetTextColor();
			dwSelectedColor = ((CGUIListControlEx*)pReference)->GetSelectedColor();
			strButton				=	((CGUIListControlEx*)pReference)->GetButtonNoFocusName();
			strButtonFocus	=	((CGUIListControlEx*)pReference)->GetButtonFocusName();
			strSuffix				=	((CGUIListControlEx*)pReference)->GetSuffix();
			dwitemWidth			=	((CGUIListControlEx*)pReference)->GetImageWidth();
			dwitemHeight		=	((CGUIListControlEx*)pReference)->GetImageHeight();
			iTextureHeight	=	((CGUIListControlEx*)pReference)->GetItemHeight();
			iSpace					=	((CGUIListControlEx*)pReference)->GetSpace();
			dwTextColor2		=	((CGUIListControlEx*)pReference)->GetTextColor2(); 
			dwSelectedColor2=	((CGUIListControlEx*)pReference)->GetSelectedColor2();
			strFont2				=	((CGUIListControlEx*)pReference)->GetFontName2();
			dwTextOffsetX	= ((CGUIListControlEx*)pReference)->GetTextOffsetX();
			dwTextOffsetY	= ((CGUIListControlEx*)pReference)->GetTextOffsetY();
		}
		if (strType=="textbox")
		{
				strFont			= ((CGUITextBox*)pReference)->GetFontName();
				dwTextColor		= ((CGUITextBox*)pReference)->GetTextColor();
				dwSpinWidth		= ((CGUITextBox*)pReference)->GetSpinWidth();
				dwSpinHeight	= ((CGUITextBox*)pReference)->GetSpinHeight();
				strUp			= ((CGUITextBox*)pReference)->GetTexutureUpName();
				strDown			= ((CGUITextBox*)pReference)->GetTexutureDownName();
				strUpFocus		= ((CGUITextBox*)pReference)->GetTexutureUpFocusName();
				strDownFocus	= ((CGUITextBox*)pReference)->GetTexutureDownFocusName();
				dwSpinColor		= ((CGUITextBox*)pReference)->GetSpinTextColor();
				dwSpinPosX		= ((CGUITextBox*)pReference)->GetSpinX();
				dwSpinPosY		= ((CGUITextBox*)pReference)->GetSpinY();
				dwSpinWidth		= ((CGUITextBox*)pReference)->GetSpinWidth();
				dwSpinHeight 	= ((CGUITextBox*)pReference)->GetSpinHeight();
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
			dwTextOffsetX	= ((CGUISelectButtonControl*)pReference)->GetTextOffsetX();
			dwTextOffsetY	= ((CGUISelectButtonControl*)pReference)->GetTextOffsetY();
		}
	}


  if (!GetDWORD(pControlNode, "id", dwID)) return NULL; // NO id????
  
	GetDWORD(pControlNode, "posX", dwPosX);
	GetDWORD(pControlNode, "posY", dwPosY);
	GetDWORD(pControlNode, "width", dwWidth);
	GetDWORD(pControlNode, "height", dwHeight);
	GetDWORD(pControlNode, "textOffsetX", dwTextOffsetX);
	GetDWORD(pControlNode, "textOffsetY", dwTextOffsetY);
	GetDWORD(pControlNode, "textSpaceY",  dwTextSpaceY);

	GetDWORD(pControlNode, "gfxThumbWidth",  dwThumbWidth);
	GetDWORD(pControlNode, "gfxThumbHeight", dwThumbHeight);
	GetDWORD(pControlNode, "gfxThumbSpaceX", dwThumbSpaceX);
	GetDWORD(pControlNode, "gfxThumbSpaceY", dwThumbSpaceY);
	GetString(pControlNode, "gfxThumbDefault", strDefaultThumb);

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
	GetPath(pControlNode,"textureDownFocus",strTextureDownFocus);
	GetPath(pControlNode,"textureUpFocus",strTextureUpFocus);
	GetPath(pControlNode,"textureFocus",strTextureFocus);
	GetPath(pControlNode,"textureNoFocus",strTextureNoFocus);
	GetPath(pControlNode,"AltTextureFocus",strTextureAltFocus);
	GetPath(pControlNode,"AltTextureNoFocus",strTextureAltNoFocus);
	GetDWORD(pControlNode,"bitmaps", dwItems);
	GetHex(pControlNode, "textcolor", dwTextColor);

 	GetBoolean(pControlNode,"hasPath",bHasPath);
 	GetBoolean(pControlNode,"shadow",bShadow);

	GetPath(pControlNode,"textureUp",strUp);
	GetPath(pControlNode,"textureDown",strDown);
	GetPath(pControlNode,"textureUpFocus",strUpFocus);
	GetPath(pControlNode,"textureDownFocus",strDownFocus);

	GetPath(pControlNode,"textureLeft",strLeft);
	GetPath(pControlNode,"textureRight",strRight);
	GetPath(pControlNode,"textureLeftFocus",strLeftFocus);
	GetPath(pControlNode,"textureRightFocus",strRightFocus);

	GetHex(pControlNode,"spinColor",dwSpinColor);
	GetDWORD(pControlNode,"spinWidth",dwSpinWidth);
	GetDWORD(pControlNode,"spinHeight",dwSpinHeight);
	GetDWORD(pControlNode,"spinPosX",dwSpinPosX);
	GetDWORD(pControlNode,"spinPosY",dwSpinPosY);      

	GetDWORD(pControlNode,"MarkWidth",dwCheckWidth);
	GetDWORD(pControlNode,"MarkHeight",dwCheckHeight);
	GetPath(pControlNode,"textureCheckmark",strTextureCheckMark);
        GetPath(pControlNode,"textureCheckmarkNoFocus",strTextureCheckMarkNF);
	GetPath(pControlNode,"textureRadioFocus",strTextureRadioFocus);
	GetPath(pControlNode,"textureRadioNoFocus",strTextureRadioNoFocus);

	GetPath(pControlNode,"textureSliderBar", strTextureBg);
	GetPath(pControlNode,"textureSliderNib", strMid);
	GetPath(pControlNode,"textureSliderNibFocus", strMidFocus);
	GetDWORD(pControlNode,"disposition",dwDisposition);
	GetString(pControlNode,"feed",strRSSUrl);
	GetString(pControlNode,"title",strTitle);
	GetHex(pControlNode, "headlinecolor", dwTextColor2);
	GetHex(pControlNode, "titlecolor", dwTextColor3);

	if (GetString(pControlNode,"subtype",strSubType))
	{
		strSubType.ToLower();
		if ( strSubType=="int") iType=SPIN_CONTROL_TYPE_INT;
		else if ( strSubType=="float") iType=SPIN_CONTROL_TYPE_FLOAT;
		else iType=SPIN_CONTROL_TYPE_TEXT;
	}

	GetBoolean(pControlNode,"reverse",bReverse);

	GetPath(pControlNode,"texturebg",strTextureBg);
	GetPath(pControlNode,"lefttexture",strLeft);
	GetPath(pControlNode,"midtexture",strMid);
	GetPath(pControlNode,"righttexture",strRight);
	GetPath(pControlNode,"texture",strTexture);
	GetHex(pControlNode,"colorkey",dwColorKey);

	GetHex  (pControlNode,"selectedColor",dwSelectedColor);
	dwSelectedColor2=dwSelectedColor;

	GetPath(pControlNode,"textureNoFocus",strButton);
	GetPath(pControlNode,"textureFocus",strButtonFocus);
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

	GetPath(pControlNode,"imageFolder",strImage);
	GetPath(pControlNode,"imageFolderFocus",strImageFocus);
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
				if ((strTmp[0]>='A')&&(strTmp[0]<='z'))
				{
					WCHAR wszTmp[256];
					swprintf(wszTmp,L"%S",strTmp.c_str());
					strLabel = wszTmp;
				}
				else
				{
					DWORD dwLabelID=atol(strTmp);
					strLabel=g_localizeStrings.Get(dwLabelID);
				}
			}
		}
	}

  if (strType=="label")
  {
	  g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight, res);
      CGUILabelControl* pControl = new CGUILabelControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strFont,strLabel,dwTextColor,dwDisabledColor,dwAlign, bHasPath);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }

  if (strType=="videowindow")
  {
		g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
    CGUIVideoControl* pControl = new CGUIVideoControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight);
    return pControl;
  }

	if (strType=="fadelabel")
  {
	g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
      CGUIFadeLabelControl* pControl = new CGUIFadeLabelControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strFont,dwTextColor,dwAlign);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }
  
  
	if (strType=="spinbutton")
	{
	        g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);

		CGUISpinButtonControl* pControl = new CGUISpinButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,strTextureNoFocus,strTextureFocus,strTextureUpFocus,strTextureDownFocus,strFont,dwTextColor,(int)dwDisposition);
		pControl->SetDisabledColor(dwDisabledColor);
		pControl->SetNavigation(up,down,left,right);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
		return pControl;
	}
  
	if (strType=="rss")
	{
	        g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
      
		CGUIRSSControl* pControl = new CGUIRSSControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strFont,dwTextColor3,dwTextColor2,dwTextColor,strRSSUrl);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
		return pControl;
	}
	if (strType=="ram")
	{
	    g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
      
		CGUIRAMControl* pControl = new CGUIRAMControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strFont,strFont2,dwTextColor3,dwTextColor,dwSelectedColor,dwTextOffsetX,dwTextOffsetY);
		pControl->SetTextSpacing(dwTextSpaceY);
		pControl->SetThumbAttributes(dwThumbWidth,dwThumbHeight,dwThumbSpaceX,dwThumbSpaceY,strDefaultThumb);
		pControl->SetColourDiffuse(dwColorDiffuse);
		pControl->SetVisible(bVisible);
	    pControl->SetNavigation(up,down,left,right);

		return pControl;
	}
  if (strType=="button")
	{
	g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
      CGUIButtonControl* pControl = new CGUIButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus,dwTextOffsetX,dwTextOffsetY);
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
	g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
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
	g_graphicsContext.ScalePosToScreenResolution(dwPosX,dwPosY,res);
      CGUIMButtonControl* pControl = new CGUIMButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,dwItems,strTextureFocus,strTextureNoFocus, dwTextOffsetX, dwTextOffsetY);
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
			g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
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
	  g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
      CGUIRadioButtonControl* pControl = new CGUIRadioButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus, dwTextOffsetX, dwTextOffsetY, strTextureRadioFocus,strTextureRadioNoFocus);
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
			g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);

      CGUISpinControl* pControl = new CGUISpinControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strUp,strDown,strUpFocus,strDownFocus,strFont,dwTextColor,iType,dwAlign);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
			pControl->SetReverse(bReverse);
	  pControl->SetBuddyControlID(dwBuddyControlID);
	  pControl->SetDisabledColor(dwDisabledColor);
	  pControl->SetTextOffsetX(dwTextOffsetX);
	  pControl->SetTextOffsetY(dwTextOffsetY);
      return pControl;
  }

  if (strType=="slider")
  {
			g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
	  
      CGUISliderControl* pControl= new CGUISliderControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth,dwHeight,strTextureBg,strMid,strMidFocus,iType);
      pControl->SetVisible(bVisible);
	  pControl->SetNavigation(up,down,left,right);
      return pControl;
  } 
  if (strType=="progress")
  {
			g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
      CGUIProgressControl* pControl= new CGUIProgressControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureBg,strLeft,strMid,strRight);
      pControl->SetVisible(bVisible);
      return pControl;
  } 

  if (strType=="image")
  {
			g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
			CGUIImage* pControl = new CGUIImage(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTexture,dwColorKey);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetVisible(bVisible);
      return pControl;
  }
 if (strType=="listcontrol")
 {
			g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
      CGUIListControl* pControl = new CGUIListControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
                                                      strFont,
                                                      dwSpinWidth,dwSpinHeight,
                                                      strUp,strDown,
                                                      strUpFocus,strDownFocus,
                                                      dwSpinColor,dwSpinPosX,dwSpinPosY,
                                                      strFont,dwTextColor,dwSelectedColor,
                                                      strButton,strButtonFocus,
													  dwTextOffsetX, dwTextOffsetY);
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
 if (strType=="listcontrolex")
 {
      g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);

      CGUIListControlEx* pControl = new CGUIListControlEx(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
                                                      strFont,
                                                      dwSpinWidth,dwSpinHeight,
                                                      strUp,strDown,
                                                      strUpFocus,strDownFocus,
                                                      dwSpinColor,dwSpinPosX,dwSpinPosY,
                                                      strFont,dwTextColor,dwSelectedColor,
                                                      strButton,strButtonFocus,
													  dwTextOffsetX, dwTextOffsetY);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pControl->SetScrollySuffix(strSuffix);
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
			g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
      
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
			g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
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
		g_graphicsContext.ScaleRectToScreenResolution(dwPosX,dwPosY,dwWidth, dwHeight,res);
		CGUISelectButtonControl* pControl = new CGUISelectButtonControl(dwParentId,dwID,dwPosX,dwPosY,
									dwWidth, dwHeight, strTextureFocus,strTextureNoFocus, 
									dwTextOffsetX, dwTextOffsetY,
                                    strTextureBg, strLeft, strLeftFocus, strRight, strRightFocus);
		pControl->SetLabel(strFont,strLabel,dwTextColor);
    pControl->SetDisabledColor(dwDisabledColor);
    pControl->SetNavigation(up,down,left,right);
    pControl->SetColourDiffuse(dwColorDiffuse);
    pControl->SetVisible(bVisible);

    return pControl;
	}
  return NULL;
}
