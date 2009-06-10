#pragma once
/*
*      Copyright (C) 2005-2009 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

typedef struct
{
public:
  int           type;
  char*         name;
  int           current;
  char**        entry;
  unsigned int  entry_elements;
} DllSettingStruct;

////////////////////////////////////////////////////////////////////
// The DllSetting class for GUI settings for addons.
////////////////////////////////////////////////////////////////////
class DllSetting
{
public:
  enum SETTING_TYPE {
    NONE=0, 
    CHECK, 
    SPIN,

  };

  DllSetting(SETTING_TYPE t, const char *label)
  {
    name = NULL;
    if (label)
    {
      name = new char[strlen(label)+1];
      strcpy(name, label);
    }
    current = 0;
    type = t;
  }

  DllSetting(const DllSetting &rhs) // copy constructor
  {
    name = NULL;
    if (rhs.name)
    {
      name = new char[strlen(rhs.name)+1];
      strcpy(name, rhs.name);
    }
    current = rhs.current;
    type = rhs.type;
    for (unsigned int i = 0; i < rhs.entry.size(); i++)
    {
      char *lab = new char[strlen(rhs.entry[i]) + 1];
      strcpy(lab, rhs.entry[i]);
      entry.push_back(lab);
    }
  }

  ~DllSetting()
  {
    if (name)
      delete[] name;
    for (unsigned int i=0; i < entry.size(); i++)
      delete[] entry[i];
  }

  void AddEntry(const char *label)
  {
    if (!label || type != SPIN) return;
    char *lab = new char[strlen(label) + 1];
    strcpy(lab, label);
    entry.push_back(lab);
  }

  // data members
  SETTING_TYPE type;
  char*        name;
  int          current;
  std::vector<const char *> entry;
};

class DllUtils
{
public:

  static unsigned int VecToStruct(std::vector<DllSetting> &vecSet, DllSettingStruct*** sSet) 
  {
    *sSet = NULL;
    if(vecSet.size() == 0)
      return 0;

    unsigned int uiElements=0;

    *sSet = (DllSettingStruct**)malloc(vecSet.size()*sizeof(DllSettingStruct*));
    for(unsigned int i=0;i<vecSet.size();i++)
    {
      (*sSet)[i] = NULL;
      (*sSet)[i] = (DllSettingStruct*)malloc(sizeof(DllSettingStruct));
      (*sSet)[i]->name = NULL;
      uiElements++;

      if (vecSet[i].name)
      {
        (*sSet)[i]->name = (char*)malloc(strlen(vecSet[i].name)*sizeof(char*)+1);
        strcpy((*sSet)[i]->name, vecSet[i].name);
        (*sSet)[i]->type = vecSet[i].type;
        (*sSet)[i]->current = vecSet[i].current;
        if(vecSet[i].type == DllSetting::SPIN && vecSet[i].entry.size() > 0)
        {
          (*sSet)[i]->entry = (char**)malloc(vecSet[i].entry.size()*sizeof(char**));
          (*sSet)[i]->entry_elements = 0;
          for(unsigned int j=0;j<vecSet[i].entry.size();j++)
          {
            (*sSet)[i]->entry[j] = NULL;
            if(strlen(vecSet[i].entry[j]) > 0)
            {
              (*sSet)[i]->entry[j] = (char*)malloc(strlen(vecSet[i].entry[j])*sizeof(char*)+1);
              strcpy((*sSet)[i]->entry[j], vecSet[i].entry[j]);
              (*sSet)[i]->entry_elements++;
            }
          }
        }
      }
    }
    return uiElements;
  }

  static void StructToVec(unsigned int iElements, DllSettingStruct*** sSet, std::vector<DllSetting> *vecSet) 
  {
    if(iElements == 0)
      return;

    vecSet->clear();
    for(unsigned int i=0;i<iElements;i++)
    {
      DllSetting vSet((DllSetting::SETTING_TYPE)(*sSet)[i]->type, (*sSet)[i]->name);
      if((*sSet)[i]->type == DllSetting::SPIN)
      {
        for(unsigned int j=0;j<(*sSet)[i]->entry_elements;j++)
        {
          vSet.AddEntry((*sSet)[i]->entry[j]);
        }
      }
      vSet.current = (*sSet)[i]->current;
      vecSet->push_back(vSet);
    }
  }

  static void FreeStruct(unsigned int iElements, DllSettingStruct*** sSet)
  {
    if(iElements == 0)
      return;

    for(unsigned int i=0;i<iElements;i++)
    {
      if((*sSet)[i]->type == DllSetting::SPIN)
      {
        for(unsigned int j=0;j<(*sSet)[i]->entry_elements;j++)
        {
          if((*sSet)[i]->entry[j])
            free((*sSet)[i]->entry[j]);
        }
        if((*sSet)[i]->entry)
          free((*sSet)[i]->entry);
      }
      if((*sSet)[i]->name)
        free((*sSet)[i]->name);
      if((*sSet)[i])
        free((*sSet)[i]);
    }
    if(*sSet)
      free(*sSet);
  }
};
