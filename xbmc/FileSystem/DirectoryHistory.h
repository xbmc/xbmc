#pragma once

class CDirectoryHistory
{
public:
	class CHistoryItem
	{
	public:
		CHistoryItem(){};
		virtual ~CHistoryItem(){};
		CStdString m_strItem;
		CStdString m_strDirectory;
	};
	CDirectoryHistory();
	virtual ~CDirectoryHistory();

	void							Set(const CStdString& strSelectedItem, const CStdString& strDirectory);
	const CStdString& Get(const CStdString& strDirectory) const;
  void              Remove(const CStdString& strDirectory);
private:
	vector<CHistoryItem> m_vecHistory;
	CStdString					 m_strNull;
};
