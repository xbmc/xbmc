/*
 *  Copyright © 2010-2012 Team XBMC
 *  http://xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _XBMC_TYPES_H_
#define _XBMC_TYPES_H_

// Having to duplicate some of the XBMC types here due to multiply externed
// vars if xbmc_addon_dll.h is included in the lib

#include <vector>
#include <string.h>
#include <stdlib.h>


typedef struct
{
	int           type;
	char*         id;
	char*         label;
	int           current;
	char**        entry;
	unsigned int  entry_elements;
} StructSetting;

class DllSetting
{
public:
	enum SETTING_TYPE { NONE=0, CHECK, SPIN };

	DllSetting(SETTING_TYPE t, const char *n, const char *l)
	{
		id = NULL;
		label = NULL;
		if (n)
		{
			id = new char[strlen(n)+1];
			strcpy(id, n);
		}
		if (l)
		{
			label = new char[strlen(l)+1];
			strcpy(label, l);
		}
		current = 0;
		type = t;
	}

	DllSetting(const DllSetting &rhs) // copy constructor
	{
		id = NULL;
		label = NULL;
		if (rhs.id)
		{
			id = new char[strlen(rhs.id)+1];
			strcpy(id, rhs.id);
		}
		if (rhs.label)
		{
			label = new char[strlen(rhs.label)+1];
			strcpy(label, rhs.label);
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
		delete[] id;
		delete[] label;
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
	char*        id;
	char*        label;
	int          current;
	std::vector<const char *> entry;
};

class DllUtils
{
public:

	static unsigned int VecToStruct(std::vector<DllSetting> &vecSet, StructSetting*** sSet) 
	{
		*sSet = NULL;
		if(vecSet.size() == 0)
			return 0;

		unsigned int uiElements=0;

		*sSet = (StructSetting**)malloc(vecSet.size()*sizeof(StructSetting*));
		for(unsigned int i=0;i<vecSet.size();i++)
		{
			(*sSet)[i] = NULL;
			(*sSet)[i] = (StructSetting*)malloc(sizeof(StructSetting));
			(*sSet)[i]->id = NULL;
			(*sSet)[i]->label = NULL;
			uiElements++;

			if (vecSet[i].id && vecSet[i].label)
			{
				(*sSet)[i]->id = strdup(vecSet[i].id);
				(*sSet)[i]->label = strdup(vecSet[i].label);
				(*sSet)[i]->type = vecSet[i].type;
				(*sSet)[i]->current = vecSet[i].current;
				(*sSet)[i]->entry_elements = 0;
				(*sSet)[i]->entry = NULL;
				if(vecSet[i].type == DllSetting::SPIN && vecSet[i].entry.size() > 0)
				{
					(*sSet)[i]->entry = (char**)malloc(vecSet[i].entry.size()*sizeof(char**));
					for(unsigned int j=0;j<vecSet[i].entry.size();j++)
					{
						if(strlen(vecSet[i].entry[j]) > 0)
						{
							(*sSet)[i]->entry[j] = strdup(vecSet[i].entry[j]);
							(*sSet)[i]->entry_elements++;
						}
					}
				}
			}
		}
		return uiElements;
	}

	static void StructToVec(unsigned int iElements, StructSetting*** sSet, std::vector<DllSetting> *vecSet) 
	{
		if(iElements == 0)
			return;

		vecSet->clear();
		for(unsigned int i=0;i<iElements;i++)
		{
			DllSetting vSet((DllSetting::SETTING_TYPE)(*sSet)[i]->type, (*sSet)[i]->id, (*sSet)[i]->label);
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

	static void FreeStruct(unsigned int iElements, StructSetting*** sSet)
	{
		if(iElements == 0)
			return;

		for(unsigned int i=0;i<iElements;i++)
		{
			if((*sSet)[i]->type == DllSetting::SPIN)
			{
				for(unsigned int j=0;j<(*sSet)[i]->entry_elements;j++)
				{
					free((*sSet)[i]->entry[j]);
				}
				free((*sSet)[i]->entry);
			}
			free((*sSet)[i]->id);
			free((*sSet)[i]->label);
			free((*sSet)[i]);
		}
		free(*sSet);
	}
};
#endif
