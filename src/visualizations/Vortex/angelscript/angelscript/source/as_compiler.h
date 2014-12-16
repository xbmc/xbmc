/*
   AngelCode Scripting Library
   Copyright (c) 2003-2009 Andreas Jonsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/


//
// as_compiler.h
//
// The class that does the actual compilation of the functions
//



#ifndef AS_COMPILER_H
#define AS_COMPILER_H

#include "as_config.h"
#include "as_builder.h"
#include "as_scriptfunction.h"
#include "as_variablescope.h"
#include "as_bytecode.h"
#include "as_array.h"
#include "as_datatype.h"
#include "as_typeinfo.h"

BEGIN_AS_NAMESPACE

struct asSExprContext;

struct asSDeferredParam
{
	asSDeferredParam() {argNode = 0; origExpr = 0;}

	asCScriptNode  *argNode;
	asCTypeInfo     argType;
	int             argInOutFlags;
	asSExprContext *origExpr;
};

struct asSExprContext
{
	asSExprContext(asCScriptEngine *engine) : bc(engine) {exprNode = 0; origExpr = 0; property_get = 0; property_set = 0; }

	asCByteCode bc;
	asCTypeInfo type;
	int  property_get;
	int  property_set;
	bool property_const; // If the object that is being accessed through property accessor is read-only
	bool property_handle; // If the property accessor is called on an object stored in a handle
	asCArray<asSDeferredParam> deferredParams;
	asCScriptNode  *exprNode;
	asSExprContext *origExpr;
};

enum EImplicitConv
{
	asIC_IMPLICIT_CONV,
	asIC_EXPLICIT_REF_CAST,
	asIC_EXPLICIT_VAL_CAST
};

class asCCompiler
{
public:
	asCCompiler(asCScriptEngine *engine);
	~asCCompiler();

	int CompileFunction(asCBuilder *builder, asCScriptCode *script, asCScriptNode *func, asCScriptFunction *outFunc);
	int CompileDefaultConstructor(asCBuilder *builder, asCScriptCode *script, asCScriptFunction *outFunc);
	int CompileFactory(asCBuilder *builder, asCScriptCode *script, asCScriptFunction *outFunc);
	int CompileTemplateFactoryStub(asCBuilder *builder, int trueFactoryId, asCObjectType *objType, asCScriptFunction *outFunc);
	int CompileGlobalVariable(asCBuilder *builder, asCScriptCode *script, asCScriptNode *expr, sGlobalVariableDescription *gvar, asCScriptFunction *outFunc);

	asCByteCode byteCode;
	asCArray<asCObjectType*> objVariableTypes;
	asCArray<int> objVariablePos;

protected:
	friend class asCBuilder;

	void Reset(asCBuilder *builder, asCScriptCode *script, asCScriptFunction *outFunc);

	// Statements
	void CompileStatementBlock(asCScriptNode *block, bool ownVariableScope, bool *hasReturn, asCByteCode *bc);
	void CompileDeclaration(asCScriptNode *decl, asCByteCode *bc);
	void CompileStatement(asCScriptNode *statement, bool *hasReturn, asCByteCode *bc);
	void CompileIfStatement(asCScriptNode *node, bool *hasReturn, asCByteCode *bc);
	void CompileSwitchStatement(asCScriptNode *node, bool *hasReturn, asCByteCode *bc);
	void CompileCase(asCScriptNode *node, asCByteCode *bc);
	void CompileForStatement(asCScriptNode *node, asCByteCode *bc);
	void CompileWhileStatement(asCScriptNode *node, asCByteCode *bc);
	void CompileDoWhileStatement(asCScriptNode *node, asCByteCode *bc);
	void CompileBreakStatement(asCScriptNode *node, asCByteCode *bc);
	void CompileContinueStatement(asCScriptNode *node, asCByteCode *bc);
	void CompileReturnStatement(asCScriptNode *node, asCByteCode *bc);
	void CompileExpressionStatement(asCScriptNode *node, asCByteCode *bc);

	// Expressions
	int  CompileAssignment(asCScriptNode *expr, asSExprContext *out);
	int  CompileCondition(asCScriptNode *expr, asSExprContext *out);
	int  CompileExpression(asCScriptNode *expr, asSExprContext *out);
	int  CompilePostFixExpression(asCArray<asCScriptNode *> *postfix, asSExprContext *out);
	int  CompileExpressionTerm(asCScriptNode *node, asSExprContext *out);
	int  CompileExpressionPreOp(asCScriptNode *node, asSExprContext *out);
	int  CompileExpressionPostOp(asCScriptNode *node, asSExprContext *out);
	int  CompileExpressionValue(asCScriptNode *node, asSExprContext *out);
	void CompileFunctionCall(asCScriptNode *node, asSExprContext *out, asCObjectType *objectType, bool objIsConst, const asCString &scope = "");
	void CompileConstructCall(asCScriptNode *node, asSExprContext *out);
	void CompileConversion(asCScriptNode *node, asSExprContext *out);
	int  CompileOperator(asCScriptNode *node, asSExprContext *l, asSExprContext *r, asSExprContext *out);
	void CompileOperatorOnHandles(asCScriptNode *node, asSExprContext *l, asSExprContext *r, asSExprContext *out);
	void CompileMathOperator(asCScriptNode *node, asSExprContext *l, asSExprContext *r, asSExprContext *out);
	void CompileBitwiseOperator(asCScriptNode *node, asSExprContext *l, asSExprContext *r, asSExprContext *out);
	void CompileComparisonOperator(asCScriptNode *node, asSExprContext *l, asSExprContext *r, asSExprContext *out);
	void CompileBooleanOperator(asCScriptNode *node, asSExprContext *l, asSExprContext *r, asSExprContext *out);
	bool CompileOverloadedDualOperator(asCScriptNode *node, asSExprContext *l, asSExprContext *r, asSExprContext *out);
	int  CompileOverloadedDualOperator2(asCScriptNode *node, const char *methodName, asSExprContext *l, asSExprContext *r, asSExprContext *out, bool specificReturn = false, const asCDataType &returnType = asCDataType::CreatePrimitive(ttVoid, false));

	void CompileInitList(asCTypeInfo *var, asCScriptNode *node, asCByteCode *bc);

	int  CallDefaultConstructor(asCDataType &type, int offset, asCByteCode *bc, asCScriptNode *node, bool isGlobalVar = false);
	void CallDestructor(asCDataType &type, int offset, asCByteCode *bc);
	int  CompileArgumentList(asCScriptNode *node, asCArray<asSExprContext *> &args);
	void MatchFunctions(asCArray<int> &funcs, asCArray<asSExprContext*> &args, asCScriptNode *node, const char *name, asCObjectType *objectType = NULL, bool isConstMethod = false, bool silent = false, bool allowObjectConstruct = true, const asCString &scope = "");

	// Helper functions
	void ProcessPropertyGetAccessor(asSExprContext *ctx, asCScriptNode *node);
	int  ProcessPropertySetAccessor(asSExprContext *ctx, asSExprContext *arg, asCScriptNode *node);
	int  FindPropertyAccessor(const asCString &name, asSExprContext *ctx, asCScriptNode *node);
	void SwapPostFixOperands(asCArray<asCScriptNode *> &postfix, asCArray<asCScriptNode *> &target);
	void PrepareTemporaryObject(asCScriptNode *node, asSExprContext *ctx, asCArray<int> *reservedVars);
	void PrepareOperand(asSExprContext *ctx, asCScriptNode *node);
	void PrepareForAssignment(asCDataType *lvalue, asSExprContext *rvalue, asCScriptNode *node, asSExprContext *lvalueExpr = 0);
	void PerformAssignment(asCTypeInfo *lvalue, asCTypeInfo *rvalue, asCByteCode *bc, asCScriptNode *node);
	bool IsVariableInitialized(asCTypeInfo *type, asCScriptNode *node);
	void Dereference(asSExprContext *ctx, bool generateCode);
	bool CompileRefCast(asSExprContext *ctx, const asCDataType &to, bool isExplicit, asCScriptNode *node, bool generateCode = true);
	int  MatchArgument(asCArray<int> &funcs, asCArray<int> &matches, const asCTypeInfo *argType, int paramNum, bool allowObjectConstruct = true);
	void PerformFunctionCall(int funcId, asSExprContext *out, bool isConstructor = false, asCArray<asSExprContext*> *args = 0, asCObjectType *objTypeForConstruct = 0, bool useVariable = false, int varOffset = 0);
	void MoveArgsToStack(int funcId, asCByteCode *bc, asCArray<asSExprContext *> &args, bool addOneToOffset);
	void MakeFunctionCall(asSExprContext *ctx, int funcId, asCObjectType *objectType, asCArray<asSExprContext*> &args, asCScriptNode *node, bool useVariable = false, int stackOffset = 0);
	void PrepareFunctionCall(int funcId, asCByteCode *bc, asCArray<asSExprContext *> &args);
	void AfterFunctionCall(int funcId, asCArray<asSExprContext*> &args, asSExprContext *ctx, bool deferAll);
	void ProcessDeferredParams(asSExprContext *ctx);
	void PrepareArgument(asCDataType *paramType, asSExprContext *ctx, asCScriptNode *node, bool isFunction = false, int refType = 0, asCArray<int> *reservedVars = 0);
	void PrepareArgument2(asSExprContext *ctx, asSExprContext *arg, asCDataType *paramType, bool isFunction = false, int refType = 0, asCArray<int> *reservedVars = 0);
	bool IsLValue(asCTypeInfo &type);
	int  DoAssignment(asSExprContext *out, asSExprContext *lctx, asSExprContext *rctx, asCScriptNode *lexpr, asCScriptNode *rexpr, int op, asCScriptNode *opNode);
	void MergeExprContexts(asSExprContext *before, asSExprContext *after);
	void FilterConst(asCArray<int> &funcs);
	void ConvertToVariable(asSExprContext *ctx);
	void ConvertToVariableNotIn(asSExprContext *ctx, asSExprContext *exclude);
	void ConvertToVariableNotIn(asSExprContext *ctx, asCArray<int> *reservedVars);
	void ConvertToTempVariable(asSExprContext *ctx);
	void ConvertToTempVariableNotIn(asSExprContext *ctx, asSExprContext *exclude);
	void ConvertToTempVariableNotIn(asSExprContext *ctx, asCArray<int> *reservedVars);
	void ConvertToReference(asSExprContext *ctx);
	void PushVariableOnStack(asSExprContext *ctx, bool asReference);
	asCString GetScopeFromNode(asCScriptNode *node);

	void ImplicitConversion(asSExprContext *ctx, const asCDataType &to, asCScriptNode *node, EImplicitConv convType, bool generateCode = true, asCArray<int> *reservedVars = 0, bool allowObjectConstruct = true);
	void ImplicitConvPrimitiveToPrimitive(asSExprContext *ctx, const asCDataType &to, asCScriptNode *node, EImplicitConv convType, bool generateCode = true, asCArray<int> *reservedVars = 0);
	void ImplicitConvObjectToPrimitive(asSExprContext *ctx, const asCDataType &to, asCScriptNode *node, EImplicitConv convType, bool generateCode = true, asCArray<int> *reservedVars = 0);
	void ImplicitConvPrimitiveToObject(asSExprContext *ctx, const asCDataType &to, asCScriptNode *node, EImplicitConv convType, bool generateCode = true, asCArray<int> *reservedVars = 0, bool allowObjectConstruct = true);
	void ImplicitConvObjectToObject(asSExprContext *ctx, const asCDataType &to, asCScriptNode *node, EImplicitConv convType, bool generateCode = true, asCArray<int> *reservedVars = 0, bool allowObjectConstruct = true);
	void ImplicitConversionConstant(asSExprContext *ctx, const asCDataType &to, asCScriptNode *node, EImplicitConv convType);

	void LineInstr(asCByteCode *bc, size_t pos);

	asUINT ProcessStringConstant(asCString &str, asCScriptNode *node, bool processEscapeSequences = true);
	void ProcessHeredocStringConstant(asCString &str, asCScriptNode *node);
	int  GetPrecedence(asCScriptNode *op);

	void Error(const char *msg, asCScriptNode *node);
	void Warning(const char *msg, asCScriptNode *node);
	void PrintMatchingFuncs(asCArray<int> &funcs, asCScriptNode *node);


	void AddVariableScope(bool isBreakScope = false, bool isContinueScope = false);
	void RemoveVariableScope();

	bool hasCompileErrors;

	int nextLabel;

	asCVariableScope *variables;
	asCBuilder *builder;
	asCScriptEngine *engine;
	asCScriptCode *script;
	asCScriptFunction *outFunc;

	bool m_isConstructor;
	bool m_isConstructorCalled;

	asCArray<int> breakLabels;
	asCArray<int> continueLabels;

	int AllocateVariable(const asCDataType &type, bool isTemporary);
	int AllocateVariableNotIn(const asCDataType &type, bool isTemporary, asCArray<int> *vars);
	int GetVariableOffset(int varIndex);
	int GetVariableSlot(int varOffset);
	void DeallocateVariable(int pos);
	void ReleaseTemporaryVariable(asCTypeInfo &t, asCByteCode *bc);
	void ReleaseTemporaryVariable(int offset, asCByteCode *bc);

	asCArray<asCDataType> variableAllocations;
	asCArray<bool> variableIsTemporary;
	asCArray<int> freeVariables;
	asCArray<int> tempVariables;

	bool globalExpression;
	bool isProcessingDeferredParams;
	int noCodeOutput;
};

END_AS_NAMESPACE

#endif
