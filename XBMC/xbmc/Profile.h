#pragma once

class CProfile
{
public:
  CProfile(void);
  ~CProfile(void);

  const CStdString& getName() const {return _name;}
  const CStdString& getFileName() const {return _fileName;}

  void setName(const CStdString& name) {_name = name;}
  void setFileName(const CStdString& filename) {_fileName = filename;}
private:
  CStdString _fileName;
  CStdString _name;
};
