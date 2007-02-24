void get_url(const CStdString&,CScraperUrl&);

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

    CStdString s_result;
    CScraperParser s_parser;
  public:
    Scraper(const string);
    string s_xml;
    bool SetBuffer(int, string);
    void PrintBuffer(int);
    int Parse(const CStdString&);
};