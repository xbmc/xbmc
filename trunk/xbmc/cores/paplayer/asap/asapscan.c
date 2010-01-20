/*
 * asapscan.c - 8-bit Atari music analyzer
 *
 * Copyright (C) 2007-2009  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asap.h"
#include "asap_internal.h"

static abool detect_time = FALSE;
static int scan_player_calls;
static int silence_player_calls;
static int loop_check_player_calls;
static int loop_min_player_calls;
static byte *registers_dump;

static ASAP_State asap;
static abool dump = FALSE;

#define FEATURE_CHECK          1
#define FEATURE_15_KHZ         2
#define FEATURE_HIPASS_FILTER  4
#define FEATURE_LOW_OF_16_BIT  8
#define FEATURE_9_BIT_POLY     16
static int features = 0;

#define CPU_TRACE_PRINT        1
#define CPU_TRACE_UNOFFICIAL   2
int cpu_trace = 0;

static const char cpu_mnemonics[256][10] = {
	"BRK", "ORA (1,X)", "CIM", "ASO (1,X)", "NOP 1", "ORA 1", "ASL 1", "ASO 1",
	"PHP", "ORA #1", "ASL", "ANC #1", "NOP 2", "ORA 2", "ASL 2", "ASO 2",
	"BPL 0", "ORA (1),Y", "CIM", "ASO (1),Y", "NOP 1,X", "ORA 1,X", "ASL 1,X", "ASO 1,X",
	"CLC", "ORA 2,Y", "NOP !", "ASO 2,Y", "NOP 2,X", "ORA 2,X", "ASL 2,X", "ASO 2,X",
	"JSR 2", "AND (1,X)", "CIM", "RLA (1,X)", "BIT 1", "AND 1", "ROL 1", "RLA 1",
	"PLP", "AND #1", "ROL", "ANC #1", "BIT 2", "AND 2", "ROL 2", "RLA 2",
	"BMI 0", "AND (1),Y", "CIM", "RLA (1),Y", "NOP 1,X", "AND 1,X", "ROL 1,X", "RLA 1,X",
	"SEC", "AND 2,Y", "NOP !", "RLA 2,Y", "NOP 2,X", "AND 2,X", "ROL 2,X", "RLA 2,X",

	"RTI", "EOR (1,X)", "CIM", "LSE (1,X)", "NOP 1", "EOR 1", "LSR 1", "LSE 1",
	"PHA", "EOR #1", "LSR", "ALR #1", "JMP 2", "EOR 2", "LSR 2", "LSE 2",
	"BVC 0", "EOR (1),Y", "CIM", "LSE (1),Y", "NOP 1,X", "EOR 1,X", "LSR 1,X", "LSE 1,X",
	"CLI", "EOR 2,Y", "NOP !", "LSE 2,Y", "NOP 2,X", "EOR 2,X", "LSR 2,X", "LSE 2,X",
	"RTS", "ADC (1,X)", "CIM", "RRA (1,X)", "NOP 1", "ADC 1", "ROR 1", "RRA 1",
	"PLA", "ADC #1", "ROR", "ARR #1", "JMP (2)", "ADC 2", "ROR 2", "RRA 2",
	"BVS 0", "ADC (1),Y", "CIM", "RRA (1),Y", "NOP 1,X", "ADC 1,X", "ROR 1,X", "RRA 1,X",
	"SEI", "ADC 2,Y", "NOP !", "RRA 2,Y", "NOP 2,X", "ADC 2,X", "ROR 2,X", "RRA 2,X",

	"NOP #1", "STA (1,X)", "NOP #1", "SAX (1,X)", "STY 1", "STA 1", "STX 1", "SAX 1",
	"DEY", "NOP #1", "TXA", "ANE #1", "STY 2", "STA 2", "STX 2", "SAX 2",
	"BCC 0", "STA (1),Y", "CIM", "SHA (1),Y", "STY 1,X", "STA 1,X", "STX 1,Y", "SAX 1,Y",
	"TYA", "STA 2,Y", "TXS", "SHS 2,Y", "SHY 2,X", "STA 2,X", "SHX 2,Y", "SHA 2,Y",
	"LDY #1", "LDA (1,X)", "LDX #1", "LAX (1,X)", "LDY 1", "LDA 1", "LDX 1", "LAX 1",
	"TAY", "LDA #1", "TAX", "ANX #1", "LDY 2", "LDA 2", "LDX 2", "LAX 2",
	"BCS 0", "LDA (1),Y", "CIM", "LAX (1),Y", "LDY 1,X", "LDA 1,X", "LDX 1,Y", "LAX 1,X",
	"CLV", "LDA 2,Y", "TSX", "LAS 2,Y", "LDY 2,X", "LDA 2,X", "LDX 2,Y", "LAX 2,Y",

	"CPY #1", "CMP (1,X)", "NOP #1", "DCM (1,X)", "CPY 1", "CMP 1", "DEC 1", "DCM 1",
	"INY", "CMP #1", "DEX", "SBX #1", "CPY 2", "CMP 2", "DEC 2", "DCM 2",
	"BNE 0", "CMP (1),Y", "CIM", "DCM (1),Y", "NOP 1,X", "CMP 1,X", "DEC 1,X", "DCM 1,X",
	"CLD", "CMP 2,Y", "NOP !", "DCM 2,Y", "NOP 2,X", "CMP 2,X", "DEC 2,X", "DCM 2,X",

	"CPX #1", "SBC (1,X)", "NOP #1", "INS (1,X)", "CPX 1", "SBC 1", "INC 1", "INS 1",
	"INX", "SBC #1", "NOP", "SBC #1 !", "CPX 2", "SBC 2", "INC 2", "INS 2",
	"BEQ 0", "SBC (1),Y", "CIM", "INS (1),Y", "NOP 1,X", "SBC 1,X", "INC 1,X", "INS 1,X",
	"SED", "SBC 2,Y", "NOP !", "INS 2,Y", "NOP 2,X", "SBC 2,X", "INC 2,X", "INS 2,X"
};

#define CPU_OPCODE_UNOFFICIAL  1
#define CPU_OPCODE_USED        2
static char cpu_opcodes[256] = {
	0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1,
	0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
	0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
	0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
	0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
	0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
	0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
	0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
	1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1,
	0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1,
	0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
	0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
	0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
	0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
	0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
	0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1
};

static void show_instruction(const ASAP_State *ast, int pc)
{
	int addr = pc;
	int opcode;
	const char *mnemonic;
	const char *p;

	opcode = dGetByte(pc++);
	mnemonic = cpu_mnemonics[opcode];
	for (p = mnemonic + 3; *p != '\0'; p++) {
		if (*p == '1') {
			int value = dGetByte(pc);
			printf("%04X: %02X %02X     %.*s$%02X%s\n",
			       addr, opcode, value, (int) (p - mnemonic), mnemonic, value, p + 1);
			return;
		}
		if (*p == '2') {
			int lo = dGetByte(pc);
			int hi = dGetByte(pc + 1);
			printf("%04X: %02X %02X %02X  %.*s$%02X%02X%s\n",
			       addr, opcode, lo, hi, (int) (p - mnemonic), mnemonic, hi, lo, p + 1);
			return;
		}
		if (*p == '0') {
			int offset = dGetByte(pc++);
			int target = (pc + (signed char) offset) & 0xffff;
			printf("%04X: %02X %02X     %.4s$%04X\n", addr, opcode, offset, mnemonic, target);
			return;
		}
	}
	printf("%04X: %02X        %s\n", addr, opcode, mnemonic);
}

void trace_cpu(const ASAP_State *ast, int pc, int a, int x, int y, int s, int nz, int vdi, int c)
{
	if ((cpu_trace & CPU_TRACE_PRINT) != 0) {
		printf("%3d %3d A=%02X X=%02X Y=%02X S=%02X P=%c%c*-%c%c%c%c PC=",
			ast->scanline_number, ast->cycle + 114 - ast->next_scanline_cycle, a, x, y, s,
			nz >= 0x80 ? 'N' : '-', (vdi & V_FLAG) != 0 ? 'V' : '-', (vdi & D_FLAG) != 0 ? 'D' : '-',
			(vdi & I_FLAG) != 0 ? 'I' : '-', (nz & 0xff) == 0 ? 'Z' : '-', c != 0 ? 'C' : '-');
		show_instruction(ast, pc);
	}
	if (pc != 0xd20a) /* don't count 0xd2 used by call_6502() */
		cpu_opcodes[dGetByte(pc)] |= CPU_OPCODE_USED;
}

