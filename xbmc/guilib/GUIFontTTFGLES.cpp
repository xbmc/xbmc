/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFontTTFGLES.h"

#include "GUIComponent.h"
#include "ServiceBroker.h"
#include "Texture.h"
#include "TextureManager.h"
#include "rendering/MatrixGL.h"
#include "utils/GLUtils.h"
#include "windowing/GraphicContext.h"

CGUIFontTTFGLES::CGUIFontTTFGLES(const std::string& strFileName) : CGUIFontTTFGLBase(strFileName)
{
  m_renderSystem = dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
}

bool CGUIFontTTFGLES::FirstBegin()
{
  GLenum pixformat = GL_ALPHA; // deprecated
  GLenum internalFormat = GL_ALPHA;

  if (m_textureStatus == TEXTURE_REALLOCATED)
  {
    if (glIsTexture(m_nTexture))
      CServiceBroker::GetGUI()->GetTextureManager().ReleaseHwTexture(m_nTexture);

    m_textureStatus = TEXTURE_VOID;
  }

  if (m_textureStatus == TEXTURE_VOID)
  {
    // Have OpenGL generate a texture object handle for us
    glGenTextures(1, (GLuint*)&m_nTexture);

    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, m_nTexture);

    // Set the texture's stretching properties
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Set the texture image -- THIS WORKS, so the pixels must be wrong.
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_texture->GetWidth(), m_texture->GetHeight(), 0,
                 pixformat, GL_UNSIGNED_BYTE, 0);

    VerifyGLState();
    m_textureStatus = TEXTURE_UPDATED;
  }

  if (m_textureStatus == TEXTURE_UPDATED)
  {
    glBindTexture(GL_TEXTURE_2D, m_nTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, m_updateY1, m_texture->GetWidth(), m_updateY2 - m_updateY1,
                    pixformat, GL_UNSIGNED_BYTE,
                    m_texture->GetPixels() + m_updateY1 * m_texture->GetPitch());

    m_updateY1 = m_updateY2 = 0;
    m_textureStatus = TEXTURE_READY;
  }

  // Turn Blending On
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
  glEnable(GL_BLEND);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_nTexture);

  return true;
}

void CGUIFontTTFGLES::LastEnd()
{
  m_renderSystem->EnableGUIShader(SM_FONTS);

  GLint posLoc = m_renderSystem->GUIShaderGetPos();
  GLint colLoc = m_renderSystem->GUIShaderGetCol();
  GLint tex0Loc = m_renderSystem->GUIShaderGetCoord0();
  GLint modelLoc = m_renderSystem->GUIShaderGetModel();

  CreateStaticVertexBuffers();

  // Enable the attributes used by this shader
  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);
  glEnableVertexAttribArray(tex0Loc);

  if (!m_vertex.empty())
  {
    // Deal with vertices that had to use software clipping
    std::vector<SVertex> vecVertices(6 * (m_vertex.size() / 4));
    SVertex* vertices = &vecVertices[0];
    for (size_t i = 0; i < m_vertex.size(); i += 4)
    {
      *vertices++ = m_vertex[i];
      *vertices++ = m_vertex[i + 1];
      *vertices++ = m_vertex[i + 2];
      *vertices++ = m_vertex[i + 1];
      *vertices++ = m_vertex[i + 3];
      *vertices++ = m_vertex[i + 2];
    }

    vertices = &vecVertices[0];

    glVertexAttribPointer(posLoc,  3, GL_FLOAT, GL_FALSE, sizeof(SVertex), (char*)vertices + offsetof(SVertex, x));
    glVertexAttribPointer(colLoc,  4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SVertex), (char*)vertices + offsetof(SVertex, r));
    glVertexAttribPointer(tex0Loc, 2, GL_FLOAT,  GL_FALSE, sizeof(SVertex), (char*)vertices + offsetof(SVertex, u));

    glDrawArrays(GL_TRIANGLES, 0, vecVertices.size());
  }

  if (!m_vertexTrans.empty())
  {
    // Deal with the vertices that can be hardware clipped and therefore translated

    // Bind our pre-calculated array to GL_ELEMENT_ARRAY_BUFFER
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementArrayHandle);
    // Store current scissor
    CRect scissor = CServiceBroker::GetWinSystem()->GetGfxContext().StereoCorrection(
        CServiceBroker::GetWinSystem()->GetGfxContext().GetScissors());

    for (size_t i = 0; i < m_vertexTrans.size(); i++)
    {
      if (m_vertexTrans[i].vertexBuffer->bufferHandle == 0)
      {
        continue;
      }

      // Apply the clip rectangle
      CRect clip = m_renderSystem->ClipRectToScissorRect(m_vertexTrans[i].clip);
      if (!clip.IsEmpty())
      {
        // intersect with current scissor
        clip.Intersect(scissor);
        // skip empty clip
        if (clip.IsEmpty())
        {
          continue;
        }

        m_renderSystem->SetScissors(clip);
      }

      // Apply the translation to the currently active (top-of-stack) model view matrix
      glMatrixModview.Push();
      glMatrixModview.Get().Translatef(m_vertexTrans[i].translateX, m_vertexTrans[i].translateY,
                                       m_vertexTrans[i].translateZ);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glMatrixModview.Get());

      // Bind the buffer to the OpenGL context's GL_ARRAY_BUFFER binding point
      glBindBuffer(GL_ARRAY_BUFFER, m_vertexTrans[i].vertexBuffer->bufferHandle);

      // Do the actual drawing operation, split into groups of characters no
      // larger than the pre-determined size of the element array
      for (size_t character = 0; m_vertexTrans[i].vertexBuffer->size > character; character += 1000)
      {
        size_t count = m_vertexTrans[i].vertexBuffer->size - character;
        count = std::min<size_t>(count, 1000);

        // Set up the offsets of the various vertex attributes within the buffer
        // object bound to GL_ARRAY_BUFFER
        glVertexAttribPointer(
            posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex),
            reinterpret_cast<const GLvoid*>(
                reinterpret_cast<char*>(character * sizeof(SVertex) * 4) + offsetof(SVertex, x)));
        glVertexAttribPointer(
            colLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SVertex),
            reinterpret_cast<const GLvoid*>(
                reinterpret_cast<char*>(character * sizeof(SVertex) * 4) + offsetof(SVertex, r)));
        glVertexAttribPointer(
            tex0Loc, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex),
            reinterpret_cast<const GLvoid*>(
                reinterpret_cast<char*>(character * sizeof(SVertex) * 4) + offsetof(SVertex, u)));

        glDrawElements(GL_TRIANGLES, 6 * count, GL_UNSIGNED_SHORT, 0);
      }

      glMatrixModview.Pop();
    }

    // Restore the original scissor rectangle
    m_renderSystem->SetScissors(scissor);
    // Restore the original model view matrix
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glMatrixModview.Get());
    // Unbind GL_ARRAY_BUFFER and GL_ELEMENT_ARRAY_BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  // Disable the attributes used by this shader
  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);
  glDisableVertexAttribArray(tex0Loc);

  m_renderSystem->DisableGUIShader();
}
