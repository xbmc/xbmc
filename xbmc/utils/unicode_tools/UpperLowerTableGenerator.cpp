/*
 * CaseFoldingTableGenerator.cpp
 *
 *  Created on: Nov 1, 2022
 *      Author: fbacher
 *
 *  Creates C++ static data for simple folding for use in StringUtils.
 *
 *  The input data (unicode_fold_upper & unicode_fold_lower) was derived from
 *  https://www.unicode.org/Public/UCD/latest/ucd/CaseFolding.txt. A copy of
 *  this table is included with this source. The layout of the data in CaseFolding.txt
 *  is straightforward and documented fully there. To summarize:
 *    Each line of data is of the format:
 *
 *    <upper_case_hex_value>; Status_code; <folded_case_hex_value>; <comment>
 *
 *    The status_code is one of:
 *    C - line applies to both simple and complex case folding (included in table below)
 *    F - only applies to full case folding (not supported here and so not included
 *        below)
 *    S - line applies to simple case folding (included in table below)
 *    T - special (Turkic) NOT included below since we don't want to follow Turkic
 *        rules.
 *
 *
 *  The tables unicode_fold_upper & unicode_fold_lower are parallel arrays.
 *  If you want to fold the case of character X, you lookup up X in unicode_fold_upper.
 *  If present, the folded case for X, located at unicode_fold_upper[n] is at
 *  unicode_fold_lower[n].
 *
 *  Note that there are some cases where more than one character in unicode_fold_upper
 *  maps to the same unicode_fold_lower. This prevents the ability to have a
 *  "FoldCaseUpper" where case folding produces upper-case versions of letters
 *  instead of lower-case ones. Further, note that although most FoldCase characters
 *  are the lower-case versions. This is not universal.
 *
 *  Running this program produces c++ code that is pasted (after some massaging) into
 *  StringUtils.cpp. The comments below, which describe the pasted code, are also present
 *  in StringUtils.cpp.
 *
 *  The tables below are logically indexed by the 32-bit Unicode value of the
 *  character which is to be case folded. The value found in the table is
 *  the case folded value, or 0 if no such value exists.
 *
 *  A char32_t contains a 32-bit Unicode codepoint, although only 24 bits is
 *  used. The array FOLDCASE_INDEX is indexed by the upper 16-bits of the
 *  the 24-bit codepoint, yielding a pointer to another table indexed by
 *  the lower 8-bits of the codepoint (FOLDCASE_0x0001, etc.) to get the lower-case equivalent
 *  for the original codepoint (see StringUtils::FoldCaseChar).
 *
 *  Specifically, FOLDCASE_0x...[0] contains the number of elements in the
 *  array. This helps reduce the size of the table. All other non-zero elements
 *  contain the Unicode value for a codepoint which can be case folded into another codepoint.
 *   This means that for "A", 0x041, the FoldCase value can be found by:
 *
 *    high_bytes = 0x41 >> 8; => 0
 *    char32_t* table = FOLDCASE_INDEX[high_bytes]; => address of FOLDCASE_0000
 *    uint16_t low_byte = c & 0xFF; => 0x41
 *    char32_t foldedChar = table[low_byte + 1]; => 0x61 'a'
 *
 */

#include <algorithm>
#include <codecvt>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <ranges>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string_view>
#include <vector>

using namespace std::literals;

class CaseMap
{
public:
  char32_t codepoint;
  char32_t upperCaseValue;
  char32_t lowerCaseValue;
  char32_t titleCaseValue;

  CaseMap(char32_t p_codepoint,
          char32_t p_upperCaseValue,
          char32_t p_lowerCaseValue,
          char32_t p_titleCaseValue)
    : codepoint(p_codepoint),
      upperCaseValue(p_upperCaseValue),
      lowerCaseValue(p_lowerCaseValue),
      titleCaseValue(p_titleCaseValue)
  {
  }

