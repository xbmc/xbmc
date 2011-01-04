#include "scriptbuilder.h"
#include <vector>
using namespace std;

#include <stdio.h>
#if defined(_MSC_VER) && !defined(_WIN32_WCE)
#include <direct.h>
#endif
#ifdef _WIN32_WCE
#include <windows.h> // For GetModuleFileName
#endif


BEGIN_AS_NAMESPACE

// Helper functions
static const char *GetCurrentDir(char *buf, size_t size);

CScriptBuilder::CScriptBuilder()
{
	engine = 0;
	module = 0;

	includeCallback = 0;
	callbackParam   = 0;
}

void CScriptBuilder::SetIncludeCallback(INCLUDECALLBACK_t callback, void *userParam)
{
	includeCallback = callback;
	callbackParam   = userParam;
}

int CScriptBuilder::StartNewModule(asIScriptEngine *engine, const char *moduleName)
{
	this->engine = engine;
	module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
	if( module == 0 )
		return -1;

	ClearAll();

	return 0;
}

int CScriptBuilder::AddSectionFromFile(const char *filename)
{
	if( IncludeIfNotAlreadyIncluded(filename) )
	{
		int r = LoadScriptSection(filename);
		if( r < 0 )
			return r;
	}

	return 0;
}

int CScriptBuilder::AddSectionFromMemory(const char *scriptCode, const char *sectionName)
{
	if( IncludeIfNotAlreadyIncluded(sectionName) )
	{
		int r = ProcessScriptSection(scriptCode, sectionName);
		if( r < 0 )
			return r;
	}

	return 0;
}

int CScriptBuilder::BuildModule()
{
	return Build();
}

void CScriptBuilder::DefineWord(const char *word)
{
	string sword = word;
	if( definedWords.find(sword) == definedWords.end() )
	{
		definedWords.insert(sword);
	}
}

void CScriptBuilder::ClearAll()
{
	includedScripts.clear();

#if AS_PROCESS_METADATA == 1	
	foundDeclarations.clear();
	typeMetadataMap.clear();
	funcMetadataMap.clear();
	varMetadataMap.clear();
#endif
}

bool CScriptBuilder::IncludeIfNotAlreadyIncluded(const char *filename)
{
	string scriptFile = filename;
	if( includedScripts.find(scriptFile) != includedScripts.end() )
	{
		// Already included
		return false;
	}

	// Add the file to the set of included sections
	includedScripts.insert(scriptFile);

	return true;
}

int CScriptBuilder::LoadScriptSection(const char *filename)
{
	// TODO: The file name stored in the set should be the fully resolved name because
	// it is possible to name the same file in multiple ways using relative paths.

	// Open the script file
	string scriptFile = filename;
#if _MSC_VER >= 1500
	FILE *f = 0;
	fopen_s(&f, scriptFile.c_str(), "rb");
#else
	FILE *f = fopen(scriptFile.c_str(), "rb");
#endif
	if( f == 0 )
	{
		// Write a message to the engine's message callback
		char buf[256];
		string msg = "Failed to open script file in path: '" + string(GetCurrentDir(buf, 256)) + "'";
		engine->WriteMessage(filename, 0, 0, asMSGTYPE_ERROR, msg.c_str());
		return -1;
	}
	
	// Determine size of the file
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	fseek(f, 0, SEEK_SET);

	// On Win32 it is possible to do the following instead
	// int len = _filelength(_fileno(f));

	// Read the entire file
	string code;
	code.resize(len);
	size_t c = fread(&code[0], len, 1, f);

	fclose(f);

	if( c == 0 ) 
	{
		// Write a message to the engine's message callback
		char buf[256];
		string msg = "Failed to load script file in path: '" + string(GetCurrentDir(buf, 256)) + "'";
		engine->WriteMessage(filename, 0, 0, asMSGTYPE_ERROR, msg.c_str());
		return -1;
	}

	return ProcessScriptSection(code.c_str(), filename);
}

