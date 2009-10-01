/*
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 *
 *                         All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Digital Equipment
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 *
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * The following list of definitions is generated from VTparse.def using the
 * following command line:
 *
 *     grep '^CASE_' VTparse.def | awk '{printf "#define %s %d\n", $1, n++}'
 *
 * You you need to change something, change VTparse.def and regenerate the
 * definitions.  This would have been automatic, but since this doesn't change
 * very often, it isn't worth the makefile hassle.
 */

#define CASE_GROUND_STATE 0
#define CASE_IGNORE_STATE 1
#define CASE_IGNORE_ESC 2
#define CASE_IGNORE 3
#define CASE_BELL 4
#define CASE_BS 5
#define CASE_CR 6
#define CASE_ESC 7
#define CASE_VMOT 8
#define CASE_TAB 9
#define CASE_SI 10
#define CASE_SO 11
#define CASE_SCR_STATE 12
#define CASE_SCS0_STATE 13
#define CASE_SCS1_STATE 14
#define CASE_SCS2_STATE 15
#define CASE_SCS3_STATE 16
#define CASE_ESC_IGNORE 17
#define CASE_ESC_DIGIT 18
#define CASE_ESC_SEMI 19
#define CASE_DEC_STATE 20
#define CASE_ICH 21
#define CASE_CUU 22
#define CASE_CUD 23
#define CASE_CUF 24
#define CASE_CUB 25
#define CASE_CUP 26
#define CASE_ED 27
#define CASE_EL 28
#define CASE_IL 29
#define CASE_DL 30
#define CASE_DCH 31
#define CASE_DA1 32
#define CASE_TRACK_MOUSE 33
#define CASE_TBC 34
#define CASE_SET 35
#define CASE_RST 36
#define CASE_SGR 37
#define CASE_CPR 38
#define CASE_DECSTBM 39
#define CASE_DECREQTPARM 40
#define CASE_DECSET 41
#define CASE_DECRST 42
#define CASE_DECALN 43
#define CASE_GSETS 44
#define CASE_DECSC 45
#define CASE_DECRC 46
#define CASE_DECKPAM 47
#define CASE_DECKPNM 48
#define CASE_IND 49
#define CASE_NEL 50
#define CASE_HTS 51
#define CASE_RI 52
#define CASE_SS2 53
#define CASE_SS3 54
#define CASE_CSI_STATE 55
#define CASE_OSC 56
#define CASE_RIS 57
#define CASE_LS2 58
#define CASE_LS3 59
#define CASE_LS3R 60
#define CASE_LS2R 61
#define CASE_LS1R 62
#define CASE_PRINT 63
#define CASE_XTERM_SAVE 64
#define CASE_XTERM_RESTORE 65
#define CASE_XTERM_TITLE 66
#define CASE_DECID 67
#define CASE_HP_MEM_LOCK 68
#define CASE_HP_MEM_UNLOCK 69
#define CASE_HP_BUGGY_LL 70
#define CASE_TO_STATUS 71
#define CASE_FROM_STATUS 72
#define CASE_SHOW_STATUS 73
#define CASE_HIDE_STATUS 74
#define CASE_ERASE_STATUS 75
#define CASE_MBCS 76
#define CASE_SCS_STATE 77
#define CASE_MY_GRAPHIC_CMD 78
#define CASE_MY_LINE 79
/* 80 (Not used) */
/* 81 (Not used) */
#define CASE_BACK_BYTE 82