static void print_unofficial_mnemonic(int opcode)
{
	const char *mnemonic = cpu_mnemonics[opcode];
	const char *p;
	for (p = mnemonic + 3; *p != '\0'; p++) {
		if (*p == '1') {
			printf("%02X: %.*s$xx%s\n", opcode, (int) (p - mnemonic), mnemonic, p + 1);
			return;
		}
		if (*p == '2') {
			printf("%02X: %.*s$xxxx%s\n", opcode, (int) (p - mnemonic), mnemonic, p + 1);
			return;
		}
		/* no undocumented branches ('0') */
	}
	printf("%02X: %s\n", opcode, mnemonic);
}

static void print_help(void)
{
	printf(
		"Usage: asapscan COMMAND [OPTIONS] INPUTFILE\n"
		"Commands:\n"
		"-d  Dump POKEY registers\n"
		"-f  List POKEY features used\n"
		"-t  Detect silence and loops\n"
		"-c  Dump 6502 trace\n"
		"-u  List unofficial 6502 instructions used\n"
		"-v  Display version information\n"
		"Options:\n"
		"-s SONG  Process the specified subsong (zero-based)\n"
	);
}

static abool store_pokey(byte *p, PokeyState *pst)
{
	abool is_silence = TRUE;
#define STORE_CHANNEL(ch) \
	if ((pst->audc##ch & 0xf) != 0) { \
		is_silence = FALSE; \
		p[ch * 2 - 2] = pst->audf##ch; \
		p[ch * 2 - 1] = pst->audc##ch; \
	} \
	else { \
		p[ch * 2 - 2] = 0; \
		p[ch * 2 - 1] = 0; \
	}
	STORE_CHANNEL(1)
	STORE_CHANNEL(2)
	STORE_CHANNEL(3)
	STORE_CHANNEL(4)
	p[8] = pst->audctl;
	return is_silence;
}

static void print_pokey(PokeyState *pst)
{
	printf(
		"%02X %02X  %02X %02X  %02X %02X  %02X %02X  %02X",
		pst->audf1, pst->audc1, pst->audf2, pst->audc2,
		pst->audf3, pst->audc3, pst->audf4, pst->audc4, pst->audctl
	);
}

static int seconds_to_player_calls(int seconds)
{
	return (int) (seconds * 1773447.0 / 114.0 / asap.module_info.fastplay);
}

static int player_calls_to_milliseconds(int player_calls)
{
	return (int) ceil(player_calls * asap.module_info.fastplay * 114.0 * 1000 / 1773447.0);
}

void scan_song(int song)
{
	int i;
	int silence_run = 0;
	int loop_bytes = 18 * loop_check_player_calls;
	ASAP_PlaySong(&asap, song, -1);
	for (i = 0; i < scan_player_calls; i++) {
		call_6502_player(&asap);
		if (dump) {
			printf("%6.2f: ", i * asap.module_info.fastplay * 114.0 / 1773447.0);
			print_pokey(&asap.base_pokey);
			if (asap.module_info.channels == 2) {
				printf("  |  ");
				print_pokey(&asap.extra_pokey);
			}
			printf("\n");
		}
		if (features != 0) {
			int c1 = asap.base_pokey.audctl;
			int c2 = asap.extra_pokey.audctl;
			if (((c1 | c2) & 1) != 0)
				features |= FEATURE_15_KHZ;
			if (((c1 | c2) & 6) != 0)
				features |= FEATURE_HIPASS_FILTER;
			if (((c1 & 0x40) != 0 && (asap.base_pokey.audc1 & 0xf) != 0)
			|| ((c1 & 0x20) != 0 && (asap.base_pokey.audc3 & 0xf) != 0))
				features |= FEATURE_LOW_OF_16_BIT;
			if (((c1 | c2) & 0x80) != 0)
				features |= FEATURE_9_BIT_POLY;
		}
		if (detect_time) {
			byte *p = registers_dump + 18 * i;
			abool is_silence = store_pokey(p, &asap.base_pokey);
			is_silence &= store_pokey(p + 9, &asap.extra_pokey);
			if (is_silence) {
				silence_run++;
				if (silence_run >= silence_player_calls && /* do not trigger at the initial silence */ silence_run < i) {
					int duration = player_calls_to_milliseconds(i + 1 - silence_run);
					printf("TIME %02d:%02d.%02d\n", duration / 60000, duration / 1000 % 60, duration / 10 % 100);
					return;
				}
			}
			else
				silence_run = 0;
			if (i > loop_check_player_calls) {
				byte *q;
				if (memcmp(p - loop_bytes - 18, p - loop_bytes, loop_bytes) == 0) {
					/* POKEY registers do not change - probably an ultrasound */
					int duration = player_calls_to_milliseconds(i - loop_check_player_calls);
					printf("TIME %02d:%02d.%02d\n", duration / 60000, duration / 1000 % 60, duration / 10 % 100);
					return;
				}
				for (q = registers_dump; q < p - loop_bytes - 18 * loop_min_player_calls; q += 18) {
					if (memcmp(q, p - loop_bytes, loop_bytes) == 0) {
						int duration = player_calls_to_milliseconds(i - loop_check_player_calls);
						printf("TIME %02d:%02d.%02d LOOP\n", duration / 60000, duration / 1000 % 60, duration / 10 % 100);
						return;
					}
				}
			}
		}
	}
	if (detect_time)
		printf("No silence or loop detected in song %d\n", song);
}

int main(int argc, char *argv[])
{
	int i;
	const char *input_file = NULL;
	int song = -1;
	int scan_seconds = 15 * 60;
	int silence_seconds = 5;
	int loop_check_seconds = 3 * 60;
	int loop_min_seconds = 5;
	FILE *fp;
	static byte module[ASAP_MODULE_MAX];
	int module_len;
	
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-d") == 0)
			dump = TRUE;
		else if (strcmp(argv[i], "-f") == 0)
			features = FEATURE_CHECK;
		else if (strcmp(argv[i], "-t") == 0)
			detect_time = TRUE;
		else if (strcmp(argv[i], "-c") == 0)
			cpu_trace |= CPU_TRACE_PRINT;
		else if (strcmp(argv[i], "-u") == 0)
			cpu_trace |= CPU_TRACE_UNOFFICIAL;
		else if (strcmp(argv[i], "-s") == 0)
			song = atoi(argv[++i]);
		else if (strcmp(argv[i], "-v") == 0) {
			printf("asapscan " ASAP_VERSION "\n");
			return 0;
		}
		else {
			if (input_file != NULL) {
				print_help();
				return 1;
			}
			input_file = argv[i];
		}
	}
	if (dump + features + detect_time + cpu_trace == 0 || input_file == NULL) {
		print_help();
		return 1;
	}
	fp = fopen(input_file, "rb");
	if (fp == NULL) {
		fprintf(stderr, "asapscan: cannot open %s\n", input_file);
		return 1;
	}
	module_len = fread(module, 1, sizeof(module), fp);
	fclose(fp);
	if (!ASAP_Load(&asap, input_file, module, module_len)) {
		fprintf(stderr, "asapscan: %s: format not supported\n", input_file);
		return 1;
	}
	scan_player_calls = seconds_to_player_calls(scan_seconds);
	silence_player_calls = seconds_to_player_calls(silence_seconds);
	loop_check_player_calls = seconds_to_player_calls(loop_check_seconds);
	loop_min_player_calls = seconds_to_player_calls(loop_min_seconds);
	registers_dump = malloc(scan_player_calls * 18);
	if (registers_dump == NULL) {
		fprintf(stderr, "asapscan: out of memory\n");
		return 1;
	}
	if (song >= 0)
		scan_song(song);
	else
		for (song = 0; song < asap.module_info.songs; song++)
			scan_song(song);
	free(registers_dump);
	if (features != 0) {
		if ((features & FEATURE_15_KHZ) != 0)
			printf("15 kHz clock\n");
		if ((features & FEATURE_HIPASS_FILTER) != 0)
			printf("Hi-pass filter\n");
		if ((features & FEATURE_LOW_OF_16_BIT) != 0)
			printf("Low byte of 16-bit counter\n");
		if ((features & FEATURE_9_BIT_POLY) != 0)
			printf("9-bit poly\n");
	}
	if ((cpu_trace & CPU_TRACE_UNOFFICIAL) != 0) {
		for (i = 0; i < 256; i++) {
			if (cpu_opcodes[i] == (CPU_OPCODE_UNOFFICIAL | CPU_OPCODE_USED))
				print_unofficial_mnemonic(i);
		}                             
	}
	return 0;
}