int CScriptBuilder::ProcessScriptSection(const char *script, const char *sectionname)
{
	vector<string> includes;

	// Perform a superficial parsing of the script first to store the metadata
	modifiedScript = script;

	// First perform the checks for #if directives to exclude code that shouldn't be compiled
	int pos = 0;
	int nested = 0;
	while( pos < (int)modifiedScript.size() )
	{
		int len;
		asETokenClass t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
		if( t == asTC_UNKNOWN && modifiedScript[pos] == '#' )
		{
			int start = pos++;

			// Is this an #if directive?
			asETokenClass t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);

			string token;
			token.assign(&modifiedScript[pos], len);

			pos += len;

			if( token == "if" )
			{
				t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
				if( t == asTC_WHITESPACE )
				{
					pos += len;
					t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
				}

				if( t == asTC_IDENTIFIER )
				{
					string word;
					word.assign(&modifiedScript[pos], len);

					// Overwrite the #if directive with space characters to avoid compiler error
					pos += len;
					OverwriteCode(start, pos-start);

					// Has this identifier been defined by the application or not?
					if( definedWords.find(word) == definedWords.end() )
					{
						// Exclude all the code until and including the #endif
						pos = ExcludeCode(pos);
					}
					else
					{
						nested++;
					}
				}
			}
			else if( token == "endif" )
			{
				// Only remove the #endif if there was a matching #if
				if( nested > 0 )
				{
					OverwriteCode(start, pos-start);
					nested--;
				}
			}			
		}
		else
			pos += len;
	}

#if AS_PROCESS_METADATA == 1
	// Preallocate memory
	string metadata, declaration;
	metadata.reserve(500);
	declaration.reserve(100);
#endif

	// Then check for meta data and #include directives
	pos = 0;
	while( pos < (int)modifiedScript.size() )
	{
		int len;
		asETokenClass t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
		if( t == asTC_COMMENT || t == asTC_WHITESPACE )
		{
			pos += len;
			continue;
		}

#if AS_PROCESS_METADATA == 1
		// Is this the start of metadata?
		if( modifiedScript[pos] == '[' )
		{
			// Get the metadata string
			pos = ExtractMetadataString(pos, metadata);

			// Determine what this metadata is for
			int type;
			pos = ExtractDeclaration(pos, declaration, type);
			
			// Store away the declaration in a map for lookup after the build has completed
			if( type > 0 )
			{
				SMetadataDecl decl(metadata, declaration, type);
				foundDeclarations.push_back(decl);
			}
		}
		else 
#endif
		// Is this a preprocessor directive?
		if( modifiedScript[pos] == '#' )
		{
			int start = pos++;

			asETokenClass t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
			if( t == asTC_IDENTIFIER )
			{
				string token;
				token.assign(&modifiedScript[pos], len);
				if( token == "include" )
				{
					pos += len;
					t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
					if( t == asTC_WHITESPACE )
					{
						pos += len;
						t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
					}

					if( t == asTC_VALUE && len > 2 && modifiedScript[pos] == '"' )
					{
						// Get the include file
						string includefile;
						includefile.assign(&modifiedScript[pos+1], len-2);
						pos += len;

						// Store it for later processing
						includes.push_back(includefile);

						// Overwrite the include directive with space characters to avoid compiler error
						OverwriteCode(start, pos-start);
					}
				}
			}
		}
		// Don't search for metadata/includes within statement blocks or between tokens in statements
		else 
			pos = SkipStatement(pos);
	}

	// Build the actual script
	engine->SetEngineProperty(asEP_COPY_SCRIPT_SECTIONS, true);
	module->AddScriptSection(sectionname, modifiedScript.c_str(), modifiedScript.size());

	if( includes.size() > 0 )
	{
		// If the callback has been set, then call it for each included file
		if( includeCallback )
		{
			for( int n = 0; n < (int)includes.size(); n++ )
			{
				int r = includeCallback(includes[n].c_str(), sectionname, this, callbackParam);
				if( r < 0 )
					return r;
			}
		}
		else
		{
			// By default we try to load the included file from the relative directory of the current file

			// Determine the path of the current script so that we can resolve relative paths for includes
			string path = sectionname;
			size_t posOfSlash = path.find_last_of("/\\");
			if( posOfSlash != string::npos )
				path.resize(posOfSlash+1);
			else
				path = "";

			// Load the included scripts
			for( int n = 0; n < (int)includes.size(); n++ )
			{
				// If the include is a relative path, then prepend the path of the originating script
				if( includes[n].find_first_of("/\\") != 0 &&
					includes[n].find_first_of(":") == string::npos )
				{
					includes[n] = path + includes[n];
				}

				// Include the script section
				int r = AddSectionFromFile(includes[n].c_str());
				if( r < 0 )
					return r;
			}
		}
	}

	return 0;
}

