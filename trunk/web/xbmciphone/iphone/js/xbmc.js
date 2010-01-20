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

addEventListener("load", initXbmcHttp, false);

function initXbmcHttp() {
	// reset then set the response format
	xbmcHttpSimple("SetResponseFormat", function() {
		xbmcHttpSimple("SetResponseFormat(webheader;false;webfooter;false;opentag;)", function() {
			//alert("Response Format Set");
		});
	});
}

function xbmcForm() {
	xbmcCmds("xbmcForm", arguments);
}

function xbmcHttp() {
	xbmcCmds("xbmcHttp", arguments);
}

function xbmcHttpSimple(command, handler) {
	xbmcHttp(command, function(req) {
		if (req.readyState == 4) {
	        if (req.status == 200) {
				if (req.responseText.substring(0, 5) == "Error")
					alert(req.responseText);
				else
					if (handler) handler(req);
	        } else {
	            //alert("Error: " + req.statusText);
	        }
	    }
	});
}

function xbmcCmds(type, args) {
	// build the command string:
	var functionCall = args[0];
	for (var i = 1; i < args.length-1; i++)
		functionCall += ((i==1)?"(":";") + args[i] + ((i==args.length-2)?")":"");
		
	// build the url:
	var url = "http://"+window.location.host+"/xbmcCmds/"+type+"?command="+functionCall;
	
	loadUrl(url, args[args.length-1]);
}

function loadUrl(url, statechangehandler) {
	var req = xhrFactory();
	if (req) {
		req.onreadystatechange = function() { if (statechangehandler != null) statechangehandler(req); };
		req.open("GET", url, true);
		req.send("");
	}
}


function xhrFactory() {
	var req = false;
    // branch for native XMLHttpRequest object
    if(window.XMLHttpRequest && !(window.ActiveXObject)) {
    	try {
			req = new XMLHttpRequest();
        } catch(e) {
			req = false;
        }
    // branch for IE/Windows ActiveX version
    } else if(window.ActiveXObject) {
       	try {
        	req = new ActiveXObject("Msxml2.XMLHTTP");
      	} catch(e) {
        	try {
          		req = new ActiveXObject("Microsoft.XMLHTTP");
        	} catch(e) {
          		req = false;
        	}
		}
    }
	return req;
}

function formatSize(original) {
	var size = parseInt(original);
	var units = ["B", "KB", "MB", "GB", "TB"];
	var unitNum = 0;
	while (size > 1024) {
		size /= 1024;
		unitNum++;
	}
	return (Math.round(size*100)/100) + " " + units[unitNum];
}
