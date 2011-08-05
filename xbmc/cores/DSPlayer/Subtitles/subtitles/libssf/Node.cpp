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

#include "stdafx.h"
#include "Node.h"
#include "NodeFactory.h"
#include "Exception.h"
#include "Split.h"

#include <math.h>

namespace ssf
{
  Node::Node(NodeFactory* pnf, CStdStringW name)
    : m_pnf(pnf)
    , m_type("?")
    , m_name(name)
    , m_priority(PNormal)
    , m_predefined(false)
    , m_parent(NULL)
  {
    ASSERT(m_pnf);
  }

  void Node::push_back(Node* pNode)
  {
    std::list<Node *>::iterator it = std::find(m_nodes.begin(), m_nodes.end(), pNode); 
    if(it != m_nodes.end()) // TODO: slow
    {
      m_nodes.erase(it);
      m_nodes.push_back(*it);
      return;
    }

    m_nodes.push_back(pNode);
    m_name2node[pNode->m_name] = pNode;
  }

  bool Node::IsNameUnknown()
  {
    return m_name.empty() || !!iswdigit(m_name[0]);
  }

  bool Node::IsTypeUnknown()
  {
    return m_type.empty() || m_type == L"?";
  }

  bool Node::IsType(CStdStringW type)
  {
    return m_type == type;
  }

  void Node::GetChildDefs(std::list<Definition*>& l, LPCWSTR type, bool fFirst)
  {
    std::list<Definition*> rdl[3];

    if(fFirst)
    {
      if(Definition* pDef = m_pnf->GetDefByName(m_type))
      {
        pDef->GetChildDefs(rdl[pDef->m_priority], type, false);
      }
    }

    std::list<Node *>::iterator it = m_nodes.begin();
    for(; it != m_nodes.end(); ++it)
    {
      if(Node* pNode = *it)
      {
        pNode->GetChildDefs(rdl[pNode->m_priority], type, false);
      }
    }

    for(int i = 0; i < sizeof(rdl)/sizeof(rdl[0]); i++)
    {
      l.insert(l.end(), rdl[i].begin(), rdl[i].end());
    }
  }

  // Reference

  Reference::Reference(NodeFactory* pnf, CStdStringW name)
    : Node(pnf, name)
  {
  }

  Reference::~Reference()
  {
  }

  void Reference::GetChildDefs(std::list<Definition*>& l, LPCWSTR type, bool fFirst)
  {
    std::list<Definition*> rdl[3];

    std::list<Node *>::iterator it = m_nodes.begin();
    for(; it != m_nodes.end(); ++it)
    {
      if(Definition* pDef = dynamic_cast<Definition*>(*it))
      {
        if(!type || pDef->m_type == type) // TODO: faster lookup
        {
          rdl[pDef->m_priority].push_back(pDef);
        }
      }
    }

    for(int i = 0; i < sizeof(rdl)/sizeof(rdl[0]); i++)
    {
      l.insert(l.end(), rdl[i].begin(), rdl[i].end());
    }
  }

  void Reference::Dump(OutputStream& s, int level, bool fLast)
  {
    if(m_predefined) return;

    CStdStringW tabs(' ', level*4);

    // s.PutString(tabs + '\n' + tabs + L" {\n");
    s.PutString(L" {\n");

    std::list<Node *>::iterator it = m_nodes.begin();
    for(; it != m_nodes.end(); ++it)
    {
      Node* pNode = *it;

      if(Definition* pDef = dynamic_cast<Definition*>(pNode))
      {
        pDef->Dump(s, level + 1, it == m_nodes.end());
      }
    }

    s.PutString(tabs + L'}');
  }

  // Definition

  Definition::Definition(NodeFactory* pnf, CStdStringW name)
    : Node(pnf, name)
    , m_status(node)
    , m_autotype(false)
  {
  }

  Definition::~Definition()
  {
    RemoveFromCache();
  }

  bool Definition::IsVisible(Definition* pDef)
  {
    Node* pNode = m_parent;

    while(pNode)
    {
      if(pNode->m_name2node.find(pDef->m_name) != pNode->m_name2node.end())
      {
        return true;
      }

      pNode = pNode->m_parent;
    }

    return false;
  }

  void Definition::push_back(Node* pNode)
  {
//    if(Reference* pRef = dynamic_cast<Reference*>(pNode))
    {
      ASSERT(m_status == node);

      m_status = node;

      if(IsTypeUnknown() && !pNode->IsTypeUnknown())
      {
        m_type = pNode->m_type; 
        m_autotype = true;
      }

      RemoveFromCache(pNode->m_type);
    }

    __super::push_back(pNode);
  }