int CScriptBuilder::Build()
{
	int r = module->Build();
	if( r < 0 )
		return r;

#if AS_PROCESS_METADATA == 1
	// After the script has been built, the metadata strings should be 
	// stored for later lookup by function id, type id, and variable index
	for( int n = 0; n < (int)foundDeclarations.size(); n++ )
	{
		SMetadataDecl *decl = &foundDeclarations[n];
		if( decl->type == 1 )
		{
			// Find the type id
			int typeId = module->GetTypeIdByDecl(decl->declaration.c_str());
			if( typeId >= 0 )
				typeMetadataMap.insert(map<int, string>::value_type(typeId, decl->metadata));
		}
		else if( decl->type == 2 )
		{
			// Find the function id
			int funcId = module->GetFunctionIdByDecl(decl->declaration.c_str());
			if( funcId >= 0 )
				funcMetadataMap.insert(map<int, string>::value_type(funcId, decl->metadata));
		}
		else if( decl->type == 3 )
		{
			// Find the global variable index
			int varIdx = module->GetGlobalVarIndexByDecl(decl->declaration.c_str());
			if( varIdx >= 0 )
				varMetadataMap.insert(map<int, string>::value_type(varIdx, decl->metadata));
		}
	}
#endif

	return 0;
}

int CScriptBuilder::SkipStatement(int pos)
{
	int len;

	// Skip until ; or { whichever comes first
	while( pos < (int)modifiedScript.length() && modifiedScript[pos] != ';' && modifiedScript[pos] != '{' )
	{
		engine->ParseToken(&modifiedScript[pos], 0, &len);
		pos += len;
	}

	// Skip entire statement block
	if( pos < (int)modifiedScript.length() && modifiedScript[pos] == '{' )
	{
		pos += 1;

		// Find the end of the statement block
		int level = 1;
		while( level > 0 && pos < (int)modifiedScript.size() )
		{
			asETokenClass t = engine->ParseToken(&modifiedScript[pos], 0, &len);
			if( t == asTC_KEYWORD )
			{
				if( modifiedScript[pos] == '{' )
					level++;
				else if( modifiedScript[pos] == '}' )
					level--;
			}

			pos += len;
		}
	}
	else
		pos += 1;

	return pos;
}

// Overwrite all code with blanks until the matching #endif
int CScriptBuilder::ExcludeCode(int pos)
{
	int len;
	int nested = 0;
	while( pos < (int)modifiedScript.size() )
	{
		asETokenClass t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
		if( modifiedScript[pos] == '#' )
		{
			modifiedScript[pos] = ' ';
			pos++;

			// Is it an #if or #endif directive?
			t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
			string token;
			token.assign(&modifiedScript[pos], len);
			OverwriteCode(pos, len);

			if( token == "if" )
			{
				nested++;
			}
			else if( token == "endif" )
			{
				if( nested-- == 0 )
				{
					pos += len;
					break;
				}
			}
		}
		else if( modifiedScript[pos] != '\n' )
		{
			OverwriteCode(pos, len);
		}
		pos += len;
	}

	return pos;
}

// Overwrite all characters except line breaks with blanks 
void CScriptBuilder::OverwriteCode(int start, int len)
{
	char *code = &modifiedScript[start];
	for( int n = 0; n < len; n++ )
	{
		if( *code != '\n' )
			*code = ' ';
		code++;
	}
}

#if AS_PROCESS_METADATA == 1
int CScriptBuilder::ExtractMetadataString(int pos, string &metadata)
{
	metadata = "";

	// Overwrite the metadata with space characters to allow compilation
	modifiedScript[pos] = ' ';

	// Skip opening brackets
	pos += 1;

	int level = 1;
	int len;
	while( level > 0 && pos < (int)modifiedScript.size() )
	{
		asETokenClass t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
		if( t == asTC_KEYWORD )
		{
			if( modifiedScript[pos] == '[' )
				level++;
			else if( modifiedScript[pos] == ']' )
				level--;
		}

		// Copy the metadata to our buffer
		if( level > 0 )
			metadata.append(&modifiedScript[pos], len);

		// Overwrite the metadata with space characters to allow compilation
		if( t != asTC_WHITESPACE )
			OverwriteCode(pos, len);

		pos += len;
	}

	return pos;
}