  CaseMap() : codepoint(0), upperCaseValue(0), lowerCaseValue(0), titleCaseValue(0) {}
  CaseMap(int p_tableLength)
    : codepoint(p_tableLength), upperCaseValue(0), lowerCaseValue(0), titleCaseValue(0)
  {
  }
  CaseMap(const CaseMap& o)
    : codepoint(o.codepoint),
      upperCaseValue(o.upperCaseValue),
      lowerCaseValue(o.lowerCaseValue),
      titleCaseValue(o.titleCaseValue)
  {
  }

  std::string print()
  {
    std::stringstream ss;
    std::cout << std::hex << std::noshowbase // manually show the 0x prefix
              << std::internal // fill between the prefix and the number
              << std::setfill('0'); // fill with 0s

    if (codepoint > 0x0)
    {
      ss << "CaseMap(U'\\x" << std::setw(5) << std::hex << std::noshowbase << std::internal
         << std::setfill('0') << codepoint << "', "
         << "U'\\x" << std::setw(5) << std::hex << upperCaseValue << "', "
         << "U'\\x" << std::setw(5) << std::hex << lowerCaseValue << "', "
         << "U'\\x" << std::setw(5) << std::hex << titleCaseValue << "')";
    }
    else
    {
      ss << "CaseMap()";
    }
    return ss.str();
  }
};

const char32_t DUMMY_CHAR32_T = 0;
std::vector<CaseMap> caseMapTable;

bool compare(const CaseMap& left, const CaseMap& right)
{
  bool less = left.codepoint < right.codepoint;
  bool failed = false;
  if ((left.codepoint == 0) or (right.codepoint == 0))
  {
    failed = true;
  }
  return less;
}

static void loadData()
{

  std::ios_base::openmode mode = std::ios_base::in;

  std::ifstream ifstream("UnicodeData.txt", mode);
  std::string line;
  std::stringstream ss;

  // std::getline(ifstream, line);

  int idx = 0;

  while (std::getline(ifstream, line))
  {

    // Field info from: https://www.unicode.org/L2/L1999/UnicodeData.html#Case%20Mappings

    std::stringstream linestream(line);
    char32_t codepoint;
    std::string charCaseCode;
    char32_t charCaseValue = 0;
    char32_t upperCaseValue = 0;
    char32_t lowerCaseValue = 0;
    char32_t titleCaseValue = 0;

    int tmpValue;
    std::string tmp;
    std::getline(linestream, tmp, ';'); // codepoint
    if (linestream.eof())
      continue;

    codepoint = static_cast<char32_t>(stoi(tmp, 0, 16));

    std::string ignore;
    std::getline(linestream, ignore, ';'); // character name
    if (linestream.eof())
      continue;

    std::getline(linestream, charCaseCode, ';'); // General category
    // Normally we think that only letters can be upper/lower cased,
    // however, some non-letter marks also can change depending upon casing.
    // An example is codepoint 0x0345 'Iota subscript' (see SpecialCasing.txt).
    // So, as long as casing information is specified in upper/lower/title case
    // mapping fields, then we process them, even if not letters.
    //
    //
    // bool skip = true;
    // if (charCaseCode == "Lu" || charCaseCode == "Ll" || charCaseCode == "Lt")
    //  skip = false;
    // if (skip)
    //  continue;

    if (linestream.eof())
      continue;
    // At field 2, Skip to field 12

    std::getline(linestream, ignore, ';'); // Canonical combining classes
    if (linestream.eof())
      continue;
    std::getline(linestream, ignore, ';'); // Bidirectional category
    if (linestream.eof())
      continue;
    std::getline(linestream, ignore, ';'); // Character decomposition mapping
    if (linestream.eof())
      continue;
    std::getline(linestream, ignore, ';'); // Decimal digit value
    if (linestream.eof())
      continue;
    std::getline(linestream, ignore, ';'); // Digit value
    if (linestream.eof())
      continue;
    std::getline(linestream, ignore, ';'); // Numeric Value
    if (linestream.eof())
      continue;
    std::getline(linestream, ignore, ';'); // Mirrored
    if (linestream.eof())
      continue;
    std::getline(linestream, ignore, ';'); // Unicode 1.0 name
    if (linestream.eof())
      continue;
    std::getline(linestream, ignore, ';'); // 10646 comment field
    if (linestream.eof())
      continue;

    /*
    if (charCaseCode == "Lu")
    {
       upperCaseValue = static_cast<char32_t> (tmpValue);
    }
    if (charCaseCode == "Ll")
    {
       lowerCaseValue = static_cast<char32_t> (tmpValue);
    }
    if (charCaseCode == "Lt")
    {
       titleCaseValue = static_cast<char32_t> (tmpValue);
    }
     */

    bool add = false;
    std::getline(linestream, tmp, ';'); // Uppercase mapping
    if (tmp.length() > 0)
    {
      add = true;
      upperCaseValue = static_cast<char32_t>(stoi(tmp, 0, 16));
    }
    std::getline(linestream, tmp, ';'); // lowercase mapping
    if (tmp.length() > 0)
    {
      add = true;
      lowerCaseValue = static_cast<char32_t>(stoi(tmp, 0, 16));
    }
    std::getline(linestream, tmp, ';'); // titlecase mapping
    if (tmp.length() > 0)
    {
      add = true;
      titleCaseValue = static_cast<char32_t>(stoi(tmp, 0, 16));
    }

    if (add)
    {
      caseMapTable.emplace_back(CaseMap(codepoint, upperCaseValue, lowerCaseValue, titleCaseValue));
      idx++;
    }
  }

  sort(caseMapTable.begin(), caseMapTable.end(), compare);
}

