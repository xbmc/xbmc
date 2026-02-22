/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI::UTILS::I18N
{
struct BaseSubTag;
struct LanguageSubTag;
struct ExtLangSubTag;
struct ScriptSubTag;
struct RegionSubTag;
struct VariantSubTag;
struct GrandfatheredTag;
struct RedundantTag;
struct RegistryFileRecord;

/*!
 * \brief Load the common attributes shared by all subtags and tags from a registry file record.
 * \param[in,out] subTag The object to load into.
 * \param[in] record The source record information.
 * \return True for success, false otherwise (ex. missing mandatory fields, invalid field values)
 */
bool LoadBaseSubTag(BaseSubTag& subTag, const RegistryFileRecord& record);

/*!
 * \brief Load a Language subtag object from a registry file record.
 * \param[in,out] subTag The object to load .
 * \param[in] record The source record information.
 * \return True for success, false otherwise (ex. missing mandatory fields, invalid field values)
 */
bool LoadLanguageSubTag(LanguageSubTag& subTag, const RegistryFileRecord& record);

/*!
 * \brief Load an ExtLang subtag object from a registry file record.
 * \param[in,out] subTag The object to load into.
 * \param[in] record The source record information.
 * \return True for success, false otherwise (ex. missing mandatory fields, invalid field values)
 */
bool LoadExtLangSubTag(ExtLangSubTag& subTag, const RegistryFileRecord& record);

/*!
 * \brief Load a Variant subtag object from a registry file record.
 * \param[in,out] subTag The object to load into.
 * \param[in] record The source record information.
 * \return True for success, false otherwise (ex. missing mandatory fields, invalid field values)
 */
bool LoadVariantSubTag(VariantSubTag& subTag, const RegistryFileRecord& record);
} // namespace KODI::UTILS::I18N
