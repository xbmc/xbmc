/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "Node.h"

namespace ssf
{
  class NodeFactory
  {
    Reference* m_root;
    StringMapW<Node*> m_nodes;
    std::list<CStdStringW> m_newnodes;
    bool m_predefined;

    unsigned __int64 m_counter;
    CStdStringW GenName();

  public:
    NodeFactory();
    virtual ~NodeFactory();

    virtual void clear();

    void SetPredefined(bool predefined) {m_predefined = predefined;}

    void Commit();
    void Rollback();

    Reference* CreateRootRef();
    Reference* GetRootRef() const;
    Reference* CreateRef(Definition* pParentDef);
    Definition* CreateDef(Reference* pParentRef = NULL, CStdStringW type = L"", CStdStringW name = L"", NodePriority priority = PNormal);
    Definition* GetDefByName(CStdStringW name) const;
    void GetNewDefs(std::list<Definition*>& defs);

    void Dump(OutputStream& s) const;
  };
}