std::string To_UTF8(const std::u32string& s)
{
  // TODO: Need destructor

  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
  return conv.to_bytes(s);
}

static void dump_second_level_table(const std::string label,
                                    const char32_t tableIndex,
                                    std::vector<CaseMap> secondLevelTable)
{

  std::cout << std::hex << std::noshowbase // manually show the 0x prefix
            << std::internal // fill between the prefix and the number
            << std::setfill('0'); // fill with 0s

  // Table declaration
  // static const CaseMap T_CASEMAP_0x0000[] =

  std::cout << "static const CaseMap T_" << label << "_0x" << std::setw(4) << tableIndex
            << "[] = " << std::endl;
  std::cout << "{" << std::endl;

  if (secondLevelTable.size() == 0)
    return;

  CaseMap length = CaseMap(secondLevelTable.size());

  int items_on_row = 0;
  for (int i = 0; i < secondLevelTable.size(); i++)
  {
    if (i > 0)
    {
      std::cout << ",  ";
    }
    if (items_on_row > 0)
    {
      std::cout << std::endl;
      items_on_row = 0;
    }
    if (items_on_row == 0)
      std::cout << "  "; // indent

    CaseMap currentElement = secondLevelTable[i];
    std::cout << currentElement.print();

    items_on_row++;
  }
  if (items_on_row != 0)
    std::cout << std::endl;

  std::cout << "};" << std::endl << std::endl;

  // static const std::vector<CaseMap> CASEMAP_0x0000(T_CASEMAP_0x0000, T_CASEMAP_0x0000 + (sizeof(T_CASEMAP_0x0000) / sizeof(CaseMap)));

  std::stringstream ss;

  ss << std::hex << std::noshowbase // manually show the 0x prefix
     << std::internal // fill between the prefix and the number
     << std::setfill('0'); // fill with 0s

  ss << label << "_0x" << std::setw(4) << tableIndex;
  std::string vectorName = ss.str();
  ss.str("");
  ss << "T_" << vectorName;
  std::string tableName = ss.str();
  std::cout << "static const std::vector<CaseMap> " << vectorName << "(" << tableName << ", "
            << tableName << " + (sizeof(" << tableName << ") / sizeof(CaseMap)));" << std::endl
            << std::endl;
}

