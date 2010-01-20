/*
 * ASAPPlayer.java - ASAP Flash player
 *
 * Copyright (C) 2009  Piotr Fusik
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

package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.events.SampleDataEvent;
	import flash.external.ExternalInterface;
	import flash.media.Sound;
	import flash.media.SoundChannel;
	import flash.net.URLLoader;
	import flash.net.URLLoaderDataFormat;
	import flash.net.URLRequest;
	import flash.utils.ByteArray;

	public class ASAPPlayer extends Sprite
	{
		private static const ONCE : int = -2;
		private var defaultPlaybackTime : int = -1;
		private var loopPlaybackTime : int = -1;

		private var filename : String;
		private var song : int;

		private var soundChannel : SoundChannel = null;

		public function setPlaybackTime(defaultPlaybackTime : String, loopPlaybackTime : String = null) : void
		{
			this.defaultPlaybackTime = ASAP.parseDuration(defaultPlaybackTime);
			if (loopPlaybackTime == "ONCE")
				this.loopPlaybackTime = ONCE;
			else
				this.loopPlaybackTime = ASAP.parseDuration(loopPlaybackTime);
		}

		private function completeHandler(event : Event) : void
		{
			var module : ByteArray = URLLoader(event.target).data;

			var asap : ASAP = new ASAP();
			asap.load(filename, module);
			var song : int = this.song;
			if (song < 0)
				song = asap.moduleInfo.default_song;
			var duration : int = asap.moduleInfo.durations[song];
			if (duration < 0)
				duration = this.defaultPlaybackTime;
			else if (asap.moduleInfo.loops[song] && this.loopPlaybackTime != ONCE)
				duration = this.loopPlaybackTime;
			asap.playSong(song, duration);

			var sound : Sound = new Sound();
			function generator(event : SampleDataEvent) : void
			{
				asap.generate(event.data, 8192);
			}
			sound.addEventListener(SampleDataEvent.SAMPLE_DATA, generator);
			this.soundChannel = sound.play();
		}

		public function play(filename : String, song : int = -1) : void
		{
			this.filename = filename;
			this.song = song;
			var loader : URLLoader = new URLLoader();
			loader.dataFormat = URLLoaderDataFormat.BINARY;
			loader.addEventListener(Event.COMPLETE, completeHandler);
			loader.load(new URLRequest(filename));
		}

		public function stop() : void
		{
			if (this.soundChannel != null)
			{
				this.soundChannel.stop();
				this.soundChannel = null;
			}
		}

		public function ASAPPlayer()
		{
			ExternalInterface.addCallback("setPlaybackTime", setPlaybackTime);
			ExternalInterface.addCallback("asapPlay", play);
			ExternalInterface.addCallback("asapStop", stop);
			var parameters : Object = this.loaderInfo.parameters;
			setPlaybackTime(parameters.defaultPlaybackTime, parameters.loopPlaybackTime);
			if (parameters.file != null)
				play(parameters.file, parameters.song);
		}
	}
}
