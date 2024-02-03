![Kodi Logo](../docs/resources/banner_slim.png)

# Kodi's licensing rules
Kodi is provided under the terms of the GNU General Public License v2.0 or later, as provided in **[LICENSES/GPL-2.0-or-later](GPL-2.0-or-later)**.

This file provides a description of how each source file should be annotated to make its license clear and unambiguous. **It doesn't replace Kodi's license**.

The license described in the **[LICENSE](../LICENSE.md)** file applies to **Kodi's source as a whole**, though individual source files can have a different license which is required to be compatible with the **GPL-2.0**. Aside from that, individual files can be provided under a dual license, e.g. one of the compatible GPL variants and alternatively under a permissive license like BSD, MIT etc.

## The SPDX License Identifier
The common way of expressing the license of a source file is to add the matching boilerplate text into the top comment of the file. Due to formatting, typos etc. these "boilerplates" are hard to validate for tools which are used in the context of license compliance.

An alternative to boilerplate text is the use of Software Package Data Exchange (SPDX) license identifiers in each source file. SPDX license identifiers are machine parsable and precise shorthands for the license under which the content of the file is contributed. SPDX license identifiers are managed by the SPDX Workgroup at the Linux Foundation and have been agreed on by partners throughout the industry, tool vendors, and legal teams. For further information see **[spdx.org](https://spdx.org/)**.

Kodi requires the precise SPDX identifier in all source files. The valid identifiers used in Kodi have been retrieved from the official **[SPDX License List](https://spdx.org/licenses/)** along with the license texts.

### License Identifier Placement
The SPDX license identifier in Kodi files shall be added at the top of the file.

### License Identifier Syntax
A `<SPDX License Expression>` is either an SPDX short form license identifier found on the **[SPDX License List](https://spdx.org/licenses/)**, or the combination of two SPDX short form license identifiers separated by `WITH` when a license exception applies. When multiple licenses apply, an expression consists of keywords `AND`, `OR` separating sub-expressions. See SPDX License Expression **[usage guide](https://spdx.org/ids)** for more information.

### License Identifier Style
The SPDX license identifier is added in the form of a comment. The comment style depends on the file type. For a C/C++ header or source file, the format must be
```
/*
 *  Copyright (C) <year> <copyright holders>
 *  This file is part of <software> - <URL>
 *
 *  SPDX-License-Identifier: <SPDX License Expression>
 *  See <license file/license index file> for more information.
 */
```

Since most source files in Kodi are `GPL-2.0-or-later` licensed, the typical copyright notice will look like this:
```
/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
```

> [!NOTE]  
> Kodi is developed and maintained by Kodi Team members and the open-source community. We thank all of our **[contributors](https://github.com/xbmc/xbmc/graphs/contributors)**! **For the detailed history of contributions** of a given file, use `git blame <filename>` to see line-by-line credits or `git log --follow <filename>` to see the changelog across renames, rewrites and code shuffle.

### License Files
All SPDX license identifiers must have a corresponding file in the **LICENSES** subdirectory. This is required to allow tool verification (e.g. **[ScanCode toolkit](https://github.com/nexB/scancode-toolkit)**) and to have the licenses ready to read and extract right from the source, which is recommended by various FOSS organizations, e.g. the **[FSFE REUSE Initiative](https://reuse.software/)**.

The typical license file looks like this:
```
Valid-License-Identifier: GPL-2.0-or-later
SPDX-URL: https://spdx.org/licenses/GPL-2.0-or-later.html
Usage-Guide:
  To use the GNU General Public License v2.0 or later put the following SPDX
  tag/value pair into a comment according to the placement guidelines in
  the licensing rules documentation:
    SPDX-License-Identifier: GPL-2.0-or-later
License-Text:
  Full license text
```

Kodi currently contains files and code licensed under:

* **[BSD-3-Clause](BSD-3-Clause)**: BSD 3-clause "New" or "Revised" License
* **[BSD-4-Clause](BSD-4-Clause)**: BSD 4-Clause "Original" or "Old" License
* **[GPL-2.0-only](GPL-2.0-only)**: GNU General Public License v2.0 only
* **[GPL-2.0-or-later](GPL-2.0-or-later)**: GNU General Public License v2.0 or later
* **[LGPL-2.1-or-later](LGPL-2.1-or-later)**: GNU Lesser General Public License v2.1 or later
* **[MIT](MIT)**: MIT License
* **[Unlicense](Unlicense)**: Unlicense (Public Domain)

These licensing rules were adapted from the **[Linux kernel licensing rules](https://github.com/torvalds/linux/blob/master/Documentation/process/license-rules.rst)**.

