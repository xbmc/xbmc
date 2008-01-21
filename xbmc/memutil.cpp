/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "memutil.h"
#ifdef _XBOX
#include "xbox/Undocumented.h"
#endif

void fast_memcpy(void* d, const void* s, unsigned n)
{
  // around 50% faster than memcpy
  // only worthwhile if the destination buffer is not likely to be read back immediately
  // and the number of bytes copied is >16
  // somewhat faster if the source and destination are a multiple of 16 bytes apart
  __asm {
    mov edx, n
    mov esi, s
    prefetchnta [esi]
    prefetchnta [esi + 32]
    mov edi, d

    // pre align
    mov eax, edi
    mov ecx, 16
    and eax, 15
    sub ecx, eax
    and ecx, 15
    cmp edx, ecx
    jb fmc_exit_main
    sub edx, ecx

    test ecx, ecx
  fmc_start_pre:
    jz fmc_exit_pre

    mov al, [esi]
    mov [edi], al

    inc esi
    inc edi
    dec ecx
    jmp fmc_start_pre

  fmc_exit_pre:
    mov eax, esi
    and eax, 15
    jnz fmc_notaligned

    // main copy, aligned
    mov ecx, edx
    shr ecx, 4
  fmc_start_main_a:
    jz fmc_exit_main

    prefetchnta [esi + 32]
    movaps xmm0, [esi]
    movntps [edi], xmm0

    add esi, 16
      add edi, 16
        dec ecx
        jmp fmc_start_main_a

  fmc_notaligned:
        // main copy, unaligned
        mov ecx, edx
        shr ecx, 4
  fmc_start_main_u:
        jz fmc_exit_main

        prefetchnta [esi + 32]
        movups xmm0, [esi]
        movntps [edi], xmm0

        add esi, 16
          add edi, 16
            dec ecx
            jmp fmc_start_main_u

  fmc_exit_main:

            // post align
            mov ecx, edx
            and ecx, 15
  fmc_start_post:
            jz fmc_exit_post

            mov al, [esi]
            mov [edi], al

            inc esi
            inc edi
            dec ecx
            jmp fmc_start_post

  fmc_exit_post:
          }
}

void fast_memset(void* d, int c, unsigned n)
{
  char __declspec(align(16)) buf[16];

  __asm {
    mov edx, n
    mov edi, d

    // pre align
    mov eax, edi
    mov ecx, 16
    and eax, 15
    sub ecx, eax
    and ecx, 15
    cmp edx, ecx
    jb fms_exit_main
    sub edx, ecx
    mov eax, c

    test ecx, ecx
  fms_start_pre:
    jz fms_exit_pre

    mov [edi], al

    inc edi
    dec ecx
    jmp fms_start_pre

  fms_exit_pre:
    test al, al
    jz fms_initzero

    // duplicate the value 16 times
    lea esi, buf
    mov [esi], al
    mov [esi + 1], al
    mov [esi + 2], al
    mov [esi + 3], al
    mov eax, [esi]
    mov [esi + 4], eax
    mov [esi + 8], eax
    mov [esi + 12], eax
    movaps xmm0, [esi]
    jmp fms_init_loop

  fms_initzero:
    // optimzed set zero
    xorps xmm0, xmm0

  fms_init_loop:
    mov ecx, edx
    shr ecx, 4

  fms_start_main:
    jz fms_exit_main

    movntps [edi], xmm0

    add edi, 16
      dec ecx
      jmp fms_start_main

  fms_exit_main:

      // post align
      mov ecx, edx
      and ecx, 15
  fms_start_post:
      jz fms_exit_post

      mov [edi], al

      inc edi
      dec ecx
      jmp fms_start_post

  fms_exit_post:
    }
  }

#ifdef _XBOX
void usleep(int t)
{
  LARGE_INTEGER li;

  li.QuadPart = (LONGLONG)t * -10;

  // Where possible, Alertable should be set to FALSE and WaitMode should be set to KernelMode,
  // in order to reduce driver complexity. The principal exception to this is when the wait is a long term wait.
  KeDelayExecutionThread(KernelMode, false, &li);
}
#endif