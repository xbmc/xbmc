#include "../stdafx.h"
#include "VisualisationFactory.h"
#include "../cores/DllLoader/dll.h"
#include "../util.h"


CVisualisationFactory::CVisualisationFactory()
{
}
CVisualisationFactory::~CVisualisationFactory()
{
}
extern "C" void __declspec(dllexport) get_module(struct Visualisation* pVisz);

CVisualisation* CVisualisationFactory::LoadVisualisation(const CStdString& strVisz) const
{
  // strip of the path & extension to get the name of the visualisation
  // like goom or spectrum
  CStdString strName=CUtil::GetFileName(strVisz);
  strName=strName.Left(strName.size()-4);

  // load visualisation 
	DllLoader* pDLL = new DllLoader(strVisz.c_str(), true);
	if( !pDLL->Parse() )
	{
    // failed,
		delete pDLL;
		return NULL;
	}
	pDLL->ResolveImports();

  // get handle to the get_module() function from the visualisation
	void (__cdecl* pGetModule)(struct Visualisation*);
	struct Visualisation* pVisz = (struct Visualisation*)malloc(sizeof(struct Visualisation));
	void* pProc;
	pDLL->ResolveExport("get_module", &pProc);
	if (!pProc)
	{
    // get_module() not found in visualition
		delete pDLL;
		return NULL;
	}
  // call get_module() to get the Visualisation struct from the visualisation
	pGetModule=(void (__cdecl*)(struct Visualisation*))pProc;
	pGetModule(pVisz);
  
  // and pass it to a new instance of CVisualisation() which will hanle the visualisation
  if(strName == "Xaraoke")
		return (CVisualisation*) new CXaraokeVisualisation(pVisz,pDLL, strName);
	else
		return new CVisualisation(pVisz,pDLL, strName);
}
