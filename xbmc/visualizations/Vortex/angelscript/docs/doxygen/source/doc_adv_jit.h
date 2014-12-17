/**

\page doc_adv_jit JIT compilation

AngelScript doesn't provide a built-in JIT compiler, instead it permits an external JIT compiler to be implemented 
through a public interface.

To use JIT compilation, the scripts must be compiled with a few extra instructions that provide hints to the JIT compiler
and also entry points so that the VM will know when to pass control to the JIT compiled function. By default this is 
turned off, and must thus be turned on by setting the engine property \ref asEP_INCLUDE_JIT_INSTRUCTIONS.

If the application sets the \ref asIJITCompiler "JIT compiler" with \ref asIScriptEngine::SetJITCompiler "SetJITCompiler" 
AngelScript will automatically invoke it to provide the \ref asJITFunction "JIT functions" with each compilation or 
\ref doc_adv_precompile "loading of pre-compiled bytecode".




\section doc_adv_jit_3 The structure of the JIT function

The \ref asJITFunction "JIT compiled function" must follow certain rules in order to behave well with the virtual machine. The intention
is that the VM will pass the control to the JIT function, and when the execution is to be suspended the JIT function
returns the control to the VM, updating the internal state of the VM so that the VM can resume the execution when
requested. Each time the JIT function returns control to the VM it must make sure that the \ref asSVMRegisters "VM registers" and stack values
have been updated according to the code that was executed.

The byte code will have a special instruction, \ref asBC_JitEntry "JitEntry", which defines the positions where the VM
can pass the control to the JIT function. These are usually placed for every script statement, and after each instruction
that calls another function. This implies that the JIT compiled function needs to be able to start the execution at 
different points based on the argument in the JitEntry instruction. The value of the argument is defined by the JIT compiler
and how it is interpreted is also up to the JIT compiler, with the exception of 0 that means that the control should not be 
passed to the JIT function.

Some byte code instructions are not meant to be converted into native code. These are usually the ones that have a more 
global effect on the VM, e.g. the instructions that setup a call to a new script function, or that return from a previous 
instruction. When these functions are encountered, the JIT function should return the control to the VM, and then the VM
will execute the instruction. 

Other byte code instructions may be partially implemented by the JIT function, for example those that can throw an exception
based on specific conditions. One such example is the instructions for divisions, if the divider is 0 the VM will set
an exception and abort the execution. For these instructions the JIT compiler should preferrably implement the condition
that doesn't throw an exception, and if an exception is to be thrown the JIT function will instead break out to the VM.

The following shows a possible structure of a JIT compiled function:

<pre>
  void jitCompiledFunc(asSVMRegisters *regs, asDWORD entry)
  {
    Read desired VM registers into CPU registers
    Jump to the current position of the function based on the 'entry' argument
  1:
    Execute code in block 1
    Jump to exit if an illegal operation is done, e.g. divide by zero. 
    Jump to exit if block ends with an instruction that should not be executed by JIT function. 
  2:
    ...
  3:
    ...
  exit:
    Update the VM registers before returning control to VM
  }
</pre>



\section doc_adv_jit_2 Traversing the byte code


\code
int CJITCompiler::CompileFunction(asIScriptFunction *func, asJITFunction *output)
{
  bool success = StartNewCompilation();

  // Get the script byte code
  asUINT   length;
  asDWORD *byteCode = func->GetByteCode(&length);
  asDWORD *end = byteCode + length;
  
  while( byteCode < end )
  {
    // Determine the instruction
    asEBCInstr op = asEBCInstr(*(asBYTE*)byteCode);
    switch( op )
    {
    // Translate each byte code instruction into native code.
    // The codes that cannot be translated should return the control
    // to the VM, so that it can continue the processing. When 
    // the VM encounters the next JitEntry instruction it will
    // transfer the control back to the JIT function.
    ...
    
    case asBC_JitEntry:
      // Update the argument for the JitEntry instruction with  
      // the argument that should be sent to the jit function.
      // Remember that 0 means that the VM should not pass
      // control to the JIT function.
      asBC_SWORDARG0(byteCode) = DetermineJitEntryArg();
      break;
    }
    
    // Move to next instruction
    byteCode += asBCTypeSize[asBCInfo[op].type];
  }
  
  if( success )
  {
    *output = GetCompiledFunction();
    return 0;
  }
  
  return -1;
}
\endcode

The following macros should be used to read the arguments from the bytecode instruction. 
The layout of the arguments is determined from the \ref asBCInfo array.

 - \ref asBC_DWORDARG
 - \ref asBC_INTARG
 - \ref asBC_QWORDARG
 - \ref asBC_FLOATARG
 - \ref asBC_PTRARG
 - \ref asBC_WORDARG0
 - \ref asBC_WORDARG1
 - \ref asBC_SWORDARG0
 - \ref asBC_SWORDARG1
 - \ref asBC_SWORDARG2

What each \ref asEBCInstr "byte code" instruction does is described in \subpage doc_adv_jit_1, but the exact 
implementation of each byte code instruction is best determined from the implementation
in the VM, i.e. the asCScriptContext::ExecuteNext method.







\page doc_adv_jit_1 Byte code instructions

This page gives a brief description of each of the byte code instructions that the virtual machine has.


 - \ref doc_adv_jit_1_1
 - \ref doc_adv_jit_1_2
 - \ref doc_adv_jit_1_3
 - \ref doc_adv_jit_1_4
 - \ref doc_adv_jit_1_5
 - \ref doc_adv_jit_1_6
 - \ref doc_adv_jit_1_7
 - \ref doc_adv_jit_1_8
 - \ref doc_adv_jit_1_9




\section doc_adv_jit_1_1 Object management

Perform a bitwise copy of a memory buffer to another

 - \ref asBC_COPY

Push the address and length of a string on the stack

 - \ref asBC_STR
 
Allocate the memory for an object and setup the VM to execute the constructor
 
 - \ref asBC_ALLOC
 
Release the memory of an object
 
 - \ref asBC_FREE
 
Move the address in an object variable to the object register. The address in the variable is then cleared.
 
 - \ref asBC_LOADOBJ
 
Move the address from the object register to an object variable. The address in the object register is then cleared.
 
 - \ref asBC_STOREOBJ

Copy the object handle from one address to another. The reference count of the object is updated to reflect the copy.

 - \ref asBC_REFCPY
 
Push the pointer of an object type on the stack
 
 - \ref asBC_OBJTYPE
 
Push the type id on the stack
 
 - \ref asBC_TYPEID
 
Pop an address to a script object from the stack. If the desired cast can be made store the address in the object register.
 
 - \ref asBC_Cast




\section doc_adv_jit_1_2 Math instructions

Negate the value in the variable. The original value is overwritten.

 - \ref asBC_NEGi
 - \ref asBC_NEGf
 - \ref asBC_NEGd
 - \ref asBC_NEGi64

Perform the operation with the value of two variables and store the result in a third variable.

 - \ref asBC_ADDi
 - \ref asBC_SUBi
 - \ref asBC_MULi
 - \ref asBC_DIVi
 - \ref asBC_MODi

 - \ref asBC_ADDf
 - \ref asBC_SUBf
 - \ref asBC_MULf
 - \ref asBC_DIVf
 - \ref asBC_MODf

 - \ref asBC_ADDd
 - \ref asBC_SUBd
 - \ref asBC_MULd
 - \ref asBC_DIVd
 - \ref asBC_MODd

 - \ref asBC_ADDi64
 - \ref asBC_SUBi64
 - \ref asBC_MULi64
 - \ref asBC_DIVi64
 - \ref asBC_MODi64

Perform the operation with a constant value and the value of the variable. The original value is overwritten.

 - \ref asBC_ADDIi
 - \ref asBC_SUBIi
 - \ref asBC_MULIi

 - \ref asBC_ADDIf
 - \ref asBC_SUBIf
 - \ref asBC_MULIf




\section doc_adv_jit_1_3 Bitwise instructions

Perform a boolean not operation on the value in the variable. The original value is overwritten.

 - \ref asBC_NOT

Perform a bitwise complement on the value in the variable. The original value is overwritten.

 - \ref asBC_BNOT
 - \ref asBC_BNOT64

Perform the operation with the value of two variables and store the result in a third variable.

 - \ref asBC_BAND
 - \ref asBC_BOR
 - \ref asBC_BXOR
 
 - \ref asBC_BAND64
 - \ref asBC_BOR64
 - \ref asBC_BXOR64
 
 - \ref asBC_BSLL
 - \ref asBC_BSRL
 - \ref asBC_BSRA

 - \ref asBC_BSLL64
 - \ref asBC_BSRL64
 - \ref asBC_BSRA64




\section doc_adv_jit_1_4 Comparisons

Compare the value of two variables and store the result in the value register.

 - \ref asBC_CMPi
 - \ref asBC_CMPu
 - \ref asBC_CMPf
 - \ref asBC_CMPd
 - \ref asBC_CMPi64
 - \ref asBC_CMPu64

Compare the value of a variable with a constant and store the result in the value register.

 - \ref asBC_CMPIi
 - \ref asBC_CMPIu
 - \ref asBC_CMPIf

Test the value in the value register. Update the value register according to the result.

 - \ref asBC_TZ
 - \ref asBC_TNZ
 - \ref asBC_TS
 - \ref asBC_TNS
 - \ref asBC_TP
 - \ref asBC_TNP




\section doc_adv_jit_1_5 Type conversions

Convert the value in the variable. The original value is overwritten.

 - \ref asBC_iTOb
 - \ref asBC_iTOw
 - \ref asBC_sbTOi
 - \ref asBC_swTOi
 - \ref asBC_ubTOi
 - \ref asBC_uwTOi

 - \ref asBC_iTOf
 - \ref asBC_fTOi
 - \ref asBC_uTOf
 - \ref asBC_fTOu

 - \ref asBC_dTOi64
 - \ref asBC_dTOu64
 - \ref asBC_i64TOd
 - \ref asBC_u64TOd

Convert the value of a variable and store the result in another variable.

 - \ref asBC_dTOi
 - \ref asBC_dTOu
 - \ref asBC_dTOf
 - \ref asBC_iTOd
 - \ref asBC_uTOd
 - \ref asBC_fTOd

 - \ref asBC_i64TOi
 - \ref asBC_i64TOf
 - \ref asBC_u64TOf
 - \ref asBC_uTOi64
 - \ref asBC_iTOi64
 - \ref asBC_fTOi64
 - \ref asBC_fTOu64
 
 
 
 
\section doc_adv_jit_1_6 Increment and decrement

Increment or decrement the value pointed to by the address in the value register.

 - \ref asBC_INCi8
 - \ref asBC_DECi8
 - \ref asBC_INCi16
 - \ref asBC_DECi16
 - \ref asBC_INCi
 - \ref asBC_DECi
 - \ref asBC_INCi64
 - \ref asBC_DECi64
 - \ref asBC_INCf
 - \ref asBC_DECf
 - \ref asBC_INCd
 - \ref asBC_DECd

Increment or decrement the value in the variable.

 - \ref asBC_IncVi
 - \ref asBC_DecVi




\section doc_adv_jit_1_7 Flow control

Setup the VM to begin execution of the other script function

 - \ref asBC_CALL
 - \ref asBC_CALLINTF
 - \ref asBC_CALLBND
 
Setup the VM to return to the calling function 
 
 - \ref asBC_RET

Make an unconditional jump to a relative position
 
 - \ref asBC_JMP
 
Make a jump to a relative position depending on the value in the value register

 - \ref asBC_JZ
 - \ref asBC_JNZ
 - \ref asBC_JS
 - \ref asBC_JNS
 - \ref asBC_JP
 - \ref asBC_JNP
 - \ref asBC_JMPP

Call an application registered function

 - \ref asBC_CALLSYS

Save the state and suspend execution, then return control to the application

 - \ref asBC_SUSPEND

Give control of execution to the JIT compiled function
 
 - \ref asBC_JitEntry
 
 
 

\section doc_adv_jit_1_8 Stack and data management

Update the stack pointer.

 - \ref asBC_POP
 - \ref asBC_PUSH
 
Push a constant value on the stack.
 
 - \ref asBC_PshC4
 - \ref asBC_PshC8

Push the stack frame pointer on the stack.
 
 - \ref asBC_PSF

Swap the top values on the stack.

 - \ref asBC_SWAP4
 - \ref asBC_SWAP8
 - \ref asBC_SWAP48
 - \ref asBC_SWAP84

Pop an address from the stack, read a value from the address and push it on the stack.

 - \ref asBC_RDS4
 - \ref asBC_RDS8

Add an offset to the top address on the stack.

 - \ref asBC_ADDSi

Push the value of a variable on the stack.

 - \ref asBC_PshV4

Initialize the value of a variable with a constant.
 
 - \ref asBC_SetV1
 - \ref asBC_SetV2
 - \ref asBC_SetV4
 - \ref asBC_SetV8

Copy the value of one variable to another.
 
 - \ref asBC_CpyVtoV4
 - \ref asBC_CpyVtoV8

Validate that an expected pointer is not null.

 - \ref asBC_CHKREF
 - \ref asBC_ChkRefS
 - \ref asBC_ChkNullV
 - \ref asBC_ChkNullS

Push the variable index with the size of a pointer on the stack.

 - \ref asBC_VAR

Replace a variable index on the stack with an address.
 
 - \ref asBC_GETREF
 - \ref asBC_GETOBJ
 - \ref asBC_GETOBJREF

Pop or push an address to or from the value register.

 - \ref asBC_PopRPtr
 - \ref asBC_PshRPtr
 
Copy a value between value register and a variable.
 
 - \ref asBC_CpyVtoR4
 - \ref asBC_CpyVtoR8
 - \ref asBC_CpyRtoV4
 - \ref asBC_CpyRtoV8

Copy a value from a variable to the address held in the value register

 - \ref asBC_WRTV1
 - \ref asBC_WRTV2
 - \ref asBC_WRTV4
 - \ref asBC_WRTV8

Copy a value from the address held in the value register to a variable
 
 - \ref asBC_RDR1
 - \ref asBC_RDR2
 - \ref asBC_RDR4
 - \ref asBC_RDR8

Load the address of the variable into the value register

 - \ref asBC_LDV
 
Clear the upper bytes of the value register
 
 - \ref asBC_ClrHi




\section doc_adv_jit_1_9 Global variables

Push the value of a global variable on the stack

 - \ref asBC_PshG4

Load the address of a global variable into the value register

 - \ref asBC_LDG
 
Load the address of a global variable into the value register and copy the value of the global variable to local variable
 
 - \ref asBC_LdGRdR4

Copy a value between local variable and global variable.

 - \ref asBC_CpyVtoG4
 - \ref asBC_CpyGtoV4
 
Push the address of the global variable on the stack.

 - \ref asBC_PGA

Initialize the variable of a global variable with a constant.

 - \ref asBC_SetG4


*/
