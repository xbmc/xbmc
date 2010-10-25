#ifndef SCRIPTANY_H
#define SCRIPTANY_H

#include <angelscript.h>

BEGIN_AS_NAMESPACE

class CScriptAny 
{
public:
	// Constructors
	CScriptAny(asIScriptEngine *engine);
	CScriptAny(void *ref, int refTypeId, asIScriptEngine *engine);

	// Memory management
	int AddRef();
	int Release();

	// Copy the stored value from another any object
	CScriptAny &operator=(const CScriptAny&);
	int CopyFrom(const CScriptAny *other);

	// Store the value, either as variable type, integer number, or real number
	void Store(void *ref, int refTypeId);
	void Store(asINT64 &value);
	void Store(double &value);

	// Retrieve the stored value, either as variable type, integer number, or real number
	bool Retrieve(void *ref, int refTypeId) const;
	bool Retrieve(asINT64 &value) const;
	bool Retrieve(double &value) const;

	// Get the type id of the stored value
	int  GetTypeId() const;

	// GC methods
	int  GetRefCount();
	void SetFlag();
	bool GetFlag();
	void EnumReferences(asIScriptEngine *engine);
	void ReleaseAllHandles(asIScriptEngine *engine);

protected:
	virtual ~CScriptAny();
	void FreeObject();

	int refCount;
	asIScriptEngine *engine;

	// The structure for holding the values
    struct valueStruct
    {
        union
        {
            asINT64 valueInt;
            double  valueFlt;
            void   *valueObj;
        };
        int   typeId;
    };

	valueStruct value;
};

void RegisterScriptAny(asIScriptEngine *engine);
void RegisterScriptAny_Native(asIScriptEngine *engine);
void RegisterScriptAny_Generic(asIScriptEngine *engine);

END_AS_NAMESPACE

#endif
