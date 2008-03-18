#include "stdafx.h"
#include "VisualisationFactory.h"
#include "../Util.h"


CVisualisationFactory::CVisualisationFactory()
{

}

CVisualisationFactory::~CVisualisationFactory()
{

}

CVisualisation* CVisualisationFactory::LoadVisualisation(const CStdString& strVisz) const
{
  // strip of the path & extension to get the name of the visualisation
  // like goom or spectrum
  CStdString strName = CUtil::GetFileName(strVisz);
  strName = strName.Left(strName.size() - 4);

#ifdef HAS_VISUALISATION
  // load visualisation
  DllVisualisation* pDll = new DllVisualisation;
  pDll->SetFile(strVisz);
  //  FIXME: Some Visualisations do not work 
  //  when their dll is not unloaded immediatly
  pDll->EnableDelayedUnload(false);
  if (!pDll->Load())
  {
    delete pDll;
    return NULL;
  }

  struct Visualisation* pVisz = (struct Visualisation*)malloc(sizeof(struct Visualisation));
  ZeroMemory(pVisz, sizeof(struct Visualisation));
  pDll->GetModule(pVisz);

  // and pass it to a new instance of CVisualisation() which will hanle the visualisation
  return new CVisualisation(pVisz, pDll, strName);
#else
  return NULL;
#endif
}
