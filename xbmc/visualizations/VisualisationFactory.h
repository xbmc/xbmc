#pragma once
#include "Visualisation.h"
#include "Xaraoke/XaraokeVisualisation.h"
#include "StdString.h"
class CVisualisationFactory
{
public:
	CVisualisationFactory();
	virtual ~CVisualisationFactory();
	CVisualisation* LoadVisualisation(const CStdString& strVisz) const;
};