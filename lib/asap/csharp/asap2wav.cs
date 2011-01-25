/*
 * asap2wav.cs - converter of ASAP-supported formats to WAV files
 *
 * Copyright (C) 2008-2009  Piotr Fusik
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

using System;
using System.IO;

using ASAP;

public class asap2wav
{
	static string outputFilename = null;
	static bool outputHeader = true;
	static int song = -1;
	static ASAP_SampleFormat format = ASAP_SampleFormat.S16LE;
	static int duration = -1;
	static int muteMask = 0;

	static void PrintHelp()
	{
		Console.Write(
			"Usage: asap2wav [OPTIONS] INPUTFILE...\n" +
			"Each INPUTFILE must be in a supported format:\n" +
			"SAP, CMC, CM3, CMR, CMS, DMC, DLT, MPT, MPD, RMT, TMC, TM8 or TM2.\n" +
			"Options:\n" +
			"-o FILE     --output=FILE      Set output file name\n" +
			"-s SONG     --song=SONG        Select subsong number (zero-based)\n" +
			"-t TIME     --time=TIME        Set output length (MM:SS format)\n" +
			"-b          --byte-samples     Output 8-bit samples\n" +
			"-w          --word-samples     Output 16-bit samples (default)\n" +
			"            --raw              Output raw audio (no WAV header)\n" +
			"-m CHANNELS --mute=CHANNELS    Mute POKEY chanels (1-8)\n" +
			"-h          --help             Display this information\n" +
			"-v          --version          Display version information\n"
		);
	}

	static void SetSong(string s)
	{
		song = int.Parse(s);
	}

	static void SetTime(string s)
	{
		duration = ASAP_Player.ParseDuration(s);
	}

	static void SetMuteMask(string s)
	{
		int mask = 0;
		foreach (char c in s) {
			if (c >= '1' && c <= '8')
				mask |= 1 << (c - '1');
		}
		muteMask = mask;
	}

	static void ProcessFile(string inputFilename)
	{
		Stream s = File.OpenRead(inputFilename);
		byte[] module = new byte[ASAP_Player.ModuleMax];
		int module_len = s.Read(module, 0, module.Length);
		s.Close();
		ASAP_Player asap = new ASAP_Player();
		asap.Load(inputFilename, module, module_len);
		ASAP_ModuleInfo module_info = asap.GetModuleInfo();
		if (song < 0)
			song = module_info.default_song;
		if (duration < 0) {
			duration = module_info.durations[song];
			if (duration < 0)
				duration = 180 * 1000;
		}
		asap.PlaySong(song, duration);
		asap.MutePokeyChannels(muteMask);
		if (outputFilename == null) {
			int i = inputFilename.LastIndexOf('.');
			outputFilename = inputFilename.Substring(0, i + 1) + (outputHeader ? "wav" : "raw");
		}
		s = File.Create(outputFilename);
		byte[] buffer = new byte[8192];
		if (outputHeader) {
			asap.GetWavHeader(buffer, format);
			s.Write(buffer, 0, ASAP_Player.WavHeaderBytes);
		}
		int n_bytes;
		do {
			n_bytes = asap.Generate(buffer, format);
			s.Write(buffer, 0, n_bytes);
		} while (n_bytes == buffer.Length);
		s.Close();
		outputFilename = null;
		song = -1;
		duration = -1;
	}

	public static int Main(string[] args)
	{
		bool noInputFiles = true;
		for (int i = 0; i < args.Length; i++) {
			string arg = args[i];
			if (arg[0] != '-') {
				ProcessFile(arg);
				noInputFiles = false;
			}
			else if (arg == "-o")
				outputFilename = args[++i];
			else if (arg.StartsWith("--output="))
				outputFilename = arg.Substring(9);
			else if (arg == "-s")
				SetSong(args[++i]);
			else if (arg.StartsWith("--song="))
				SetSong(arg.Substring(7));
			else if (arg == "-t")
				SetTime(args[++i]);
			else if (arg.StartsWith("--time="))
				SetTime(arg.Substring(7));
			else if (arg == "-b" || arg == "--byte-samples")
				format = ASAP_SampleFormat.U8;
			else if (arg == "-w" || arg == "--word-samples")
				format = ASAP_SampleFormat.S16LE;
			else if (arg == "--raw")
				outputHeader = false;
			else if (arg == "-m")
				SetMuteMask(args[++i]);
			else if (arg.StartsWith("--mute="))
				SetMuteMask(arg.Substring(7));
			else if (arg == "-h" || arg == "--help") {
				PrintHelp();
				noInputFiles = false;
			}
			else if (arg == "-v" || arg == "--version") {
				Console.WriteLine("ASAP2WAV (.NET) " + ASAP_Player.Version);
				noInputFiles = false;
			}
			else
				throw new ArgumentException("unknown option: " + arg);
		}
		if (noInputFiles) {
			PrintHelp();
			return 1;
		}
		return 0;
	}
}
