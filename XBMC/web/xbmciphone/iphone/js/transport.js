/*
 * Copyright (c) 2007, Thomas Robinson
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of the tlrobinson.net nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
addEventListener("load", initTransport, false);

function $(id) { return document.getElementById(id); }

var current_pct = 0.0;

var transportbg, volume_slider, volume_knob, blue_fill, white_fill;

function initTransport() {
	volume_slider	= $("volume_slider");
	volume_knob		= $("volume_knob");
	white_fill		= $("white_fill");
	blue_fill		= $("blue_fill");
	transportbg		= $("transportbg");
	
	$("volume_slider").addEventListener("click", function(e) {
		xbmcHttpSimple("SetVolume("+Math.round(clientXToPct(e.clientX)*100)+")", updateTransportControls);
	}, false);
	$("btn_play").addEventListener("click", function(e) {
		xbmcForm("pause", updateTransportControls);
	}, false);
	$("btn_next").addEventListener("click", function(e) {
		xbmcForm("next", updateTransportControls);
	}, false);
	$("btn_prev").addEventListener("click", function(e) {
		xbmcForm("previous", updateTransportControls);
	}, false);
	
	setTimeout(updateTransportControls, 0);
	setInterval(updateTransportControls, 2000);
}

function updateTransportControls() {
	xbmcHttpSimple("GetVolume", function(req) {
		updateSlider(parseFloat(req.responseText)/100.0);
	});
	xbmcHttpSimple("GetCurrentlyPlaying", function(req) {
		var playing = false;
		if (req.responseText.substring(0, 26) == "Filename:[Nothing Playing]") {
			//alert("stopped");
		} else if (req.responseText.indexOf("PlayStatus:Playing") > 0) {
			//alert("playing");
			playing = true;
		} else if (req.responseText.indexOf("PlayStatus:Paused") > 0) {
			//alert("paused");
		} else {
			//alert("unknown");
		}
		updatePlayButton(playing);
	});
}

function updatePlayButton(playing) {
	$("btn_play").src = (playing)?"images/pause.png":"images/play.png";
}

function clientXToPct(x) {
	var transport = document.getElementById("transport");
	
	var bg_left = document.defaultView.getComputedStyle(transportbg, null).getPropertyValue("left");
	var transport_left = document.defaultView.getComputedStyle(transport, null).getPropertyValue("left");
	var volume_slider_left = document.defaultView.getComputedStyle(volume_slider, null).getPropertyValue("left");
	var volume_slider_width = document.defaultView.getComputedStyle(volume_slider, null).getPropertyValue("width");
	
	var pix = x - parseInt(volume_slider_left) - parseInt(transport_left) - parseInt(bg_left);
	if (pix < 0) pix = 0;
	return pix/parseFloat(volume_slider_width);
}

function updateSlider(pct) {
	var left = Math.round(264.0*pct);
	var right = 264 - left;
	
	white_fill.style.width = right + "px";
	blue_fill.style.width = left + "px";
	
	volume_knob.style.left = (10+left-11) + "px";
}
