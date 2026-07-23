/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GLBufferObject.h"

#include <cassert>

KODI::UTILS::GL::CGLBufferObject::~CGLBufferObject()
{
  Destroy();
}

void KODI::UTILS::GL::CGLBufferObject::SetData(const void* data, std::size_t size, GLenum usage)
{
  if (m_buffer == 0)
    glGenBuffers(1, &m_buffer);

  glBindBuffer(m_target, m_buffer);
  glBufferData(m_target, size, data, usage);
}

void KODI::UTILS::GL::CGLBufferObject::SetDataOnce(const void* data, std::size_t size)
{
  if (m_buffer == 0)
    SetData(data, size, GL_STATIC_DRAW);
  else
    glBindBuffer(m_target, m_buffer);
}

void KODI::UTILS::GL::CGLBufferObject::Bind() const
{
  assert(m_buffer != 0 && "Bind() called before SetData()/SetDataOnce()");
  glBindBuffer(m_target, m_buffer);
}

void KODI::UTILS::GL::CGLBufferObject::Destroy()
{
  if (m_buffer != 0)
  {
    glDeleteBuffers(1, &m_buffer);
    m_buffer = 0;
  }
}
