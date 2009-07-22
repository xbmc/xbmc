#include "include.h"
#include "GraphicContextFactory.h"
#include "GraphicContextGL.h"

CGraphicContextFactory::CGraphicContextFactory(void)
{

}

CGraphicContextFactory::~CGraphicContextFactory(void)
{

}

CGraphicContext& CGraphicContextFactory::GetGraphicContext()
{
  static CGraphicContextGL context;

  return context;
}