  Definition& Definition::operator[] (LPCWSTR type) 
  {
    Definition* pRetDef = NULL;
    StringMapW<Definition*>::iterator it = m_type2def.find(type);
    if(it != m_type2def.end())
      return *(it->second);

    pRetDef = DNew Definition(m_pnf, L"");
    pRetDef->m_priority = PLow;
    pRetDef->m_type = type;
    m_type2def[type] = pRetDef;

    std::list<Definition*> l;
    GetChildDefs(l, type);

    while(!l.empty())
    {
      Definition* pDef = l.front(); l.pop_front();

      pRetDef->m_priority = pDef->m_priority;
      pRetDef->m_parent = pDef->m_parent;

      if(pDef->IsValue())
      {
        pRetDef->SetAsValue(pDef->m_status, pDef->m_value, pDef->m_unit);
      }
      else
      {
        pRetDef->m_status = node; 
        pRetDef->m_nodes.insert(pRetDef->m_nodes.end(), pDef->m_nodes.begin(), pDef->m_nodes.end());
      }
    }

    return *pRetDef;
  }

  void Definition::RemoveFromCache(LPCWSTR type)
  {
    if(!type)
    {
      StringMapW<Definition*>::iterator it = m_type2def.begin();
      for(; it != m_type2def.end(); ++it) delete it->second;
    }
    else 
    {
      StringMapW<Definition*>::iterator it = m_type2def.find(type);
      if (it != m_type2def.end())
      {
        delete it->second;
        m_type2def.erase(it);
      }
    }
  }

  bool Definition::IsValue(status_t s)
  {
    return s ? m_status == s : m_status != node;
  }

  void Definition::SetAsValue(status_t s, CStdStringW v, CStdStringW u)
  {
    ASSERT(s != node);

    m_nodes.clear();
    m_name2node.clear();

    m_status = s;

    m_value = v;
    m_unit = u;
  }

  void Definition::SetAsNumber(CStdStringW v, CStdStringW u)
  {
    SetAsValue(number, v, u);

    Number<float> n;
    GetAsNumber(n); // will throw an exception if not a number
  }

  template<class T> 
  void Definition::GetAsNumber(Number<T>& n, StringMapW<T>* n2n)
  {
    CStdStringW str = m_value;
    str.Replace(L" ", L"");

    n.value = 0;
    n.unit = m_unit;
    n.sign = 0;

    if(n2n)
    {
      if(m_status == node) throw Exception(_T("expected value type"));

      StringMapW<T>::iterator it = n2n->find(str);
      if(it != n2n->end())
      {
        n.value = it->second;
        return;
      }
    }

    if(m_status != number) throw Exception(_T("expected number"));

    n.sign = str.Find('+') == 0 ? 1 : str.Find('-') == 0 ? -1 : 0;
    str.TrimLeft(L"+-");

    if(str.Find(L"0x") == 0)
    {
      if(n.sign) throw Exception(_T("hex values must be unsigned"));

      n.value = (T)wcstoul(str.Mid(2), NULL, 16);
    }
    else
    {
      CStdStringW num_string = m_value + m_unit;

      if(m_num_string != num_string)
      {
        Split sa(':', str);
        Split sa2('.', sa ? sa[sa-1] : L"");

        if(sa == 0 || sa2 == 0 || sa2 > 2) throw Exception(_T("invalid number"));

        float f = 0;
        for(size_t i = 0; i < sa; i++) {f *= 60; f += wcstoul(sa[i], NULL, 10);}
        if(sa2 > 1) f += (float)wcstoul(sa2[1], NULL, 10) / pow((float)10, sa2[1].GetLength());

        if(n.unit == L"ms") {f /= 1000; n.unit = L"s";}
        else if(n.unit == L"m") {f *= 60; n.unit = L"s";}
        else if(n.unit == L"h") {f *= 3600; n.unit = L"s";}

        m_num.value = f;
        m_num.unit = n.unit;
        m_num_string = num_string;

        n.value = (T)f;
      }
      else
      {
        n.value = (T)m_num.value;
        n.unit = m_num.unit;
      }

      if(n.sign) n.value *= n.sign;
    }
  }

  void Definition::GetAsString(CStdStringW& str)
  {
    if(m_status == node) throw Exception(_T("expected value type"));

    str = m_value; 
  }

  void Definition::GetAsNumber(Number<int>& n, StringMapW<int>* n2n) {return GetAsNumber<int>(n, n2n);}
  void Definition::GetAsNumber(Number<DWORD>& n, StringMapW<DWORD>* n2n) {return GetAsNumber<DWORD>(n, n2n);}
  void Definition::GetAsNumber(Number<float>& n, StringMapW<float>* n2n) {return GetAsNumber<float>(n, n2n);}

  void Definition::GetAsBoolean(bool& b)
  {
    static StringMapW<bool> s2b;

    if(s2b.empty())
    {
      s2b[L"true"] = true;
      s2b[L"on"] = true;
      s2b[L"yes"] = true;
      s2b[L"1"] = true;
      s2b[L"false"] = false;
      s2b[L"off"] = false;
      s2b[L"no"] = false;
      s2b[L"0"] = false;
    }

    if(s2b.find(m_value) == s2b.end()) // m_status != boolean && m_status != number || 
    {
      throw Exception(_T("expected boolean"));
    }
  }