static void dump_first_level_table(std::string label,
                                   int table_length,
                                   const uint16_t firstLevelTable[])
{
  std::cout << "static const std::vector<CaseMap> EMPTY_CASEMAP_ = {};" << std::endl << std::endl;

  std::cout << std::noshowbase // manually show the 0x prefix
            << std::internal // fill between the prefix and the number
            << std::setfill('0'); // fill with 0s

  std::cout << "static const std::vector<CaseMap> " << label << "_INDEX [] =" << std::endl;
  std::cout << "{";

  int items_on_row = 8;
  bool firstComma = true;
  for (int i = 0; i < table_length; i++)
  {
    if (not firstComma)
    {
      std::cout << ",  ";
    }
    if (items_on_row > 4)
    {
      std::cout << std::endl << "  ";
      items_on_row = 0;
    }
    if ((i != 0) and (firstLevelTable[i] == 0))
    {
      std::cout << "EMPTY_CASEMAP_";
    }
    else
    {
      std::cout << label << "_0x" << std::setw(4) << std::hex << (i);
    }
    items_on_row++;
    firstComma = false;
  }
  std::cout << std::endl << "};" << std::endl << std::endl;
}

static void doTable(std::string label, const std::vector<CaseMap> mappingTable)
{
  char32_t lastCodepoint = mappingTable.back().codepoint;
  char32_t previousCodepoint = 0x0;
  char32_t start;
  char32_t end;
  char32_t previous_inputCodepoint_high_bytes = 0xFFFFFFFF;
  CaseMap DUMMY_ENTRY = CaseMap();
  std::vector<CaseMap> secondLevelTable;
  int second_level_table_index = 0;
  int endOfSecondTable = 0;
  int first_level_table_index = 0;
  uint16_t firstLevelTable[0x200] = {}; // Initializes elements to 0

  for (CaseMap entry : mappingTable)
  {
    char32_t inputCodepoint = entry.codepoint;
    char32_t inputCodepoint_high_bytes = inputCodepoint >> 8;
    if (inputCodepoint_high_bytes != previous_inputCodepoint_high_bytes)
    {
      first_level_table_index = previous_inputCodepoint_high_bytes;

      // Don't include tables that have no CaseMap entries

      if (secondLevelTable.size() > 0)
      {
        // Truncate secondLevelTable after last added entry so that extra DUMMY
        // entries not written.

        int idx = previousCodepoint & 0xFF;
        if (idx + 1 < secondLevelTable.size())
        {
          auto unusedSpaceIter = next(secondLevelTable.begin() + idx);
          secondLevelTable.erase(unusedSpaceIter, secondLevelTable.end());
        }

        dump_second_level_table(label, previous_inputCodepoint_high_bytes, secondLevelTable);
        secondLevelTable.clear();
      }
      previous_inputCodepoint_high_bytes = inputCodepoint_high_bytes;
    }
    first_level_table_index = inputCodepoint_high_bytes;
    firstLevelTable[first_level_table_index] = inputCodepoint_high_bytes;
    if (secondLevelTable.size() == 0)
      secondLevelTable.assign(0x100, DUMMY_ENTRY);

    secondLevelTable[entry.codepoint & 0xFF] = entry;
    previousCodepoint = entry.codepoint;
  }
  if (secondLevelTable.size() > 0)
  {
    // Don't include tables that have no CaseMap entries

    // Truncate any DUMMY entries

    int idx = previousCodepoint & 0xFF;
    if (idx + 1 < secondLevelTable.size())
    {
      auto unusedSpaceIter = next(secondLevelTable.begin() + idx);
      secondLevelTable.erase(unusedSpaceIter, secondLevelTable.end());
    }

    dump_second_level_table(label, previous_inputCodepoint_high_bytes, secondLevelTable);
    secondLevelTable.clear();
  }
  dump_first_level_table(label, first_level_table_index + 1, firstLevelTable);
}

int main()
{
  loadData();

  doTable("CASEMAP", caseMapTable);
}
