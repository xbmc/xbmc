# AppStream translations

`kodi.metainfo.xml.in` is the English source for Kodi's AppStream metadata.
GNU gettext 0.19.7 or newer, including its MetaInfo ITS rules, is needed
to merge translations in the Linux build. Weblate maintains the checked-in
POT template.

## Weblate component

Create a component in Kodi's Weblate project linked to the existing Kodi
repository, with these settings:

* File format: GNU gettext PO file
* File mask: `tools/Linux/translations/*.po`
* Monolingual base language file: leave empty
* Template for new translations: `tools/Linux/translations/kodi.pot`
* Source language: English (United Kingdom), `en_GB`
* Translation license: CC0-1.0, matching the metadata license

Enable the **Update POT file (xgettext)** add-on with:

* Input source: Source file patterns
* Source file patterns: `tools/Linux/kodi.metainfo.xml.in`
* xgettext language: leave empty for automatic detection

The Weblate installation must include AppStream ITS extraction support
from [PR #21599](https://github.com/WeblateOrg/weblate/pull/21599).
Weblate supplies the ITS rules; no project-local rules are needed.

Also enable **Update PO files to match POT (msgmerge)** so source changes
reach existing translations. Use the existing repository integration to
send the updated POT template and translated PO files back to Kodi.

PO filenames must use gettext locale names, for example `de.po` or `pt_BR.po`.
The Linux build discovers these files automatically and merges them into the
installed metadata. No `LINGUAS` file needs to be maintained. Untranslated and
fuzzy strings retain the English source.

Strings use XML markup. Preserve inline tags and escape literal special
characters, for example `&amp;` for an ampersand.