  bool Definition::GetAsTime(Time& t, StringMapW<float>& offset, StringMapW<float>* n2n, int default_id)
  {
    Definition& time = (*this)[L"time"];

    CStdStringW id;
    if(time[L"id"].IsValue()) id = time[L"id"];
    else id.Format(L"%d", default_id);

    float scale = time[L"scale"].IsValue() ? time[L"scale"] : 1.0f;

    if(time[L"start"].IsValue() && time[L"stop"].IsValue())
    {
      time[L"start"].GetAsNumber(t.start, n2n);
      time[L"stop"].GetAsNumber(t.stop, n2n);

      if(t.start.unit.empty()) t.start.value *= scale;
      if(t.stop.unit.empty()) t.stop.value *= scale;

      float o = 0;
      StringMapW<float>::iterator it = offset.find(id);
      o = it->second;

      if(t.start.sign != 0) t.start.value = o + t.start.value;
      if(t.stop.sign != 0) t.stop.value = t.start.value + t.stop.value;

      offset[id] = t.stop.value;

      return true;
    }

    return false;
  }

  Definition::operator LPCWSTR()
  {
    CStdStringW str;
    GetAsString(str);
    return str;
  }

  Definition::operator float()
  {
    float d;
    GetAsNumber(d);
    return d;
  }

  Definition::operator bool()
  {
    bool b;
    GetAsBoolean(b);
    return b;
  }

  Definition* Definition::SetChildAsValue(CStdStringW path, status_t s, CStdStringW v, CStdStringW u)
  {
    Definition* pDef = this;

    Split split('.', path);

    for(size_t i = 0, j = split-1; i <= j; i++)
    {
      CStdStringW type = split[i];

      if(pDef->m_nodes.empty() || !dynamic_cast<Reference*>(pDef->m_nodes.back()))
      {
        EXECUTE_ASSERT(m_pnf->CreateRef(pDef) != NULL);
      }

      if(Reference* pRef = dynamic_cast<Reference*>(pDef->m_nodes.back()))
      {
        pDef = NULL;

        std::list<Node *>::reverse_iterator it = pRef->m_nodes.rbegin();
        for(; it != pRef->m_nodes.rend(); ++it)
        {
          Definition* pChildDef = dynamic_cast<Definition*>(*it);

          if(pChildDef->IsType(type))
          {
            if(pChildDef->IsNameUnknown()) pDef = pChildDef;
            break;
          }
        }

        if(!pDef)
        {
          pDef = m_pnf->CreateDef(pRef, type);
        }

        if(i == j)
        {
          pDef->SetAsValue(s, v, u);
          return pDef;
        }
      }
    }

    return NULL;
  }

  Definition* Definition::SetChildAsNumber(CStdStringW path, CStdStringW v, CStdStringW u)
  {
    Definition* pDef = SetChildAsValue(path, number, v, u);

    Number<float> n;
    pDef->GetAsNumber(n); // will throw an exception if not a number

    return pDef;
  }

  void Definition::Dump(OutputStream& s, int level, bool fLast)
  {
    if(m_predefined) return;

    CStdStringW tabs(' ', level*4);

    CStdStringW str = tabs;
    if(m_predefined) str += '?';
    if(m_priority == PLow) str += '*';
    else if(m_priority == PHigh) str += '!';
    if(!IsTypeUnknown() && !m_autotype) str += m_type;
    if(!IsNameUnknown()) str += '#' + m_name;
    str += ':';
    s.PutString(L"%s", str);

    if(!m_nodes.empty())
    {
      std::list<Node *>::iterator it = m_nodes.begin();
      for(; it != m_nodes.end(); ++it)
      {
        Node* pNode = *it;

        if(Reference* pRef = dynamic_cast<Reference*>(pNode))
        {
          pRef->Dump(s, level, fLast);
        }
        else 
        {
          ASSERT(!pNode->IsNameUnknown());
          s.PutString(L" %s", pNode->m_name);
        }
      }

      s.PutString(L";\n");

      if(!fLast && (!m_nodes.empty() || level == 0)) s.PutString(L"\n");
    }
    else if(m_status == string)
    {
      CStdStringW str = m_value;
      str.Replace(L"\"", L"\\\"");
      s.PutString(L" \"%s\";\n", str);
    }
    else if(m_status == number)
    {
      CStdStringW str = m_value;
      if(!m_unit.empty()) str += m_unit;
      s.PutString(L" %s;\n", str);
    }
    else if(m_status == boolean)
    {
      s.PutString(L" %s;\n", m_value);
    }
    else if(m_status == block)
    {
      s.PutString(L" {%s};\n", m_value);
    }
    else
    {
      s.PutString(L" null;\n");
    }
  }
}
