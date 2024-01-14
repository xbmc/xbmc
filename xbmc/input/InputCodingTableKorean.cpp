/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputCodingTableKorean.h"

#include "utils/CharsetConverter.h"

#include <stdlib.h>

CInputCodingTableKorean::CInputCodingTableKorean() = default;

std::vector<std::wstring> CInputCodingTableKorean::GetResponse(int)
{
  return m_words;
}

bool CInputCodingTableKorean::GetWordListPage(const std::string& strCode, bool isFirstPage)
{
  return false;
}

void CInputCodingTableKorean::SetTextPrev(const std::string& strTextPrev)
{
  m_strTextPrev = strTextPrev;
}

int CInputCodingTableKorean::MergeCode(int choseong, int jungseong, int jongseong)
{
  return (unsigned short)0xAC00 + choseong * 21 * 28 + jungseong * 28 + jongseong + 1;
}

// Reference
// https://en.wikipedia.org/wiki/Hangul
// http://www.theyt.net/wiki/%ED%95%9C%EC%98%81%ED%83%80%EB%B3%80%ED%99%98%EA%B8%B0

std::wstring CInputCodingTableKorean::InputToKorean(const std::wstring& input)
{
  std::wstring dicEnglish = // L"rRseEfaqQtTdwWczxvgkoiOjpuPhynbml";
      {0x72, 0x52, 0x73, 0x65, 0x45, 0x66, 0x61, 0x71, 0x51, 0x74, 0x54,
       0x64, 0x77, 0x57, 0x63, 0x7A, 0x78, 0x76, 0x67, 0x6B, 0x6F, 0x69,
       0x4F, 0x6A, 0x70, 0x75, 0x50, 0x68, 0x79, 0x6E, 0x62, 0x6D, 0x6C};
  std::wstring dicKorean = // L"ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎㅏㅐㅑㅒㅓㅔㅕㅖㅗㅛㅜㅠㅡㅣ";
      {0x3131, 0x3132, 0x3134, 0x3137, 0x3138, 0x3139, 0x3141, 0x3142, 0x3143, 0x3145, 0x3146,
       0x3147, 0x3148, 0x3149, 0x314a, 0x314b, 0x314c, 0x314d, 0x314e, 0x314f, 0x3150, 0x3151,
       0x3152, 0x3153, 0x3154, 0x3155, 0x3156, 0x3157, 0x315b, 0x315c, 0x3160, 0x3161, 0x3163};
  std::wstring dicChoseong = // L"ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ";
      {0x3131, 0x3132, 0x3134, 0x3137, 0x3138, 0x3139, 0x3141, 0x3142, 0x3143, 0x3145,
       0x3146, 0x3147, 0x3148, 0x3149, 0x314a, 0x314b, 0x314c, 0x314d, 0x314e};
  std::wstring dicJungseong = // L"ㅏㅐㅑㅒㅓㅔㅕㅖㅗㅘㅙㅚㅛㅜㅝㅞㅟㅠㅡㅢㅣ";
      {0x314f, 0x3150, 0x3151, 0x3152, 0x3153, 0x3154, 0x3155, 0x3156, 0x3157, 0x3158, 0x3159,
       0x315a, 0x315b, 0x315c, 0x315d, 0x315e, 0x315f, 0x3160, 0x3161, 0x3162, 0x3163};
  std::wstring dicJongseong = // L"ㄱㄲㄳㄴㄵㄶㄷㄹㄺㄻㄼㄽㄾㄿㅀㅁㅂㅄㅅㅆㅇㅈㅊㅋㅌㅍㅎ";
      {0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136, 0x3137, 0x3139, 0x313a,
       0x313b, 0x313c, 0x313d, 0x313e, 0x313f, 0x3140, 0x3141, 0x3142, 0x3144,
       0x3145, 0x3146, 0x3147, 0x3148, 0x314a, 0x314b, 0x314c, 0x314d, 0x314e};

  std::wstring korean;

  if (input.empty())
    return korean;

  int choseong = -1, jungseong = -1, jongseong = -1;

  for (unsigned int i = 0; i < input.size(); i++)
  {
    wchar_t ch = input.at(i);
    int key = dicKorean.find(ch);

    // H/W Keyboard input with English will be changed to Korean
    // because H/W input in Korean is not supported.
    if (key == -1)
      key = dicEnglish.find(ch);

    if (key == -1) // If not Korean and English
    {
      // If there is remained Korean, merge code into character
      if (choseong != -1) // There is choseong
      {
        if (jungseong != -1) // choseong+jungseong+(jongseong)
          korean += MergeCode(choseong, jungseong, jongseong);
        else // Only choseong
          korean += dicChoseong.at(choseong);
      }
      else
      {
        if (jungseong != -1) // Jungseong
          korean += dicJungseong.at(jungseong);

        if (jongseong != -1) // Jongseong
          korean += dicJongseong.at(jongseong);
      }
      choseong = -1;
      jungseong = -1;
      jongseong = -1;
      korean += ch;
    }
    else if (key < 19) // If key is consonant, key could be choseong or jungseong
    {
      if (jungseong != -1)
      {
        // Jungseong without choseong cannot have jongseong.
        // So inputted key is jungseong character, new character is begun.
        if (choseong == -1)
        {
          korean += dicJungseong.at(jungseong);
          jungseong = -1;
          choseong = key;
        }
        else // Jungseong with choseong can have jongseong.
        {
          // Chongseong can have two consonant. So this is first consonant of chongseong.
          if (jongseong == -1)
          {
            jongseong = dicJongseong.find(dicKorean.at(key));
            if (jongseong == -1) // This consonant cannot be jongseong. ex) "ㄸ", "ㅃ", "ㅉ"
            {
              korean += MergeCode(choseong, jungseong, jongseong);
              choseong = dicChoseong.find(dicKorean.at(key));
              jungseong = -1;
            }
          }
          else if (jongseong == 0 && key == 9) // "ㄳ"
            jongseong = 2;
          else if (jongseong == 3 && key == 12) // "ㄵ"
            jongseong = 4;
          else if (jongseong == 3 && key == 18) // "ㄶ"
            jongseong = 5;
          else if (jongseong == 7 && key == 0) // "ㄺ"
            jongseong = 8;
          else if (jongseong == 7 && key == 6) // "ㄻ"
            jongseong = 9;
          else if (jongseong == 7 && key == 7) // "ㄼ"
            jongseong = 10;
          else if (jongseong == 7 && key == 9) // "ㄽ"
            jongseong = 11;
          else if (jongseong == 7 && key == 16) // "ㄾ"
            jongseong = 12;
          else if (jongseong == 7 && key == 17) // "ㄿ"
            jongseong = 13;
          else if (jongseong == 7 && key == 18) // "ㅀ"
            jongseong = 14;
          else if (jongseong == 16 && key == 9) // "ㅄ"
            jongseong = 17;
          else // Jongseong is completed. So new consonant is choseong.
          {
            korean += MergeCode(choseong, jungseong, jongseong);
            choseong = dicChoseong.find(dicKorean.at(key));
            jungseong = -1;
            jongseong = -1;
          }
        }
      }
      else // If there is no jungseong, new consonant can be choseong or second part of double
          // consonant.
      {
        // New consonant is choseong. Also it could be first part of double consonant.
        if (choseong == -1)
        {
          // If choseong is already completed, new consonant is another choseong.
          // So previous character has only jongseong.
          if (jongseong != -1)
          {
            korean += dicJongseong.at(jongseong);
            jongseong = -1;
          }
          choseong = dicChoseong.find(dicKorean.at(key));
        }
        // Find double consonant of chongseong
        else if (choseong == 0 && key == 9) // "ㄳ"
        {
          choseong = -1;
          jongseong = 2;
        }
        else if (choseong == 2 && key == 12) // "ㄵ"
        {
          choseong = -1;
          jongseong = 4;
        }
        else if (choseong == 2 && key == 18) // "ㄶ"
        {
          choseong = -1;
          jongseong = 5;
        }
        else if (choseong == 5 && key == 0) // "ㄺ"
        {
          choseong = -1;
          jongseong = 8;
        }
        else if (choseong == 5 && key == 6) // "ㄻ"
        {
          choseong = -1;
          jongseong = 9;
        }
        else if (choseong == 5 && key == 7) // "ㄼ"
        {
          choseong = -1;
          jongseong = 10;
        }
        else if (choseong == 5 && key == 9) // "ㄽ"
        {
          choseong = -1;
          jongseong = 11;
        }
        else if (choseong == 5 && key == 16) // "ㄾ"
        {
          choseong = -1;
          jongseong = 12;
        }
        else if (choseong == 5 && key == 17) // "ㄿ"
        {
          choseong = -1;
          jongseong = 13;
        }
        else if (choseong == 5 && key == 18) // "ㅀ"
        {
          choseong = -1;
          jongseong = 14;
        }
        else if (choseong == 7 && key == 9) // "ㅄ"
        {
          choseong = -1;
          jongseong = 17;
        }
        else // In this case, previous character has only choseong. And new consonant is choseong.
        {
          korean += dicChoseong.at(choseong);
          choseong = dicChoseong.find(dicKorean.at(key));
        }
      }
    }
    else // If key is vowel, key is jungseong.
    {
      // If previous character has jongseong and this key is jungseong,
      // actually latest vowel is not jongseong. It's choseong of new character.
      if (jongseong != -1)
      {
        // If jongseong of previous character is double consonant, we will separate it to two vowel
        // again. First part of double consonant is jongseong of previous character. Second part of
        // double consonant is choseong of current character.
        int newCho;
        if (jongseong == 2) // "ㄱ, ㅅ"
        {
          jongseong = 0;
          newCho = 9;
        }
        else if (jongseong == 4) // "ㄴ, ㅈ"
        {
          jongseong = 3;
          newCho = 12;
        }
        else if (jongseong == 5) // "ㄴ, ㅎ"
        {
          jongseong = 3;
          newCho = 18;
        }
        else if (jongseong == 8) // "ㄹ, ㄱ"
        {
          jongseong = 7;
          newCho = 0;
        }
        else if (jongseong == 9) // "ㄹ, ㅁ"
        {
          jongseong = 7;
          newCho = 6;
        }
        else if (jongseong == 10) // "ㄹ, ㅂ"
        {
          jongseong = 7;
          newCho = 7;
        }
        else if (jongseong == 11) // "ㄹ, ㅅ"
        {
          jongseong = 7;
          newCho = 9;
        }
        else if (jongseong == 12) // "ㄹ, ㅌ"
        {
          jongseong = 7;
          newCho = 16;
        }
        else if (jongseong == 13) // "ㄹ, ㅍ"
        {
          jongseong = 7;
          newCho = 17;
        }
        else if (jongseong == 14) // "ㄹ, ㅎ"
        {
          jongseong = 7;
          newCho = 18;
        }
        else if (jongseong == 17) // "ㅂ, ㅅ"
        {
          jongseong = 16;
          newCho = 9;
        }
        else
        {
          // If jongseong is single consonant, previous character has no chongseong.
          // It's choseong of current character.
          newCho = dicChoseong.find(dicJongseong.at(jongseong));
          jongseong = -1;
        }
        if (choseong != -1) // If previous character has choseong and jungseong.
          korean += MergeCode(choseong, jungseong, jongseong);
        else // If previous character has Jongseong only.
          korean += dicJongseong.at(jongseong);

        choseong = newCho;
        jungseong = -1;
        jongseong = -1;
      }
      if (jungseong == -1) // If this key is first vowel, it's first part of jungseong.
      {
        jungseong = dicJungseong.find(dicKorean.at(key));
      }
      // If there is jungseong already, jungseong is double vowel.
      else if (jungseong == 8 && key == 19) // "ㅘ"
        jungseong = 9;
      else if (jungseong == 8 && key == 20) // "ㅙ"
        jungseong = 10;
      else if (jungseong == 8 && key == 32) // "ㅚ"
        jungseong = 11;
      else if (jungseong == 13 && key == 23) // "ㅝ"
        jungseong = 14;
      else if (jungseong == 13 && key == 24) // "ㅞ"
        jungseong = 15;
      else if (jungseong == 13 && key == 32) // "ㅟ"
        jungseong = 16;
      else if (jungseong == 18 && key == 32) // "ㅢ"
        jungseong = 19;
      else // If two vowel cannot be double vowel.
      {
        // Previous character is completed.
        // Current character is begin with jungseong.
        if (choseong != -1)
        {
          korean += MergeCode(choseong, jungseong, jongseong);
          choseong = -1;
        }
        else // Previous character has jungseon only.
          korean += dicJungseong.at(jungseong);
        jungseong = -1;
        korean += dicKorean.at(key);
      }
    }
  }

  // Process last character
  if (choseong != -1)
  {
    if (jungseong != -1) // Current character has choseong and jungseong.
      korean += MergeCode(choseong, jungseong, jongseong);
    else // Current character has choseong only.
      korean += dicChoseong.at(choseong);
  }
  else
  {
    if (jungseong != -1) // Current character has jungseong only
      korean += dicJungseong.at(jungseong);
    else if (jongseong != -1) // Current character has jongseong only
      korean += dicJongseong.at(jongseong);
  }

  return korean;
}

std::string CInputCodingTableKorean::ConvertString(const std::string& strCode)
{
  std::wstring input;
  std::string result;
  g_charsetConverter.utf8ToW(strCode, input);
  InputToKorean(input);
  g_charsetConverter.wToUTF8(InputToKorean(input), result);
  return m_strTextPrev + result;
}
