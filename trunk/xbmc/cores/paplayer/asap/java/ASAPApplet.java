/*
 * ASAPApplet.java - ASAP applet
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

import java.applet.Applet;
import java.awt.Color;
import java.awt.Graphics;
import java.io.InputStream;
import java.io.IOException;
import java.net.URL;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.LineUnavailableException;
import javax.sound.sampled.SourceDataLine;
import netscape.javascript.JSObject;

import net.sf.asap.ASAP;
import net.sf.asap.ASAP_ModuleInfo;

public class ASAPApplet extends Applet implements Runnable
{
	private final ASAP asap = new ASAP();
	private int song;
	private SourceDataLine line;
	private boolean running;
	private boolean paused;

	private static final int BITS_PER_SAMPLE = 16;
	private int defaultPlaybackTime = -1;
	private int loopPlaybackTime = -1;
	private static final int ONCE = -2; // for loopPlaybackTime
	private Color background;
	private Color foreground;

	public void update(Graphics g)
	{
		if (running)
			paint(g);
		else
			super.update(g);
	}

	public void paint(Graphics g)
	{
		if (!running)
			return;
		int channels = 4 * asap.getModuleInfo().channels;
		int channelWidth = getWidth() / channels;
		int totalHeight = getHeight();
		int unitHeight = totalHeight / 15;
		for (int i = 0; i < channels; i++) {
			int height = asap.getPokeyChannelVolume(i) * unitHeight;
			g.setColor(background);
			g.fillRect(i * channelWidth, 0, channelWidth, totalHeight - height);
			g.setColor(foreground);
			g.fillRect(i * channelWidth, totalHeight - height, channelWidth, height);
		}
	/*
		ASAP_ModuleInfo module_info = asap.getModuleInfo();
		g.drawString("Author: " + module_info.author, 10, 20);
		g.drawString("Name: " + module_info.name, 10, 40);
		g.drawString("Date: " + module_info.date, 10, 60);
		if (module_info.songs > 1)
			g.drawString("Song " + (song + 1) + " of " + module_info.songs, 10, 80);
	*/
	}

	public void run()
	{
		byte[] buffer = new byte[8192];
		int len;
		do {
			synchronized (asap) {
				len = asap.generate(buffer, BITS_PER_SAMPLE);
			}
			synchronized (this) {
				while (paused) {
					try {
						wait();
					} catch (InterruptedException e) {
						return;
					}
				}
			}
			line.write(buffer, 0, len);
			repaint();
		} while (len == buffer.length && running);
		if (running) {
			String js = getParameter("onPlaybackEnd");
			if (js != null)
				JSObject.getWindow(this).eval(js);
			running = false;
		}
		repaint();
	}

	public void setPlaybackTime(String defaultPlaybackTime, String loopPlaybackTime)
	{
		this.defaultPlaybackTime = ASAP.parseDuration(defaultPlaybackTime);
		if ("ONCE".equals(loopPlaybackTime))
			this.loopPlaybackTime = ONCE;
		else
			this.loopPlaybackTime = ASAP.parseDuration(loopPlaybackTime);
	}

	public void play(String filename, int song)
	{
		byte[] module;
		int module_len = 0;
		try {
			InputStream is = new URL(getDocumentBase(), filename).openStream();
			module = new byte[ASAP.MODULE_MAX];
			for (;;) {
				int i = is.read(module, module_len, ASAP.MODULE_MAX - module_len);
				if (i <= 0)
					break;
				module_len += i;
			}
			is.close();
		} catch (IOException e) {
			showStatus("ERROR LOADING " + filename);
			return;
		}
		ASAP_ModuleInfo module_info;
		synchronized (asap) {
			asap.load(filename, module, module_len);
			module_info = asap.getModuleInfo();
			if (song < 0)
				song = module_info.default_song;
			int duration = module_info.durations[song];
			if (duration < 0)
				duration = defaultPlaybackTime;
			else if (module_info.loops[song] && loopPlaybackTime != ONCE)
				duration = loopPlaybackTime;
			asap.playSong(song, duration);
		}
		AudioFormat format = new AudioFormat(ASAP.SAMPLE_RATE, BITS_PER_SAMPLE, module_info.channels, BITS_PER_SAMPLE != 8, false);
		try {
			line = (SourceDataLine) AudioSystem.getLine(new DataLine.Info(SourceDataLine.class, format));
			line.open(format);
		} catch (LineUnavailableException e) {
			showStatus("ERROR OPENING AUDIO");
			return;
		}
		line.start();
		if (!running) {
			running = true;
			new Thread(this).start();
		}
		synchronized (this) {
			paused = false;
			notify();
		}
		repaint();
	}

	private Color getColor(String parameter, Color defaultColor)
	{
		String s = getParameter(parameter);
		if (s == null || s.length() == 0)
			return defaultColor;
		if (s.charAt(0) == '#')
			s = s.substring(1);
		return new Color(Integer.parseInt(s, 16));
	}

	public void start()
	{
		setPlaybackTime(getParameter("defaultPlaybackTime"), getParameter("loopPlaybackTime"));
		background = getColor("background", Color.BLACK);
		setBackground(background);
		foreground = getColor("foreground", Color.GREEN);
		String filename = getParameter("file");
		if (filename == null)
			return;
		int song = -1;
		String s = getParameter("song");
		if (s == null || s.length() == 0)
			song = -1;
		else
			song = Integer.parseInt(s);
		play(filename, song);
	}

	public void stop()
	{
		running = false;
	}

	public synchronized boolean togglePause()
	{
		paused = !paused;
		if (!paused)
			notify();
		return paused;
	}

	public String getAuthor()
	{
		return asap.getModuleInfo().author;
	}

	public String getName()
	{
		return asap.getModuleInfo().name;
	}

	public String getDate()
	{
		return asap.getModuleInfo().date;
	}
}
