/*
*  Copyright © 2010-2015 Team XBMC
*  http://xbmc.org
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include <stack>
#include <DirectXMath.h>

namespace DirectX
{
// replacement of D3DXMatrixStack which implements only used methods.
__declspec(align(16)) class XMMatrixStack
{
public:
  XMMatrixStack() : m_current(XMMatrixIdentity())
  {
  }

  ~XMMatrixStack()
  {
    while (!m_stack.empty())
      m_stack.pop();
  }

  XMMATRIX* GetTop()
  {
    return &m_current;
  }

  void Pop()
  {
    if (!m_stack.empty())
    {
      m_current = m_stack.top();
      m_stack.pop();
    }
  }

  void Push()
  {
    m_stack.push(m_current);
  }

  void MultMatrix(const XMMATRIX* pMat)
  {
    m_current = XMMatrixMultiply(m_current, *pMat);
  }

  void LoadIdentity()
  {
    m_current = XMMatrixIdentity();
  }

  void TranslateLocal(float x, float y, float z)
  {
    m_current = XMMatrixMultiply(XMMatrixTranslation(x, y, z), m_current);
  }

  void ScaleLocal(float x, float y, float z)
  {
    m_current = XMMatrixMultiply(XMMatrixScaling(x, y, z), m_current);
  }

  void RotateAxisLocal(const XMVECTOR *pV, float angle)
  {
    m_current = XMMatrixMultiply(XMMatrixRotationAxis(*pV, angle), m_current);
  }

  void* operator new(size_t i)
  {
    return _aligned_malloc(i, 16);
  }

  void operator delete(void* p)
  {
    _aligned_free(p);
  }

private:
  XMMATRIX m_current;
  std::stack<XMMATRIX> m_stack;

}; // class XMMatrixStack

}; // namespace DirectX