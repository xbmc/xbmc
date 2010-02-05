#pragma once
#include <map>
#include <vector>
#include <string>
#include <stdint.h>

class CVariant
{
public:
  enum VariantType
  {
    VariantTypeInteger,
    VariantTypeUnsignedInteger,
    VariantTypeBoolean,
    VariantTypeString,
    VariantTypeArray,
    VariantTypeObject,
    VariantTypeNull
  };

  CVariant(VariantType type = VariantTypeNull);
  CVariant(int integer);
  CVariant(int64_t integer);
  CVariant(unsigned int unsignedinteger);
  CVariant(uint64_t unsignedinteger);
  CVariant(bool boolean);
  CVariant(const char *str);
  CVariant(const CVariant &variant);

  ~CVariant();

  bool isInteger() const;
  bool isUnsignedInteger() const;
  bool isBoolean() const;
  bool isString() const;
  bool isArray() const;
  bool isObject() const;
  bool isNull() const;

  int64_t asInteger() const;
  uint64_t asUnsignedInteger() const;
  bool asBoolean() const;
  const char *asString() const;

  CVariant &operator[](std::string key);
  CVariant &operator[](unsigned int position);

  CVariant &operator=(const CVariant &rhs);

  void push_back(CVariant variant);

  unsigned int size() const;
  bool empty() const;
  void clear();
  void erase(std::string key);
  void erase(unsigned int position);

  void debug();
  void internaldebug();
private:
  VariantType m_type;

  typedef std::vector<CVariant> VariantArray;
  typedef std::map<std::string, CVariant> VariantMap;

  union
  {
    int64_t integer;
    uint64_t unsignedinteger;
    bool boolean;
    std::string *string;
    VariantArray *array;
    VariantMap *map;
  } m_data;
};
