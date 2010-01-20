#include "jitc_x86.h"
#include <stdio.h>

int main(int c, char **v)
{
    int i;
    int j;
    JitcX86Env *jitc = jitc_x86_env_new(0xffff);
    JitcFunc    func = jitc_prepare_func(jitc);

    jitc_add(jitc, "mov edx, $d", 0xffffffff);
    jitc_add(jitc, "mov eax, $d", 40);
    jitc_add(jitc, "mov ebx, $d", 2);
    jitc_add(jitc, "idiv ebx");
    jitc_add(jitc, "mov eax, [$d]", 0xdeadbeaf);
    
    jitc_add(jitc, "sal edx, $d", 7);
    jitc_add(jitc, "imul ecx");
    jitc_add(jitc, "idiv ecx");
    jitc_add(jitc, "imul $d[ecx]", 2);
    jitc_add(jitc, "imul ecx, [ecx]");
    jitc_add(jitc, "mov ecx,  $d", (char*)(&i)-12);
    jitc_add(jitc, "dec $d[ecx]",  2);
    jitc_add(jitc, "add ecx, $d", 12);
    jitc_add(jitc, "dec [ecx]");
    jitc_add(jitc, "dec ecx");

    JITC_FLD_pIMM32(jitc,&i);
    JITC_FSTP_pIMM32(jitc,&j);
    
    jitc_validate_func(jitc);
    func();

    printf("i = 0x%08x\n", i);
    
    jitc_x86_delete(jitc);
    return 0;
}
