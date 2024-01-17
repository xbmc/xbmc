/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

class IInputCodingTable
{
public:
  enum
  {
    TYPE_WORD_LIST,
    TYPE_CONVERT_STRING
  };
  virtual int GetType() { return TYPE_WORD_LIST; }

  virtual ~IInputCodingTable() = default;
  /*! \brief Called for the active keyboard layout when it's loaded, stick any initialization here

      This won't be needed for most implementations so we don't set it =0 but provide a default
      implementation.
  */
  virtual void Initialize() {}

  /*! \brief Called for the active keyboard layout when it's unloaded, stick any cleanup here

      This won't be needed for most implementations so we don't set it =0 but provide a default
      implementation.
  */
  virtual void Deinitialize() {}

  /*! \brief Can be overridden if initialization is expensive to avoid calling initialize more than
      needed

      \return true if initialization has been done and was successful, false otherwise.
  */
  virtual bool IsInitialized() const { return true; }
  virtual bool GetWordListPage(const std::string& strCode, bool isFirstPage) = 0;
  virtual std::vector<std::wstring> GetResponse(int response) = 0;
  const std::string& GetCodeChars() const { return m_codechars; }

  virtual void SetTextPrev(const std::string& strTextPrev) {}
  virtual std::string ConvertString(const std::string& strCode) { return std::string(""); }

protected:
  std::string m_codechars;
};

typedef std::shared_ptr<IInputCodingTable> IInputCodingTablePtr;
