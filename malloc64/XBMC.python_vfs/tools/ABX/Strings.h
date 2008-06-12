#ifndef STRINGS_H
#define STRINGS_H

//=============================
//		strings.h
//=============================

#include "stdafx.h"

class Strings{

	public:

		string ToUpper(string str);

		string ToLower(string str);

		void RemoveExt(string& name);

		string GetExt(string name);

		string GetFileName(string name);

		bool MaxPathClip(string& path);

		string FormatTime(const float& time);

		bool IllegalCharRemover(string& txt);

		void StrClean(string& str);

		string stringReplacer(string text, const int& x,
												const int& maxNum);

		string AutoNumBasic (const int& num, const int& maxNum);

		void AutoNum (string prefix, string& name, const int& num,
								string extension);

		void AutoNumList (const bool& tidy, string& name,
									const int& num, const int& maxNum);

		void AutoNumScx (const bool& tidy, string& name,
									const int& num, const int& maxNum);

};// End strings

extern Strings str; // Global instance

#endif
