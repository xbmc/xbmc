/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/python/AddonPythonInvoker.h"

#include <memory>

class CFileItem;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

class CContextItemAddonInvoker : public CAddonPythonInvoker
{
public:
  explicit CContextItemAddonInvoker(ILanguageInvocationHandler *invocationHandler,
                                    const CFileItemPtr& item);
  ~CContextItemAddonInvoker() override;

protected:
  void onPythonModuleInitialization(void* moduleDict) override;

private:
  const CFileItemPtr m_item;
};
