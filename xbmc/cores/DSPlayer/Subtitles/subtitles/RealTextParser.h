#pragma once

#include <sstream>
using std::wostream;
using std::wostringstream;
using std::endl;

#include <string>
using std::wstring;

#include <map>
using std::map;
using std::pair;

#include <vector>
using std::vector;

#include <list>
using std::list;

#include <cwctype>
using std::towlower;

class CRealTextParser
{
public:
	CRealTextParser();
	virtual ~CRealTextParser(void);

	struct Tag
	{
		Tag(): m_bOpen(false), m_bClose(false), m_bComment(false), m_bText(false) {}

		wstring m_szName;

		bool m_bOpen;
		bool m_bClose;

		bool m_bComment;
		bool m_bText;

		map<wstring, wstring> m_mapAttributes;
	};

	struct Subtitles
	{
		Subtitles(): m_WindowTag(), m_FontTag(), m_bCenter(false) {}

		Tag m_WindowTag;
		Tag m_FontTag;

		bool m_bCenter;

		map<pair<int, int>, wstring> m_mapLines;
	};

	bool ParseRealText(wstring p_szFile);

	const Subtitles& GetParsedSubtitles();

	bool OutputSRT(wostream& p_rOutput);

private:
	bool ExtractTag(wstring& p_rszLine, Tag& p_rTag);
	bool ExtractTextTag(wstring& p_rszLine, Tag& p_rTag);
	bool ExtractString(wstring& p_rszLine, wstring& p_rszString);
	bool SkipSpaces(wstring& p_rszLine, unsigned int& p_riPos);
	bool GetString(wstring& p_rszLine, unsigned int& p_riPos, wstring& p_rszString, const wstring& p_crszEndChars);
	bool GetAttributes(wstring& p_rszLine, unsigned int& p_riPos, map<wstring, wstring>& p_rmapAttributes);

	int GetTimecode(const wstring& p_crszTimecode);
	wstring FormatTimecode(int iTimecode,
		int iMillisecondPrecision = 3,
		bool p_bPadZeroes = true,
		const wstring& p_crszSeparator = L":",
		const wstring& p_crszMillisecondSeparator = L".");

	wstring StringToLower(const wstring& p_crszString);

	wstring RenderTags(const list<Tag>& p_crlTags);

	void PopTag(list<Tag>& p_rlistTags, const wstring& p_crszTagName);

	// Filter out for example multiple font tags opened previously (font tags are not always terminated properly in realtext and can build up)
	void FilterReduntantTags(list<Tag>& p_rlistTags);

	
	Subtitles m_RealText;

	bool m_bIgnoreFont;
	bool m_bIgnoreFontSize;
	bool m_bIgnoreFontColor;
	bool m_bIgnoreFontWeight;
	bool m_bIgnoreFontFace;

	int m_iMinFontSize;
	int m_iMaxFontSize;

	int m_iDefaultSubtitleDurationInMillisecs;

	bool m_bTryToIgnoreErrors;
};
