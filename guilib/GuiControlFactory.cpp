#include "guicontrolfactory.h"
#include "localizestrings.h"
#include "guibuttoncontrol.h"
#include "guiRadiobuttoncontrol.h"
#include "guiSpinControl.h"
#include "guiListControl.h"
#include "guiImage.h"
#include "GUILabelControl.h"
#include "GUICheckMarkControl.h"
#include "GUIThumbnailPanel.h"
#include "GUIMButtonControl.h"
#include "GUIToggleButtonControl.h" 

CGUIControlFactory::CGUIControlFactory(void)
{
}

CGUIControlFactory::~CGUIControlFactory(void)
{
}


CGUIControl* CGUIControlFactory::Create(DWORD dwParentId,const TiXmlNode* pControlNode)
{
  const TiXmlNode* pType=pControlNode->FirstChild("type");
  string strType=pType->FirstChild()->Value();

  DWORD  dwPosX=0,dwPosY=0;
  DWORD  dwWidth=0, dwHeight=0;
  DWORD  dwID=0,left=0,right=0,up=0,down=0;
  TiXmlNode *pNode;
  dwID=atol(pControlNode->FirstChild("id" )->FirstChild()->Value());
  dwPosX=atol(pControlNode->FirstChild("posX" )->FirstChild()->Value());
  dwPosY=atol(pControlNode->FirstChild("posY" )->FirstChild()->Value());
  pNode=pControlNode->FirstChild("width" );
  if (pNode)
    dwWidth=atol(pNode->FirstChild()->Value());

  pNode=pControlNode->FirstChild("height" );
  if (pNode) dwHeight=atol(pNode->FirstChild()->Value());

  pNode=pControlNode->FirstChild("onup");
  if (pNode) up=atol(pNode->FirstChild()->Value());

  pNode=pControlNode->FirstChild("ondown");
  if (pNode) down=atol(pNode->FirstChild()->Value());

  pNode=pControlNode->FirstChild("onleft");
  if (pNode) left=atol(pNode->FirstChild()->Value());

  pNode=pControlNode->FirstChild("onright");
  if (pNode) right=atol(pNode->FirstChild()->Value());

  DWORD dwColorDiffuse=0xFFFFFFFF;
  pNode=pControlNode->FirstChild("colordiffuse" );
  if (pNode) sscanf(pNode->FirstChild()->Value(),"%x",&dwColorDiffuse);
  

  if (strType=="label")
  {
      wstring strLabel;
      string  strFont;
      D3DCOLOR dwTextColor;
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      DWORD dwLabelID=atol(pControlNode->FirstChild("label")->FirstChild()->Value());
      strLabel=g_localizeStrings.Get(dwLabelID);
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();

      DWORD dwAlign=XBFONT_LEFT;
      pNode=pControlNode->FirstChild("align");
      if (pNode)
      {
        string strAlign=pControlNode->FirstChild("align")->FirstChild()->Value();      
        if (strAlign=="right") dwAlign=XBFONT_RIGHT;
      }

      CGUILabelControl* pControl = new CGUILabelControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strFont,strLabel,dwTextColor,dwAlign);
      pControl->SetColourDiffuse(dwColorDiffuse);
      return pControl;
  }

  if (strType=="button")
  {
      wstring strLabel;
      string strFont,strTextureFocus,strTextureNoFocus;
      D3DCOLOR dwTextColor;
      D3DCOLOR dwDisabledColor;
      sscanf(pControlNode->FirstChild("disabledcolor" )->FirstChild()->Value(),"%x",&dwDisabledColor);
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      strTextureFocus=pControlNode->FirstChild("textureFocus")->FirstChild()->Value();
      strTextureNoFocus=pControlNode->FirstChild("textureNoFocus")->FirstChild()->Value();
      const char *szLabel=pControlNode->FirstChild("label")->FirstChild()->Value();
      if (szLabel[0]!='-')
      {
        DWORD dwLabelID=atol(pControlNode->FirstChild("label")->FirstChild()->Value());
        strLabel=g_localizeStrings.Get(dwLabelID);
      }
      else strLabel=L"";

      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();

      CGUIButtonControl* pControl = new CGUIButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);

      pNode = pControlNode->FirstChild("hyperlink");
      if (pNode) 
      {
        int iHyperLink=atoi(pNode->FirstChild()->Value());
        pControl->SetHyperLink(iHyperLink);
      }
      return pControl;
  }
  
  if (strType=="togglebutton")
  {
      wstring strLabel;
      string strFont,strTextureFocus,strTextureNoFocus;
      string strTextureAltFocus,strTextureAltNoFocus;
      D3DCOLOR dwTextColor;
      D3DCOLOR dwDisabledColor;
      sscanf(pControlNode->FirstChild("disabledcolor" )->FirstChild()->Value(),"%x",&dwDisabledColor);
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      strTextureFocus=pControlNode->FirstChild("textureFocus")->FirstChild()->Value();
      strTextureNoFocus=pControlNode->FirstChild("textureNoFocus")->FirstChild()->Value();
      strTextureAltFocus=pControlNode->FirstChild("AltTextureFocus")->FirstChild()->Value();
      strTextureAltNoFocus=pControlNode->FirstChild("AltTextureNoFocus")->FirstChild()->Value();
      const char *szLabel=pControlNode->FirstChild("label")->FirstChild()->Value();
      if (szLabel[0]!='-')
      {
        DWORD dwLabelID=atol(pControlNode->FirstChild("label")->FirstChild()->Value());
        strLabel=g_localizeStrings.Get(dwLabelID);
      }
      else strLabel=L"";

      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();

      CGUIToggleButtonControl* pControl = new CGUIToggleButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus,strTextureAltFocus,strTextureAltNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);

      pNode = pControlNode->FirstChild("hyperlink");
      if (pNode) 
      {
        int iHyperLink=atoi(pNode->FirstChild()->Value());
        pControl->SetHyperLink(iHyperLink);
      }
      return pControl;
  }

  if (strType=="buttonM")
  {
      wstring strLabel;
      string strFont,strTextureFocus,strTextureNoFocus;
      D3DCOLOR dwTextColor;
      D3DCOLOR dwDisabledColor;
      DWORD dwItems;
      sscanf(pControlNode->FirstChild("bitmaps" )->FirstChild()->Value(),"%x",&dwItems);
      sscanf(pControlNode->FirstChild("disabledcolor" )->FirstChild()->Value(),"%x",&dwDisabledColor);
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      strTextureFocus=pControlNode->FirstChild("textureFocus")->FirstChild()->Value();
      strTextureNoFocus=pControlNode->FirstChild("textureNoFocus")->FirstChild()->Value();
      const char *szLabel=pControlNode->FirstChild("label")->FirstChild()->Value();
      if (szLabel[0]!='-')
      {
        DWORD dwLabelID=atol(pControlNode->FirstChild("label")->FirstChild()->Value());
        strLabel=g_localizeStrings.Get(dwLabelID);
      }
      else strLabel=L"";

      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();

      CGUIMButtonControl* pControl = new CGUIMButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,dwItems,strTextureFocus,strTextureNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);

      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pNode = pControlNode->FirstChild("hyperlink");
      if (pNode) 
      {
        int iHyperLink=atoi(pNode->FirstChild()->Value());
        pControl->SetHyperLink(iHyperLink);
      }
      return pControl;
  }
  
  if (strType=="checkmark")
  {
      wstring strLabel;
      string strFont,strTextureCheckMark;
      D3DCOLOR dwTextColor;
      D3DCOLOR dwDisabledColor;
      DWORD dwCheckWidth, dwCheckHeight;
      sscanf(pControlNode->FirstChild("MarkWidth" )->FirstChild()->Value(),"%x",&dwCheckWidth);
      sscanf(pControlNode->FirstChild("MarkHeight" )->FirstChild()->Value(),"%x",&dwCheckHeight);
      sscanf(pControlNode->FirstChild("disabledcolor" )->FirstChild()->Value(),"%x",&dwDisabledColor);
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      strTextureCheckMark=pControlNode->FirstChild("textureCheckmark")->FirstChild()->Value();
      DWORD dwLabelID=atol(pControlNode->FirstChild("label")->FirstChild()->Value());
      strLabel=g_localizeStrings.Get(dwLabelID);
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();

      CGUICheckMarkControl* pControl = new CGUICheckMarkControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureCheckMark,dwCheckWidth,dwCheckHeight);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      return pControl;
  }


  if (strType=="radiobutton")
  {
      wstring strLabel;
      string strFont,strTextureFocus,strTextureNoFocus;
      string strTextureRadioFocus,strTextureRadioNoFocus;
      D3DCOLOR dwTextColor;
      D3DCOLOR dwDisabledColor;
      sscanf(pControlNode->FirstChild("disabledcolor" )->FirstChild()->Value(),"%x",&dwDisabledColor);
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      strTextureFocus=pControlNode->FirstChild("textureFocus")->FirstChild()->Value();
      strTextureNoFocus=pControlNode->FirstChild("textureNoFocus")->FirstChild()->Value();
      strTextureRadioFocus=pControlNode->FirstChild("textureRadioFocus")->FirstChild()->Value();
      strTextureRadioNoFocus=pControlNode->FirstChild("textureRadioNoFocus")->FirstChild()->Value();
      DWORD dwLabelID=atol(pControlNode->FirstChild("label")->FirstChild()->Value());
      strLabel=g_localizeStrings.Get(dwLabelID);
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();
      CGUIRadioButtonControl* pControl = new CGUIRadioButtonControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTextureFocus,strTextureNoFocus, strTextureRadioFocus,strTextureRadioNoFocus);
      pControl->SetLabel(strFont,strLabel,dwTextColor);
      pControl->SetDisabledColor(dwDisabledColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);

      pNode = pControlNode->FirstChild("hyperlink");
      if (pNode) 
      {
        int iHyperLink=atoi(pNode->FirstChild()->Value());
        pControl->SetHyperLink(iHyperLink);
      }
      return pControl;
  }
 

  if (strType=="spincontrol")
  {
      string strFont,strUp,strDown;
      string strUpFocus,strDownFocus;
      strUp=pControlNode->FirstChild("textureUp")->FirstChild()->Value();
      strDown=pControlNode->FirstChild("textureDown")->FirstChild()->Value();
      strUpFocus=pControlNode->FirstChild("textureUpFocus")->FirstChild()->Value();
      strDownFocus=pControlNode->FirstChild("textureDownFocus")->FirstChild()->Value();
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();
      D3DCOLOR dwTextColor;
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
  
      string strSubType=pControlNode->FirstChild("subtype")->FirstChild()->Value();
      int iType;
      if ( strSubType=="int") iType=SPIN_CONTROL_TYPE_INT;
      else if ( strSubType=="float") iType=SPIN_CONTROL_TYPE_FLOAT;
      else iType=SPIN_CONTROL_TYPE_TEXT;


      CGUISpinControl* pControl = new CGUISpinControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strUp,strDown,strUpFocus,strDownFocus,strFont,dwTextColor,iType);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      return pControl;
  }
 
  if (strType=="image")
  {
      string strTexture;
      strTexture=pControlNode->FirstChild("texture")->FirstChild()->Value();
  
      D3DCOLOR dwColorKey=0xffffffff;
      pNode=pControlNode->FirstChild("colorkey" );
      if (pNode)
        sscanf(pControlNode->FirstChild("colorkey" )->FirstChild()->Value(),"%x",&dwColorKey);

      CGUIImage* pControl = new CGUIImage(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,strTexture,dwColorKey);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      return pControl;
  }
 if (strType=="listcontrol")
 {
      string strFont,strUp,strDown;
      string strUpFocus,strDownFocus;
      strUp=pControlNode->FirstChild("textureUp")->FirstChild()->Value();
      strDown=pControlNode->FirstChild("textureDown")->FirstChild()->Value();
      strUpFocus=pControlNode->FirstChild("textureUpFocus")->FirstChild()->Value();
      strDownFocus=pControlNode->FirstChild("textureDownFocus")->FirstChild()->Value();
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();
      D3DCOLOR dwTextColor,dwSpinColor;
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      sscanf(pControlNode->FirstChild("spinColor" )->FirstChild()->Value(),"%x",&dwSpinColor);
      
      string strImage=pControlNode->FirstChild("image")->FirstChild()->Value();

      DWORD dwSpinWidth=atol(pControlNode->FirstChild("spinWidth" )->FirstChild()->Value());
      DWORD dwSpinHeight=atol(pControlNode->FirstChild("spinHeight" )->FirstChild()->Value());
      DWORD dwSpinPosX=atol(pControlNode->FirstChild("spinPosX" )->FirstChild()->Value());
			DWORD dwSpinPosY=atol(pControlNode->FirstChild("spinPosY" )->FirstChild()->Value());

      string strButton=pControlNode->FirstChild("textureNoFocus")->FirstChild()->Value();
      string strButtonFocus=pControlNode->FirstChild("textureFocus")->FirstChild()->Value();

      CGUIListControl* pControl = new CGUIListControl(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
                                                      strFont,
                                                      strImage,
                                                      dwSpinWidth,dwSpinHeight,
                                                      strUp,strDown,
                                                      strUpFocus,strDownFocus,
                                                      dwSpinColor,dwSpinPosX,dwSpinPosY,
                                                      strFont,dwTextColor,
                                                      strButton,strButtonFocus);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pNode=pControlNode->FirstChild("suffix" );
      if (pNode)
      {
        string strSuffix=pControlNode->FirstChild("suffix" )->FirstChild()->Value();
        pControl->SetScrollySuffix(strSuffix);
      }
      return pControl;
  }

 if (strType=="thumbnailpanel")
 {
      string strFont,strUp,strDown;
      string strUpFocus,strDownFocus, strSuffix;



      strUp=pControlNode->FirstChild("textureUp")->FirstChild()->Value();
      strDown=pControlNode->FirstChild("textureDown")->FirstChild()->Value();
      strUpFocus=pControlNode->FirstChild("textureUpFocus")->FirstChild()->Value();
      strDownFocus=pControlNode->FirstChild("textureDownFocus")->FirstChild()->Value();
      strFont=pControlNode->FirstChild("font")->FirstChild()->Value();
      D3DCOLOR dwTextColor,dwSpinColor;
      sscanf(pControlNode->FirstChild("textcolor" )->FirstChild()->Value(),"%x",&dwTextColor);
      sscanf(pControlNode->FirstChild("spinColor" )->FirstChild()->Value(),"%x",&dwSpinColor);
      
      DWORD dwitemWidth=64, dwitemHeight=64;
      sscanf(pControlNode->FirstChild("itemWidth" )->FirstChild()->Value(),"%i",&dwitemWidth);
      sscanf(pControlNode->FirstChild("itemHeight" )->FirstChild()->Value(),"%i",&dwitemHeight);
      
      string strImage=pControlNode->FirstChild("imageFolder")->FirstChild()->Value();
      string strImageFocus=pControlNode->FirstChild("imageFolderFocus")->FirstChild()->Value();

      DWORD dwSpinWidth=atol(pControlNode->FirstChild("spinWidth" )->FirstChild()->Value());
      DWORD dwSpinHeight=atol(pControlNode->FirstChild("spinHeight" )->FirstChild()->Value());
      DWORD dwSpinPosX=atol(pControlNode->FirstChild("spinPosX" )->FirstChild()->Value());
			DWORD dwSpinPosY=atol(pControlNode->FirstChild("spinPosY" )->FirstChild()->Value());
      

      CGUIThumbnailPanel* pControl = new CGUIThumbnailPanel(dwParentId,dwID,dwPosX,dwPosY,dwWidth, dwHeight,
                                                      strFont,
                                                      strImage,strImageFocus,
                                                      dwitemWidth,dwitemHeight,
                                                      dwSpinWidth,dwSpinHeight,
                                                      strUp,strDown,
                                                      strUpFocus,strDownFocus,
                                                      dwSpinColor,dwSpinPosX,dwSpinPosY,
                                                      strFont,dwTextColor);
      pControl->SetNavigation(up,down,left,right);
      pControl->SetColourDiffuse(dwColorDiffuse);
      pNode=pControlNode->FirstChild("suffix" );
      if (pNode)
      {
        strSuffix=pControlNode->FirstChild("suffix" )->FirstChild()->Value();
        pControl->SetScrollySuffix(strSuffix);
      }
      return pControl;
  }
 
  return NULL;
}