int CScriptBuilder::ExtractDeclaration(int pos, string &declaration, int &type)
{
	declaration = "";
	type = 0;

	int start = pos;

	std::string token;
	int len = 0;
	asETokenClass t = asTC_WHITESPACE;

	// Skip white spaces and comments
	do
	{
		pos += len;
		t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
	} while ( t == asTC_WHITESPACE || t == asTC_COMMENT );

	// We're expecting, either a class, interface, function, or variable declaration
	if( t == asTC_KEYWORD || t == asTC_IDENTIFIER )
	{
		token.assign(&modifiedScript[pos], len);
		if( token == "interface" || token == "class" )
		{
			// Skip white spaces and comments
			do
			{
				pos += len;
				t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
			} while ( t == asTC_WHITESPACE || t == asTC_COMMENT );

			if( t == asTC_IDENTIFIER )
			{
				type = 1;
				declaration.assign(&modifiedScript[pos], len);
				pos += len;
				return pos;
			}
		}
		else
		{
			// For function declarations, store everything up to the start of the statement block

			// For variable declaration store everything up until the first parenthesis, assignment, or end statement.

			// We'll only know if the declaration is a variable or function declaration when we see the statement block, or absense of a statement block.
			int varLength = 0;
			declaration.append(&modifiedScript[pos], len);
			pos += len;
			for(; pos < (int)modifiedScript.size();)
			{
				t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
				if( t == asTC_KEYWORD )
				{
					token.assign(&modifiedScript[pos], len);
					if( token == "{" )
					{
						// We've found the end of a function signature
						type = 2;
						return pos;
					}
					if( token == "=" || token == ";" )
					{
						// We've found the end of a variable declaration.
						if( varLength != 0 )
							declaration.resize(varLength);
						type = 3;
						return pos;
					}
					else if( token == "(" && varLength == 0 )
					{
						// This is the first parenthesis we encounter. If the parenthesis isn't followed 
						// by a statement block, then this is a variable declaration, in which case we 
						// should only store the type and name of the variable, not the initialization parameters.
						varLength = (int)declaration.size();
					}
				}

				declaration.append(&modifiedScript[pos], len);
				pos += len;
			}
		}
	}

	return start;
}

const char *CScriptBuilder::GetMetadataStringForType(int typeId)
{
	map<int,string>::iterator it = typeMetadataMap.find(typeId);
	if( it != typeMetadataMap.end() )
		return it->second.c_str();

	return "";
}

const char *CScriptBuilder::GetMetadataStringForFunc(int funcId)
{
	map<int,string>::iterator it = funcMetadataMap.find(funcId);
	if( it != funcMetadataMap.end() )
		return it->second.c_str();

	return "";
}

const char *CScriptBuilder::GetMetadataStringForVar(int varIdx)
{
	map<int,string>::iterator it = varMetadataMap.find(varIdx);
	if( it != varMetadataMap.end() )
		return it->second.c_str();

	return "";
}
#endif

static const char *GetCurrentDir(char *buf, size_t size)
{
#ifdef _MSC_VER
#ifdef _WIN32_WCE
    static TCHAR apppath[MAX_PATH] = TEXT("");
    if (!apppath[0])
    {
        GetModuleFileName(NULL, apppath, MAX_PATH);

        
        int appLen = _tcslen(apppath);

        // Look for the last backslash in the path, which would be the end
        // of the path itself and the start of the filename.  We only want
        // the path part of the exe's full-path filename
        // Safety is that we make sure not to walk off the front of the 
        // array (in case the path is nothing more than a filename)
        while (appLen > 1)
        {
            if (apppath[appLen-1] == TEXT('\\'))
                break;
            appLen--;
        }

        // Terminate the string after the trailing backslash
        apppath[appLen] = TEXT('\0');
    }
#ifdef _UNICODE
    wcstombs(buf, apppath, min(size, wcslen(apppath)*sizeof(wchar_t)));
#else
    memcpy(buf, apppath, min(size, strlen(apppath)));
#endif

    return buf;
#else
	return _getcwd(buf, (int)size);
#endif
#elif defined(__APPLE__)
	return getcwd(buf, size);
#else
	return "";
#endif
}

END_AS_NAMESPACE


