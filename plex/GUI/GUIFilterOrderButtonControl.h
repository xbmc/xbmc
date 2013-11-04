#ifndef GUIFILTERORDERBUTTONCONTROL_H
#define GUIFILTERORDERBUTTONCONTROL_H


#include "guilib/GUIButtonControl.h"
#include "guilib/Key.h"

class CGUIFilterOrderButtonControl : public CGUIButtonControl
{
  public:

    enum FilterOrderButtonState
    {
      OFF,
      ASCENDING,
      DESCENDING
    };

    CGUIFilterOrderButtonControl(int parentID, int controlID,
                                 float posX, float posY, float width, float height,
                                 const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus,
                                 const CLabelInfo& labelInfo,
                                 const CTextureInfo& off, const CTextureInfo& ascending, const CTextureInfo &descending);

    void SetTristate(FilterOrderButtonState state);
    FilterOrderButtonState GetTristate() const { return m_state; }

    void Render();
    void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);

    void AllocResources();
    void FreeResources(bool immediately);
    void DynamicResourceAlloc(bool bOnOff);
    void SetInvalid();
    void SetPosition(float posX, float posY);
    void SetRadioDimensions(float posX, float posY, float width, float height);
    void SetWidth(float width);
    void SetHeight(float height);
    bool UpdateColors();
    bool OnAction(const CAction &action);
    void SetStartState(FilterOrderButtonState state) { m_startState = state; }

  private:
    FilterOrderButtonState m_startState;
    FilterOrderButtonState m_state;
    float m_radioPosY, m_radioPosX;

    CGUITexture m_off;
    CGUITexture m_ascending;
    CGUITexture m_descending;

};

#endif // GUIFILTERORDERBUTTONCONTROL_H
