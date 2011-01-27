@ECHO OFF
REM Complete check of a stateless encoding.
REM Usage: check-stateless.bat SRCDIR CHARSET

.\table-from %2 > tmp-%2.TXT
.\table-to %2 | sort > tmp-%2.INVERSE.TXT
fc %1\%2.TXT tmp-%2.TXT

if not exist %1\%2.IRREVERSIBLE.TXT goto ELSE_1
  copy /a %1\%2.TXT /a + %1\%2.IRREVERSIBLE.TXT /a tmp
  sort < tmp | uniq-u > tmp-orig-%2.INVERSE.TXT
  fc tmp-orig-%2.INVERSE.TXT tmp-%2.INVERSE.TXT
  del tmp
  del tmp-orig-%2.INVERSE.TXT
  goto ENDIF_1
:ELSE_1
  fc %1\%2.TXT tmp-%2.INVERSE.TXT
:ENDIF_1

del tmp-%2.TXT
del tmp-%2.INVERSE.TXT
