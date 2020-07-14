/**
 * videojs-chromecast
 * @version 2.0.8
 * @copyright 2016 Benjipott, Inc.
 * @license Apache-2.0
 */
(function(f){if(typeof exports==="object"&&typeof module!=="undefined"){module.exports=f()}else if(typeof define==="function"&&define.amd){define([],f)}else{var g;if(typeof window!=="undefined"){g=window}else if(typeof global!=="undefined"){g=global}else if(typeof self!=="undefined"){g=self}else{g=this}(g.video || (g.video = {})).jsChromecast = f()}})(function(){var define,module,exports;return (function e(t,n,r){function s(o,u){if(!n[o]){if(!t[o]){var a=typeof require=="function"&&require;if(!u&&a)return a(o,!0);if(i)return i(o,!0);var f=new Error("Cannot find module '"+o+"'");throw f.code="MODULE_NOT_FOUND",f}var l=n[o]={exports:{}};t[o][0].call(l.exports,function(e){var n=t[o][1][e];return s(n?n:e)},l,l.exports,e,t,n,r)}return n[o].exports}var i=typeof require=="function"&&require;for(var o=0;o<r.length;o++)s(r[o]);return s})({1:[function(require,module,exports){
(function (global){
/**
 * @file chromecast-button.js
 */
'use strict';

Object.defineProperty(exports, '__esModule', {
    value: true
});

var _createClass = (function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ('value' in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; })();

var _get = function get(_x, _x2, _x3) { var _again = true; _function: while (_again) { var object = _x, property = _x2, receiver = _x3; _again = false; if (object === null) object = Function.prototype; var desc = Object.getOwnPropertyDescriptor(object, property); if (desc === undefined) { var parent = Object.getPrototypeOf(object); if (parent === null) { return undefined; } else { _x = parent; _x2 = property; _x3 = receiver; _again = true; desc = parent = undefined; continue _function; } } else if ('value' in desc) { return desc.value; } else { var getter = desc.get; if (getter === undefined) { return undefined; } return getter.call(receiver); } } };

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { 'default': obj }; }

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError('Cannot call a class as a function'); } }

function _inherits(subClass, superClass) { if (typeof superClass !== 'function' && superClass !== null) { throw new TypeError('Super expression must either be null or a function, not ' + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; }

var _videoJs = (typeof window !== "undefined" ? window['videojs'] : typeof global !== "undefined" ? global['videojs'] : null);

var _videoJs2 = _interopRequireDefault(_videoJs);

var Component = _videoJs2['default'].getComponent('Component');
var ControlBar = _videoJs2['default'].getComponent('ControlBar');
var Button = _videoJs2['default'].getComponent('Button');

/**
 * The base class for buttons that toggle chromecast video
 *
 * @param {Player|Object} player
 * @param {Object=} options
 * @extends Button
 * @class ChromeCastButton
 */

var ChromeCastButton = (function (_Button) {
    _inherits(ChromeCastButton, _Button);

    function ChromeCastButton(player, options) {
        _classCallCheck(this, ChromeCastButton);

        _get(Object.getPrototypeOf(ChromeCastButton.prototype), 'constructor', this).call(this, player, options);
        this.hide();
        this.initializeApi();
        options.appId = player.options_.chromecast.appId;
        player.chromecast = this;
    }

    /**
     * Init chromecast sdk api
     *
     * @method initializeApi
     */

    _createClass(ChromeCastButton, [{
        key: 'initializeApi',
        value: function initializeApi() {
            var apiConfig = undefined;
            var appId = undefined;
            var sessionRequest = undefined;

            if (!_videoJs2['default'].browser.IS_CHROME || _videoJs2['default'].browser.IS_EDGE) {
                return;
            }
            if (!chrome.cast || !chrome.cast.isAvailable) {
                console.log(chrome.cast);
                _videoJs2['default'].log('Cast APIs not available');
                if (this.tryingReconnect < 10) {
                    this.setTimeout(this.initializeApi, 1000);
                    ++this.tryingReconnect;
                }
                _videoJs2['default'].log('Cast APIs not available. Max reconnect attempt');
                return;
            }

            _videoJs2['default'].log('Cast APIs are available');
            appId = this.options_.appId || chrome.cast.media.DEFAULT_MEDIA_RECEIVER_APP_ID;
            sessionRequest = new chrome.cast.SessionRequest(appId);
            apiConfig = new chrome.cast.ApiConfig(sessionRequest, this.sessionJoinedListener.bind(this), this.receiverListener.bind(this));
            return chrome.cast.initialize(apiConfig, this.onInitSuccess.bind(this), this.castError.bind(this));
        }
    }, {
        key: 'castError',
        value: function castError(_castError) {

            var error = {
                code: _castError.code,
                message: _castError.description
            };

            switch (_castError.code) {
                case chrome.cast.ErrorCode.API_NOT_INITIALIZED:
                case chrome.cast.ErrorCode.EXTENSION_MISSING:
                case chrome.cast.ErrorCode.EXTENSION_NOT_COMPATIBLE:
                case chrome.cast.ErrorCode.INVALID_PARAMETER:
                case chrome.cast.ErrorCode.LOAD_MEDIA_FAILED:
                case chrome.cast.ErrorCode.RECEIVER_UNAVAILABLE:
                case chrome.cast.ErrorCode.SESSION_ERROR:
                case chrome.cast.ErrorCode.CHANNEL_ERROR:
                case chrome.cast.ErrorCode.TIMEOUT:
                    this.addClass('error');
                    break;
                case chrome.cast.ErrorCode.CANCEL:
                    break;
                default:
                    this.player_.error(error);
                    break;
            }
            return _videoJs2['default'].log('Cast Error: ' + JSON.stringify(_castError));
        }
    }, {
        key: 'onInitSuccess',
        value: function onInitSuccess() {
            return this.apiInitialized = true;
        }
    }, {
        key: 'sessionJoinedListener',
        value: function sessionJoinedListener(session) {
            if (session.media.length) {
                this.apiSession = session;
                this.onMediaDiscovered(session.media[0]);
            }
            return console.log('Session joined');
        }
    }, {
        key: 'receiverListener',
        value: function receiverListener(availability) {
            if (availability === 'available') {
                return this.show();
            }
        }
    }, {
        key: 'doLaunch',
        value: function doLaunch() {
            _videoJs2['default'].log('Cast video: ' + this.player_.cache_.src);
            if (this.apiInitialized) {
                return chrome.cast.requestSession(this.onSessionSuccess.bind(this), this.castError.bind(this));
            } else {
                return _videoJs2['default'].log('Session not initialized');
            }
        }
    }, {
        key: 'onSessionSuccess',
        value: function onSessionSuccess(session) {
            var image = undefined;
            var key = undefined;
            var loadRequest = undefined;
            var mediaInfo = undefined;
            var ref = undefined;
            var value = undefined;

            this.apiSession = session;
            var source = this.player_.cache_.src;
            var type = this.player_.currentType();

            _videoJs2['default'].log('Session initialized: ' + session.sessionId + ' source : ' + source + ' type : ' + type);

            mediaInfo = new chrome.cast.media.MediaInfo(source, type);
            mediaInfo.metadata = new chrome.cast.media.GenericMediaMetadata();
            if (this.options_.metadata) {
                ref = this.options_.metadata;
                for (key in ref) {
                    value = ref[key];
                    mediaInfo.metadata[key] = value;
                }
            }
            //Add poster image on player
            var poster = this.player().poster();
            if (poster) {
                image = new chrome.cast.Image(poster);
                mediaInfo.metadata.images = [image];
            }

            // Load/Add caption tracks
            var plTracks = this.player().textTracks();
            var remotePlTracks = this.player().remoteTextTrackEls();
            var tracks = [];
            var i = 0;
            var remotePlTrack = undefined;
            var plTrack = undefined;
            var trackId = 0;
            var track = undefined;
            if (plTracks) {
                for (i = 0; i < plTracks.length; i++) {
                    plTrack = plTracks.tracks_[i];
                    remotePlTrack = remotePlTracks && remotePlTracks.trackElements_ && remotePlTracks.trackElements_[i];
                    trackId++;
                    track = new chrome.cast.media.Track(trackId, chrome.cast.media.TrackType.TEXT);
                    track.trackContentId = remotePlTrack ? remotePlTrack.src : 'caption_' + plTrack.language;
                    track.subtype = chrome.cast.media.TextTrackType.CAPTIONS;
                    track.name = plTrack.label;
                    track.language = plTrack.language;
                    track.customData = null;
                    tracks.push(track);
                }
                mediaInfo.textTrackStyle = new chrome.cast.media.TextTrackStyle();
                mediaInfo.textTrackStyle.foregroundColor = '#FFFFFF';
                mediaInfo.textTrackStyle.backgroundColor = '#00000060';
                mediaInfo.textTrackStyle.edgeType = chrome.cast.media.TextTrackEdgeType.DROP_SHADOW;
                mediaInfo.textTrackStyle.windowType = chrome.cast.media.TextTrackWindowType.ROUNDED_CORNERS;
            }
            // Load/Add audio tracks

            try {
                plTracks = this.player().audioTracks();
                if (plTracks) {
                    for (i = 0; i < plTracks.length; i++) {
                        plTrack = plTracks.tracks_[i];
                        trackId++;
                        track = new chrome.cast.media.Track(trackId, chrome.cast.media.TrackType.AUDIO);
                        track.subtype = null;
                        track.name = plTrack.label;
                        track.language = plTrack.language;
                        track.customData = null;
                        tracks.push(track);
                    }
                }
            } catch (e) {
                _videoJs2['default'].log('get player audioTracks fail' + e);
            }

            if (tracks.length) {
                mediaInfo.tracks = tracks;
            }

            // Request load media source
            loadRequest = new chrome.cast.media.LoadRequest(mediaInfo);

            loadRequest.autoplay = true;
            loadRequest.currentTime = this.player_.currentTime();

            this.apiSession.loadMedia(loadRequest, this.onMediaDiscovered.bind(this), this.castError.bind(this));
            this.apiSession.addUpdateListener(this.onSessionUpdate.bind(this));
        }
    }, {
        key: 'onMediaDiscovered',
        value: function onMediaDiscovered(media) {
            this.player_.loadTech_('Chromecast', {
                type: 'cast',
                apiMedia: media,
                apiSession: this.apiSession
            });

            this.casting = true;
            this.inactivityTimeout = this.player_.options_.inactivityTimeout;
            this.player_.options_.inactivityTimeout = 0;
            this.player_.userActive(true);
            this.addClass('connected');
            this.removeClass('error');
        }
    }, {
        key: 'onSessionUpdate',
        value: function onSessionUpdate(isAlive) {
            if (!this.player_.apiMedia) {
                return;
            }
            if (!isAlive) {
                return this.onStopAppSuccess();
            }
        }
    }, {
        key: 'stopCasting',
        value: function stopCasting() {
            return this.apiSession.stop(this.onStopAppSuccess.bind(this), this.castError.bind(this));
        }
    }, {
        key: 'onStopAppSuccess',
        value: function onStopAppSuccess() {
            this.casting = false;
            var time = this.player_.currentTime();
            this.removeClass('connected');
            this.player_.src(this.player_.options_['sources']);
            if (!this.player_.paused()) {
                this.player_.one('seeked', function () {
                    return this.player_.play();
                });
            }
            this.player_.currentTime(time);
            this.player_.options_.inactivityTimeout = this.inactivityTimeout;
            return this.apiSession = null;
        }

        /**
         * Allow sub components to stack CSS class names
         *
         * @return {String} The constructed class name
         * @method buildCSSClass
         */
    }, {
        key: 'buildCSSClass',
        value: function buildCSSClass() {
            return 'vjs-chromecast-button ' + _get(Object.getPrototypeOf(ChromeCastButton.prototype), 'buildCSSClass', this).call(this);
        }

        /**
         * Handle click on mute
         * @method handleClick
         */
    }, {
        key: 'handleClick',
        value: function handleClick() {
            _get(Object.getPrototypeOf(ChromeCastButton.prototype), 'handleClick', this).call(this);
            if (this.casting) {
                return this.stopCasting();
            } else {
                return this.doLaunch();
            }
        }
    }]);

    return ChromeCastButton;
})(Button);

ChromeCastButton.prototype.tryingReconnect = 0;

ChromeCastButton.prototype.inactivityTimeout = 2000;

ChromeCastButton.prototype.controlText_ = 'Chromecast';

//Replace videojs CaptionButton child with this one
ControlBar.prototype.options_.children.splice(ControlBar.prototype.options_.children.length - 1, 0, 'chromeCastButton');

Component.registerComponent('ChromeCastButton', ChromeCastButton);
exports['default'] = ChromeCastButton;
module.exports = exports['default'];
}).call(this,typeof global !== "undefined" ? global : typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {})
},{}],2:[function(require,module,exports){
(function (global){
/**
 * @file chromecast.js
 * Chromecast Media Controller - Wrapper for HTML5 Media API
 */
'use strict';

Object.defineProperty(exports, '__esModule', {
    value: true
});

var _createClass = (function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ('value' in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; })();

var _get = function get(_x2, _x3, _x4) { var _again = true; _function: while (_again) { var object = _x2, property = _x3, receiver = _x4; _again = false; if (object === null) object = Function.prototype; var desc = Object.getOwnPropertyDescriptor(object, property); if (desc === undefined) { var parent = Object.getPrototypeOf(object); if (parent === null) { return undefined; } else { _x2 = parent; _x3 = property; _x4 = receiver; _again = true; desc = parent = undefined; continue _function; } } else if ('value' in desc) { return desc.value; } else { var getter = desc.get; if (getter === undefined) { return undefined; } return getter.call(receiver); } } };

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { 'default': obj }; }

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError('Cannot call a class as a function'); } }

function _inherits(subClass, superClass) { if (typeof superClass !== 'function' && superClass !== null) { throw new TypeError('Super expression must either be null or a function, not ' + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; }

var _videoJs = (typeof window !== "undefined" ? window['videojs'] : typeof global !== "undefined" ? global['videojs'] : null);

var _videoJs2 = _interopRequireDefault(_videoJs);

var Component = _videoJs2['default'].getComponent('Component');
var Tech = _videoJs2['default'].getComponent('Tech');

/**
 * Chromecast Media Controller - Wrapper for HTML5 Media API
 *
 * @param {Object=} options Object of option names and values
 * @param {Function=} ready Ready callback function
 * @extends Tech
 * @class Chromecast
 */

var Chromecast = (function (_Tech) {
    _inherits(Chromecast, _Tech);

    function Chromecast(options, ready) {
        var _this = this;

        _classCallCheck(this, Chromecast);

        _get(Object.getPrototypeOf(Chromecast.prototype), 'constructor', this).call(this, options, ready);
        this.apiMedia = this.options_.source.apiMedia;
        this.apiSession = this.options_.source.apiSession;
        this.receiver = this.apiSession.receiver.friendlyName;

        var mediaStatusUpdateHandler = this.onMediaStatusUpdate.bind(this);
        var sessionUpdateHanlder = this.onSessionUpdate.bind(this);

        this.apiMedia.addUpdateListener(mediaStatusUpdateHandler);
        this.apiSession.addUpdateListener(sessionUpdateHanlder);

        this.on('dispose', function () {
            _this.apiMedia.removeUpdateListener(mediaStatusUpdateHandler);
            _this.apiSession.removeUpdateListener(sessionUpdateHanlder);
            _this.onMediaStatusUpdate();
            _this.onSessionUpdate(false);
        });

        var tracks = this.textTracks();
        if (tracks) {
            (function () {
                var changeHandler = _this.handleTextTracksChange.bind(_this);

                tracks.addEventListener('change', changeHandler);
                _this.on('dispose', function () {
                    tracks.removeEventListener('change', changeHandler);
                });

                _this.handleTextTracksChange();
            })();
        }

        try {
            tracks = this.audioTracks();
            if (tracks) {
                (function () {
                    var changeHandler = _this.handleAudioTracksChange.bind(_this);

                    tracks.addEventListener('change', changeHandler);
                    _this.on('dispose', function () {
                        tracks.removeEventListener('change', changeHandler);
                    });
                })();
            }
        } catch (e) {
            _videoJs2['default'].log('get player audioTracks fail' + e);
        }

        try {
            tracks = this.videoTracks();
            if (tracks) {
                (function () {
                    var changeHandler = _this.handleVideoTracksChange.bind(_this);

                    tracks.addEventListener('change', changeHandler);
                    _this.on('dispose', function () {
                        tracks.removeEventListener('change', changeHandler);
                    });
                })();
            }
        } catch (e) {
            _videoJs2['default'].log('get player videoTracks fail' + e);
        }

        this.update();
        this.triggerReady();
    }

    _createClass(Chromecast, [{
        key: 'createEl',
        value: function createEl() {
            var el = _videoJs2['default'].createEl('div', {
                id: this.options_.techId,
                className: 'vjs-tech vjs-tech-chromecast'
            });
            return el;
        }
    }, {
        key: 'update',
        value: function update() {
            this.el_.innerHTML = '<div class="casting-image" style="background-image: url(\'' + this.options_.poster + '\')"></div><div class="casting-overlay"><div class="casting-information"><div class="casting-icon"></div><div class="casting-description"><small>' + this.localize('CASTING TO') + '</small><br>' + this.receiver + '</div></div></div>';
        }
    }, {
        key: 'onSessionUpdate',
        value: function onSessionUpdate(isAlive) {
            if (!this.apiMedia) {
                return;
            }
            if (!isAlive) {
                return this.onStopAppSuccess();
            }
        }
    }, {
        key: 'onStopAppSuccess',
        value: function onStopAppSuccess() {
            this.stopTrackingCurrentTime();
            this.apiMedia = null;
        }
    }, {
        key: 'onMediaStatusUpdate',
        value: function onMediaStatusUpdate() {
            if (!this.apiMedia) {
                return;
            }
            switch (this.apiMedia.playerState) {
                case chrome.cast.media.PlayerState.BUFFERING:
                    this.trigger('waiting');
                    break;
                case chrome.cast.media.PlayerState.IDLE:
                    this.trigger('timeupdate');
                    break;
                case chrome.cast.media.PlayerState.PAUSED:
                    this.trigger('pause');
                    this.paused_ = true;
                    break;
                case chrome.cast.media.PlayerState.PLAYING:
                    this.trigger('playing');
                    this.trigger('play');
                    this.paused_ = false;
                    break;
            }
        }

        /**
         * Set video
         *
         * @param {Object=} src Source object
         * @method setSrc
         */
    }, {
        key: 'src',
        value: function src(_src) {
            //do nothing
        }
    }, {
        key: 'currentSrc',
        value: function currentSrc() {
            if (!this.apiMedia) {
                return;
            }
            return this.apiMedia.media.contentId;
        }
    }, {
        key: 'handleAudioTracksChange',
        value: function handleAudioTracksChange() {
            var trackInfo = [];
            var tTracks = this.textTracks();
            var tracks = this.audioTracks();

            if (!tracks) {
                return;
            }

            for (var i = 0; i < tracks.length; i++) {
                var track = tracks[i];
                if (track['enabled']) {
                    //set id of cuurentTrack audio
                    trackInfo.push(i + 1 + tTracks.length);
                }
            }

            if (this.apiMedia && trackInfo.length) {
                this.tracksInfoRequest = new chrome.cast.media.EditTracksInfoRequest(trackInfo);
                return this.apiMedia.editTracksInfo(this.tracksInfoRequest, this.onTrackSuccess.bind(this), this.onTrackError.bind(this));
            }
        }
    }, {
        key: 'handleVideoTracksChange',
        value: function handleVideoTracksChange() {}
    }, {
        key: 'handleTextTracksChange',
        value: function handleTextTracksChange() {
            var trackInfo = [];
            var tracks = this.textTracks();

            if (!tracks) {
                return;
            }

            for (var i = 0; i < tracks.length; i++) {
                var track = tracks[i];
                if (track['mode'] === 'showing') {
                    trackInfo.push(i + 1);
                }
            }

            if (this.apiMedia && trackInfo.length) {
                this.tracksInfoRequest = new chrome.cast.media.EditTracksInfoRequest(trackInfo);
                return this.apiMedia.editTracksInfo(this.tracksInfoRequest, this.onTrackSuccess.bind(this), this.onTrackError.bind(this));
            }
        }
    }, {
        key: 'onTrackSuccess',
        value: function onTrackSuccess() {
            return _videoJs2['default'].log('track added');
        }
    }, {
        key: 'onTrackError',
        value: function onTrackError(e) {
            return _videoJs2['default'].log('Cast track Error: ' + JSON.stringify(e));
        }
    }, {
        key: 'castError',
        value: function castError(e) {
            return _videoJs2['default'].log('Cast Error: ' + JSON.stringify(e));
        }
    }, {
        key: 'play',
        value: function play() {
            if (!this.apiMedia) {
                return;
            }
            if (this.paused_) {
                this.apiMedia.play(null, this.mediaCommandSuccessCallback.bind(this, 'Playing: ' + this.apiMedia.sessionId), this.castError.bind(this));
            }
            return this.paused_ = false;
        }
    }, {
        key: 'pause',
        value: function pause() {
            if (!this.apiMedia) {
                return;
            }
            if (!this.paused_) {
                this.apiMedia.pause(null, this.mediaCommandSuccessCallback.bind(this, 'Paused: ' + this.apiMedia.sessionId), this.castError.bind(this));
                return this.paused_ = true;
            }
        }
    }, {
        key: 'paused',
        value: function paused() {
            return this.paused_;
        }
    }, {
        key: 'currentTime',
        value: function currentTime() {
            if (!this.apiMedia) {
                return 0;
            }
            return this.apiMedia.getEstimatedTime();
        }
    }, {
        key: 'setCurrentTime',
        value: function setCurrentTime(position) {

            if (!this.apiMedia) {
                return 0;
            }
            var request = undefined;
            request = new chrome.cast.media.SeekRequest();
            request.currentTime = position;
            //if (this.player_.controlBar.progressControl.seekBar.videoWasPlaying) {
            //  request.resumeState = chrome.cast.media.ResumeState.PLAYBACK_START;
            //}
            return this.apiMedia.seek(request, this.onSeekSuccess.bind(this, position), this.castError.bind(this));
        }
    }, {
        key: 'onSeekSuccess',
        value: function onSeekSuccess(position) {
            _videoJs2['default'].log('seek success' + position);
        }
    }, {
        key: 'volume',
        value: function volume() {
            return this.volume_;
        }
    }, {
        key: 'duration',
        value: function duration() {
            if (!this.apiMedia) {
                return 0;
            }
            return this.apiMedia.media.duration || Infinity;
        }
    }, {
        key: 'controls',
        value: function controls() {
            return false;
        }
    }, {
        key: 'setVolume',
        value: function setVolume(level) {
            var mute = arguments.length <= 1 || arguments[1] === undefined ? false : arguments[1];

            var request = undefined;
            var volume = undefined;
            if (!this.apiMedia) {
                return;
            }
            volume = new chrome.cast.Volume();
            volume.level = level;
            volume.muted = mute;
            this.volume_ = volume.level;
            this.muted_ = mute;
            request = new chrome.cast.media.VolumeRequest();
            request.volume = volume;
            this.apiMedia.setVolume(request, this.mediaCommandSuccessCallback.bind(this, 'Volume changed'), this.castError.bind(this));
            return this.trigger('volumechange');
        }
    }, {
        key: 'mediaCommandSuccessCallback',
        value: function mediaCommandSuccessCallback(information) {
            _videoJs2['default'].log(information);
        }
    }, {
        key: 'muted',
        value: function muted() {
            return this.muted_;
        }
    }, {
        key: 'setMuted',
        value: function setMuted(muted) {
            return this.setVolume(this.volume_, muted);
        }
    }, {
        key: 'supportsFullScreen',
        value: function supportsFullScreen() {
            return false;
        }
    }, {
        key: 'resetSrc_',
        value: function resetSrc_(callback) {
            callback();
        }
    }, {
        key: 'dispose',
        value: function dispose() {
            this.resetSrc_(Function.prototype);
            _get(Object.getPrototypeOf(Chromecast.prototype), 'dispose', this).call(this, this);
        }
    }]);

    return Chromecast;
})(Tech);

Chromecast.prototype.paused_ = false;

Chromecast.prototype.options_ = {};

Chromecast.prototype.timerStep = 1000;

/* Chromecast Support Testing -------------------------------------------------------- */

Chromecast.isSupported = function () {
    return true;
};

// Add Source Handler pattern functions to this tech
Tech.withSourceHandlers(Chromecast);

/*
 * The default native source handler.
 * This simply passes the source to the video element. Nothing fancy.
 *
 * @param  {Object} source   The source object
 * @param  {Flash} tech  The instance of the Flash tech
 */
Chromecast.nativeSourceHandler = {};

/**
 * Check if Flash can play the given videotype
 * @param  {String} type    The mimetype to check
 * @return {String}         'probably', 'maybe', or '' (empty string)
 */
Chromecast.nativeSourceHandler.canPlayType = function (source) {

    var dashTypeRE = /^application\/(?:dash\+xml|(x-|vnd\.apple\.)mpegurl)/i;
    var dashExtRE = /^video\/(mpd|mp4|webm|m3u8)/i;

    if (dashTypeRE.test(source)) {
        return 'probably';
    } else if (dashExtRE.test(source)) {
        return 'maybe';
    } else {
        return '';
    }
};

/*
 * Check Flash can handle the source natively
 *
 * @param  {Object} source  The source object
 * @return {String}         'probably', 'maybe', or '' (empty string)
 */
Chromecast.nativeSourceHandler.canHandleSource = function (source) {

    // If a type was provided we should rely on that
    if (source.type) {
        return Chromecast.nativeSourceHandler.canPlayType(source.type);
    } else if (source.src) {
        return Chromecast.nativeSourceHandler.canPlayType(source.src);
    }

    return '';
};

/*
 * Pass the source to the flash object
 * Adaptive source handlers will have more complicated workflows before passing
 * video data to the video element
 *
 * @param  {Object} source    The source object
 * @param  {Flash} tech   The instance of the Flash tech
 */
Chromecast.nativeSourceHandler.handleSource = function (source, tech) {
    tech.src(source.src);
};

/*
 * Clean up the source handler when disposing the player or switching sources..
 * (no cleanup is needed when supporting the format natively)
 */
Chromecast.nativeSourceHandler.dispose = function () {};

// Register the native source handler
Chromecast.registerSourceHandler(Chromecast.nativeSourceHandler);

/*
 * Set the tech's volume control support status
 *
 * @type {Boolean}
 */
Chromecast.prototype['featuresVolumeControl'] = true;

/*
 * Set the tech's playbackRate support status
 *
 * @type {Boolean}
 */
Chromecast.prototype['featuresPlaybackRate'] = false;

/*
 * Set the tech's status on moving the video element.
 * In iOS, if you move a video element in the DOM, it breaks video playback.
 *
 * @type {Boolean}
 */
Chromecast.prototype['movingMediaElementInDOM'] = false;

/*
 * Set the the tech's fullscreen resize support status.
 * HTML video is able to automatically resize when going to fullscreen.
 * (No longer appears to be used. Can probably be removed.)
 */
Chromecast.prototype['featuresFullscreenResize'] = false;

/*
 * Set the tech's timeupdate event support status
 * (this disables the manual timeupdate events of the Tech)
 */
Chromecast.prototype['featuresTimeupdateEvents'] = false;

/*
 * Set the tech's progress event support status
 * (this disables the manual progress events of the Tech)
 */
Chromecast.prototype['featuresProgressEvents'] = false;

/*
 * Sets the tech's status on native text track support
 *
 * @type {Boolean}
 */
Chromecast.prototype['featuresNativeTextTracks'] = true;

/*
 * Sets the tech's status on native audio track support
 *
 * @type {Boolean}
 */
Chromecast.prototype['featuresNativeAudioTracks'] = true;

/*
 * Sets the tech's status on native video track support
 *
 * @type {Boolean}
 */
Chromecast.prototype['featuresNativeVideoTracks'] = false;

_videoJs2['default'].options.chromecast = {};

Component.registerComponent('Chromecast', Chromecast);
Tech.registerTech('Chromecast', Chromecast);
exports['default'] = Chromecast;
module.exports = exports['default'];
}).call(this,typeof global !== "undefined" ? global : typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {})
},{}],3:[function(require,module,exports){
(function (global){
/**
 * ! videojs-chromecast - v1.0.0 - 2016-02-15
 * Copyright (c) 2015 benjipott
 * Licensed under the Apache-2.0 license.
 * @file videojs-chromecast.js
 **/
'use strict';

Object.defineProperty(exports, '__esModule', {
    value: true
});

var _get = function get(_x, _x2, _x3) { var _again = true; _function: while (_again) { var object = _x, property = _x2, receiver = _x3; _again = false; if (object === null) object = Function.prototype; var desc = Object.getOwnPropertyDescriptor(object, property); if (desc === undefined) { var parent = Object.getPrototypeOf(object); if (parent === null) { return undefined; } else { _x = parent; _x2 = property; _x3 = receiver; _again = true; desc = parent = undefined; continue _function; } } else if ('value' in desc) { return desc.value; } else { var getter = desc.get; if (getter === undefined) { return undefined; } return getter.call(receiver); } } };

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { 'default': obj }; }

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError('Cannot call a class as a function'); } }

function _inherits(subClass, superClass) { if (typeof superClass !== 'function' && superClass !== null) { throw new TypeError('Super expression must either be null or a function, not ' + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; }

var _videoJs = (typeof window !== "undefined" ? window['videojs'] : typeof global !== "undefined" ? global['videojs'] : null);

var _videoJs2 = _interopRequireDefault(_videoJs);

var _componentControlBarChromecastButton = require('./component/control-bar/chromecast-button');

var _componentControlBarChromecastButton2 = _interopRequireDefault(_componentControlBarChromecastButton);

var _techChromecast = require('./tech/chromecast');

var _techChromecast2 = _interopRequireDefault(_techChromecast);

var Component = _videoJs2['default'].getComponent('Component');

/**
 * Initialize the plugin.
 * @param options (optional) {object} configuration for the plugin
 */

var Chromecast = (function (_Component) {
    _inherits(Chromecast, _Component);

    function Chromecast(player, options) {
        _classCallCheck(this, Chromecast);

        _get(Object.getPrototypeOf(Chromecast.prototype), 'constructor', this).call(this, player, options);
    }

    return Chromecast;
})(Component);

Chromecast.prototype.options_ = {};

// register the plugin
_videoJs2['default'].options.children.push('chromecast');

_videoJs2['default'].addLanguage('en', {
    'CASTING TO': 'WIEDERGABE AUF'
});

_videoJs2['default'].addLanguage('de', {
    'CASTING TO': 'WIEDERGABE AUF'
});

_videoJs2['default'].addLanguage('it', {
    'CASTING TO': 'PLAYBACK SU'
});

_videoJs2['default'].addLanguage('fr', {
    'CASTING TO': 'CAST EN COURS SUR'
});

var USER_AGENT = window.navigator.userAgent;

_videoJs2['default'].browser.IS_EDGE = /Edge/i.test(USER_AGENT);

Component.registerComponent('Chromecast', Chromecast);
exports['default'] = Chromecast;
module.exports = exports['default'];
}).call(this,typeof global !== "undefined" ? global : typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {})
},{"./component/control-bar/chromecast-button":1,"./tech/chromecast":2}],4:[function(require,module,exports){
(function (global){
'use strict';

Object.defineProperty(exports, '__esModule', {
  value: true
});

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { 'default': obj }; }

var _videoJs = (typeof window !== "undefined" ? window['videojs'] : typeof global !== "undefined" ? global['videojs'] : null);

var _videoJs2 = _interopRequireDefault(_videoJs);

var _videojsChromecast = require('./videojs-chromecast');

var _videojsChromecast2 = _interopRequireDefault(_videojsChromecast);

/**
 * The video.js playlist plugin. Invokes the playlist-maker to create a
 * playlist function on the specific player.
 *
 * @param {Array} list
 */
var plugin = function plugin(options) {
  var player = this;
  player.addChild('Chromecast', options);
};

_videoJs2['default'].plugin('chromecast', plugin);

exports['default'] = plugin;
module.exports = exports['default'];
}).call(this,typeof global !== "undefined" ? global : typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {})
},{"./videojs-chromecast":3}]},{},[4])(4)
});
