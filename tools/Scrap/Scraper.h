class Scraper
{
  protected:
    bool Load();
    bool s_load;
    bool WriteResult(const CStdString&);
    bool PrepareParsing(const CStdString&);
    CStdString PrepareSearchString(CStdString&);
    int CustomFunctions(const CStdString&);
    string readFile(const CStdString&);

    CStdString m_result;
    CScraperParser m_parser;
  public:
    Scraper(const string);
    string s_xml;
    bool SetBuffer(int, string);
    void PrintBuffer(int);
    int Parse(const CStdString&);
};