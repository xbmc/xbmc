/*
 * ASAP2WAV.java - converter of ASAP-supported formats to WAV files
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

import java.io.*;
import net.sf.asap.ASAP;
import net.sf.asap.ASAP_ModuleInfo;

public class ASAP2WAV
{
	private static String outputFilename = null;
	private static boolean outputHeader = true;
	private static int song = -1;
	private static int format = ASAP.FORMAT_S16_LE;
	private static int duration = -1;
	private static int muteMask = 0;

	private static void printHelp()
	{
		System.out.print(
			"Usage: java -jar asap2wav.jar [OPTIONS] INPUTFILE...\n" +
			"Each INPUTFILE must be in a supported format:\n" +
			"SAP, CMC, CM3, CMR, CMS, DMC, DLT, MPT, MPD, RMT, TMC, TM8 or TM2.\n" +
			"Options:\n" +
			"-o FILE     --output=FILE      Set output file name\n" +
			"-o -        --output=-         Write to standard output\n" +
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

	private static void setSong(String s)
	{
		song = Integer.parseInt(s);
	}

	private static void setTime(String s)
	{
		duration = ASAP.parseDuration(s);
	}

	private static void setMuteMask(String s)
	{
		int mask = 0;
		for (int i = 0; i < s.length(); i++) {
			int c = s.charAt(i);
			if (c >= '1' && c <= '8')
				mask |= 1 << (c - '1');
		}
		muteMask = mask;
	}

	private static void processFile(String inputFilename) throws IOException
	{
		InputStream is = new FileInputStream(inputFilename);
		byte[] module = new byte[ASAP.MODULE_MAX];
		int module_len = is.read(module);
		is.close();
		ASAP asap = new ASAP();
		asap.load(inputFilename, module, module_len);
		ASAP_ModuleInfo module_info = asap.getModuleInfo();
		if (song < 0)
			song = module_info.default_song;
		if (duration < 0) {
			duration = module_info.durations[song];
			if (duration < 0)
				duration = 180 * 1000;
		}
		asap.playSong(song, duration);
		asap.mutePokeyChannels(muteMask);
		if (outputFilename == null) {
			int i = inputFilename.lastIndexOf('.');
			outputFilename = inputFilename.substring(0, i + 1) + (outputHeader ? "wav" : "raw");
		}
		OutputStream os;
		if (outputFilename.equals("-"))
			os = System.out;
		else
			os = new FileOutputStream(outputFilename);
		byte[] buffer = new byte[8192];
		if (outputHeader) {
			asap.getWavHeader(buffer, format);
			os.write(buffer, 0, ASAP.WAV_HEADER_BYTES);
		}
		int n_bytes;
		do {
			n_bytes = asap.generate(buffer, format);
			os.write(buffer, 0, n_bytes);
		} while (n_bytes == buffer.length);
		os.close();
		outputFilename = null;
		song = -1;
		duration = -1;
	}

	public static void main(String[] args) throws IOException
	{
		boolean noInputFiles = true;
		for (int i = 0; i < args.length; i++) {
			String arg = args[i];
			if (arg.charAt(0) != '-') {
				processFile(arg);
				noInputFiles = false;
			}
			else if (arg.equals("-o"))
				outputFilename = args[++i];
			else if (arg.startsWith("--output="))
				outputFilename = arg.substring(9);
			else if (arg.equals("-s"))
				setSong(args[++i]);
			else if (arg.startsWith("--song="))
				setSong(arg.substring(7));
			else if (arg.equals("-t"))
				setTime(args[++i]);
			else if (arg.startsWith("--time="))
				setTime(arg.substring(7));
			else if (arg.equals("-b") || arg.equals("--byte-samples"))
				format = ASAP.FORMAT_U8;
			else if (arg.equals("-w") || arg.equals("--word-samples"))
				format = ASAP.FORMAT_S16_LE;
			else if (arg.equals("--raw"))
				outputHeader = false;
			else if (arg.equals("-m"))
				setMuteMask(args[++i]);
			else if (arg.startsWith("--mute="))
				setMuteMask(arg.substring(7));
			else if (arg.equals("-h") || arg.equals("--help")) {
				printHelp();
				noInputFiles = false;
			}
			else if (arg.equals("-v") || arg.equals("--version")) {
				System.out.println("ASAP2WAV (Java) " + ASAP.VERSION);
				noInputFiles = false;
			}
			else
				throw new IllegalArgumentException("unknown option: " + arg);
		}
		if (noInputFiles) {
			printHelp();
			System.exit(1);
		}
	}
}
