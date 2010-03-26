/*
 * $Id: TextPassThruFilter.h 1457 2010-01-01 03:13:59Z xhmikosr $
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2010 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once
#include "Streams.h"
#include "DSGraph.h"
class CTextPassThruInputPin;

[uuid("E2BA9B7B-B65D-4804-ACB2-89C3E55511DB")]
class CTextPassThruFilter : public CBaseFilter, public CCritSec
{
	friend class CTextPassThruInputPin;
	friend class CTextPassThruOutputPin;

	CTextPassThruInputPin* m_pInput;
	CTextPassThruOutputPin* m_pOutput;

	CDSGraph* m_pMainGraph;

public:
	CTextPassThruFilter(CDSGraph* pMainGraph);
	virtual ~CTextPassThruFilter();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	int GetPinCount();
	CBasePin* GetPin(int n);
};
