/*! Chorus 2 - A web interface for Kodi. Created by Jeremy Graham - built on 21-10-2016 */
!function(a,b){"object"==typeof module&&"object"==typeof module.exports?module.exports=a.document?b(a,!0):function(a){if(!a.document)throw new Error("jQuery requires a window with a document");return b(a)}:b(a)}("undefined"!=typeof window?window:this,function(a,b){function c(a){var b=a.length,c=ea.type(a);return"function"===c||ea.isWindow(a)?!1:1===a.nodeType&&b?!0:"array"===c||0===b||"number"==typeof b&&b>0&&b-1 in a}function d(a,b,c){if(ea.isFunction(b))return ea.grep(a,function(a,d){return!!b.call(a,d,a)!==c});if(b.nodeType)return ea.grep(a,function(a){return a===b!==c});if("string"==typeof b){if(ma.test(b))return ea.filter(b,a,c);b=ea.filter(b,a)}return ea.grep(a,function(a){return ea.inArray(a,b)>=0!==c})}function e(a,b){do a=a[b];while(a&&1!==a.nodeType);return a}function f(a){var b=ua[a]={};return ea.each(a.match(ta)||[],function(a,c){b[c]=!0}),b}function g(){oa.addEventListener?(oa.removeEventListener("DOMContentLoaded",h,!1),a.removeEventListener("load",h,!1)):(oa.detachEvent("onreadystatechange",h),a.detachEvent("onload",h))}function h(){(oa.addEventListener||"load"===event.type||"complete"===oa.readyState)&&(g(),ea.ready())}function i(a,b,c){if(void 0===c&&1===a.nodeType){var d="data-"+b.replace(za,"-$1").toLowerCase();if(c=a.getAttribute(d),"string"==typeof c){try{c="true"===c?!0:"false"===c?!1:"null"===c?null:+c+""===c?+c:ya.test(c)?ea.parseJSON(c):c}catch(e){}ea.data(a,b,c)}else c=void 0}return c}function j(a){var b;for(b in a)if(("data"!==b||!ea.isEmptyObject(a[b]))&&"toJSON"!==b)return!1;return!0}function k(a,b,c,d){if(ea.acceptData(a)){var e,f,g=ea.expando,h=a.nodeType,i=h?ea.cache:a,j=h?a[g]:a[g]&&g;if(j&&i[j]&&(d||i[j].data)||void 0!==c||"string"!=typeof b)return j||(j=h?a[g]=W.pop()||ea.guid++:g),i[j]||(i[j]=h?{}:{toJSON:ea.noop}),("object"==typeof b||"function"==typeof b)&&(d?i[j]=ea.extend(i[j],b):i[j].data=ea.extend(i[j].data,b)),f=i[j],d||(f.data||(f.data={}),f=f.data),void 0!==c&&(f[ea.camelCase(b)]=c),"string"==typeof b?(e=f[b],null==e&&(e=f[ea.camelCase(b)])):e=f,e}}function l(a,b,c){if(ea.acceptData(a)){var d,e,f=a.nodeType,g=f?ea.cache:a,h=f?a[ea.expando]:ea.expando;if(g[h]){if(b&&(d=c?g[h]:g[h].data)){ea.isArray(b)?b=b.concat(ea.map(b,ea.camelCase)):b in d?b=[b]:(b=ea.camelCase(b),b=b in d?[b]:b.split(" ")),e=b.length;for(;e--;)delete d[b[e]];if(c?!j(d):!ea.isEmptyObject(d))return}(c||(delete g[h].data,j(g[h])))&&(f?ea.cleanData([a],!0):ca.deleteExpando||g!=g.window?delete g[h]:g[h]=null)}}}function m(){return!0}function n(){return!1}function o(){try{return oa.activeElement}catch(a){}}function p(a){var b=Ka.split("|"),c=a.createDocumentFragment();if(c.createElement)for(;b.length;)c.createElement(b.pop());return c}function q(a,b){var c,d,e=0,f=typeof a.getElementsByTagName!==xa?a.getElementsByTagName(b||"*"):typeof a.querySelectorAll!==xa?a.querySelectorAll(b||"*"):void 0;if(!f)for(f=[],c=a.childNodes||a;null!=(d=c[e]);e++)!b||ea.nodeName(d,b)?f.push(d):ea.merge(f,q(d,b));return void 0===b||b&&ea.nodeName(a,b)?ea.merge([a],f):f}function r(a){Ea.test(a.type)&&(a.defaultChecked=a.checked)}function s(a,b){return ea.nodeName(a,"table")&&ea.nodeName(11!==b.nodeType?b:b.firstChild,"tr")?a.getElementsByTagName("tbody")[0]||a.appendChild(a.ownerDocument.createElement("tbody")):a}function t(a){return a.type=(null!==ea.find.attr(a,"type"))+"/"+a.type,a}function u(a){var b=Va.exec(a.type);return b?a.type=b[1]:a.removeAttribute("type"),a}function v(a,b){for(var c,d=0;null!=(c=a[d]);d++)ea._data(c,"globalEval",!b||ea._data(b[d],"globalEval"))}function w(a,b){if(1===b.nodeType&&ea.hasData(a)){var c,d,e,f=ea._data(a),g=ea._data(b,f),h=f.events;if(h){delete g.handle,g.events={};for(c in h)for(d=0,e=h[c].length;e>d;d++)ea.event.add(b,c,h[c][d])}g.data&&(g.data=ea.extend({},g.data))}}function x(a,b){var c,d,e;if(1===b.nodeType){if(c=b.nodeName.toLowerCase(),!ca.noCloneEvent&&b[ea.expando]){e=ea._data(b);for(d in e.events)ea.removeEvent(b,d,e.handle);b.removeAttribute(ea.expando)}"script"===c&&b.text!==a.text?(t(b).text=a.text,u(b)):"object"===c?(b.parentNode&&(b.outerHTML=a.outerHTML),ca.html5Clone&&a.innerHTML&&!ea.trim(b.innerHTML)&&(b.innerHTML=a.innerHTML)):"input"===c&&Ea.test(a.type)?(b.defaultChecked=b.checked=a.checked,b.value!==a.value&&(b.value=a.value)):"option"===c?b.defaultSelected=b.selected=a.defaultSelected:("input"===c||"textarea"===c)&&(b.defaultValue=a.defaultValue)}}function y(b,c){var d,e=ea(c.createElement(b)).appendTo(c.body),f=a.getDefaultComputedStyle&&(d=a.getDefaultComputedStyle(e[0]))?d.display:ea.css(e[0],"display");return e.detach(),f}function z(a){var b=oa,c=_a[a];return c||(c=y(a,b),"none"!==c&&c||($a=($a||ea("<iframe frameborder='0' width='0' height='0'/>")).appendTo(b.documentElement),b=($a[0].contentWindow||$a[0].contentDocument).document,b.write(),b.close(),c=y(a,b),$a.detach()),_a[a]=c),c}function A(a,b){return{get:function(){var c=a();if(null!=c)return c?void delete this.get:(this.get=b).apply(this,arguments)}}}function B(a,b){if(b in a)return b;for(var c=b.charAt(0).toUpperCase()+b.slice(1),d=b,e=mb.length;e--;)if(b=mb[e]+c,b in a)return b;return d}function C(a,b){for(var c,d,e,f=[],g=0,h=a.length;h>g;g++)d=a[g],d.style&&(f[g]=ea._data(d,"olddisplay"),c=d.style.display,b?(f[g]||"none"!==c||(d.style.display=""),""===d.style.display&&Ca(d)&&(f[g]=ea._data(d,"olddisplay",z(d.nodeName)))):(e=Ca(d),(c&&"none"!==c||!e)&&ea._data(d,"olddisplay",e?c:ea.css(d,"display"))));for(g=0;h>g;g++)d=a[g],d.style&&(b&&"none"!==d.style.display&&""!==d.style.display||(d.style.display=b?f[g]||"":"none"));return a}function D(a,b,c){var d=ib.exec(b);return d?Math.max(0,d[1]-(c||0))+(d[2]||"px"):b}function E(a,b,c,d,e){for(var f=c===(d?"border":"content")?4:"width"===b?1:0,g=0;4>f;f+=2)"margin"===c&&(g+=ea.css(a,c+Ba[f],!0,e)),d?("content"===c&&(g-=ea.css(a,"padding"+Ba[f],!0,e)),"margin"!==c&&(g-=ea.css(a,"border"+Ba[f]+"Width",!0,e))):(g+=ea.css(a,"padding"+Ba[f],!0,e),"padding"!==c&&(g+=ea.css(a,"border"+Ba[f]+"Width",!0,e)));return g}function F(a,b,c){var d=!0,e="width"===b?a.offsetWidth:a.offsetHeight,f=ab(a),g=ca.boxSizing&&"border-box"===ea.css(a,"boxSizing",!1,f);if(0>=e||null==e){if(e=bb(a,b,f),(0>e||null==e)&&(e=a.style[b]),db.test(e))return e;d=g&&(ca.boxSizingReliable()||e===a.style[b]),e=parseFloat(e)||0}return e+E(a,b,c||(g?"border":"content"),d,f)+"px"}function G(a,b,c,d,e){return new G.prototype.init(a,b,c,d,e)}function H(){return setTimeout(function(){nb=void 0}),nb=ea.now()}function I(a,b){var c,d={height:a},e=0;for(b=b?1:0;4>e;e+=2-b)c=Ba[e],d["margin"+c]=d["padding"+c]=a;return b&&(d.opacity=d.width=a),d}function J(a,b,c){for(var d,e=(tb[b]||[]).concat(tb["*"]),f=0,g=e.length;g>f;f++)if(d=e[f].call(c,b,a))return d}function K(a,b,c){var d,e,f,g,h,i,j,k,l=this,m={},n=a.style,o=a.nodeType&&Ca(a),p=ea._data(a,"fxshow");c.queue||(h=ea._queueHooks(a,"fx"),null==h.unqueued&&(h.unqueued=0,i=h.empty.fire,h.empty.fire=function(){h.unqueued||i()}),h.unqueued++,l.always(function(){l.always(function(){h.unqueued--,ea.queue(a,"fx").length||h.empty.fire()})})),1===a.nodeType&&("height"in b||"width"in b)&&(c.overflow=[n.overflow,n.overflowX,n.overflowY],j=ea.css(a,"display"),k="none"===j?ea._data(a,"olddisplay")||z(a.nodeName):j,"inline"===k&&"none"===ea.css(a,"float")&&(ca.inlineBlockNeedsLayout&&"inline"!==z(a.nodeName)?n.zoom=1:n.display="inline-block")),c.overflow&&(n.overflow="hidden",ca.shrinkWrapBlocks()||l.always(function(){n.overflow=c.overflow[0],n.overflowX=c.overflow[1],n.overflowY=c.overflow[2]}));for(d in b)if(e=b[d],pb.exec(e)){if(delete b[d],f=f||"toggle"===e,e===(o?"hide":"show")){if("show"!==e||!p||void 0===p[d])continue;o=!0}m[d]=p&&p[d]||ea.style(a,d)}else j=void 0;if(ea.isEmptyObject(m))"inline"===("none"===j?z(a.nodeName):j)&&(n.display=j);else{p?"hidden"in p&&(o=p.hidden):p=ea._data(a,"fxshow",{}),f&&(p.hidden=!o),o?ea(a).show():l.done(function(){ea(a).hide()}),l.done(function(){var b;ea._removeData(a,"fxshow");for(b in m)ea.style(a,b,m[b])});for(d in m)g=J(o?p[d]:0,d,l),d in p||(p[d]=g.start,o&&(g.end=g.start,g.start="width"===d||"height"===d?1:0))}}function L(a,b){var c,d,e,f,g;for(c in a)if(d=ea.camelCase(c),e=b[d],f=a[c],ea.isArray(f)&&(e=f[1],f=a[c]=f[0]),c!==d&&(a[d]=f,delete a[c]),g=ea.cssHooks[d],g&&"expand"in g){f=g.expand(f),delete a[d];for(c in f)c in a||(a[c]=f[c],b[c]=e)}else b[d]=e}function M(a,b,c){var d,e,f=0,g=sb.length,h=ea.Deferred().always(function(){delete i.elem}),i=function(){if(e)return!1;for(var b=nb||H(),c=Math.max(0,j.startTime+j.duration-b),d=c/j.duration||0,f=1-d,g=0,i=j.tweens.length;i>g;g++)j.tweens[g].run(f);return h.notifyWith(a,[j,f,c]),1>f&&i?c:(h.resolveWith(a,[j]),!1)},j=h.promise({elem:a,props:ea.extend({},b),opts:ea.extend(!0,{specialEasing:{}},c),originalProperties:b,originalOptions:c,startTime:nb||H(),duration:c.duration,tweens:[],createTween:function(b,c){var d=ea.Tween(a,j.opts,b,c,j.opts.specialEasing[b]||j.opts.easing);return j.tweens.push(d),d},stop:function(b){var c=0,d=b?j.tweens.length:0;if(e)return this;for(e=!0;d>c;c++)j.tweens[c].run(1);return b?h.resolveWith(a,[j,b]):h.rejectWith(a,[j,b]),this}}),k=j.props;for(L(k,j.opts.specialEasing);g>f;f++)if(d=sb[f].call(j,a,k,j.opts))return d;return ea.map(k,J,j),ea.isFunction(j.opts.start)&&j.opts.start.call(a,j),ea.fx.timer(ea.extend(i,{elem:a,anim:j,queue:j.opts.queue})),j.progress(j.opts.progress).done(j.opts.done,j.opts.complete).fail(j.opts.fail).always(j.opts.always)}function N(a){return function(b,c){"string"!=typeof b&&(c=b,b="*");var d,e=0,f=b.toLowerCase().match(ta)||[];if(ea.isFunction(c))for(;d=f[e++];)"+"===d.charAt(0)?(d=d.slice(1)||"*",(a[d]=a[d]||[]).unshift(c)):(a[d]=a[d]||[]).push(c)}}function O(a,b,c,d){function e(h){var i;return f[h]=!0,ea.each(a[h]||[],function(a,h){var j=h(b,c,d);return"string"!=typeof j||g||f[j]?g?!(i=j):void 0:(b.dataTypes.unshift(j),e(j),!1)}),i}var f={},g=a===Rb;return e(b.dataTypes[0])||!f["*"]&&e("*")}function P(a,b){var c,d,e=ea.ajaxSettings.flatOptions||{};for(d in b)void 0!==b[d]&&((e[d]?a:c||(c={}))[d]=b[d]);return c&&ea.extend(!0,a,c),a}function Q(a,b,c){for(var d,e,f,g,h=a.contents,i=a.dataTypes;"*"===i[0];)i.shift(),void 0===e&&(e=a.mimeType||b.getResponseHeader("Content-Type"));if(e)for(g in h)if(h[g]&&h[g].test(e)){i.unshift(g);break}if(i[0]in c)f=i[0];else{for(g in c){if(!i[0]||a.converters[g+" "+i[0]]){f=g;break}d||(d=g)}f=f||d}return f?(f!==i[0]&&i.unshift(f),c[f]):void 0}function R(a,b,c,d){var e,f,g,h,i,j={},k=a.dataTypes.slice();if(k[1])for(g in a.converters)j[g.toLowerCase()]=a.converters[g];for(f=k.shift();f;)if(a.responseFields[f]&&(c[a.responseFields[f]]=b),!i&&d&&a.dataFilter&&(b=a.dataFilter(b,a.dataType)),i=f,f=k.shift())if("*"===f)f=i;else if("*"!==i&&i!==f){if(g=j[i+" "+f]||j["* "+f],!g)for(e in j)if(h=e.split(" "),h[1]===f&&(g=j[i+" "+h[0]]||j["* "+h[0]])){g===!0?g=j[e]:j[e]!==!0&&(f=h[0],k.unshift(h[1]));break}if(g!==!0)if(g&&a["throws"])b=g(b);else try{b=g(b)}catch(l){return{state:"parsererror",error:g?l:"No conversion from "+i+" to "+f}}}return{state:"success",data:b}}function S(a,b,c,d){var e;if(ea.isArray(b))ea.each(b,function(b,e){c||Vb.test(a)?d(a,e):S(a+"["+("object"==typeof e?b:"")+"]",e,c,d)});else if(c||"object"!==ea.type(b))d(a,b);else for(e in b)S(a+"["+e+"]",b[e],c,d)}function T(){try{return new a.XMLHttpRequest}catch(b){}}function U(){try{return new a.ActiveXObject("Microsoft.XMLHTTP")}catch(b){}}function V(a){return ea.isWindow(a)?a:9===a.nodeType?a.defaultView||a.parentWindow:!1}var W=[],X=W.slice,Y=W.concat,Z=W.push,$=W.indexOf,_={},aa=_.toString,ba=_.hasOwnProperty,ca={},da="1.11.1",ea=function(a,b){return new ea.fn.init(a,b)},fa=/^[\s\uFEFF\xA0]+|[\s\uFEFF\xA0]+$/g,ga=/^-ms-/,ha=/-([\da-z])/gi,ia=function(a,b){return b.toUpperCase()};ea.fn=ea.prototype={jquery:da,constructor:ea,selector:"",length:0,toArray:function(){return X.call(this)},get:function(a){return null!=a?0>a?this[a+this.length]:this[a]:X.call(this)},pushStack:function(a){var b=ea.merge(this.constructor(),a);return b.prevObject=this,b.context=this.context,b},each:function(a,b){return ea.each(this,a,b)},map:function(a){return this.pushStack(ea.map(this,function(b,c){return a.call(b,c,b)}))},slice:function(){return this.pushStack(X.apply(this,arguments))},first:function(){return this.eq(0)},last:function(){return this.eq(-1)},eq:function(a){var b=this.length,c=+a+(0>a?b:0);return this.pushStack(c>=0&&b>c?[this[c]]:[])},end:function(){return this.prevObject||this.constructor(null)},push:Z,sort:W.sort,splice:W.splice},ea.extend=ea.fn.extend=function(){var a,b,c,d,e,f,g=arguments[0]||{},h=1,i=arguments.length,j=!1;for("boolean"==typeof g&&(j=g,g=arguments[h]||{},h++),"object"==typeof g||ea.isFunction(g)||(g={}),h===i&&(g=this,h--);i>h;h++)if(null!=(e=arguments[h]))for(d in e)a=g[d],c=e[d],g!==c&&(j&&c&&(ea.isPlainObject(c)||(b=ea.isArray(c)))?(b?(b=!1,f=a&&ea.isArray(a)?a:[]):f=a&&ea.isPlainObject(a)?a:{},g[d]=ea.extend(j,f,c)):void 0!==c&&(g[d]=c));return g},ea.extend({expando:"jQuery"+(da+Math.random()).replace(/\D/g,""),isReady:!0,error:function(a){throw new Error(a)},noop:function(){},isFunction:function(a){return"function"===ea.type(a)},isArray:Array.isArray||function(a){return"array"===ea.type(a)},isWindow:function(a){return null!=a&&a==a.window},isNumeric:function(a){return!ea.isArray(a)&&a-parseFloat(a)>=0},isEmptyObject:function(a){var b;for(b in a)return!1;return!0},isPlainObject:function(a){var b;if(!a||"object"!==ea.type(a)||a.nodeType||ea.isWindow(a))return!1;try{if(a.constructor&&!ba.call(a,"constructor")&&!ba.call(a.constructor.prototype,"isPrototypeOf"))return!1}catch(c){return!1}if(ca.ownLast)for(b in a)return ba.call(a,b);for(b in a);return void 0===b||ba.call(a,b)},type:function(a){return null==a?a+"":"object"==typeof a||"function"==typeof a?_[aa.call(a)]||"object":typeof a},globalEval:function(b){b&&ea.trim(b)&&(a.execScript||function(b){a.eval.call(a,b)})(b)},camelCase:function(a){return a.replace(ga,"ms-").replace(ha,ia)},nodeName:function(a,b){return a.nodeName&&a.nodeName.toLowerCase()===b.toLowerCase()},each:function(a,b,d){var e,f=0,g=a.length,h=c(a);if(d){if(h)for(;g>f&&(e=b.apply(a[f],d),e!==!1);f++);else for(f in a)if(e=b.apply(a[f],d),e===!1)break}else if(h)for(;g>f&&(e=b.call(a[f],f,a[f]),e!==!1);f++);else for(f in a)if(e=b.call(a[f],f,a[f]),e===!1)break;return a},trim:function(a){return null==a?"":(a+"").replace(fa,"")},makeArray:function(a,b){var d=b||[];return null!=a&&(c(Object(a))?ea.merge(d,"string"==typeof a?[a]:a):Z.call(d,a)),d},inArray:function(a,b,c){var d;if(b){if($)return $.call(b,a,c);for(d=b.length,c=c?0>c?Math.max(0,d+c):c:0;d>c;c++)if(c in b&&b[c]===a)return c}return-1},merge:function(a,b){for(var c=+b.length,d=0,e=a.length;c>d;)a[e++]=b[d++];if(c!==c)for(;void 0!==b[d];)a[e++]=b[d++];return a.length=e,a},grep:function(a,b,c){for(var d,e=[],f=0,g=a.length,h=!c;g>f;f++)d=!b(a[f],f),d!==h&&e.push(a[f]);return e},map:function(a,b,d){var e,f=0,g=a.length,h=c(a),i=[];if(h)for(;g>f;f++)e=b(a[f],f,d),null!=e&&i.push(e);else for(f in a)e=b(a[f],f,d),null!=e&&i.push(e);return Y.apply([],i)},guid:1,proxy:function(a,b){var c,d,e;return"string"==typeof b&&(e=a[b],b=a,a=e),ea.isFunction(a)?(c=X.call(arguments,2),d=function(){return a.apply(b||this,c.concat(X.call(arguments)))},d.guid=a.guid=a.guid||ea.guid++,d):void 0},now:function(){return+new Date},support:ca}),ea.each("Boolean Number String Function Array Date RegExp Object Error".split(" "),function(a,b){_["[object "+b+"]"]=b.toLowerCase()});var ja=function(a){function b(a,b,c,d){var e,f,g,h,i,j,l,n,o,p;if((b?b.ownerDocument||b:O)!==G&&F(b),b=b||G,c=c||[],!a||"string"!=typeof a)return c;if(1!==(h=b.nodeType)&&9!==h)return[];if(I&&!d){if(e=sa.exec(a))if(g=e[1]){if(9===h){if(f=b.getElementById(g),!f||!f.parentNode)return c;if(f.id===g)return c.push(f),c}else if(b.ownerDocument&&(f=b.ownerDocument.getElementById(g))&&M(b,f)&&f.id===g)return c.push(f),c}else{if(e[2])return _.apply(c,b.getElementsByTagName(a)),c;if((g=e[3])&&v.getElementsByClassName&&b.getElementsByClassName)return _.apply(c,b.getElementsByClassName(g)),c}if(v.qsa&&(!J||!J.test(a))){if(n=l=N,o=b,p=9===h&&a,1===h&&"object"!==b.nodeName.toLowerCase()){for(j=z(a),(l=b.getAttribute("id"))?n=l.replace(ua,"\\$&"):b.setAttribute("id",n),n="[id='"+n+"'] ",i=j.length;i--;)j[i]=n+m(j[i]);o=ta.test(a)&&k(b.parentNode)||b,p=j.join(",")}if(p)try{return _.apply(c,o.querySelectorAll(p)),c}catch(q){}finally{l||b.removeAttribute("id")}}}return B(a.replace(ia,"$1"),b,c,d)}function c(){function a(c,d){return b.push(c+" ")>w.cacheLength&&delete a[b.shift()],a[c+" "]=d}var b=[];return a}function d(a){return a[N]=!0,a}function e(a){var b=G.createElement("div");try{return!!a(b)}catch(c){return!1}finally{b.parentNode&&b.parentNode.removeChild(b),b=null}}function f(a,b){for(var c=a.split("|"),d=a.length;d--;)w.attrHandle[c[d]]=b}function g(a,b){var c=b&&a,d=c&&1===a.nodeType&&1===b.nodeType&&(~b.sourceIndex||W)-(~a.sourceIndex||W);if(d)return d;if(c)for(;c=c.nextSibling;)if(c===b)return-1;return a?1:-1}function h(a){return function(b){var c=b.nodeName.toLowerCase();return"input"===c&&b.type===a}}function i(a){return function(b){var c=b.nodeName.toLowerCase();return("input"===c||"button"===c)&&b.type===a}}function j(a){return d(function(b){return b=+b,d(function(c,d){for(var e,f=a([],c.length,b),g=f.length;g--;)c[e=f[g]]&&(c[e]=!(d[e]=c[e]))})})}function k(a){return a&&typeof a.getElementsByTagName!==V&&a}function l(){}function m(a){for(var b=0,c=a.length,d="";c>b;b++)d+=a[b].value;return d}function n(a,b,c){var d=b.dir,e=c&&"parentNode"===d,f=Q++;return b.first?function(b,c,f){for(;b=b[d];)if(1===b.nodeType||e)return a(b,c,f)}:function(b,c,g){var h,i,j=[P,f];if(g){for(;b=b[d];)if((1===b.nodeType||e)&&a(b,c,g))return!0}else for(;b=b[d];)if(1===b.nodeType||e){if(i=b[N]||(b[N]={}),(h=i[d])&&h[0]===P&&h[1]===f)return j[2]=h[2];if(i[d]=j,j[2]=a(b,c,g))return!0}}}function o(a){return a.length>1?function(b,c,d){for(var e=a.length;e--;)if(!a[e](b,c,d))return!1;return!0}:a[0]}function p(a,c,d){for(var e=0,f=c.length;f>e;e++)b(a,c[e],d);return d}function q(a,b,c,d,e){for(var f,g=[],h=0,i=a.length,j=null!=b;i>h;h++)(f=a[h])&&(!c||c(f,d,e))&&(g.push(f),j&&b.push(h));return g}function r(a,b,c,e,f,g){return e&&!e[N]&&(e=r(e)),f&&!f[N]&&(f=r(f,g)),d(function(d,g,h,i){var j,k,l,m=[],n=[],o=g.length,r=d||p(b||"*",h.nodeType?[h]:h,[]),s=!a||!d&&b?r:q(r,m,a,h,i),t=c?f||(d?a:o||e)?[]:g:s;if(c&&c(s,t,h,i),e)for(j=q(t,n),e(j,[],h,i),k=j.length;k--;)(l=j[k])&&(t[n[k]]=!(s[n[k]]=l));if(d){if(f||a){if(f){for(j=[],k=t.length;k--;)(l=t[k])&&j.push(s[k]=l);f(null,t=[],j,i)}for(k=t.length;k--;)(l=t[k])&&(j=f?ba.call(d,l):m[k])>-1&&(d[j]=!(g[j]=l))}}else t=q(t===g?t.splice(o,t.length):t),f?f(null,g,t,i):_.apply(g,t)})}function s(a){for(var b,c,d,e=a.length,f=w.relative[a[0].type],g=f||w.relative[" "],h=f?1:0,i=n(function(a){return a===b},g,!0),j=n(function(a){return ba.call(b,a)>-1},g,!0),k=[function(a,c,d){return!f&&(d||c!==C)||((b=c).nodeType?i(a,c,d):j(a,c,d))}];e>h;h++)if(c=w.relative[a[h].type])k=[n(o(k),c)];else{if(c=w.filter[a[h].type].apply(null,a[h].matches),c[N]){for(d=++h;e>d&&!w.relative[a[d].type];d++);return r(h>1&&o(k),h>1&&m(a.slice(0,h-1).concat({value:" "===a[h-2].type?"*":""})).replace(ia,"$1"),c,d>h&&s(a.slice(h,d)),e>d&&s(a=a.slice(d)),e>d&&m(a))}k.push(c)}return o(k)}function t(a,c){var e=c.length>0,f=a.length>0,g=function(d,g,h,i,j){var k,l,m,n=0,o="0",p=d&&[],r=[],s=C,t=d||f&&w.find.TAG("*",j),u=P+=null==s?1:Math.random()||.1,v=t.length;for(j&&(C=g!==G&&g);o!==v&&null!=(k=t[o]);o++){if(f&&k){for(l=0;m=a[l++];)if(m(k,g,h)){i.push(k);break}j&&(P=u)}e&&((k=!m&&k)&&n--,d&&p.push(k))}if(n+=o,e&&o!==n){for(l=0;m=c[l++];)m(p,r,g,h);if(d){if(n>0)for(;o--;)p[o]||r[o]||(r[o]=Z.call(i));r=q(r)}_.apply(i,r),j&&!d&&r.length>0&&n+c.length>1&&b.uniqueSort(i)}return j&&(P=u,C=s),p};return e?d(g):g}var u,v,w,x,y,z,A,B,C,D,E,F,G,H,I,J,K,L,M,N="sizzle"+-new Date,O=a.document,P=0,Q=0,R=c(),S=c(),T=c(),U=function(a,b){return a===b&&(E=!0),0},V="undefined",W=1<<31,X={}.hasOwnProperty,Y=[],Z=Y.pop,$=Y.push,_=Y.push,aa=Y.slice,ba=Y.indexOf||function(a){for(var b=0,c=this.length;c>b;b++)if(this[b]===a)return b;return-1},ca="checked|selected|async|autofocus|autoplay|controls|defer|disabled|hidden|ismap|loop|multiple|open|readonly|required|scoped",da="[\\x20\\t\\r\\n\\f]",ea="(?:\\\\.|[\\w-]|[^\\x00-\\xa0])+",fa=ea.replace("w","w#"),ga="\\["+da+"*("+ea+")(?:"+da+"*([*^$|!~]?=)"+da+"*(?:'((?:\\\\.|[^\\\\'])*)'|\"((?:\\\\.|[^\\\\\"])*)\"|("+fa+"))|)"+da+"*\\]",ha=":("+ea+")(?:\\((('((?:\\\\.|[^\\\\'])*)'|\"((?:\\\\.|[^\\\\\"])*)\")|((?:\\\\.|[^\\\\()[\\]]|"+ga+")*)|.*)\\)|)",ia=new RegExp("^"+da+"+|((?:^|[^\\\\])(?:\\\\.)*)"+da+"+$","g"),ja=new RegExp("^"+da+"*,"+da+"*"),ka=new RegExp("^"+da+"*([>+~]|"+da+")"+da+"*"),la=new RegExp("="+da+"*([^\\]'\"]*?)"+da+"*\\]","g"),ma=new RegExp(ha),na=new RegExp("^"+fa+"$"),oa={ID:new RegExp("^#("+ea+")"),CLASS:new RegExp("^\\.("+ea+")"),TAG:new RegExp("^("+ea.replace("w","w*")+")"),ATTR:new RegExp("^"+ga),PSEUDO:new RegExp("^"+ha),CHILD:new RegExp("^:(only|first|last|nth|nth-last)-(child|of-type)(?:\\("+da+"*(even|odd|(([+-]|)(\\d*)n|)"+da+"*(?:([+-]|)"+da+"*(\\d+)|))"+da+"*\\)|)","i"),bool:new RegExp("^(?:"+ca+")$","i"),needsContext:new RegExp("^"+da+"*[>+~]|:(even|odd|eq|gt|lt|nth|first|last)(?:\\("+da+"*((?:-\\d)?\\d*)"+da+"*\\)|)(?=[^-]|$)","i")},pa=/^(?:input|select|textarea|button)$/i,qa=/^h\d$/i,ra=/^[^{]+\{\s*\[native \w/,sa=/^(?:#([\w-]+)|(\w+)|\.([\w-]+))$/,ta=/[+~]/,ua=/'|\\/g,va=new RegExp("\\\\([\\da-f]{1,6}"+da+"?|("+da+")|.)","ig"),wa=function(a,b,c){var d="0x"+b-65536;return d!==d||c?b:0>d?String.fromCharCode(d+65536):String.fromCharCode(d>>10|55296,1023&d|56320)};try{_.apply(Y=aa.call(O.childNodes),O.childNodes),Y[O.childNodes.length].nodeType}catch(xa){_={apply:Y.length?function(a,b){$.apply(a,aa.call(b))}:function(a,b){for(var c=a.length,d=0;a[c++]=b[d++];);a.length=c-1}}}v=b.support={},y=b.isXML=function(a){var b=a&&(a.ownerDocument||a).documentElement;return b?"HTML"!==b.nodeName:!1},F=b.setDocument=function(a){var b,c=a?a.ownerDocument||a:O,d=c.defaultView;return c!==G&&9===c.nodeType&&c.documentElement?(G=c,H=c.documentElement,I=!y(c),d&&d!==d.top&&(d.addEventListener?d.addEventListener("unload",function(){F()},!1):d.attachEvent&&d.attachEvent("onunload",function(){F()})),v.attributes=e(function(a){return a.className="i",!a.getAttribute("className")}),v.getElementsByTagName=e(function(a){return a.appendChild(c.createComment("")),!a.getElementsByTagName("*").length}),v.getElementsByClassName=ra.test(c.getElementsByClassName)&&e(function(a){return a.innerHTML="<div class='a'></div><div class='a i'></div>",a.firstChild.className="i",2===a.getElementsByClassName("i").length}),v.getById=e(function(a){return H.appendChild(a).id=N,!c.getElementsByName||!c.getElementsByName(N).length}),v.getById?(w.find.ID=function(a,b){if(typeof b.getElementById!==V&&I){var c=b.getElementById(a);return c&&c.parentNode?[c]:[]}},w.filter.ID=function(a){var b=a.replace(va,wa);return function(a){return a.getAttribute("id")===b}}):(delete w.find.ID,w.filter.ID=function(a){var b=a.replace(va,wa);return function(a){var c=typeof a.getAttributeNode!==V&&a.getAttributeNode("id");return c&&c.value===b}}),w.find.TAG=v.getElementsByTagName?function(a,b){return typeof b.getElementsByTagName!==V?b.getElementsByTagName(a):void 0}:function(a,b){var c,d=[],e=0,f=b.getElementsByTagName(a);if("*"===a){for(;c=f[e++];)1===c.nodeType&&d.push(c);return d}return f},w.find.CLASS=v.getElementsByClassName&&function(a,b){return typeof b.getElementsByClassName!==V&&I?b.getElementsByClassName(a):void 0},K=[],J=[],(v.qsa=ra.test(c.querySelectorAll))&&(e(function(a){a.innerHTML="<select msallowclip=''><option selected=''></option></select>",a.querySelectorAll("[msallowclip^='']").length&&J.push("[*^$]="+da+"*(?:''|\"\")"),a.querySelectorAll("[selected]").length||J.push("\\["+da+"*(?:value|"+ca+")"),a.querySelectorAll(":checked").length||J.push(":checked")}),e(function(a){var b=c.createElement("input");b.setAttribute("type","hidden"),a.appendChild(b).setAttribute("name","D"),a.querySelectorAll("[name=d]").length&&J.push("name"+da+"*[*^$|!~]?="),a.querySelectorAll(":enabled").length||J.push(":enabled",":disabled"),a.querySelectorAll("*,:x"),J.push(",.*:")})),(v.matchesSelector=ra.test(L=H.matches||H.webkitMatchesSelector||H.mozMatchesSelector||H.oMatchesSelector||H.msMatchesSelector))&&e(function(a){v.disconnectedMatch=L.call(a,"div"),L.call(a,"[s!='']:x"),K.push("!=",ha)}),J=J.length&&new RegExp(J.join("|")),K=K.length&&new RegExp(K.join("|")),b=ra.test(H.compareDocumentPosition),M=b||ra.test(H.contains)?function(a,b){var c=9===a.nodeType?a.documentElement:a,d=b&&b.parentNode;return a===d||!(!d||1!==d.nodeType||!(c.contains?c.contains(d):a.compareDocumentPosition&&16&a.compareDocumentPosition(d)))}:function(a,b){if(b)for(;b=b.parentNode;)if(b===a)return!0;return!1},U=b?function(a,b){if(a===b)return E=!0,0;var d=!a.compareDocumentPosition-!b.compareDocumentPosition;return d?d:(d=(a.ownerDocument||a)===(b.ownerDocument||b)?a.compareDocumentPosition(b):1,1&d||!v.sortDetached&&b.compareDocumentPosition(a)===d?a===c||a.ownerDocument===O&&M(O,a)?-1:b===c||b.ownerDocument===O&&M(O,b)?1:D?ba.call(D,a)-ba.call(D,b):0:4&d?-1:1)}:function(a,b){if(a===b)return E=!0,0;var d,e=0,f=a.parentNode,h=b.parentNode,i=[a],j=[b];if(!f||!h)return a===c?-1:b===c?1:f?-1:h?1:D?ba.call(D,a)-ba.call(D,b):0;if(f===h)return g(a,b);for(d=a;d=d.parentNode;)i.unshift(d);for(d=b;d=d.parentNode;)j.unshift(d);for(;i[e]===j[e];)e++;return e?g(i[e],j[e]):i[e]===O?-1:j[e]===O?1:0},c):G},b.matches=function(a,c){return b(a,null,null,c)},b.matchesSelector=function(a,c){if((a.ownerDocument||a)!==G&&F(a),c=c.replace(la,"='$1']"),v.matchesSelector&&I&&(!K||!K.test(c))&&(!J||!J.test(c)))try{var d=L.call(a,c);if(d||v.disconnectedMatch||a.document&&11!==a.document.nodeType)return d}catch(e){}return b(c,G,null,[a]).length>0},b.contains=function(a,b){return(a.ownerDocument||a)!==G&&F(a),M(a,b)},b.attr=function(a,b){(a.ownerDocument||a)!==G&&F(a);var c=w.attrHandle[b.toLowerCase()],d=c&&X.call(w.attrHandle,b.toLowerCase())?c(a,b,!I):void 0;return void 0!==d?d:v.attributes||!I?a.getAttribute(b):(d=a.getAttributeNode(b))&&d.specified?d.value:null},b.error=function(a){throw new Error("Syntax error, unrecognized expression: "+a)},b.uniqueSort=function(a){var b,c=[],d=0,e=0;if(E=!v.detectDuplicates,D=!v.sortStable&&a.slice(0),a.sort(U),E){for(;b=a[e++];)b===a[e]&&(d=c.push(e));for(;d--;)a.splice(c[d],1)}return D=null,a},x=b.getText=function(a){var b,c="",d=0,e=a.nodeType;if(e){if(1===e||9===e||11===e){if("string"==typeof a.textContent)return a.textContent;for(a=a.firstChild;a;a=a.nextSibling)c+=x(a)}else if(3===e||4===e)return a.nodeValue}else for(;b=a[d++];)c+=x(b);return c},w=b.selectors={cacheLength:50,createPseudo:d,match:oa,attrHandle:{},find:{},relative:{">":{dir:"parentNode",first:!0}," ":{dir:"parentNode"},"+":{dir:"previousSibling",first:!0},"~":{dir:"previousSibling"}},preFilter:{ATTR:function(a){return a[1]=a[1].replace(va,wa),a[3]=(a[3]||a[4]||a[5]||"").replace(va,wa),"~="===a[2]&&(a[3]=" "+a[3]+" "),a.slice(0,4)},CHILD:function(a){return a[1]=a[1].toLowerCase(),"nth"===a[1].slice(0,3)?(a[3]||b.error(a[0]),a[4]=+(a[4]?a[5]+(a[6]||1):2*("even"===a[3]||"odd"===a[3])),a[5]=+(a[7]+a[8]||"odd"===a[3])):a[3]&&b.error(a[0]),a},PSEUDO:function(a){var b,c=!a[6]&&a[2];return oa.CHILD.test(a[0])?null:(a[3]?a[2]=a[4]||a[5]||"":c&&ma.test(c)&&(b=z(c,!0))&&(b=c.indexOf(")",c.length-b)-c.length)&&(a[0]=a[0].slice(0,b),a[2]=c.slice(0,b)),a.slice(0,3))}},filter:{TAG:function(a){var b=a.replace(va,wa).toLowerCase();return"*"===a?function(){return!0}:function(a){return a.nodeName&&a.nodeName.toLowerCase()===b}},CLASS:function(a){var b=R[a+" "];return b||(b=new RegExp("(^|"+da+")"+a+"("+da+"|$)"))&&R(a,function(a){return b.test("string"==typeof a.className&&a.className||typeof a.getAttribute!==V&&a.getAttribute("class")||"")})},ATTR:function(a,c,d){return function(e){var f=b.attr(e,a);return null==f?"!="===c:c?(f+="","="===c?f===d:"!="===c?f!==d:"^="===c?d&&0===f.indexOf(d):"*="===c?d&&f.indexOf(d)>-1:"$="===c?d&&f.slice(-d.length)===d:"~="===c?(" "+f+" ").indexOf(d)>-1:"|="===c?f===d||f.slice(0,d.length+1)===d+"-":!1):!0}},CHILD:function(a,b,c,d,e){var f="nth"!==a.slice(0,3),g="last"!==a.slice(-4),h="of-type"===b;return 1===d&&0===e?function(a){return!!a.parentNode}:function(b,c,i){var j,k,l,m,n,o,p=f!==g?"nextSibling":"previousSibling",q=b.parentNode,r=h&&b.nodeName.toLowerCase(),s=!i&&!h;if(q){if(f){for(;p;){for(l=b;l=l[p];)if(h?l.nodeName.toLowerCase()===r:1===l.nodeType)return!1;o=p="only"===a&&!o&&"nextSibling"}return!0}if(o=[g?q.firstChild:q.lastChild],g&&s){for(k=q[N]||(q[N]={}),j=k[a]||[],n=j[0]===P&&j[1],m=j[0]===P&&j[2],l=n&&q.childNodes[n];l=++n&&l&&l[p]||(m=n=0)||o.pop();)if(1===l.nodeType&&++m&&l===b){k[a]=[P,n,m];break}}else if(s&&(j=(b[N]||(b[N]={}))[a])&&j[0]===P)m=j[1];else for(;(l=++n&&l&&l[p]||(m=n=0)||o.pop())&&((h?l.nodeName.toLowerCase()!==r:1!==l.nodeType)||!++m||(s&&((l[N]||(l[N]={}))[a]=[P,m]),l!==b)););return m-=e,m===d||m%d===0&&m/d>=0}}},PSEUDO:function(a,c){var e,f=w.pseudos[a]||w.setFilters[a.toLowerCase()]||b.error("unsupported pseudo: "+a);return f[N]?f(c):f.length>1?(e=[a,a,"",c],w.setFilters.hasOwnProperty(a.toLowerCase())?d(function(a,b){for(var d,e=f(a,c),g=e.length;g--;)d=ba.call(a,e[g]),a[d]=!(b[d]=e[g])}):function(a){return f(a,0,e)}):f}},pseudos:{not:d(function(a){var b=[],c=[],e=A(a.replace(ia,"$1"));return e[N]?d(function(a,b,c,d){for(var f,g=e(a,null,d,[]),h=a.length;h--;)(f=g[h])&&(a[h]=!(b[h]=f))}):function(a,d,f){return b[0]=a,e(b,null,f,c),!c.pop()}}),has:d(function(a){return function(c){return b(a,c).length>0}}),contains:d(function(a){return function(b){return(b.textContent||b.innerText||x(b)).indexOf(a)>-1}}),lang:d(function(a){return na.test(a||"")||b.error("unsupported lang: "+a),a=a.replace(va,wa).toLowerCase(),function(b){var c;do if(c=I?b.lang:b.getAttribute("xml:lang")||b.getAttribute("lang"))return c=c.toLowerCase(),c===a||0===c.indexOf(a+"-");while((b=b.parentNode)&&1===b.nodeType);return!1}}),target:function(b){var c=a.location&&a.location.hash;return c&&c.slice(1)===b.id},root:function(a){return a===H},focus:function(a){return a===G.activeElement&&(!G.hasFocus||G.hasFocus())&&!!(a.type||a.href||~a.tabIndex)},enabled:function(a){return a.disabled===!1},disabled:function(a){return a.disabled===!0},checked:function(a){var b=a.nodeName.toLowerCase();return"input"===b&&!!a.checked||"option"===b&&!!a.selected},selected:function(a){return a.parentNode&&a.parentNode.selectedIndex,a.selected===!0},empty:function(a){for(a=a.firstChild;a;a=a.nextSibling)if(a.nodeType<6)return!1;return!0},parent:function(a){return!w.pseudos.empty(a)},header:function(a){return qa.test(a.nodeName)},input:function(a){return pa.test(a.nodeName)},button:function(a){var b=a.nodeName.toLowerCase();return"input"===b&&"button"===a.type||"button"===b},text:function(a){var b;return"input"===a.nodeName.toLowerCase()&&"text"===a.type&&(null==(b=a.getAttribute("type"))||"text"===b.toLowerCase())},first:j(function(){return[0]}),last:j(function(a,b){return[b-1]}),eq:j(function(a,b,c){return[0>c?c+b:c]}),even:j(function(a,b){for(var c=0;b>c;c+=2)a.push(c);return a}),odd:j(function(a,b){for(var c=1;b>c;c+=2)a.push(c);return a}),lt:j(function(a,b,c){for(var d=0>c?c+b:c;--d>=0;)a.push(d);return a}),gt:j(function(a,b,c){for(var d=0>c?c+b:c;++d<b;)a.push(d);return a})}},w.pseudos.nth=w.pseudos.eq;for(u in{radio:!0,checkbox:!0,file:!0,password:!0,image:!0})w.pseudos[u]=h(u);for(u in{submit:!0,reset:!0})w.pseudos[u]=i(u);return l.prototype=w.filters=w.pseudos,w.setFilters=new l,z=b.tokenize=function(a,c){var d,e,f,g,h,i,j,k=S[a+" "];if(k)return c?0:k.slice(0);for(h=a,i=[],j=w.preFilter;h;){(!d||(e=ja.exec(h)))&&(e&&(h=h.slice(e[0].length)||h),i.push(f=[])),d=!1,(e=ka.exec(h))&&(d=e.shift(),f.push({value:d,type:e[0].replace(ia," ")}),h=h.slice(d.length));for(g in w.filter)!(e=oa[g].exec(h))||j[g]&&!(e=j[g](e))||(d=e.shift(),f.push({value:d,type:g,matches:e
}),h=h.slice(d.length));if(!d)break}return c?h.length:h?b.error(a):S(a,i).slice(0)},A=b.compile=function(a,b){var c,d=[],e=[],f=T[a+" "];if(!f){for(b||(b=z(a)),c=b.length;c--;)f=s(b[c]),f[N]?d.push(f):e.push(f);f=T(a,t(e,d)),f.selector=a}return f},B=b.select=function(a,b,c,d){var e,f,g,h,i,j="function"==typeof a&&a,l=!d&&z(a=j.selector||a);if(c=c||[],1===l.length){if(f=l[0]=l[0].slice(0),f.length>2&&"ID"===(g=f[0]).type&&v.getById&&9===b.nodeType&&I&&w.relative[f[1].type]){if(b=(w.find.ID(g.matches[0].replace(va,wa),b)||[])[0],!b)return c;j&&(b=b.parentNode),a=a.slice(f.shift().value.length)}for(e=oa.needsContext.test(a)?0:f.length;e--&&(g=f[e],!w.relative[h=g.type]);)if((i=w.find[h])&&(d=i(g.matches[0].replace(va,wa),ta.test(f[0].type)&&k(b.parentNode)||b))){if(f.splice(e,1),a=d.length&&m(f),!a)return _.apply(c,d),c;break}}return(j||A(a,l))(d,b,!I,c,ta.test(a)&&k(b.parentNode)||b),c},v.sortStable=N.split("").sort(U).join("")===N,v.detectDuplicates=!!E,F(),v.sortDetached=e(function(a){return 1&a.compareDocumentPosition(G.createElement("div"))}),e(function(a){return a.innerHTML="<a href='#'></a>","#"===a.firstChild.getAttribute("href")})||f("type|href|height|width",function(a,b,c){return c?void 0:a.getAttribute(b,"type"===b.toLowerCase()?1:2)}),v.attributes&&e(function(a){return a.innerHTML="<input/>",a.firstChild.setAttribute("value",""),""===a.firstChild.getAttribute("value")})||f("value",function(a,b,c){return c||"input"!==a.nodeName.toLowerCase()?void 0:a.defaultValue}),e(function(a){return null==a.getAttribute("disabled")})||f(ca,function(a,b,c){var d;return c?void 0:a[b]===!0?b.toLowerCase():(d=a.getAttributeNode(b))&&d.specified?d.value:null}),b}(a);ea.find=ja,ea.expr=ja.selectors,ea.expr[":"]=ea.expr.pseudos,ea.unique=ja.uniqueSort,ea.text=ja.getText,ea.isXMLDoc=ja.isXML,ea.contains=ja.contains;var ka=ea.expr.match.needsContext,la=/^<(\w+)\s*\/?>(?:<\/\1>|)$/,ma=/^.[^:#\[\.,]*$/;ea.filter=function(a,b,c){var d=b[0];return c&&(a=":not("+a+")"),1===b.length&&1===d.nodeType?ea.find.matchesSelector(d,a)?[d]:[]:ea.find.matches(a,ea.grep(b,function(a){return 1===a.nodeType}))},ea.fn.extend({find:function(a){var b,c=[],d=this,e=d.length;if("string"!=typeof a)return this.pushStack(ea(a).filter(function(){for(b=0;e>b;b++)if(ea.contains(d[b],this))return!0}));for(b=0;e>b;b++)ea.find(a,d[b],c);return c=this.pushStack(e>1?ea.unique(c):c),c.selector=this.selector?this.selector+" "+a:a,c},filter:function(a){return this.pushStack(d(this,a||[],!1))},not:function(a){return this.pushStack(d(this,a||[],!0))},is:function(a){return!!d(this,"string"==typeof a&&ka.test(a)?ea(a):a||[],!1).length}});var na,oa=a.document,pa=/^(?:\s*(<[\w\W]+>)[^>]*|#([\w-]*))$/,qa=ea.fn.init=function(a,b){var c,d;if(!a)return this;if("string"==typeof a){if(c="<"===a.charAt(0)&&">"===a.charAt(a.length-1)&&a.length>=3?[null,a,null]:pa.exec(a),!c||!c[1]&&b)return!b||b.jquery?(b||na).find(a):this.constructor(b).find(a);if(c[1]){if(b=b instanceof ea?b[0]:b,ea.merge(this,ea.parseHTML(c[1],b&&b.nodeType?b.ownerDocument||b:oa,!0)),la.test(c[1])&&ea.isPlainObject(b))for(c in b)ea.isFunction(this[c])?this[c](b[c]):this.attr(c,b[c]);return this}if(d=oa.getElementById(c[2]),d&&d.parentNode){if(d.id!==c[2])return na.find(a);this.length=1,this[0]=d}return this.context=oa,this.selector=a,this}return a.nodeType?(this.context=this[0]=a,this.length=1,this):ea.isFunction(a)?"undefined"!=typeof na.ready?na.ready(a):a(ea):(void 0!==a.selector&&(this.selector=a.selector,this.context=a.context),ea.makeArray(a,this))};qa.prototype=ea.fn,na=ea(oa);var ra=/^(?:parents|prev(?:Until|All))/,sa={children:!0,contents:!0,next:!0,prev:!0};ea.extend({dir:function(a,b,c){for(var d=[],e=a[b];e&&9!==e.nodeType&&(void 0===c||1!==e.nodeType||!ea(e).is(c));)1===e.nodeType&&d.push(e),e=e[b];return d},sibling:function(a,b){for(var c=[];a;a=a.nextSibling)1===a.nodeType&&a!==b&&c.push(a);return c}}),ea.fn.extend({has:function(a){var b,c=ea(a,this),d=c.length;return this.filter(function(){for(b=0;d>b;b++)if(ea.contains(this,c[b]))return!0})},closest:function(a,b){for(var c,d=0,e=this.length,f=[],g=ka.test(a)||"string"!=typeof a?ea(a,b||this.context):0;e>d;d++)for(c=this[d];c&&c!==b;c=c.parentNode)if(c.nodeType<11&&(g?g.index(c)>-1:1===c.nodeType&&ea.find.matchesSelector(c,a))){f.push(c);break}return this.pushStack(f.length>1?ea.unique(f):f)},index:function(a){return a?"string"==typeof a?ea.inArray(this[0],ea(a)):ea.inArray(a.jquery?a[0]:a,this):this[0]&&this[0].parentNode?this.first().prevAll().length:-1},add:function(a,b){return this.pushStack(ea.unique(ea.merge(this.get(),ea(a,b))))},addBack:function(a){return this.add(null==a?this.prevObject:this.prevObject.filter(a))}}),ea.each({parent:function(a){var b=a.parentNode;return b&&11!==b.nodeType?b:null},parents:function(a){return ea.dir(a,"parentNode")},parentsUntil:function(a,b,c){return ea.dir(a,"parentNode",c)},next:function(a){return e(a,"nextSibling")},prev:function(a){return e(a,"previousSibling")},nextAll:function(a){return ea.dir(a,"nextSibling")},prevAll:function(a){return ea.dir(a,"previousSibling")},nextUntil:function(a,b,c){return ea.dir(a,"nextSibling",c)},prevUntil:function(a,b,c){return ea.dir(a,"previousSibling",c)},siblings:function(a){return ea.sibling((a.parentNode||{}).firstChild,a)},children:function(a){return ea.sibling(a.firstChild)},contents:function(a){return ea.nodeName(a,"iframe")?a.contentDocument||a.contentWindow.document:ea.merge([],a.childNodes)}},function(a,b){ea.fn[a]=function(c,d){var e=ea.map(this,b,c);return"Until"!==a.slice(-5)&&(d=c),d&&"string"==typeof d&&(e=ea.filter(d,e)),this.length>1&&(sa[a]||(e=ea.unique(e)),ra.test(a)&&(e=e.reverse())),this.pushStack(e)}});var ta=/\S+/g,ua={};ea.Callbacks=function(a){a="string"==typeof a?ua[a]||f(a):ea.extend({},a);var b,c,d,e,g,h,i=[],j=!a.once&&[],k=function(f){for(c=a.memory&&f,d=!0,g=h||0,h=0,e=i.length,b=!0;i&&e>g;g++)if(i[g].apply(f[0],f[1])===!1&&a.stopOnFalse){c=!1;break}b=!1,i&&(j?j.length&&k(j.shift()):c?i=[]:l.disable())},l={add:function(){if(i){var d=i.length;!function f(b){ea.each(b,function(b,c){var d=ea.type(c);"function"===d?a.unique&&l.has(c)||i.push(c):c&&c.length&&"string"!==d&&f(c)})}(arguments),b?e=i.length:c&&(h=d,k(c))}return this},remove:function(){return i&&ea.each(arguments,function(a,c){for(var d;(d=ea.inArray(c,i,d))>-1;)i.splice(d,1),b&&(e>=d&&e--,g>=d&&g--)}),this},has:function(a){return a?ea.inArray(a,i)>-1:!(!i||!i.length)},empty:function(){return i=[],e=0,this},disable:function(){return i=j=c=void 0,this},disabled:function(){return!i},lock:function(){return j=void 0,c||l.disable(),this},locked:function(){return!j},fireWith:function(a,c){return!i||d&&!j||(c=c||[],c=[a,c.slice?c.slice():c],b?j.push(c):k(c)),this},fire:function(){return l.fireWith(this,arguments),this},fired:function(){return!!d}};return l},ea.extend({Deferred:function(a){var b=[["resolve","done",ea.Callbacks("once memory"),"resolved"],["reject","fail",ea.Callbacks("once memory"),"rejected"],["notify","progress",ea.Callbacks("memory")]],c="pending",d={state:function(){return c},always:function(){return e.done(arguments).fail(arguments),this},then:function(){var a=arguments;return ea.Deferred(function(c){ea.each(b,function(b,f){var g=ea.isFunction(a[b])&&a[b];e[f[1]](function(){var a=g&&g.apply(this,arguments);a&&ea.isFunction(a.promise)?a.promise().done(c.resolve).fail(c.reject).progress(c.notify):c[f[0]+"With"](this===d?c.promise():this,g?[a]:arguments)})}),a=null}).promise()},promise:function(a){return null!=a?ea.extend(a,d):d}},e={};return d.pipe=d.then,ea.each(b,function(a,f){var g=f[2],h=f[3];d[f[1]]=g.add,h&&g.add(function(){c=h},b[1^a][2].disable,b[2][2].lock),e[f[0]]=function(){return e[f[0]+"With"](this===e?d:this,arguments),this},e[f[0]+"With"]=g.fireWith}),d.promise(e),a&&a.call(e,e),e},when:function(a){var b,c,d,e=0,f=X.call(arguments),g=f.length,h=1!==g||a&&ea.isFunction(a.promise)?g:0,i=1===h?a:ea.Deferred(),j=function(a,c,d){return function(e){c[a]=this,d[a]=arguments.length>1?X.call(arguments):e,d===b?i.notifyWith(c,d):--h||i.resolveWith(c,d)}};if(g>1)for(b=new Array(g),c=new Array(g),d=new Array(g);g>e;e++)f[e]&&ea.isFunction(f[e].promise)?f[e].promise().done(j(e,d,f)).fail(i.reject).progress(j(e,c,b)):--h;return h||i.resolveWith(d,f),i.promise()}});var va;ea.fn.ready=function(a){return ea.ready.promise().done(a),this},ea.extend({isReady:!1,readyWait:1,holdReady:function(a){a?ea.readyWait++:ea.ready(!0)},ready:function(a){if(a===!0?!--ea.readyWait:!ea.isReady){if(!oa.body)return setTimeout(ea.ready);ea.isReady=!0,a!==!0&&--ea.readyWait>0||(va.resolveWith(oa,[ea]),ea.fn.triggerHandler&&(ea(oa).triggerHandler("ready"),ea(oa).off("ready")))}}}),ea.ready.promise=function(b){if(!va)if(va=ea.Deferred(),"complete"===oa.readyState)setTimeout(ea.ready);else if(oa.addEventListener)oa.addEventListener("DOMContentLoaded",h,!1),a.addEventListener("load",h,!1);else{oa.attachEvent("onreadystatechange",h),a.attachEvent("onload",h);var c=!1;try{c=null==a.frameElement&&oa.documentElement}catch(d){}c&&c.doScroll&&!function e(){if(!ea.isReady){try{c.doScroll("left")}catch(a){return setTimeout(e,50)}g(),ea.ready()}}()}return va.promise(b)};var wa,xa="undefined";for(wa in ea(ca))break;ca.ownLast="0"!==wa,ca.inlineBlockNeedsLayout=!1,ea(function(){var a,b,c,d;c=oa.getElementsByTagName("body")[0],c&&c.style&&(b=oa.createElement("div"),d=oa.createElement("div"),d.style.cssText="position:absolute;border:0;width:0;height:0;top:0;left:-9999px",c.appendChild(d).appendChild(b),typeof b.style.zoom!==xa&&(b.style.cssText="display:inline;margin:0;border:0;padding:1px;width:1px;zoom:1",ca.inlineBlockNeedsLayout=a=3===b.offsetWidth,a&&(c.style.zoom=1)),c.removeChild(d))}),function(){var a=oa.createElement("div");if(null==ca.deleteExpando){ca.deleteExpando=!0;try{delete a.test}catch(b){ca.deleteExpando=!1}}a=null}(),ea.acceptData=function(a){var b=ea.noData[(a.nodeName+" ").toLowerCase()],c=+a.nodeType||1;return 1!==c&&9!==c?!1:!b||b!==!0&&a.getAttribute("classid")===b};var ya=/^(?:\{[\w\W]*\}|\[[\w\W]*\])$/,za=/([A-Z])/g;ea.extend({cache:{},noData:{"applet ":!0,"embed ":!0,"object ":"clsid:D27CDB6E-AE6D-11cf-96B8-444553540000"},hasData:function(a){return a=a.nodeType?ea.cache[a[ea.expando]]:a[ea.expando],!!a&&!j(a)},data:function(a,b,c){return k(a,b,c)},removeData:function(a,b){return l(a,b)},_data:function(a,b,c){return k(a,b,c,!0)},_removeData:function(a,b){return l(a,b,!0)}}),ea.fn.extend({data:function(a,b){var c,d,e,f=this[0],g=f&&f.attributes;if(void 0===a){if(this.length&&(e=ea.data(f),1===f.nodeType&&!ea._data(f,"parsedAttrs"))){for(c=g.length;c--;)g[c]&&(d=g[c].name,0===d.indexOf("data-")&&(d=ea.camelCase(d.slice(5)),i(f,d,e[d])));ea._data(f,"parsedAttrs",!0)}return e}return"object"==typeof a?this.each(function(){ea.data(this,a)}):arguments.length>1?this.each(function(){ea.data(this,a,b)}):f?i(f,a,ea.data(f,a)):void 0},removeData:function(a){return this.each(function(){ea.removeData(this,a)})}}),ea.extend({queue:function(a,b,c){var d;return a?(b=(b||"fx")+"queue",d=ea._data(a,b),c&&(!d||ea.isArray(c)?d=ea._data(a,b,ea.makeArray(c)):d.push(c)),d||[]):void 0},dequeue:function(a,b){b=b||"fx";var c=ea.queue(a,b),d=c.length,e=c.shift(),f=ea._queueHooks(a,b),g=function(){ea.dequeue(a,b)};"inprogress"===e&&(e=c.shift(),d--),e&&("fx"===b&&c.unshift("inprogress"),delete f.stop,e.call(a,g,f)),!d&&f&&f.empty.fire()},_queueHooks:function(a,b){var c=b+"queueHooks";return ea._data(a,c)||ea._data(a,c,{empty:ea.Callbacks("once memory").add(function(){ea._removeData(a,b+"queue"),ea._removeData(a,c)})})}}),ea.fn.extend({queue:function(a,b){var c=2;return"string"!=typeof a&&(b=a,a="fx",c--),arguments.length<c?ea.queue(this[0],a):void 0===b?this:this.each(function(){var c=ea.queue(this,a,b);ea._queueHooks(this,a),"fx"===a&&"inprogress"!==c[0]&&ea.dequeue(this,a)})},dequeue:function(a){return this.each(function(){ea.dequeue(this,a)})},clearQueue:function(a){return this.queue(a||"fx",[])},promise:function(a,b){var c,d=1,e=ea.Deferred(),f=this,g=this.length,h=function(){--d||e.resolveWith(f,[f])};for("string"!=typeof a&&(b=a,a=void 0),a=a||"fx";g--;)c=ea._data(f[g],a+"queueHooks"),c&&c.empty&&(d++,c.empty.add(h));return h(),e.promise(b)}});var Aa=/[+-]?(?:\d*\.|)\d+(?:[eE][+-]?\d+|)/.source,Ba=["Top","Right","Bottom","Left"],Ca=function(a,b){return a=b||a,"none"===ea.css(a,"display")||!ea.contains(a.ownerDocument,a)},Da=ea.access=function(a,b,c,d,e,f,g){var h=0,i=a.length,j=null==c;if("object"===ea.type(c)){e=!0;for(h in c)ea.access(a,b,h,c[h],!0,f,g)}else if(void 0!==d&&(e=!0,ea.isFunction(d)||(g=!0),j&&(g?(b.call(a,d),b=null):(j=b,b=function(a,b,c){return j.call(ea(a),c)})),b))for(;i>h;h++)b(a[h],c,g?d:d.call(a[h],h,b(a[h],c)));return e?a:j?b.call(a):i?b(a[0],c):f},Ea=/^(?:checkbox|radio)$/i;!function(){var a=oa.createElement("input"),b=oa.createElement("div"),c=oa.createDocumentFragment();if(b.innerHTML="  <link/><table></table><a href='/a'>a</a><input type='checkbox'/>",ca.leadingWhitespace=3===b.firstChild.nodeType,ca.tbody=!b.getElementsByTagName("tbody").length,ca.htmlSerialize=!!b.getElementsByTagName("link").length,ca.html5Clone="<:nav></:nav>"!==oa.createElement("nav").cloneNode(!0).outerHTML,a.type="checkbox",a.checked=!0,c.appendChild(a),ca.appendChecked=a.checked,b.innerHTML="<textarea>x</textarea>",ca.noCloneChecked=!!b.cloneNode(!0).lastChild.defaultValue,c.appendChild(b),b.innerHTML="<input type='radio' checked='checked' name='t'/>",ca.checkClone=b.cloneNode(!0).cloneNode(!0).lastChild.checked,ca.noCloneEvent=!0,b.attachEvent&&(b.attachEvent("onclick",function(){ca.noCloneEvent=!1}),b.cloneNode(!0).click()),null==ca.deleteExpando){ca.deleteExpando=!0;try{delete b.test}catch(d){ca.deleteExpando=!1}}}(),function(){var b,c,d=oa.createElement("div");for(b in{submit:!0,change:!0,focusin:!0})c="on"+b,(ca[b+"Bubbles"]=c in a)||(d.setAttribute(c,"t"),ca[b+"Bubbles"]=d.attributes[c].expando===!1);d=null}();var Fa=/^(?:input|select|textarea)$/i,Ga=/^key/,Ha=/^(?:mouse|pointer|contextmenu)|click/,Ia=/^(?:focusinfocus|focusoutblur)$/,Ja=/^([^.]*)(?:\.(.+)|)$/;ea.event={global:{},add:function(a,b,c,d,e){var f,g,h,i,j,k,l,m,n,o,p,q=ea._data(a);if(q){for(c.handler&&(i=c,c=i.handler,e=i.selector),c.guid||(c.guid=ea.guid++),(g=q.events)||(g=q.events={}),(k=q.handle)||(k=q.handle=function(a){return typeof ea===xa||a&&ea.event.triggered===a.type?void 0:ea.event.dispatch.apply(k.elem,arguments)},k.elem=a),b=(b||"").match(ta)||[""],h=b.length;h--;)f=Ja.exec(b[h])||[],n=p=f[1],o=(f[2]||"").split(".").sort(),n&&(j=ea.event.special[n]||{},n=(e?j.delegateType:j.bindType)||n,j=ea.event.special[n]||{},l=ea.extend({type:n,origType:p,data:d,handler:c,guid:c.guid,selector:e,needsContext:e&&ea.expr.match.needsContext.test(e),namespace:o.join(".")},i),(m=g[n])||(m=g[n]=[],m.delegateCount=0,j.setup&&j.setup.call(a,d,o,k)!==!1||(a.addEventListener?a.addEventListener(n,k,!1):a.attachEvent&&a.attachEvent("on"+n,k))),j.add&&(j.add.call(a,l),l.handler.guid||(l.handler.guid=c.guid)),e?m.splice(m.delegateCount++,0,l):m.push(l),ea.event.global[n]=!0);a=null}},remove:function(a,b,c,d,e){var f,g,h,i,j,k,l,m,n,o,p,q=ea.hasData(a)&&ea._data(a);if(q&&(k=q.events)){for(b=(b||"").match(ta)||[""],j=b.length;j--;)if(h=Ja.exec(b[j])||[],n=p=h[1],o=(h[2]||"").split(".").sort(),n){for(l=ea.event.special[n]||{},n=(d?l.delegateType:l.bindType)||n,m=k[n]||[],h=h[2]&&new RegExp("(^|\\.)"+o.join("\\.(?:.*\\.|)")+"(\\.|$)"),i=f=m.length;f--;)g=m[f],!e&&p!==g.origType||c&&c.guid!==g.guid||h&&!h.test(g.namespace)||d&&d!==g.selector&&("**"!==d||!g.selector)||(m.splice(f,1),g.selector&&m.delegateCount--,l.remove&&l.remove.call(a,g));i&&!m.length&&(l.teardown&&l.teardown.call(a,o,q.handle)!==!1||ea.removeEvent(a,n,q.handle),delete k[n])}else for(n in k)ea.event.remove(a,n+b[j],c,d,!0);ea.isEmptyObject(k)&&(delete q.handle,ea._removeData(a,"events"))}},trigger:function(b,c,d,e){var f,g,h,i,j,k,l,m=[d||oa],n=ba.call(b,"type")?b.type:b,o=ba.call(b,"namespace")?b.namespace.split("."):[];if(h=k=d=d||oa,3!==d.nodeType&&8!==d.nodeType&&!Ia.test(n+ea.event.triggered)&&(n.indexOf(".")>=0&&(o=n.split("."),n=o.shift(),o.sort()),g=n.indexOf(":")<0&&"on"+n,b=b[ea.expando]?b:new ea.Event(n,"object"==typeof b&&b),b.isTrigger=e?2:3,b.namespace=o.join("."),b.namespace_re=b.namespace?new RegExp("(^|\\.)"+o.join("\\.(?:.*\\.|)")+"(\\.|$)"):null,b.result=void 0,b.target||(b.target=d),c=null==c?[b]:ea.makeArray(c,[b]),j=ea.event.special[n]||{},e||!j.trigger||j.trigger.apply(d,c)!==!1)){if(!e&&!j.noBubble&&!ea.isWindow(d)){for(i=j.delegateType||n,Ia.test(i+n)||(h=h.parentNode);h;h=h.parentNode)m.push(h),k=h;k===(d.ownerDocument||oa)&&m.push(k.defaultView||k.parentWindow||a)}for(l=0;(h=m[l++])&&!b.isPropagationStopped();)b.type=l>1?i:j.bindType||n,f=(ea._data(h,"events")||{})[b.type]&&ea._data(h,"handle"),f&&f.apply(h,c),f=g&&h[g],f&&f.apply&&ea.acceptData(h)&&(b.result=f.apply(h,c),b.result===!1&&b.preventDefault());if(b.type=n,!e&&!b.isDefaultPrevented()&&(!j._default||j._default.apply(m.pop(),c)===!1)&&ea.acceptData(d)&&g&&d[n]&&!ea.isWindow(d)){k=d[g],k&&(d[g]=null),ea.event.triggered=n;try{d[n]()}catch(p){}ea.event.triggered=void 0,k&&(d[g]=k)}return b.result}},dispatch:function(a){a=ea.event.fix(a);var b,c,d,e,f,g=[],h=X.call(arguments),i=(ea._data(this,"events")||{})[a.type]||[],j=ea.event.special[a.type]||{};if(h[0]=a,a.delegateTarget=this,!j.preDispatch||j.preDispatch.call(this,a)!==!1){for(g=ea.event.handlers.call(this,a,i),b=0;(e=g[b++])&&!a.isPropagationStopped();)for(a.currentTarget=e.elem,f=0;(d=e.handlers[f++])&&!a.isImmediatePropagationStopped();)(!a.namespace_re||a.namespace_re.test(d.namespace))&&(a.handleObj=d,a.data=d.data,c=((ea.event.special[d.origType]||{}).handle||d.handler).apply(e.elem,h),void 0!==c&&(a.result=c)===!1&&(a.preventDefault(),a.stopPropagation()));return j.postDispatch&&j.postDispatch.call(this,a),a.result}},handlers:function(a,b){var c,d,e,f,g=[],h=b.delegateCount,i=a.target;if(h&&i.nodeType&&(!a.button||"click"!==a.type))for(;i!=this;i=i.parentNode||this)if(1===i.nodeType&&(i.disabled!==!0||"click"!==a.type)){for(e=[],f=0;h>f;f++)d=b[f],c=d.selector+" ",void 0===e[c]&&(e[c]=d.needsContext?ea(c,this).index(i)>=0:ea.find(c,this,null,[i]).length),e[c]&&e.push(d);e.length&&g.push({elem:i,handlers:e})}return h<b.length&&g.push({elem:this,handlers:b.slice(h)}),g},fix:function(a){if(a[ea.expando])return a;var b,c,d,e=a.type,f=a,g=this.fixHooks[e];for(g||(this.fixHooks[e]=g=Ha.test(e)?this.mouseHooks:Ga.test(e)?this.keyHooks:{}),d=g.props?this.props.concat(g.props):this.props,a=new ea.Event(f),b=d.length;b--;)c=d[b],a[c]=f[c];return a.target||(a.target=f.srcElement||oa),3===a.target.nodeType&&(a.target=a.target.parentNode),a.metaKey=!!a.metaKey,g.filter?g.filter(a,f):a},props:"altKey bubbles cancelable ctrlKey currentTarget eventPhase metaKey relatedTarget shiftKey target timeStamp view which".split(" "),fixHooks:{},keyHooks:{props:"char charCode key keyCode".split(" "),filter:function(a,b){return null==a.which&&(a.which=null!=b.charCode?b.charCode:b.keyCode),a}},mouseHooks:{props:"button buttons clientX clientY fromElement offsetX offsetY pageX pageY screenX screenY toElement".split(" "),filter:function(a,b){var c,d,e,f=b.button,g=b.fromElement;return null==a.pageX&&null!=b.clientX&&(d=a.target.ownerDocument||oa,e=d.documentElement,c=d.body,a.pageX=b.clientX+(e&&e.scrollLeft||c&&c.scrollLeft||0)-(e&&e.clientLeft||c&&c.clientLeft||0),a.pageY=b.clientY+(e&&e.scrollTop||c&&c.scrollTop||0)-(e&&e.clientTop||c&&c.clientTop||0)),!a.relatedTarget&&g&&(a.relatedTarget=g===a.target?b.toElement:g),a.which||void 0===f||(a.which=1&f?1:2&f?3:4&f?2:0),a}},special:{load:{noBubble:!0},focus:{trigger:function(){if(this!==o()&&this.focus)try{return this.focus(),!1}catch(a){}},delegateType:"focusin"},blur:{trigger:function(){return this===o()&&this.blur?(this.blur(),!1):void 0},delegateType:"focusout"},click:{trigger:function(){return ea.nodeName(this,"input")&&"checkbox"===this.type&&this.click?(this.click(),!1):void 0},_default:function(a){return ea.nodeName(a.target,"a")}},beforeunload:{postDispatch:function(a){void 0!==a.result&&a.originalEvent&&(a.originalEvent.returnValue=a.result)}}},simulate:function(a,b,c,d){var e=ea.extend(new ea.Event,c,{type:a,isSimulated:!0,originalEvent:{}});d?ea.event.trigger(e,null,b):ea.event.dispatch.call(b,e),e.isDefaultPrevented()&&c.preventDefault()}},ea.removeEvent=oa.removeEventListener?function(a,b,c){a.removeEventListener&&a.removeEventListener(b,c,!1)}:function(a,b,c){var d="on"+b;a.detachEvent&&(typeof a[d]===xa&&(a[d]=null),a.detachEvent(d,c))},ea.Event=function(a,b){return this instanceof ea.Event?(a&&a.type?(this.originalEvent=a,this.type=a.type,this.isDefaultPrevented=a.defaultPrevented||void 0===a.defaultPrevented&&a.returnValue===!1?m:n):this.type=a,b&&ea.extend(this,b),this.timeStamp=a&&a.timeStamp||ea.now(),void(this[ea.expando]=!0)):new ea.Event(a,b)},ea.Event.prototype={isDefaultPrevented:n,isPropagationStopped:n,isImmediatePropagationStopped:n,preventDefault:function(){var a=this.originalEvent;this.isDefaultPrevented=m,a&&(a.preventDefault?a.preventDefault():a.returnValue=!1)},stopPropagation:function(){var a=this.originalEvent;this.isPropagationStopped=m,a&&(a.stopPropagation&&a.stopPropagation(),a.cancelBubble=!0)},stopImmediatePropagation:function(){var a=this.originalEvent;this.isImmediatePropagationStopped=m,a&&a.stopImmediatePropagation&&a.stopImmediatePropagation(),this.stopPropagation()}},ea.each({mouseenter:"mouseover",mouseleave:"mouseout",pointerenter:"pointerover",pointerleave:"pointerout"},function(a,b){ea.event.special[a]={delegateType:b,bindType:b,handle:function(a){var c,d=this,e=a.relatedTarget,f=a.handleObj;return(!e||e!==d&&!ea.contains(d,e))&&(a.type=f.origType,c=f.handler.apply(this,arguments),a.type=b),c}}}),ca.submitBubbles||(ea.event.special.submit={setup:function(){return ea.nodeName(this,"form")?!1:void ea.event.add(this,"click._submit keypress._submit",function(a){var b=a.target,c=ea.nodeName(b,"input")||ea.nodeName(b,"button")?b.form:void 0;c&&!ea._data(c,"submitBubbles")&&(ea.event.add(c,"submit._submit",function(a){a._submit_bubble=!0}),ea._data(c,"submitBubbles",!0))})},postDispatch:function(a){a._submit_bubble&&(delete a._submit_bubble,this.parentNode&&!a.isTrigger&&ea.event.simulate("submit",this.parentNode,a,!0))},teardown:function(){return ea.nodeName(this,"form")?!1:void ea.event.remove(this,"._submit")}}),ca.changeBubbles||(ea.event.special.change={setup:function(){return Fa.test(this.nodeName)?(("checkbox"===this.type||"radio"===this.type)&&(ea.event.add(this,"propertychange._change",function(a){"checked"===a.originalEvent.propertyName&&(this._just_changed=!0)}),ea.event.add(this,"click._change",function(a){this._just_changed&&!a.isTrigger&&(this._just_changed=!1),ea.event.simulate("change",this,a,!0)})),!1):void ea.event.add(this,"beforeactivate._change",function(a){var b=a.target;Fa.test(b.nodeName)&&!ea._data(b,"changeBubbles")&&(ea.event.add(b,"change._change",function(a){!this.parentNode||a.isSimulated||a.isTrigger||ea.event.simulate("change",this.parentNode,a,!0)}),ea._data(b,"changeBubbles",!0))})},handle:function(a){var b=a.target;return this!==b||a.isSimulated||a.isTrigger||"radio"!==b.type&&"checkbox"!==b.type?a.handleObj.handler.apply(this,arguments):void 0},teardown:function(){return ea.event.remove(this,"._change"),!Fa.test(this.nodeName)}}),ca.focusinBubbles||ea.each({focus:"focusin",blur:"focusout"},function(a,b){var c=function(a){ea.event.simulate(b,a.target,ea.event.fix(a),!0)};ea.event.special[b]={setup:function(){var d=this.ownerDocument||this,e=ea._data(d,b);e||d.addEventListener(a,c,!0),ea._data(d,b,(e||0)+1)},teardown:function(){var d=this.ownerDocument||this,e=ea._data(d,b)-1;e?ea._data(d,b,e):(d.removeEventListener(a,c,!0),ea._removeData(d,b))}}}),ea.fn.extend({on:function(a,b,c,d,e){var f,g;if("object"==typeof a){"string"!=typeof b&&(c=c||b,b=void 0);for(f in a)this.on(f,b,c,a[f],e);return this}if(null==c&&null==d?(d=b,c=b=void 0):null==d&&("string"==typeof b?(d=c,c=void 0):(d=c,c=b,b=void 0)),d===!1)d=n;else if(!d)return this;return 1===e&&(g=d,d=function(a){return ea().off(a),g.apply(this,arguments)},d.guid=g.guid||(g.guid=ea.guid++)),this.each(function(){ea.event.add(this,a,d,c,b)})},one:function(a,b,c,d){return this.on(a,b,c,d,1)},off:function(a,b,c){var d,e;if(a&&a.preventDefault&&a.handleObj)return d=a.handleObj,ea(a.delegateTarget).off(d.namespace?d.origType+"."+d.namespace:d.origType,d.selector,d.handler),this;if("object"==typeof a){for(e in a)this.off(e,b,a[e]);return this}return(b===!1||"function"==typeof b)&&(c=b,b=void 0),c===!1&&(c=n),this.each(function(){ea.event.remove(this,a,c,b)})},trigger:function(a,b){return this.each(function(){ea.event.trigger(a,b,this)})},triggerHandler:function(a,b){var c=this[0];return c?ea.event.trigger(a,b,c,!0):void 0}});var Ka="abbr|article|aside|audio|bdi|canvas|data|datalist|details|figcaption|figure|footer|header|hgroup|mark|meter|nav|output|progress|section|summary|time|video",La=/ jQuery\d+="(?:null|\d+)"/g,Ma=new RegExp("<(?:"+Ka+")[\\s/>]","i"),Na=/^\s+/,Oa=/<(?!area|br|col|embed|hr|img|input|link|meta|param)(([\w:]+)[^>]*)\/>/gi,Pa=/<([\w:]+)/,Qa=/<tbody/i,Ra=/<|&#?\w+;/,Sa=/<(?:script|style|link)/i,Ta=/checked\s*(?:[^=]|=\s*.checked.)/i,Ua=/^$|\/(?:java|ecma)script/i,Va=/^true\/(.*)/,Wa=/^\s*<!(?:\[CDATA\[|--)|(?:\]\]|--)>\s*$/g,Xa={option:[1,"<select multiple='multiple'>","</select>"],legend:[1,"<fieldset>","</fieldset>"],area:[1,"<map>","</map>"],param:[1,"<object>","</object>"],thead:[1,"<table>","</table>"],tr:[2,"<table><tbody>","</tbody></table>"],col:[2,"<table><tbody></tbody><colgroup>","</colgroup></table>"],td:[3,"<table><tbody><tr>","</tr></tbody></table>"],_default:ca.htmlSerialize?[0,"",""]:[1,"X<div>","</div>"]},Ya=p(oa),Za=Ya.appendChild(oa.createElement("div"));Xa.optgroup=Xa.option,Xa.tbody=Xa.tfoot=Xa.colgroup=Xa.caption=Xa.thead,Xa.th=Xa.td,ea.extend({clone:function(a,b,c){var d,e,f,g,h,i=ea.contains(a.ownerDocument,a);if(ca.html5Clone||ea.isXMLDoc(a)||!Ma.test("<"+a.nodeName+">")?f=a.cloneNode(!0):(Za.innerHTML=a.outerHTML,Za.removeChild(f=Za.firstChild)),!(ca.noCloneEvent&&ca.noCloneChecked||1!==a.nodeType&&11!==a.nodeType||ea.isXMLDoc(a)))for(d=q(f),h=q(a),g=0;null!=(e=h[g]);++g)d[g]&&x(e,d[g]);if(b)if(c)for(h=h||q(a),d=d||q(f),g=0;null!=(e=h[g]);g++)w(e,d[g]);else w(a,f);return d=q(f,"script"),d.length>0&&v(d,!i&&q(a,"script")),d=h=e=null,f},buildFragment:function(a,b,c,d){for(var e,f,g,h,i,j,k,l=a.length,m=p(b),n=[],o=0;l>o;o++)if(f=a[o],f||0===f)if("object"===ea.type(f))ea.merge(n,f.nodeType?[f]:f);else if(Ra.test(f)){for(h=h||m.appendChild(b.createElement("div")),i=(Pa.exec(f)||["",""])[1].toLowerCase(),k=Xa[i]||Xa._default,h.innerHTML=k[1]+f.replace(Oa,"<$1></$2>")+k[2],e=k[0];e--;)h=h.lastChild;if(!ca.leadingWhitespace&&Na.test(f)&&n.push(b.createTextNode(Na.exec(f)[0])),!ca.tbody)for(f="table"!==i||Qa.test(f)?"<table>"!==k[1]||Qa.test(f)?0:h:h.firstChild,e=f&&f.childNodes.length;e--;)ea.nodeName(j=f.childNodes[e],"tbody")&&!j.childNodes.length&&f.removeChild(j);for(ea.merge(n,h.childNodes),h.textContent="";h.firstChild;)h.removeChild(h.firstChild);h=m.lastChild}else n.push(b.createTextNode(f));for(h&&m.removeChild(h),ca.appendChecked||ea.grep(q(n,"input"),r),o=0;f=n[o++];)if((!d||-1===ea.inArray(f,d))&&(g=ea.contains(f.ownerDocument,f),h=q(m.appendChild(f),"script"),g&&v(h),c))for(e=0;f=h[e++];)Ua.test(f.type||"")&&c.push(f);return h=null,m},cleanData:function(a,b){for(var c,d,e,f,g=0,h=ea.expando,i=ea.cache,j=ca.deleteExpando,k=ea.event.special;null!=(c=a[g]);g++)if((b||ea.acceptData(c))&&(e=c[h],f=e&&i[e])){if(f.events)for(d in f.events)k[d]?ea.event.remove(c,d):ea.removeEvent(c,d,f.handle);i[e]&&(delete i[e],j?delete c[h]:typeof c.removeAttribute!==xa?c.removeAttribute(h):c[h]=null,W.push(e))}}}),ea.fn.extend({text:function(a){return Da(this,function(a){return void 0===a?ea.text(this):this.empty().append((this[0]&&this[0].ownerDocument||oa).createTextNode(a))},null,a,arguments.length)},append:function(){return this.domManip(arguments,function(a){if(1===this.nodeType||11===this.nodeType||9===this.nodeType){var b=s(this,a);b.appendChild(a)}})},prepend:function(){return this.domManip(arguments,function(a){if(1===this.nodeType||11===this.nodeType||9===this.nodeType){var b=s(this,a);b.insertBefore(a,b.firstChild)}})},before:function(){return this.domManip(arguments,function(a){this.parentNode&&this.parentNode.insertBefore(a,this)})},after:function(){return this.domManip(arguments,function(a){this.parentNode&&this.parentNode.insertBefore(a,this.nextSibling)})},remove:function(a,b){for(var c,d=a?ea.filter(a,this):this,e=0;null!=(c=d[e]);e++)b||1!==c.nodeType||ea.cleanData(q(c)),c.parentNode&&(b&&ea.contains(c.ownerDocument,c)&&v(q(c,"script")),c.parentNode.removeChild(c));return this},empty:function(){for(var a,b=0;null!=(a=this[b]);b++){for(1===a.nodeType&&ea.cleanData(q(a,!1));a.firstChild;)a.removeChild(a.firstChild);a.options&&ea.nodeName(a,"select")&&(a.options.length=0)}return this},clone:function(a,b){return a=null==a?!1:a,b=null==b?a:b,this.map(function(){return ea.clone(this,a,b)})},html:function(a){return Da(this,function(a){var b=this[0]||{},c=0,d=this.length;if(void 0===a)return 1===b.nodeType?b.innerHTML.replace(La,""):void 0;if("string"==typeof a&&!Sa.test(a)&&(ca.htmlSerialize||!Ma.test(a))&&(ca.leadingWhitespace||!Na.test(a))&&!Xa[(Pa.exec(a)||["",""])[1].toLowerCase()]){a=a.replace(Oa,"<$1></$2>");try{for(;d>c;c++)b=this[c]||{},1===b.nodeType&&(ea.cleanData(q(b,!1)),b.innerHTML=a);b=0}catch(e){}}b&&this.empty().append(a)},null,a,arguments.length)},replaceWith:function(){var a=arguments[0];return this.domManip(arguments,function(b){a=this.parentNode,ea.cleanData(q(this)),a&&a.replaceChild(b,this)}),a&&(a.length||a.nodeType)?this:this.remove()},detach:function(a){return this.remove(a,!0)},domManip:function(a,b){a=Y.apply([],a);var c,d,e,f,g,h,i=0,j=this.length,k=this,l=j-1,m=a[0],n=ea.isFunction(m);if(n||j>1&&"string"==typeof m&&!ca.checkClone&&Ta.test(m))return this.each(function(c){var d=k.eq(c);n&&(a[0]=m.call(this,c,d.html())),d.domManip(a,b)});if(j&&(h=ea.buildFragment(a,this[0].ownerDocument,!1,this),c=h.firstChild,1===h.childNodes.length&&(h=c),c)){for(f=ea.map(q(h,"script"),t),e=f.length;j>i;i++)d=h,i!==l&&(d=ea.clone(d,!0,!0),e&&ea.merge(f,q(d,"script"))),b.call(this[i],d,i);if(e)for(g=f[f.length-1].ownerDocument,ea.map(f,u),i=0;e>i;i++)d=f[i],Ua.test(d.type||"")&&!ea._data(d,"globalEval")&&ea.contains(g,d)&&(d.src?ea._evalUrl&&ea._evalUrl(d.src):ea.globalEval((d.text||d.textContent||d.innerHTML||"").replace(Wa,"")));h=c=null}return this}}),ea.each({appendTo:"append",prependTo:"prepend",insertBefore:"before",insertAfter:"after",replaceAll:"replaceWith"},function(a,b){ea.fn[a]=function(a){for(var c,d=0,e=[],f=ea(a),g=f.length-1;g>=d;d++)c=d===g?this:this.clone(!0),ea(f[d])[b](c),Z.apply(e,c.get());return this.pushStack(e)}});var $a,_a={};!function(){var a;ca.shrinkWrapBlocks=function(){if(null!=a)return a;a=!1;var b,c,d;return c=oa.getElementsByTagName("body")[0],c&&c.style?(b=oa.createElement("div"),d=oa.createElement("div"),d.style.cssText="position:absolute;border:0;width:0;height:0;top:0;left:-9999px",c.appendChild(d).appendChild(b),typeof b.style.zoom!==xa&&(b.style.cssText="-webkit-box-sizing:content-box;-moz-box-sizing:content-box;box-sizing:content-box;display:block;margin:0;border:0;padding:1px;width:1px;zoom:1",b.appendChild(oa.createElement("div")).style.width="5px",a=3!==b.offsetWidth),c.removeChild(d),a):void 0}}();var ab,bb,cb=/^margin/,db=new RegExp("^("+Aa+")(?!px)[a-z%]+$","i"),eb=/^(top|right|bottom|left)$/;a.getComputedStyle?(ab=function(a){return a.ownerDocument.defaultView.getComputedStyle(a,null)},bb=function(a,b,c){var d,e,f,g,h=a.style;return c=c||ab(a),g=c?c.getPropertyValue(b)||c[b]:void 0,c&&(""!==g||ea.contains(a.ownerDocument,a)||(g=ea.style(a,b)),db.test(g)&&cb.test(b)&&(d=h.width,e=h.minWidth,f=h.maxWidth,h.minWidth=h.maxWidth=h.width=g,g=c.width,h.width=d,h.minWidth=e,h.maxWidth=f)),
void 0===g?g:g+""}):oa.documentElement.currentStyle&&(ab=function(a){return a.currentStyle},bb=function(a,b,c){var d,e,f,g,h=a.style;return c=c||ab(a),g=c?c[b]:void 0,null==g&&h&&h[b]&&(g=h[b]),db.test(g)&&!eb.test(b)&&(d=h.left,e=a.runtimeStyle,f=e&&e.left,f&&(e.left=a.currentStyle.left),h.left="fontSize"===b?"1em":g,g=h.pixelLeft+"px",h.left=d,f&&(e.left=f)),void 0===g?g:g+""||"auto"}),function(){function b(){var b,c,d,e;c=oa.getElementsByTagName("body")[0],c&&c.style&&(b=oa.createElement("div"),d=oa.createElement("div"),d.style.cssText="position:absolute;border:0;width:0;height:0;top:0;left:-9999px",c.appendChild(d).appendChild(b),b.style.cssText="-webkit-box-sizing:border-box;-moz-box-sizing:border-box;box-sizing:border-box;display:block;margin-top:1%;top:1%;border:1px;padding:1px;width:4px;position:absolute",f=g=!1,i=!0,a.getComputedStyle&&(f="1%"!==(a.getComputedStyle(b,null)||{}).top,g="4px"===(a.getComputedStyle(b,null)||{width:"4px"}).width,e=b.appendChild(oa.createElement("div")),e.style.cssText=b.style.cssText="-webkit-box-sizing:content-box;-moz-box-sizing:content-box;box-sizing:content-box;display:block;margin:0;border:0;padding:0",e.style.marginRight=e.style.width="0",b.style.width="1px",i=!parseFloat((a.getComputedStyle(e,null)||{}).marginRight)),b.innerHTML="<table><tr><td></td><td>t</td></tr></table>",e=b.getElementsByTagName("td"),e[0].style.cssText="margin:0;border:0;padding:0;display:none",h=0===e[0].offsetHeight,h&&(e[0].style.display="",e[1].style.display="none",h=0===e[0].offsetHeight),c.removeChild(d))}var c,d,e,f,g,h,i;c=oa.createElement("div"),c.innerHTML="  <link/><table></table><a href='/a'>a</a><input type='checkbox'/>",e=c.getElementsByTagName("a")[0],d=e&&e.style,d&&(d.cssText="float:left;opacity:.5",ca.opacity="0.5"===d.opacity,ca.cssFloat=!!d.cssFloat,c.style.backgroundClip="content-box",c.cloneNode(!0).style.backgroundClip="",ca.clearCloneStyle="content-box"===c.style.backgroundClip,ca.boxSizing=""===d.boxSizing||""===d.MozBoxSizing||""===d.WebkitBoxSizing,ea.extend(ca,{reliableHiddenOffsets:function(){return null==h&&b(),h},boxSizingReliable:function(){return null==g&&b(),g},pixelPosition:function(){return null==f&&b(),f},reliableMarginRight:function(){return null==i&&b(),i}}))}(),ea.swap=function(a,b,c,d){var e,f,g={};for(f in b)g[f]=a.style[f],a.style[f]=b[f];e=c.apply(a,d||[]);for(f in b)a.style[f]=g[f];return e};var fb=/alpha\([^)]*\)/i,gb=/opacity\s*=\s*([^)]*)/,hb=/^(none|table(?!-c[ea]).+)/,ib=new RegExp("^("+Aa+")(.*)$","i"),jb=new RegExp("^([+-])=("+Aa+")","i"),kb={position:"absolute",visibility:"hidden",display:"block"},lb={letterSpacing:"0",fontWeight:"400"},mb=["Webkit","O","Moz","ms"];ea.extend({cssHooks:{opacity:{get:function(a,b){if(b){var c=bb(a,"opacity");return""===c?"1":c}}}},cssNumber:{columnCount:!0,fillOpacity:!0,flexGrow:!0,flexShrink:!0,fontWeight:!0,lineHeight:!0,opacity:!0,order:!0,orphans:!0,widows:!0,zIndex:!0,zoom:!0},cssProps:{"float":ca.cssFloat?"cssFloat":"styleFloat"},style:function(a,b,c,d){if(a&&3!==a.nodeType&&8!==a.nodeType&&a.style){var e,f,g,h=ea.camelCase(b),i=a.style;if(b=ea.cssProps[h]||(ea.cssProps[h]=B(i,h)),g=ea.cssHooks[b]||ea.cssHooks[h],void 0===c)return g&&"get"in g&&void 0!==(e=g.get(a,!1,d))?e:i[b];if(f=typeof c,"string"===f&&(e=jb.exec(c))&&(c=(e[1]+1)*e[2]+parseFloat(ea.css(a,b)),f="number"),null!=c&&c===c&&("number"!==f||ea.cssNumber[h]||(c+="px"),ca.clearCloneStyle||""!==c||0!==b.indexOf("background")||(i[b]="inherit"),!(g&&"set"in g&&void 0===(c=g.set(a,c,d)))))try{i[b]=c}catch(j){}}},css:function(a,b,c,d){var e,f,g,h=ea.camelCase(b);return b=ea.cssProps[h]||(ea.cssProps[h]=B(a.style,h)),g=ea.cssHooks[b]||ea.cssHooks[h],g&&"get"in g&&(f=g.get(a,!0,c)),void 0===f&&(f=bb(a,b,d)),"normal"===f&&b in lb&&(f=lb[b]),""===c||c?(e=parseFloat(f),c===!0||ea.isNumeric(e)?e||0:f):f}}),ea.each(["height","width"],function(a,b){ea.cssHooks[b]={get:function(a,c,d){return c?hb.test(ea.css(a,"display"))&&0===a.offsetWidth?ea.swap(a,kb,function(){return F(a,b,d)}):F(a,b,d):void 0},set:function(a,c,d){var e=d&&ab(a);return D(a,c,d?E(a,b,d,ca.boxSizing&&"border-box"===ea.css(a,"boxSizing",!1,e),e):0)}}}),ca.opacity||(ea.cssHooks.opacity={get:function(a,b){return gb.test((b&&a.currentStyle?a.currentStyle.filter:a.style.filter)||"")?.01*parseFloat(RegExp.$1)+"":b?"1":""},set:function(a,b){var c=a.style,d=a.currentStyle,e=ea.isNumeric(b)?"alpha(opacity="+100*b+")":"",f=d&&d.filter||c.filter||"";c.zoom=1,(b>=1||""===b)&&""===ea.trim(f.replace(fb,""))&&c.removeAttribute&&(c.removeAttribute("filter"),""===b||d&&!d.filter)||(c.filter=fb.test(f)?f.replace(fb,e):f+" "+e)}}),ea.cssHooks.marginRight=A(ca.reliableMarginRight,function(a,b){return b?ea.swap(a,{display:"inline-block"},bb,[a,"marginRight"]):void 0}),ea.each({margin:"",padding:"",border:"Width"},function(a,b){ea.cssHooks[a+b]={expand:function(c){for(var d=0,e={},f="string"==typeof c?c.split(" "):[c];4>d;d++)e[a+Ba[d]+b]=f[d]||f[d-2]||f[0];return e}},cb.test(a)||(ea.cssHooks[a+b].set=D)}),ea.fn.extend({css:function(a,b){return Da(this,function(a,b,c){var d,e,f={},g=0;if(ea.isArray(b)){for(d=ab(a),e=b.length;e>g;g++)f[b[g]]=ea.css(a,b[g],!1,d);return f}return void 0!==c?ea.style(a,b,c):ea.css(a,b)},a,b,arguments.length>1)},show:function(){return C(this,!0)},hide:function(){return C(this)},toggle:function(a){return"boolean"==typeof a?a?this.show():this.hide():this.each(function(){Ca(this)?ea(this).show():ea(this).hide()})}}),ea.Tween=G,G.prototype={constructor:G,init:function(a,b,c,d,e,f){this.elem=a,this.prop=c,this.easing=e||"swing",this.options=b,this.start=this.now=this.cur(),this.end=d,this.unit=f||(ea.cssNumber[c]?"":"px")},cur:function(){var a=G.propHooks[this.prop];return a&&a.get?a.get(this):G.propHooks._default.get(this)},run:function(a){var b,c=G.propHooks[this.prop];return this.options.duration?this.pos=b=ea.easing[this.easing](a,this.options.duration*a,0,1,this.options.duration):this.pos=b=a,this.now=(this.end-this.start)*b+this.start,this.options.step&&this.options.step.call(this.elem,this.now,this),c&&c.set?c.set(this):G.propHooks._default.set(this),this}},G.prototype.init.prototype=G.prototype,G.propHooks={_default:{get:function(a){var b;return null==a.elem[a.prop]||a.elem.style&&null!=a.elem.style[a.prop]?(b=ea.css(a.elem,a.prop,""),b&&"auto"!==b?b:0):a.elem[a.prop]},set:function(a){ea.fx.step[a.prop]?ea.fx.step[a.prop](a):a.elem.style&&(null!=a.elem.style[ea.cssProps[a.prop]]||ea.cssHooks[a.prop])?ea.style(a.elem,a.prop,a.now+a.unit):a.elem[a.prop]=a.now}}},G.propHooks.scrollTop=G.propHooks.scrollLeft={set:function(a){a.elem.nodeType&&a.elem.parentNode&&(a.elem[a.prop]=a.now)}},ea.easing={linear:function(a){return a},swing:function(a){return.5-Math.cos(a*Math.PI)/2}},ea.fx=G.prototype.init,ea.fx.step={};var nb,ob,pb=/^(?:toggle|show|hide)$/,qb=new RegExp("^(?:([+-])=|)("+Aa+")([a-z%]*)$","i"),rb=/queueHooks$/,sb=[K],tb={"*":[function(a,b){var c=this.createTween(a,b),d=c.cur(),e=qb.exec(b),f=e&&e[3]||(ea.cssNumber[a]?"":"px"),g=(ea.cssNumber[a]||"px"!==f&&+d)&&qb.exec(ea.css(c.elem,a)),h=1,i=20;if(g&&g[3]!==f){f=f||g[3],e=e||[],g=+d||1;do h=h||".5",g/=h,ea.style(c.elem,a,g+f);while(h!==(h=c.cur()/d)&&1!==h&&--i)}return e&&(g=c.start=+g||+d||0,c.unit=f,c.end=e[1]?g+(e[1]+1)*e[2]:+e[2]),c}]};ea.Animation=ea.extend(M,{tweener:function(a,b){ea.isFunction(a)?(b=a,a=["*"]):a=a.split(" ");for(var c,d=0,e=a.length;e>d;d++)c=a[d],tb[c]=tb[c]||[],tb[c].unshift(b)},prefilter:function(a,b){b?sb.unshift(a):sb.push(a)}}),ea.speed=function(a,b,c){var d=a&&"object"==typeof a?ea.extend({},a):{complete:c||!c&&b||ea.isFunction(a)&&a,duration:a,easing:c&&b||b&&!ea.isFunction(b)&&b};return d.duration=ea.fx.off?0:"number"==typeof d.duration?d.duration:d.duration in ea.fx.speeds?ea.fx.speeds[d.duration]:ea.fx.speeds._default,(null==d.queue||d.queue===!0)&&(d.queue="fx"),d.old=d.complete,d.complete=function(){ea.isFunction(d.old)&&d.old.call(this),d.queue&&ea.dequeue(this,d.queue)},d},ea.fn.extend({fadeTo:function(a,b,c,d){return this.filter(Ca).css("opacity",0).show().end().animate({opacity:b},a,c,d)},animate:function(a,b,c,d){var e=ea.isEmptyObject(a),f=ea.speed(b,c,d),g=function(){var b=M(this,ea.extend({},a),f);(e||ea._data(this,"finish"))&&b.stop(!0)};return g.finish=g,e||f.queue===!1?this.each(g):this.queue(f.queue,g)},stop:function(a,b,c){var d=function(a){var b=a.stop;delete a.stop,b(c)};return"string"!=typeof a&&(c=b,b=a,a=void 0),b&&a!==!1&&this.queue(a||"fx",[]),this.each(function(){var b=!0,e=null!=a&&a+"queueHooks",f=ea.timers,g=ea._data(this);if(e)g[e]&&g[e].stop&&d(g[e]);else for(e in g)g[e]&&g[e].stop&&rb.test(e)&&d(g[e]);for(e=f.length;e--;)f[e].elem!==this||null!=a&&f[e].queue!==a||(f[e].anim.stop(c),b=!1,f.splice(e,1));(b||!c)&&ea.dequeue(this,a)})},finish:function(a){return a!==!1&&(a=a||"fx"),this.each(function(){var b,c=ea._data(this),d=c[a+"queue"],e=c[a+"queueHooks"],f=ea.timers,g=d?d.length:0;for(c.finish=!0,ea.queue(this,a,[]),e&&e.stop&&e.stop.call(this,!0),b=f.length;b--;)f[b].elem===this&&f[b].queue===a&&(f[b].anim.stop(!0),f.splice(b,1));for(b=0;g>b;b++)d[b]&&d[b].finish&&d[b].finish.call(this);delete c.finish})}}),ea.each(["toggle","show","hide"],function(a,b){var c=ea.fn[b];ea.fn[b]=function(a,d,e){return null==a||"boolean"==typeof a?c.apply(this,arguments):this.animate(I(b,!0),a,d,e)}}),ea.each({slideDown:I("show"),slideUp:I("hide"),slideToggle:I("toggle"),fadeIn:{opacity:"show"},fadeOut:{opacity:"hide"},fadeToggle:{opacity:"toggle"}},function(a,b){ea.fn[a]=function(a,c,d){return this.animate(b,a,c,d)}}),ea.timers=[],ea.fx.tick=function(){var a,b=ea.timers,c=0;for(nb=ea.now();c<b.length;c++)a=b[c],a()||b[c]!==a||b.splice(c--,1);b.length||ea.fx.stop(),nb=void 0},ea.fx.timer=function(a){ea.timers.push(a),a()?ea.fx.start():ea.timers.pop()},ea.fx.interval=13,ea.fx.start=function(){ob||(ob=setInterval(ea.fx.tick,ea.fx.interval))},ea.fx.stop=function(){clearInterval(ob),ob=null},ea.fx.speeds={slow:600,fast:200,_default:400},ea.fn.delay=function(a,b){return a=ea.fx?ea.fx.speeds[a]||a:a,b=b||"fx",this.queue(b,function(b,c){var d=setTimeout(b,a);c.stop=function(){clearTimeout(d)}})},function(){var a,b,c,d,e;b=oa.createElement("div"),b.setAttribute("className","t"),b.innerHTML="  <link/><table></table><a href='/a'>a</a><input type='checkbox'/>",d=b.getElementsByTagName("a")[0],c=oa.createElement("select"),e=c.appendChild(oa.createElement("option")),a=b.getElementsByTagName("input")[0],d.style.cssText="top:1px",ca.getSetAttribute="t"!==b.className,ca.style=/top/.test(d.getAttribute("style")),ca.hrefNormalized="/a"===d.getAttribute("href"),ca.checkOn=!!a.value,ca.optSelected=e.selected,ca.enctype=!!oa.createElement("form").enctype,c.disabled=!0,ca.optDisabled=!e.disabled,a=oa.createElement("input"),a.setAttribute("value",""),ca.input=""===a.getAttribute("value"),a.value="t",a.setAttribute("type","radio"),ca.radioValue="t"===a.value}();var ub=/\r/g;ea.fn.extend({val:function(a){var b,c,d,e=this[0];{if(arguments.length)return d=ea.isFunction(a),this.each(function(c){var e;1===this.nodeType&&(e=d?a.call(this,c,ea(this).val()):a,null==e?e="":"number"==typeof e?e+="":ea.isArray(e)&&(e=ea.map(e,function(a){return null==a?"":a+""})),b=ea.valHooks[this.type]||ea.valHooks[this.nodeName.toLowerCase()],b&&"set"in b&&void 0!==b.set(this,e,"value")||(this.value=e))});if(e)return b=ea.valHooks[e.type]||ea.valHooks[e.nodeName.toLowerCase()],b&&"get"in b&&void 0!==(c=b.get(e,"value"))?c:(c=e.value,"string"==typeof c?c.replace(ub,""):null==c?"":c)}}}),ea.extend({valHooks:{option:{get:function(a){var b=ea.find.attr(a,"value");return null!=b?b:ea.trim(ea.text(a))}},select:{get:function(a){for(var b,c,d=a.options,e=a.selectedIndex,f="select-one"===a.type||0>e,g=f?null:[],h=f?e+1:d.length,i=0>e?h:f?e:0;h>i;i++)if(c=d[i],(c.selected||i===e)&&(ca.optDisabled?!c.disabled:null===c.getAttribute("disabled"))&&(!c.parentNode.disabled||!ea.nodeName(c.parentNode,"optgroup"))){if(b=ea(c).val(),f)return b;g.push(b)}return g},set:function(a,b){for(var c,d,e=a.options,f=ea.makeArray(b),g=e.length;g--;)if(d=e[g],ea.inArray(ea.valHooks.option.get(d),f)>=0)try{d.selected=c=!0}catch(h){d.scrollHeight}else d.selected=!1;return c||(a.selectedIndex=-1),e}}}}),ea.each(["radio","checkbox"],function(){ea.valHooks[this]={set:function(a,b){return ea.isArray(b)?a.checked=ea.inArray(ea(a).val(),b)>=0:void 0}},ca.checkOn||(ea.valHooks[this].get=function(a){return null===a.getAttribute("value")?"on":a.value})});var vb,wb,xb=ea.expr.attrHandle,yb=/^(?:checked|selected)$/i,zb=ca.getSetAttribute,Ab=ca.input;ea.fn.extend({attr:function(a,b){return Da(this,ea.attr,a,b,arguments.length>1)},removeAttr:function(a){return this.each(function(){ea.removeAttr(this,a)})}}),ea.extend({attr:function(a,b,c){var d,e,f=a.nodeType;if(a&&3!==f&&8!==f&&2!==f)return typeof a.getAttribute===xa?ea.prop(a,b,c):(1===f&&ea.isXMLDoc(a)||(b=b.toLowerCase(),d=ea.attrHooks[b]||(ea.expr.match.bool.test(b)?wb:vb)),void 0===c?d&&"get"in d&&null!==(e=d.get(a,b))?e:(e=ea.find.attr(a,b),null==e?void 0:e):null!==c?d&&"set"in d&&void 0!==(e=d.set(a,c,b))?e:(a.setAttribute(b,c+""),c):void ea.removeAttr(a,b))},removeAttr:function(a,b){var c,d,e=0,f=b&&b.match(ta);if(f&&1===a.nodeType)for(;c=f[e++];)d=ea.propFix[c]||c,ea.expr.match.bool.test(c)?Ab&&zb||!yb.test(c)?a[d]=!1:a[ea.camelCase("default-"+c)]=a[d]=!1:ea.attr(a,c,""),a.removeAttribute(zb?c:d)},attrHooks:{type:{set:function(a,b){if(!ca.radioValue&&"radio"===b&&ea.nodeName(a,"input")){var c=a.value;return a.setAttribute("type",b),c&&(a.value=c),b}}}}}),wb={set:function(a,b,c){return b===!1?ea.removeAttr(a,c):Ab&&zb||!yb.test(c)?a.setAttribute(!zb&&ea.propFix[c]||c,c):a[ea.camelCase("default-"+c)]=a[c]=!0,c}},ea.each(ea.expr.match.bool.source.match(/\w+/g),function(a,b){var c=xb[b]||ea.find.attr;xb[b]=Ab&&zb||!yb.test(b)?function(a,b,d){var e,f;return d||(f=xb[b],xb[b]=e,e=null!=c(a,b,d)?b.toLowerCase():null,xb[b]=f),e}:function(a,b,c){return c?void 0:a[ea.camelCase("default-"+b)]?b.toLowerCase():null}}),Ab&&zb||(ea.attrHooks.value={set:function(a,b,c){return ea.nodeName(a,"input")?void(a.defaultValue=b):vb&&vb.set(a,b,c)}}),zb||(vb={set:function(a,b,c){var d=a.getAttributeNode(c);return d||a.setAttributeNode(d=a.ownerDocument.createAttribute(c)),d.value=b+="","value"===c||b===a.getAttribute(c)?b:void 0}},xb.id=xb.name=xb.coords=function(a,b,c){var d;return c?void 0:(d=a.getAttributeNode(b))&&""!==d.value?d.value:null},ea.valHooks.button={get:function(a,b){var c=a.getAttributeNode(b);return c&&c.specified?c.value:void 0},set:vb.set},ea.attrHooks.contenteditable={set:function(a,b,c){vb.set(a,""===b?!1:b,c)}},ea.each(["width","height"],function(a,b){ea.attrHooks[b]={set:function(a,c){return""===c?(a.setAttribute(b,"auto"),c):void 0}}})),ca.style||(ea.attrHooks.style={get:function(a){return a.style.cssText||void 0},set:function(a,b){return a.style.cssText=b+""}});var Bb=/^(?:input|select|textarea|button|object)$/i,Cb=/^(?:a|area)$/i;ea.fn.extend({prop:function(a,b){return Da(this,ea.prop,a,b,arguments.length>1)},removeProp:function(a){return a=ea.propFix[a]||a,this.each(function(){try{this[a]=void 0,delete this[a]}catch(b){}})}}),ea.extend({propFix:{"for":"htmlFor","class":"className"},prop:function(a,b,c){var d,e,f,g=a.nodeType;if(a&&3!==g&&8!==g&&2!==g)return f=1!==g||!ea.isXMLDoc(a),f&&(b=ea.propFix[b]||b,e=ea.propHooks[b]),void 0!==c?e&&"set"in e&&void 0!==(d=e.set(a,c,b))?d:a[b]=c:e&&"get"in e&&null!==(d=e.get(a,b))?d:a[b]},propHooks:{tabIndex:{get:function(a){var b=ea.find.attr(a,"tabindex");return b?parseInt(b,10):Bb.test(a.nodeName)||Cb.test(a.nodeName)&&a.href?0:-1}}}}),ca.hrefNormalized||ea.each(["href","src"],function(a,b){ea.propHooks[b]={get:function(a){return a.getAttribute(b,4)}}}),ca.optSelected||(ea.propHooks.selected={get:function(a){var b=a.parentNode;return b&&(b.selectedIndex,b.parentNode&&b.parentNode.selectedIndex),null}}),ea.each(["tabIndex","readOnly","maxLength","cellSpacing","cellPadding","rowSpan","colSpan","useMap","frameBorder","contentEditable"],function(){ea.propFix[this.toLowerCase()]=this}),ca.enctype||(ea.propFix.enctype="encoding");var Db=/[\t\r\n\f]/g;ea.fn.extend({addClass:function(a){var b,c,d,e,f,g,h=0,i=this.length,j="string"==typeof a&&a;if(ea.isFunction(a))return this.each(function(b){ea(this).addClass(a.call(this,b,this.className))});if(j)for(b=(a||"").match(ta)||[];i>h;h++)if(c=this[h],d=1===c.nodeType&&(c.className?(" "+c.className+" ").replace(Db," "):" ")){for(f=0;e=b[f++];)d.indexOf(" "+e+" ")<0&&(d+=e+" ");g=ea.trim(d),c.className!==g&&(c.className=g)}return this},removeClass:function(a){var b,c,d,e,f,g,h=0,i=this.length,j=0===arguments.length||"string"==typeof a&&a;if(ea.isFunction(a))return this.each(function(b){ea(this).removeClass(a.call(this,b,this.className))});if(j)for(b=(a||"").match(ta)||[];i>h;h++)if(c=this[h],d=1===c.nodeType&&(c.className?(" "+c.className+" ").replace(Db," "):"")){for(f=0;e=b[f++];)for(;d.indexOf(" "+e+" ")>=0;)d=d.replace(" "+e+" "," ");g=a?ea.trim(d):"",c.className!==g&&(c.className=g)}return this},toggleClass:function(a,b){var c=typeof a;return"boolean"==typeof b&&"string"===c?b?this.addClass(a):this.removeClass(a):ea.isFunction(a)?this.each(function(c){ea(this).toggleClass(a.call(this,c,this.className,b),b)}):this.each(function(){if("string"===c)for(var b,d=0,e=ea(this),f=a.match(ta)||[];b=f[d++];)e.hasClass(b)?e.removeClass(b):e.addClass(b);else(c===xa||"boolean"===c)&&(this.className&&ea._data(this,"__className__",this.className),this.className=this.className||a===!1?"":ea._data(this,"__className__")||"")})},hasClass:function(a){for(var b=" "+a+" ",c=0,d=this.length;d>c;c++)if(1===this[c].nodeType&&(" "+this[c].className+" ").replace(Db," ").indexOf(b)>=0)return!0;return!1}}),ea.each("blur focus focusin focusout load resize scroll unload click dblclick mousedown mouseup mousemove mouseover mouseout mouseenter mouseleave change select submit keydown keypress keyup error contextmenu".split(" "),function(a,b){ea.fn[b]=function(a,c){return arguments.length>0?this.on(b,null,a,c):this.trigger(b)}}),ea.fn.extend({hover:function(a,b){return this.mouseenter(a).mouseleave(b||a)},bind:function(a,b,c){return this.on(a,null,b,c)},unbind:function(a,b){return this.off(a,null,b)},delegate:function(a,b,c,d){return this.on(b,a,c,d)},undelegate:function(a,b,c){return 1===arguments.length?this.off(a,"**"):this.off(b,a||"**",c)}});var Eb=ea.now(),Fb=/\?/,Gb=/(,)|(\[|{)|(}|])|"(?:[^"\\\r\n]|\\["\\\/bfnrt]|\\u[\da-fA-F]{4})*"\s*:?|true|false|null|-?(?!0\d)\d+(?:\.\d+|)(?:[eE][+-]?\d+|)/g;ea.parseJSON=function(b){if(a.JSON&&a.JSON.parse)return a.JSON.parse(b+"");var c,d=null,e=ea.trim(b+"");return e&&!ea.trim(e.replace(Gb,function(a,b,e,f){return c&&b&&(d=0),0===d?a:(c=e||b,d+=!f-!e,"")}))?Function("return "+e)():ea.error("Invalid JSON: "+b)},ea.parseXML=function(b){var c,d;if(!b||"string"!=typeof b)return null;try{a.DOMParser?(d=new DOMParser,c=d.parseFromString(b,"text/xml")):(c=new ActiveXObject("Microsoft.XMLDOM"),c.async="false",c.loadXML(b))}catch(e){c=void 0}return c&&c.documentElement&&!c.getElementsByTagName("parsererror").length||ea.error("Invalid XML: "+b),c};var Hb,Ib,Jb=/#.*$/,Kb=/([?&])_=[^&]*/,Lb=/^(.*?):[ \t]*([^\r\n]*)\r?$/gm,Mb=/^(?:about|app|app-storage|.+-extension|file|res|widget):$/,Nb=/^(?:GET|HEAD)$/,Ob=/^\/\//,Pb=/^([\w.+-]+:)(?:\/\/(?:[^\/?#]*@|)([^\/?#:]*)(?::(\d+)|)|)/,Qb={},Rb={},Sb="*/".concat("*");try{Ib=location.href}catch(Tb){Ib=oa.createElement("a"),Ib.href="",Ib=Ib.href}Hb=Pb.exec(Ib.toLowerCase())||[],ea.extend({active:0,lastModified:{},etag:{},ajaxSettings:{url:Ib,type:"GET",isLocal:Mb.test(Hb[1]),global:!0,processData:!0,async:!0,contentType:"application/x-www-form-urlencoded; charset=UTF-8",accepts:{"*":Sb,text:"text/plain",html:"text/html",xml:"application/xml, text/xml",json:"application/json, text/javascript"},contents:{xml:/xml/,html:/html/,json:/json/},responseFields:{xml:"responseXML",text:"responseText",json:"responseJSON"},converters:{"* text":String,"text html":!0,"text json":ea.parseJSON,"text xml":ea.parseXML},flatOptions:{url:!0,context:!0}},ajaxSetup:function(a,b){return b?P(P(a,ea.ajaxSettings),b):P(ea.ajaxSettings,a)},ajaxPrefilter:N(Qb),ajaxTransport:N(Rb),ajax:function(a,b){function c(a,b,c,d){var e,k,r,s,u,w=b;2!==t&&(t=2,h&&clearTimeout(h),j=void 0,g=d||"",v.readyState=a>0?4:0,e=a>=200&&300>a||304===a,c&&(s=Q(l,v,c)),s=R(l,s,v,e),e?(l.ifModified&&(u=v.getResponseHeader("Last-Modified"),u&&(ea.lastModified[f]=u),u=v.getResponseHeader("etag"),u&&(ea.etag[f]=u)),204===a||"HEAD"===l.type?w="nocontent":304===a?w="notmodified":(w=s.state,k=s.data,r=s.error,e=!r)):(r=w,(a||!w)&&(w="error",0>a&&(a=0))),v.status=a,v.statusText=(b||w)+"",e?o.resolveWith(m,[k,w,v]):o.rejectWith(m,[v,w,r]),v.statusCode(q),q=void 0,i&&n.trigger(e?"ajaxSuccess":"ajaxError",[v,l,e?k:r]),p.fireWith(m,[v,w]),i&&(n.trigger("ajaxComplete",[v,l]),--ea.active||ea.event.trigger("ajaxStop")))}"object"==typeof a&&(b=a,a=void 0),b=b||{};var d,e,f,g,h,i,j,k,l=ea.ajaxSetup({},b),m=l.context||l,n=l.context&&(m.nodeType||m.jquery)?ea(m):ea.event,o=ea.Deferred(),p=ea.Callbacks("once memory"),q=l.statusCode||{},r={},s={},t=0,u="canceled",v={readyState:0,getResponseHeader:function(a){var b;if(2===t){if(!k)for(k={};b=Lb.exec(g);)k[b[1].toLowerCase()]=b[2];b=k[a.toLowerCase()]}return null==b?null:b},getAllResponseHeaders:function(){return 2===t?g:null},setRequestHeader:function(a,b){var c=a.toLowerCase();return t||(a=s[c]=s[c]||a,r[a]=b),this},overrideMimeType:function(a){return t||(l.mimeType=a),this},statusCode:function(a){var b;if(a)if(2>t)for(b in a)q[b]=[q[b],a[b]];else v.always(a[v.status]);return this},abort:function(a){var b=a||u;return j&&j.abort(b),c(0,b),this}};if(o.promise(v).complete=p.add,v.success=v.done,v.error=v.fail,l.url=((a||l.url||Ib)+"").replace(Jb,"").replace(Ob,Hb[1]+"//"),l.type=b.method||b.type||l.method||l.type,l.dataTypes=ea.trim(l.dataType||"*").toLowerCase().match(ta)||[""],null==l.crossDomain&&(d=Pb.exec(l.url.toLowerCase()),l.crossDomain=!(!d||d[1]===Hb[1]&&d[2]===Hb[2]&&(d[3]||("http:"===d[1]?"80":"443"))===(Hb[3]||("http:"===Hb[1]?"80":"443")))),l.data&&l.processData&&"string"!=typeof l.data&&(l.data=ea.param(l.data,l.traditional)),O(Qb,l,b,v),2===t)return v;i=l.global,i&&0===ea.active++&&ea.event.trigger("ajaxStart"),l.type=l.type.toUpperCase(),l.hasContent=!Nb.test(l.type),f=l.url,l.hasContent||(l.data&&(f=l.url+=(Fb.test(f)?"&":"?")+l.data,delete l.data),l.cache===!1&&(l.url=Kb.test(f)?f.replace(Kb,"$1_="+Eb++):f+(Fb.test(f)?"&":"?")+"_="+Eb++)),l.ifModified&&(ea.lastModified[f]&&v.setRequestHeader("If-Modified-Since",ea.lastModified[f]),ea.etag[f]&&v.setRequestHeader("If-None-Match",ea.etag[f])),(l.data&&l.hasContent&&l.contentType!==!1||b.contentType)&&v.setRequestHeader("Content-Type",l.contentType),v.setRequestHeader("Accept",l.dataTypes[0]&&l.accepts[l.dataTypes[0]]?l.accepts[l.dataTypes[0]]+("*"!==l.dataTypes[0]?", "+Sb+"; q=0.01":""):l.accepts["*"]);for(e in l.headers)v.setRequestHeader(e,l.headers[e]);if(l.beforeSend&&(l.beforeSend.call(m,v,l)===!1||2===t))return v.abort();u="abort";for(e in{success:1,error:1,complete:1})v[e](l[e]);if(j=O(Rb,l,b,v)){v.readyState=1,i&&n.trigger("ajaxSend",[v,l]),l.async&&l.timeout>0&&(h=setTimeout(function(){v.abort("timeout")},l.timeout));try{t=1,j.send(r,c)}catch(w){if(!(2>t))throw w;c(-1,w)}}else c(-1,"No Transport");return v},getJSON:function(a,b,c){return ea.get(a,b,c,"json")},getScript:function(a,b){return ea.get(a,void 0,b,"script")}}),ea.each(["get","post"],function(a,b){ea[b]=function(a,c,d,e){return ea.isFunction(c)&&(e=e||d,d=c,c=void 0),ea.ajax({url:a,type:b,dataType:e,data:c,success:d})}}),ea.each(["ajaxStart","ajaxStop","ajaxComplete","ajaxError","ajaxSuccess","ajaxSend"],function(a,b){ea.fn[b]=function(a){return this.on(b,a)}}),ea._evalUrl=function(a){return ea.ajax({url:a,type:"GET",dataType:"script",async:!1,global:!1,"throws":!0})},ea.fn.extend({wrapAll:function(a){if(ea.isFunction(a))return this.each(function(b){ea(this).wrapAll(a.call(this,b))});if(this[0]){var b=ea(a,this[0].ownerDocument).eq(0).clone(!0);this[0].parentNode&&b.insertBefore(this[0]),b.map(function(){for(var a=this;a.firstChild&&1===a.firstChild.nodeType;)a=a.firstChild;return a}).append(this)}return this},wrapInner:function(a){return ea.isFunction(a)?this.each(function(b){ea(this).wrapInner(a.call(this,b))}):this.each(function(){var b=ea(this),c=b.contents();c.length?c.wrapAll(a):b.append(a)})},wrap:function(a){var b=ea.isFunction(a);return this.each(function(c){ea(this).wrapAll(b?a.call(this,c):a)})},unwrap:function(){return this.parent().each(function(){ea.nodeName(this,"body")||ea(this).replaceWith(this.childNodes)}).end()}}),ea.expr.filters.hidden=function(a){return a.offsetWidth<=0&&a.offsetHeight<=0||!ca.reliableHiddenOffsets()&&"none"===(a.style&&a.style.display||ea.css(a,"display"))},ea.expr.filters.visible=function(a){return!ea.expr.filters.hidden(a)};var Ub=/%20/g,Vb=/\[\]$/,Wb=/\r?\n/g,Xb=/^(?:submit|button|image|reset|file)$/i,Yb=/^(?:input|select|textarea|keygen)/i;ea.param=function(a,b){var c,d=[],e=function(a,b){b=ea.isFunction(b)?b():null==b?"":b,d[d.length]=encodeURIComponent(a)+"="+encodeURIComponent(b)};if(void 0===b&&(b=ea.ajaxSettings&&ea.ajaxSettings.traditional),ea.isArray(a)||a.jquery&&!ea.isPlainObject(a))ea.each(a,function(){e(this.name,this.value)});else for(c in a)S(c,a[c],b,e);return d.join("&").replace(Ub,"+")},ea.fn.extend({serialize:function(){return ea.param(this.serializeArray())},serializeArray:function(){return this.map(function(){var a=ea.prop(this,"elements");return a?ea.makeArray(a):this}).filter(function(){var a=this.type;return this.name&&!ea(this).is(":disabled")&&Yb.test(this.nodeName)&&!Xb.test(a)&&(this.checked||!Ea.test(a))}).map(function(a,b){var c=ea(this).val();return null==c?null:ea.isArray(c)?ea.map(c,function(a){return{name:b.name,value:a.replace(Wb,"\r\n")}}):{name:b.name,value:c.replace(Wb,"\r\n")}}).get()}}),ea.ajaxSettings.xhr=void 0!==a.ActiveXObject?function(){return!this.isLocal&&/^(get|post|head|put|delete|options)$/i.test(this.type)&&T()||U()}:T;var Zb=0,$b={},_b=ea.ajaxSettings.xhr();a.ActiveXObject&&ea(a).on("unload",function(){for(var a in $b)$b[a](void 0,!0)}),ca.cors=!!_b&&"withCredentials"in _b,_b=ca.ajax=!!_b,_b&&ea.ajaxTransport(function(a){if(!a.crossDomain||ca.cors){var b;return{send:function(c,d){var e,f=a.xhr(),g=++Zb;if(f.open(a.type,a.url,a.async,a.username,a.password),a.xhrFields)for(e in a.xhrFields)f[e]=a.xhrFields[e];a.mimeType&&f.overrideMimeType&&f.overrideMimeType(a.mimeType),a.crossDomain||c["X-Requested-With"]||(c["X-Requested-With"]="XMLHttpRequest");for(e in c)void 0!==c[e]&&f.setRequestHeader(e,c[e]+"");f.send(a.hasContent&&a.data||null),b=function(c,e){var h,i,j;if(b&&(e||4===f.readyState))if(delete $b[g],b=void 0,f.onreadystatechange=ea.noop,e)4!==f.readyState&&f.abort();else{j={},h=f.status,"string"==typeof f.responseText&&(j.text=f.responseText);try{i=f.statusText}catch(k){i=""}h||!a.isLocal||a.crossDomain?1223===h&&(h=204):h=j.text?200:404}j&&d(h,i,j,f.getAllResponseHeaders())},a.async?4===f.readyState?setTimeout(b):f.onreadystatechange=$b[g]=b:b()},abort:function(){b&&b(void 0,!0)}}}}),ea.ajaxSetup({accepts:{script:"text/javascript, application/javascript, application/ecmascript, application/x-ecmascript"},contents:{script:/(?:java|ecma)script/},converters:{"text script":function(a){return ea.globalEval(a),a}}}),ea.ajaxPrefilter("script",function(a){void 0===a.cache&&(a.cache=!1),a.crossDomain&&(a.type="GET",a.global=!1)}),ea.ajaxTransport("script",function(a){if(a.crossDomain){var b,c=oa.head||ea("head")[0]||oa.documentElement;return{send:function(d,e){b=oa.createElement("script"),b.async=!0,a.scriptCharset&&(b.charset=a.scriptCharset),b.src=a.url,b.onload=b.onreadystatechange=function(a,c){(c||!b.readyState||/loaded|complete/.test(b.readyState))&&(b.onload=b.onreadystatechange=null,b.parentNode&&b.parentNode.removeChild(b),b=null,c||e(200,"success"))},c.insertBefore(b,c.firstChild)},abort:function(){b&&b.onload(void 0,!0)}}}});var ac=[],bc=/(=)\?(?=&|$)|\?\?/;ea.ajaxSetup({jsonp:"callback",jsonpCallback:function(){var a=ac.pop()||ea.expando+"_"+Eb++;return this[a]=!0,a}}),ea.ajaxPrefilter("json jsonp",function(b,c,d){var e,f,g,h=b.jsonp!==!1&&(bc.test(b.url)?"url":"string"==typeof b.data&&!(b.contentType||"").indexOf("application/x-www-form-urlencoded")&&bc.test(b.data)&&"data");return h||"jsonp"===b.dataTypes[0]?(e=b.jsonpCallback=ea.isFunction(b.jsonpCallback)?b.jsonpCallback():b.jsonpCallback,h?b[h]=b[h].replace(bc,"$1"+e):b.jsonp!==!1&&(b.url+=(Fb.test(b.url)?"&":"?")+b.jsonp+"="+e),b.converters["script json"]=function(){return g||ea.error(e+" was not called"),g[0]},b.dataTypes[0]="json",f=a[e],a[e]=function(){g=arguments},d.always(function(){a[e]=f,b[e]&&(b.jsonpCallback=c.jsonpCallback,ac.push(e)),g&&ea.isFunction(f)&&f(g[0]),g=f=void 0}),"script"):void 0}),ea.parseHTML=function(a,b,c){if(!a||"string"!=typeof a)return null;"boolean"==typeof b&&(c=b,b=!1),b=b||oa;var d=la.exec(a),e=!c&&[];return d?[b.createElement(d[1])]:(d=ea.buildFragment([a],b,e),e&&e.length&&ea(e).remove(),ea.merge([],d.childNodes))};var cc=ea.fn.load;ea.fn.load=function(a,b,c){if("string"!=typeof a&&cc)return cc.apply(this,arguments);var d,e,f,g=this,h=a.indexOf(" ");return h>=0&&(d=ea.trim(a.slice(h,a.length)),a=a.slice(0,h)),ea.isFunction(b)?(c=b,b=void 0):b&&"object"==typeof b&&(f="POST"),g.length>0&&ea.ajax({url:a,type:f,dataType:"html",data:b}).done(function(a){e=arguments,g.html(d?ea("<div>").append(ea.parseHTML(a)).find(d):a)}).complete(c&&function(a,b){g.each(c,e||[a.responseText,b,a])}),this},ea.expr.filters.animated=function(a){return ea.grep(ea.timers,function(b){return a===b.elem}).length};var dc=a.document.documentElement;ea.offset={setOffset:function(a,b,c){var d,e,f,g,h,i,j,k=ea.css(a,"position"),l=ea(a),m={};"static"===k&&(a.style.position="relative"),h=l.offset(),f=ea.css(a,"top"),i=ea.css(a,"left"),j=("absolute"===k||"fixed"===k)&&ea.inArray("auto",[f,i])>-1,j?(d=l.position(),g=d.top,e=d.left):(g=parseFloat(f)||0,e=parseFloat(i)||0),ea.isFunction(b)&&(b=b.call(a,c,h)),null!=b.top&&(m.top=b.top-h.top+g),null!=b.left&&(m.left=b.left-h.left+e),"using"in b?b.using.call(a,m):l.css(m)}},ea.fn.extend({offset:function(a){if(arguments.length)return void 0===a?this:this.each(function(b){ea.offset.setOffset(this,a,b)});var b,c,d={top:0,left:0},e=this[0],f=e&&e.ownerDocument;if(f)return b=f.documentElement,ea.contains(b,e)?(typeof e.getBoundingClientRect!==xa&&(d=e.getBoundingClientRect()),c=V(f),{top:d.top+(c.pageYOffset||b.scrollTop)-(b.clientTop||0),left:d.left+(c.pageXOffset||b.scrollLeft)-(b.clientLeft||0)}):d},position:function(){if(this[0]){var a,b,c={top:0,left:0},d=this[0];return"fixed"===ea.css(d,"position")?b=d.getBoundingClientRect():(a=this.offsetParent(),b=this.offset(),ea.nodeName(a[0],"html")||(c=a.offset()),c.top+=ea.css(a[0],"borderTopWidth",!0),c.left+=ea.css(a[0],"borderLeftWidth",!0)),{top:b.top-c.top-ea.css(d,"marginTop",!0),left:b.left-c.left-ea.css(d,"marginLeft",!0)}}},offsetParent:function(){return this.map(function(){for(var a=this.offsetParent||dc;a&&!ea.nodeName(a,"html")&&"static"===ea.css(a,"position");)a=a.offsetParent;return a||dc})}}),ea.each({scrollLeft:"pageXOffset",scrollTop:"pageYOffset"},function(a,b){var c=/Y/.test(b);ea.fn[a]=function(d){return Da(this,function(a,d,e){var f=V(a);return void 0===e?f?b in f?f[b]:f.document.documentElement[d]:a[d]:void(f?f.scrollTo(c?ea(f).scrollLeft():e,c?e:ea(f).scrollTop()):a[d]=e)},a,d,arguments.length,null)}}),ea.each(["top","left"],function(a,b){ea.cssHooks[b]=A(ca.pixelPosition,function(a,c){return c?(c=bb(a,b),db.test(c)?ea(a).position()[b]+"px":c):void 0})}),ea.each({Height:"height",Width:"width"},function(a,b){ea.each({padding:"inner"+a,content:b,"":"outer"+a},function(c,d){ea.fn[d]=function(d,e){var f=arguments.length&&(c||"boolean"!=typeof d),g=c||(d===!0||e===!0?"margin":"border");return Da(this,function(b,c,d){var e;return ea.isWindow(b)?b.document.documentElement["client"+a]:9===b.nodeType?(e=b.documentElement,
Math.max(b.body["scroll"+a],e["scroll"+a],b.body["offset"+a],e["offset"+a],e["client"+a])):void 0===d?ea.css(b,c,g):ea.style(b,c,d,g)},b,f?d:void 0,f,null)}})}),ea.fn.size=function(){return this.length},ea.fn.andSelf=ea.fn.addBack,"function"==typeof define&&define.amd&&define("jquery",[],function(){return ea});var ec=a.jQuery,fc=a.$;return ea.noConflict=function(b){return a.$===ea&&(a.$=fc),b&&a.jQuery===ea&&(a.jQuery=ec),ea},typeof b===xa&&(a.jQuery=a.$=ea),ea}),function(){function a(a,b){if(a!==b){var c=a===a,d=b===b;if(a>b||!c||"undefined"==typeof a&&d)return 1;if(b>a||!d||"undefined"==typeof b&&c)return-1}return 0}function b(a,b,c){if(b!==b)return m(a,c);for(var d=(c||0)-1,e=a.length;++d<e;)if(a[d]===b)return d;return-1}function c(a,b){var c=a.length;for(a.sort(b);c--;)a[c]=a[c].value;return a}function d(a){return"string"==typeof a?a:null==a?"":a+""}function e(a){return a.charCodeAt(0)}function f(a,b){for(var c=-1,d=a.length;++c<d&&b.indexOf(a.charAt(c))>-1;);return c}function g(a,b){for(var c=a.length;c--&&b.indexOf(a.charAt(c))>-1;);return c}function h(b,c){return a(b.criteria,c.criteria)||b.index-c.index}function i(b,c){for(var d=-1,e=b.criteria,f=c.criteria,g=e.length;++d<g;){var h=a(e[d],f[d]);if(h)return h}return b.index-c.index}function j(a){return Ma[a]}function k(a){return Na[a]}function l(a){return"\\"+Qa[a]}function m(a,b,c){for(var d=a.length,e=c?b||d:(b||0)-1;c?e--:++e<d;){var f=a[e];if(f!==f)return e}return-1}function n(a){return a&&"object"==typeof a||!1}function o(a){return 160>=a&&a>=9&&13>=a||32==a||160==a||5760==a||6158==a||a>=8192&&(8202>=a||8232==a||8233==a||8239==a||8287==a||12288==a||65279==a)}function p(a,b){for(var c=-1,d=a.length,e=-1,f=[];++c<d;)a[c]===b&&(a[c]=O,f[++e]=c);return f}function q(a,b){for(var c,d=-1,e=a.length,f=-1,g=[];++d<e;){var h=a[d],i=b?b(h,d,a):h;d&&c===i||(c=i,g[++f]=h)}return g}function r(a){for(var b=-1,c=a.length;++b<c&&o(a.charCodeAt(b)););return b}function s(a){for(var b=a.length;b--&&o(a.charCodeAt(b)););return b}function t(a){return Oa[a]}function u(o){function V(a){if(n(a)&&!Pg(a)){if(a instanceof Z)return a;if(Tf.call(a,"__wrapped__"))return new Z(a.__wrapped__,a.__chain__,Za(a.__actions__))}return new Z(a)}function Z(a,b,c){this.__actions__=c||[],this.__chain__=!!b,this.__wrapped__=a}function _(a){this.actions=null,this.dir=1,this.dropCount=0,this.filtered=!1,this.iteratees=null,this.takeCount=vg,this.views=null,this.wrapped=a}function Ma(){var a=this.actions,b=this.iteratees,c=this.views,d=new _(this.wrapped);return d.actions=a?Za(a):null,d.dir=this.dir,d.dropCount=this.dropCount,d.filtered=this.filtered,d.iteratees=b?Za(b):null,d.takeCount=this.takeCount,d.views=c?Za(c):null,d}function Na(){var a=this.filtered,b=a?new _(this):this.clone();return b.dir=-1*this.dir,b.filtered=a,b}function Oa(){var a=this.wrapped.value();if(!Pg(a))return Ub(a,this.actions);var b=this.dir,c=0>b,d=a.length,e=pc(0,d,this.views),f=e.start,g=e.end,h=this.dropCount,i=pg(g-f,this.takeCount-h),j=c?g:f-1,k=this.iteratees,l=k?k.length:0,m=0,n=[];a:for(;d--&&i>m;){j+=b;for(var o=-1,p=a[j];++o<l;){var q=k[o],r=q.iteratee,s=r(p,j,a),t=q.type;if(t==L)p=s;else if(!s){if(t==K)continue a;break a}}h?h--:n[m++]=p}return c?n.reverse():n}function Pa(){this.__data__={}}function Qa(a){return this.has(a)&&delete this.__data__[a]}function Sa(a){return"__proto__"==a?v:this.__data__[a]}function Ta(a){return"__proto__"!=a&&Tf.call(this.__data__,a)}function Ua(a,b){return"__proto__"!=a&&(this.__data__[a]=b),this}function Va(a){var b=a?a.length:0;for(this.data={hash:lg(null),set:new eg};b--;)this.push(a[b])}function Xa(a,b){var c=a.data,d="string"==typeof b||te(b)?c.set.has(b):c.hash[b];return d?0:-1}function Ya(a){var b=this.data;"string"==typeof a||te(a)?b.set.add(a):b.hash[a]=!0}function Za(a,b){var c=-1,d=a.length;for(b||(b=Ef(d));++c<d;)b[c]=a[c];return b}function $a(a,b){for(var c=-1,d=a.length;++c<d&&b(a[c],c,a)!==!1;);return a}function _a(a,b){for(var c=a.length;c--&&b(a[c],c,a)!==!1;);return a}function ab(a,b){for(var c=-1,d=a.length;++c<d;)if(!b(a[c],c,a))return!1;return!0}function bb(a,b){for(var c=-1,d=a.length,e=-1,f=[];++c<d;){var g=a[c];b(g,c,a)&&(f[++e]=g)}return f}function cb(a,b){for(var c=-1,d=a.length,e=Ef(d);++c<d;)e[c]=b(a[c],c,a);return e}function db(a){for(var b=-1,c=a.length,d=ug;++b<c;){var e=a[b];e>d&&(d=e)}return d}function eb(a){for(var b=-1,c=a.length,d=vg;++b<c;){var e=a[b];d>e&&(d=e)}return d}function fb(a,b,c,d){var e=-1,f=a.length;for(d&&f&&(c=a[++e]);++e<f;)c=b(c,a[e],e,a);return c}function gb(a,b,c,d){var e=a.length;for(d&&e&&(c=a[--e]);e--;)c=b(c,a[e],e,a);return c}function hb(a,b){for(var c=-1,d=a.length;++c<d;)if(b(a[c],c,a))return!0;return!1}function ib(a,b){return"undefined"==typeof a?b:a}function jb(a,b,c,d){return"undefined"!=typeof a&&Tf.call(d,c)?a:b}function kb(a,b,c){var d=Tg(b);if(!c)return mb(b,a,d);for(var e=-1,f=d.length;++e<f;){var g=d[e],h=a[g],i=c(h,b[g],g,a,b);(i===i?i===h:h!==h)&&("undefined"!=typeof h||g in a)||(a[g]=i)}return a}function lb(a,b){for(var c=-1,d=a.length,e=wc(d),f=b.length,g=Ef(f);++c<f;){var h=b[c];e?(h=parseFloat(h),g[c]=uc(h,d)?a[h]:v):g[c]=a[h]}return g}function mb(a,b,c){c||(c=b,b={});for(var d=-1,e=c.length;++d<e;){var f=c[d];b[f]=a[f]}return b}function nb(a,b){for(var c=-1,d=b.length;++c<d;){var e=b[c];a[e]=ic(a[e],x,a)}return a}function ob(a,b,c){var e=typeof a;return"function"==e?"undefined"!=typeof b&&tc(a)?Xb(a,b,c):a:null==a?uf:"object"==e?Jb(a,!c):Mb(c?d(a):a)}function pb(a,b,c,d,e,f,g){var h;if(c&&(h=e?c(a,d,e):c(a)),"undefined"!=typeof h)return h;if(!te(a))return a;var i=Pg(a);if(i){if(h=qc(a),!b)return Za(a,h)}else{var j=Vf.call(a),k=j==U;if(j!=X&&j!=P&&(!k||e))return Ka[j]?sc(a,j,b):e?a:{};if(h=rc(k?{}:a),!b)return mb(a,h,Tg(a))}f||(f=[]),g||(g=[]);for(var l=f.length;l--;)if(f[l]==a)return g[l];return f.push(a),g.push(h),(i?$a:Bb)(a,function(d,e){h[e]=pb(d,b,c,e,a,f,g)}),h}function qb(a,b,c,d){if(!se(a))throw new Nf(N);return fg(function(){a.apply(v,Qb(c,d))},b)}function rb(a,c){var d=a?a.length:0,e=[];if(!d)return e;var f=-1,g=oc(),h=g==b,i=h&&c.length>=200&&Fg(c),j=c.length;i&&(g=Xa,h=!1,c=i);a:for(;++f<d;){var k=a[f];if(h&&k===k){for(var l=j;l--;)if(c[l]===k)continue a;e.push(k)}else g(c,k)<0&&e.push(k)}return e}function sb(a,b){var c=a?a.length:0;if(!wc(c))return Bb(a,b);for(var d=-1,e=Fc(a);++d<c&&b(e[d],d,e)!==!1;);return a}function tb(a,b){var c=a?a.length:0;if(!wc(c))return Cb(a,b);for(var d=Fc(a);c--&&b(d[c],c,d)!==!1;);return a}function ub(a,b){var c=!0;return sb(a,function(a,d,e){return c=!!b(a,d,e)}),c}function vb(a,b){var c=[];return sb(a,function(a,d,e){b(a,d,e)&&c.push(a)}),c}function wb(a,b,c,d){var e;return c(a,function(a,c,f){return b(a,c,f)?(e=d?c:a,!1):void 0}),e}function xb(a,b,c,d){for(var e=(d||0)-1,f=a.length,g=-1,h=[];++e<f;){var i=a[e];if(n(i)&&wc(i.length)&&(Pg(i)||le(i))){b&&(i=xb(i,b,c));var j=-1,k=i.length;for(h.length+=k;++j<k;)h[++g]=i[j]}else c||(h[++g]=i)}return h}function yb(a,b,c){for(var d=-1,e=Fc(a),f=c(a),g=f.length;++d<g;){var h=f[d];if(b(e[h],h,e)===!1)break}return a}function zb(a,b,c){for(var d=Fc(a),e=c(a),f=e.length;f--;){var g=e[f];if(b(d[g],g,d)===!1)break}return a}function Ab(a,b){return yb(a,b,Qe)}function Bb(a,b){return yb(a,b,Tg)}function Cb(a,b){return zb(a,b,Tg)}function Db(a,b){for(var c=-1,d=b.length,e=-1,f=[];++c<d;){var g=b[c];se(a[g])&&(f[++e]=g)}return f}function Eb(a,b,c){var d=-1,e="function"==typeof b,f=a?a.length:0,g=wc(f)?Ef(f):[];return sb(a,function(a){var f=e?b:null!=a&&a[b];g[++d]=f?f.apply(a,c):v}),g}function Fb(a,b,c,d,e,f){if(a===b)return 0!==a||1/a==1/b;var g=typeof a,h=typeof b;return"function"!=g&&"object"!=g&&"function"!=h&&"object"!=h||null==a||null==b?a!==a&&b!==b:Gb(a,b,Fb,c,d,e,f)}function Gb(a,b,c,d,e,f,g){var h=Pg(a),i=Pg(b),j=Q,k=Q;h||(j=Vf.call(a),j==P?j=X:j!=X&&(h=Be(a))),i||(k=Vf.call(b),k==P?k=X:k!=X&&(i=Be(b)));var l=j==X,m=k==X,n=j==k;if(n&&!h&&!l)return kc(a,b,j);var o=l&&Tf.call(a,"__wrapped__"),p=m&&Tf.call(b,"__wrapped__");if(o||p)return c(o?a.value():a,p?b.value():b,d,e,f,g);if(!n)return!1;f||(f=[]),g||(g=[]);for(var q=f.length;q--;)if(f[q]==a)return g[q]==b;f.push(a),g.push(b);var r=(h?jc:lc)(a,b,c,d,e,f,g);return f.pop(),g.pop(),r}function Hb(a,b,c,d,e){var f=b.length;if(null==a)return!f;for(var g=-1,h=!e;++g<f;)if(h&&d[g]?c[g]!==a[b[g]]:!Tf.call(a,b[g]))return!1;for(g=-1;++g<f;){var i=b[g];if(h&&d[g])var j=Tf.call(a,i);else{var k=a[i],l=c[g];j=e?e(k,l,i):v,"undefined"==typeof j&&(j=Fb(l,k,e,!0))}if(!j)return!1}return!0}function Ib(a,b){var c=[];return sb(a,function(a,d,e){c.push(b(a,d,e))}),c}function Jb(a,b){var c=Tg(a),d=c.length;if(1==d){var e=c[0],f=a[e];if(xc(f))return function(a){return null!=a&&f===a[e]&&Tf.call(a,e)}}b&&(a=pb(a,!0));for(var g=Ef(d),h=Ef(d);d--;)f=a[c[d]],g[d]=f,h[d]=xc(f);return function(a){return Hb(a,c,g,h)}}function Kb(a,b,c,d,e){var f=wc(b.length)&&(Pg(b)||Be(b));return(f?$a:Bb)(b,function(b,g,h){if(n(b))return d||(d=[]),e||(e=[]),Lb(a,h,g,Kb,c,d,e);var i=a[g],j=c?c(i,b,g,a,h):v,k="undefined"==typeof j;k&&(j=b),!f&&"undefined"==typeof j||!k&&(j===j?j===i:i!==i)||(a[g]=j)}),a}function Lb(a,b,c,d,e,f,g){for(var h=f.length,i=b[c];h--;)if(f[h]==i)return void(a[c]=g[h]);var j=a[c],k=e?e(j,i,c,a,b):v,l="undefined"==typeof k;l&&(k=i,wc(i.length)&&(Pg(i)||Be(i))?k=Pg(j)?j:j?Za(j):[]:(Rg(i)||le(i))&&(k=le(j)?Ee(j):Rg(j)?j:{})),f.push(i),g.push(k),l?a[c]=d(k,i,e,f,g):(k===k?k!==j:j===j)&&(a[c]=k)}function Mb(a){return function(b){return null==b?v:b[a]}}function Nb(b,c){var d=c.length,e=lb(b,c);for(c.sort(a);d--;){var f=parseFloat(c[d]);if(f!=g&&uc(f)){var g=f;gg.call(b,f,1)}}return e}function Ob(a,b){return a+ag(tg()*(b-a+1))}function Pb(a,b,c,d,e){return e(a,function(a,e,f){c=d?(d=!1,a):b(c,a,e,f)}),c}function Qb(a,b,c){var d=-1,e=a.length;b=null==b?0:+b||0,0>b&&(b=-b>e?0:e+b),c="undefined"==typeof c||c>e?e:+c||0,0>c&&(c+=e),e=b>c?0:c-b;for(var f=Ef(e);++d<e;)f[d]=a[d+b];return f}function Rb(a,b){var c;return sb(a,function(a,d,e){return c=b(a,d,e),!c}),!!c}function Sb(a,c){var d=-1,e=oc(),f=a.length,g=e==b,h=g&&f>=200,i=h&&Fg(),j=[];i?(e=Xa,g=!1):(h=!1,i=c?[]:j);a:for(;++d<f;){var k=a[d],l=c?c(k,d,a):k;if(g&&k===k){for(var m=i.length;m--;)if(i[m]===l)continue a;c&&i.push(l),j.push(k)}else e(i,l)<0&&((c||h)&&i.push(l),j.push(k))}return j}function Tb(a,b){for(var c=-1,d=b.length,e=Ef(d);++c<d;)e[c]=a[b[c]];return e}function Ub(a,b){var c=a;c instanceof _&&(c=c.value());for(var d=-1,e=b.length;++d<e;){var f=[c],g=b[d];cg.apply(f,g.args),c=g.func.apply(g.thisArg,f)}return c}function Vb(a,b,c){var d=0,e=a?a.length:d;if("number"==typeof b&&b===b&&yg>=e){for(;e>d;){var f=d+e>>>1,g=a[f];(c?b>=g:b>g)?d=f+1:e=f}return e}return Wb(a,b,uf,c)}function Wb(a,b,c,d){b=c(b);for(var e=0,f=a?a.length:0,g=b!==b,h="undefined"==typeof b;f>e;){var i=ag((e+f)/2),j=c(a[i]),k=j===j;if(g)var l=k||d;else l=h?k&&(d||"undefined"!=typeof j):d?b>=j:b>j;l?e=i+1:f=i}return pg(f,xg)}function Xb(a,b,c){if("function"!=typeof a)return uf;if("undefined"==typeof b)return a;switch(c){case 1:return function(c){return a.call(b,c)};case 3:return function(c,d,e){return a.call(b,c,d,e)};case 4:return function(c,d,e,f){return a.call(b,c,d,e,f)};case 5:return function(c,d,e,f,g){return a.call(b,c,d,e,f,g)}}return function(){return a.apply(b,arguments)}}function Yb(a){return Zf.call(a,0)}function Zb(a,b,c){for(var d=c.length,e=-1,f=og(a.length-d,0),g=-1,h=b.length,i=Ef(f+h);++g<h;)i[g]=b[g];for(;++e<d;)i[c[e]]=a[e];for(;f--;)i[g++]=a[e++];return i}function $b(a,b,c){for(var d=-1,e=c.length,f=-1,g=og(a.length-e,0),h=-1,i=b.length,j=Ef(g+i);++f<g;)j[f]=a[f];for(var k=f;++h<i;)j[k+h]=b[h];for(;++d<e;)j[k+c[d]]=a[f++];return j}function _b(a,b){return function(c,d,e){var f=b?b():{};if(d=nc(d,e,3),Pg(c))for(var g=-1,h=c.length;++g<h;){var i=c[g];a(f,i,d(i,g,c),c)}else sb(c,function(b,c,e){a(f,b,d(b,c,e),e)});return f}}function ac(a){return function(){var b=arguments.length,c=arguments[0];if(2>b||null==c)return c;if(b>3&&vc(arguments[1],arguments[2],arguments[3])&&(b=2),b>3&&"function"==typeof arguments[b-2])var d=Xb(arguments[--b-1],arguments[b--],5);else b>2&&"function"==typeof arguments[b-1]&&(d=arguments[--b]);for(var e=0;++e<b;){var f=arguments[e];f&&a(c,f,d)}return c}}function bc(a,b){function c(){return(this instanceof c?d:a).apply(b,arguments)}var d=dc(a);return c}function cc(a){return function(b){for(var c=-1,d=qf(_e(b)),e=d.length,f="";++c<e;)f=a(f,d[c],c);return f}}function dc(a){return function(){var b=Dg(a.prototype),c=a.apply(b,arguments);return te(c)?c:b}}function ec(a,b){return function(c,d,f){f&&vc(c,d,f)&&(d=null);var g=nc(),h=null==d;if(g===ob&&h||(h=!1,d=g(d,f,3)),h){var i=Pg(c);if(i||!Ae(c))return a(i?c:Ec(c));d=e}return mc(c,d,b)}}function fc(a,b,c,d,e,f,g,h,i,j){function k(){for(var u=arguments.length,v=u,w=Ef(u);v--;)w[v]=arguments[v];if(d&&(w=Zb(w,d,e)),f&&(w=$b(w,f,g)),o||r){var z=k.placeholder,A=p(w,z);if(u-=A.length,j>u){var B=h?Za(h):null,E=og(j-u,0),F=o?A:null,G=o?null:A,H=o?w:null,I=o?null:w;b|=o?C:D,b&=~(o?D:C),q||(b&=~(x|y));var J=fc(a,b,c,H,F,I,G,B,i,E);return J.placeholder=z,J}}var K=m?c:this;return n&&(a=K[t]),h&&(w=Bc(w,h)),l&&i<w.length&&(w.length=i),(this instanceof k?s||dc(a):a).apply(K,w)}var l=b&F,m=b&x,n=b&y,o=b&A,q=b&z,r=b&B,s=!n&&dc(a),t=a;return k}function gc(a,b,c){var e=a.length;if(b=+b,e>=b||!mg(b))return"";var f=b-e;return c=null==c?" ":d(c),hf(c,$f(f/c.length)).slice(0,f)}function hc(a,b,c,d){function e(){for(var b=-1,h=arguments.length,i=-1,j=d.length,k=Ef(h+j);++i<j;)k[i]=d[i];for(;h--;)k[i++]=arguments[++b];return(this instanceof e?g:a).apply(f?c:this,k)}var f=b&x,g=dc(a);return e}function ic(a,b,c,d,e,f,g,h){var i=b&y;if(!i&&!se(a))throw new Nf(N);var j=d?d.length:0;if(j||(b&=~(C|D),d=e=null),j-=e?e.length:0,b&D){var k=d,l=e;d=e=null}var m=!i&&Gg(a),n=[a,b,c,d,e,k,l,f,g,h];if(m&&m!==!0&&(yc(n,m),b=n[1],h=n[9]),n[9]=null==h?i?0:a.length:og(h-j,0)||0,b==x)var o=bc(n[0],n[2]);else o=b!=C&&b!=(x|C)||n[4].length?fc.apply(null,n):hc.apply(null,n);var p=m?Eg:Hg;return p(o,n)}function jc(a,b,c,d,e,f,g){var h=-1,i=a.length,j=b.length,k=!0;if(i!=j&&!(e&&j>i))return!1;for(;k&&++h<i;){var l=a[h],m=b[h];if(k=v,d&&(k=e?d(m,l,h):d(l,m,h)),"undefined"==typeof k)if(e)for(var n=j;n--&&(m=b[n],!(k=l&&l===m||c(l,m,d,e,f,g))););else k=l&&l===m||c(l,m,d,e,f,g)}return!!k}function kc(a,b,c){switch(c){case R:case S:return+a==+b;case T:return a.name==b.name&&a.message==b.message;case W:return a!=+a?b!=+b:0==a?1/a==1/b:a==+b;case Y:case $:return a==d(b)}return!1}function lc(a,b,c,d,e,f,g){var h=Tg(a),i=h.length,j=Tg(b),k=j.length;if(i!=k&&!e)return!1;for(var l,m=-1;++m<i;){var n=h[m],o=Tf.call(b,n);if(o){var p=a[n],q=b[n];o=v,d&&(o=e?d(q,p,n):d(p,q,n)),"undefined"==typeof o&&(o=p&&p===q||c(p,q,d,e,f,g))}if(!o)return!1;l||(l="constructor"==n)}if(!l){var r=a.constructor,s=b.constructor;if(r!=s&&"constructor"in a&&"constructor"in b&&!("function"==typeof r&&r instanceof r&&"function"==typeof s&&s instanceof s))return!1}return!0}function mc(a,b,c){var d=c?vg:ug,e=d,f=e;return sb(a,function(a,g,h){var i=b(a,g,h);((c?e>i:i>e)||i===d&&i===f)&&(e=i,f=a)}),f}function nc(a,b,c){var d=V.callback||sf;return d=d===sf?ob:d,c?d(a,b,c):d}function oc(a,c,d){var e=V.indexOf||Sc;return e=e===Sc?b:e,a?e(a,c,d):e}function pc(a,b,c){for(var d=-1,e=c?c.length:0;++d<e;){var f=c[d],g=f.size;switch(f.type){case"drop":a+=g;break;case"dropRight":b-=g;break;case"take":b=pg(b,a+g);break;case"takeRight":a=og(a,b-g)}}return{start:a,end:b}}function qc(a){var b=a.length,c=new a.constructor(b);return b&&"string"==typeof a[0]&&Tf.call(a,"index")&&(c.index=a.index,c.input=a.input),c}function rc(a){var b=a.constructor;return"function"==typeof b&&b instanceof b||(b=Kf),new b}function sc(a,b,c){var d=a.constructor;switch(b){case aa:return Yb(a);case R:case S:return new d(+a);case ba:case ca:case da:case ea:case fa:case ga:case ha:case ia:case ja:var e=a.buffer;return new d(c?Yb(e):e,a.byteOffset,a.length);case W:case $:return new d(a);case Y:var f=new d(a.source,va.exec(a));f.lastIndex=a.lastIndex}return f}function tc(a){var b=V.support,c=!(b.funcNames?a.name:b.funcDecomp);if(!c){var d=Rf.call(a);b.funcNames||(c=!wa.test(d)),c||(c=Da.test(d)||we(a),Eg(a,c))}return c}function uc(a,b){return a=+a,b=null==b?Ag:b,a>-1&&a%1==0&&b>a}function vc(a,b,c){if(!te(c))return!1;var d=typeof b;if("number"==d)var e=c.length,f=wc(e)&&uc(b,e);else f="string"==d&&b in a;return f&&c[b]===a}function wc(a){return"number"==typeof a&&a>-1&&a%1==0&&Ag>=a}function xc(a){return a===a&&(0===a?1/a>0:!te(a))}function yc(a,b){var c=a[1],d=b[1],e=c|d,f=F|E,g=x|y,h=f|g|z|B,i=c&F&&!(d&F),j=c&E&&!(d&E),k=(j?a:b)[7],l=(i?a:b)[8],m=!(c>=E&&d>g||c>g&&d>=E),n=e>=f&&h>=e&&(E>c||(j||i)&&k.length<=l);if(!m&&!n)return a;d&x&&(a[2]=b[2],e|=c&x?0:z);var o=b[3];if(o){var q=a[3];a[3]=q?Zb(q,o,b[4]):Za(o),a[4]=q?p(a[3],O):Za(b[4])}return o=b[5],o&&(q=a[5],a[5]=q?$b(q,o,b[6]):Za(o),a[6]=q?p(a[5],O):Za(b[6])),o=b[7],o&&(a[7]=Za(o)),d&F&&(a[8]=null==a[8]?b[8]:pg(a[8],b[8])),null==a[9]&&(a[9]=b[9]),a[0]=b[0],a[1]=e,a}function zc(a,b){a=Fc(a);for(var c=-1,d=b.length,e={};++c<d;){var f=b[c];f in a&&(e[f]=a[f])}return e}function Ac(a,b){var c={};return Ab(a,function(a,d,e){b(a,d,e)&&(c[d]=a)}),c}function Bc(a,b){for(var c=a.length,d=pg(b.length,c),e=Za(a);d--;){var f=b[d];a[d]=uc(f,c)?e[f]:v}return a}function Cc(a){var b;V.support;if(!n(a)||Vf.call(a)!=X||!Tf.call(a,"constructor")&&(b=a.constructor,"function"==typeof b&&!(b instanceof b)))return!1;var c;return Ab(a,function(a,b){c=b}),"undefined"==typeof c||Tf.call(a,c)}function Dc(a){for(var b=Qe(a),c=b.length,d=c&&a.length,e=V.support,f=d&&wc(d)&&(Pg(a)||e.nonEnumArgs&&le(a)),g=-1,h=[];++g<c;){var i=b[g];(f&&uc(i,d)||Tf.call(a,i))&&h.push(i)}return h}function Ec(a){return null==a?[]:wc(a.length)?te(a)?a:Kf(a):Xe(a)}function Fc(a){return te(a)?a:Kf(a)}function Gc(a,b,c){b=(c?vc(a,b,c):null==b)?1:og(+b||1,1);for(var d=0,e=a?a.length:0,f=-1,g=Ef($f(e/b));e>d;)g[++f]=Qb(a,d,d+=b);return g}function Hc(a){for(var b=-1,c=a?a.length:0,d=-1,e=[];++b<c;){var f=a[b];f&&(e[++d]=f)}return e}function Ic(){for(var a=-1,b=arguments.length;++a<b;){var c=arguments[a];if(Pg(c)||le(c))break}return rb(c,xb(arguments,!1,!0,++a))}function Jc(a,b,c){var d=a?a.length:0;return d?((c?vc(a,b,c):null==b)&&(b=1),Qb(a,0>b?0:b)):[]}function Kc(a,b,c){var d=a?a.length:0;return d?((c?vc(a,b,c):null==b)&&(b=1),b=d-(+b||0),Qb(a,0,0>b?0:b)):[]}function Lc(a,b,c){var d=a?a.length:0;if(!d)return[];for(b=nc(b,c,3);d--&&b(a[d],d,a););return Qb(a,0,d+1)}function Mc(a,b,c){var d=a?a.length:0;if(!d)return[];var e=-1;for(b=nc(b,c,3);++e<d&&b(a[e],e,a););return Qb(a,e)}function Nc(a,b,c){var d=-1,e=a?a.length:0;for(b=nc(b,c,3);++d<e;)if(b(a[d],d,a))return d;return-1}function Oc(a,b,c){var d=a?a.length:0;for(b=nc(b,c,3);d--;)if(b(a[d],d,a))return d;return-1}function Pc(a){return a?a[0]:v}function Qc(a,b,c){var d=a?a.length:0;return c&&vc(a,b,c)&&(b=!1),d?xb(a,b):[]}function Rc(a){var b=a?a.length:0;return b?xb(a,!0):[]}function Sc(a,c,d){var e=a?a.length:0;if(!e)return-1;if("number"==typeof d)d=0>d?og(e+d,0):d||0;else if(d){var f=Vb(a,c),g=a[f];return(c===c?c===g:g!==g)?f:-1}return b(a,c,d)}function Tc(a){return Kc(a,1)}function Uc(){for(var a=[],c=-1,d=arguments.length,e=[],f=oc(),g=f==b;++c<d;){var h=arguments[c];(Pg(h)||le(h))&&(a.push(h),e.push(g&&h.length>=120&&Fg(c&&h)))}d=a.length;var i=a[0],j=-1,k=i?i.length:0,l=[],m=e[0];a:for(;++j<k;)if(h=i[j],(m?Xa(m,h):f(l,h))<0){for(c=d;--c;){var n=e[c];if((n?Xa(n,h):f(a[c],h))<0)continue a}m&&m.push(h),l.push(h)}return l}function Vc(a){var b=a?a.length:0;return b?a[b-1]:v}function Wc(a,b,c){var d=a?a.length:0;if(!d)return-1;var e=d;if("number"==typeof c)e=(0>c?og(d+c,0):pg(c||0,d-1))+1;else if(c){e=Vb(a,b,!0)-1;var f=a[e];return(b===b?b===f:f!==f)?e:-1}if(b!==b)return m(a,e,!0);for(;e--;)if(a[e]===b)return e;return-1}function Xc(){var a=arguments[0];if(!a||!a.length)return a;for(var b=0,c=oc(),d=arguments.length;++b<d;)for(var e=0,f=arguments[b];(e=c(a,f,e))>-1;)gg.call(a,e,1);return a}function Yc(a){return Nb(a||[],xb(arguments,!1,!1,1))}function Zc(a,b,c){var d=-1,e=a?a.length:0,f=[];for(b=nc(b,c,3);++d<e;){var g=a[d];b(g,d,a)&&(f.push(g),gg.call(a,d--,1),e--)}return f}function $c(a){return Jc(a,1)}function _c(a,b,c){var d=a?a.length:0;return d?(c&&"number"!=typeof c&&vc(a,b,c)&&(b=0,c=d),Qb(a,b,c)):[]}function ad(a,b,c,d){var e=nc(c);return e===ob&&null==c?Vb(a,b):Wb(a,b,e(c,d,1))}function bd(a,b,c,d){var e=nc(c);return e===ob&&null==c?Vb(a,b,!0):Wb(a,b,e(c,d,1),!0)}function cd(a,b,c){var d=a?a.length:0;return d?((c?vc(a,b,c):null==b)&&(b=1),Qb(a,0,0>b?0:b)):[]}function dd(a,b,c){var d=a?a.length:0;return d?((c?vc(a,b,c):null==b)&&(b=1),b=d-(+b||0),Qb(a,0>b?0:b)):[]}function ed(a,b,c){var d=a?a.length:0;if(!d)return[];for(b=nc(b,c,3);d--&&b(a[d],d,a););return Qb(a,d+1)}function fd(a,b,c){var d=a?a.length:0;if(!d)return[];var e=-1;for(b=nc(b,c,3);++e<d&&b(a[e],e,a););return Qb(a,0,e)}function gd(){return Sb(xb(arguments,!1,!0))}function hd(a,c,d,e){var f=a?a.length:0;if(!f)return[];"boolean"!=typeof c&&null!=c&&(e=d,d=vc(a,c,e)?null:c,c=!1);var g=nc();return(g!==ob||null!=d)&&(d=g(d,e,3)),c&&oc()==b?q(a,d):Sb(a,d)}function id(a){for(var b=-1,c=(a&&a.length&&db(cb(a,Sf)))>>>0,d=Ef(c);++b<c;)d[b]=cb(a,Mb(b));return d}function jd(a){return rb(a,Qb(arguments,1))}function kd(){for(var a=-1,b=arguments.length;++a<b;){var c=arguments[a];if(Pg(c)||le(c))var d=d?rb(d,c).concat(rb(c,d)):c}return d?Sb(d):[]}function ld(){for(var a=arguments.length,b=Ef(a);a--;)b[a]=arguments[a];return id(b)}function md(a,b){var c=-1,d=a?a.length:0,e={};for(!d||b||Pg(a[0])||(b=[]);++c<d;){var f=a[c];b?e[f]=b[c]:f&&(e[f[0]]=f[1])}return e}function nd(a){var b=V(a);return b.__chain__=!0,b}function od(a,b,c){return b.call(c,a),a}function pd(a,b,c){return b.call(c,a)}function qd(){return nd(this)}function rd(){var a=this.__wrapped__;return a instanceof _?new Z(a.reverse()):this.thru(function(a){return a.reverse()})}function sd(){return this.value()+""}function td(){return Ub(this.__wrapped__,this.__actions__)}function ud(a){var b=a?a.length:0;return wc(b)&&(a=Ec(a)),lb(a,xb(arguments,!1,!1,1))}function vd(a,b,c){var d=a?a.length:0;return wc(d)||(a=Xe(a),d=a.length),d?(c="number"==typeof c?0>c?og(d+c,0):c||0:0,"string"==typeof a||!Pg(a)&&Ae(a)?d>c&&a.indexOf(b,c)>-1:oc(a,b,c)>-1):!1}function wd(a,b,c){var d=Pg(a)?ab:ub;return("function"!=typeof b||"undefined"!=typeof c)&&(b=nc(b,c,3)),d(a,b)}function xd(a,b,c){var d=Pg(a)?bb:vb;return b=nc(b,c,3),d(a,b)}function yd(a,b,c){if(Pg(a)){var d=Nc(a,b,c);return d>-1?a[d]:v}return b=nc(b,c,3),wb(a,b,sb)}function zd(a,b,c){return b=nc(b,c,3),wb(a,b,tb)}function Ad(a,b){return yd(a,vf(b))}function Bd(a,b,c){return"function"==typeof b&&"undefined"==typeof c&&Pg(a)?$a(a,b):sb(a,Xb(b,c,3))}function Cd(a,b,c){return"function"==typeof b&&"undefined"==typeof c&&Pg(a)?_a(a,b):tb(a,Xb(b,c,3))}function Dd(a,b){return Eb(a,b,Qb(arguments,2))}function Ed(a,b,c){var d=Pg(a)?cb:Ib;return b=nc(b,c,3),d(a,b)}function Fd(a,b){return Ed(a,zf(b))}function Gd(a,b,c,d){var e=Pg(a)?fb:Pb;return e(a,nc(b,d,4),c,arguments.length<3,sb)}function Hd(a,b,c,d){var e=Pg(a)?gb:Pb;return e(a,nc(b,d,4),c,arguments.length<3,tb)}function Id(a,b,c){var d=Pg(a)?bb:vb;return b=nc(b,c,3),d(a,function(a,c,d){return!b(a,c,d)})}function Jd(a,b,c){if(c?vc(a,b,c):null==b){a=Ec(a);var d=a.length;return d>0?a[Ob(0,d-1)]:v}var e=Kd(a);return e.length=pg(0>b?0:+b||0,e.length),e}function Kd(a){a=Ec(a);for(var b=-1,c=a.length,d=Ef(c);++b<c;){var e=Ob(0,b);b!=e&&(d[b]=d[e]),d[e]=a[b]}return d}function Ld(a){var b=a?a.length:0;return wc(b)?b:Tg(a).length}function Md(a,b,c){var d=Pg(a)?hb:Rb;return("function"!=typeof b||"undefined"!=typeof c)&&(b=nc(b,c,3)),d(a,b)}function Nd(a,b,d){var e=-1,f=a?a.length:0,g=wc(f)?Ef(f):[];return d&&vc(a,b,d)&&(b=null),b=nc(b,d,3),sb(a,function(a,c,d){g[++e]={criteria:b(a,c,d),index:e,value:a}}),c(g,h)}function Od(a){var b=arguments;b.length>3&&vc(b[1],b[2],b[3])&&(b=[a,b[1]]);var d=-1,e=a?a.length:0,f=xb(b,!1,!1,1),g=wc(e)?Ef(e):[];return sb(a,function(a,b,c){for(var e=f.length,h=Ef(e);e--;)h[e]=null==a?v:a[f[e]];g[++d]={criteria:h,index:d,value:a}}),c(g,i)}function Pd(a,b){return xd(a,vf(b))}function Qd(a,b){if(!se(b)){if(!se(a))throw new Nf(N);var c=a;a=b,b=c}return a=mg(a=+a)?a:0,function(){return--a<1?b.apply(this,arguments):void 0}}function Rd(a,b,c){return c&&vc(a,b,c)&&(b=null),b=a&&null==b?a.length:og(+b||0,0),ic(a,F,null,null,null,null,b)}function Sd(a,b){var c;if(!se(b)){if(!se(a))throw new Nf(N);var d=a;a=b,b=d}return function(){return--a>0?c=b.apply(this,arguments):b=null,c}}function Td(a,b){var c=x;if(arguments.length>2){var d=Qb(arguments,2),e=p(d,Td.placeholder);c|=C}return ic(a,c,b,d,e)}function Ud(a){return nb(a,arguments.length>1?xb(arguments,!1,!1,1):Ne(a))}function Vd(a,b){var c=x|y;if(arguments.length>2){var d=Qb(arguments,2),e=p(d,Vd.placeholder);c|=C}return ic(b,c,a,d,e)}function Wd(a,b,c){c&&vc(a,b,c)&&(b=null);var d=ic(a,A,null,null,null,null,null,b);return d.placeholder=Wd.placeholder,d}function Xd(a,b,c){c&&vc(a,b,c)&&(b=null);var d=ic(a,B,null,null,null,null,null,b);return d.placeholder=Xd.placeholder,d}function Yd(a,b,c){function d(){m&&_f(m),i&&_f(i),i=m=n=v}function e(){var c=b-(Og()-k);if(0>=c||c>b){i&&_f(i);var d=n;i=m=n=v,d&&(o=Og(),j=a.apply(l,h),m||i||(h=l=null))}else m=fg(e,c)}function f(){m&&_f(m),i=m=n=v,(q||p!==b)&&(o=Og(),j=a.apply(l,h),m||i||(h=l=null))}function g(){if(h=arguments,k=Og(),l=this,n=q&&(m||!r),p===!1)var c=r&&!m;else{i||r||(o=k);var d=p-(k-o),g=0>=d||d>p;g?(i&&(i=_f(i)),o=k,j=a.apply(l,h)):i||(i=fg(f,d))}return g&&m?m=_f(m):m||b===p||(m=fg(e,b)),c&&(g=!0,j=a.apply(l,h)),!g||m||i||(h=l=null),j}var h,i,j,k,l,m,n,o=0,p=!1,q=!0;if(!se(a))throw new Nf(N);if(b=0>b?0:b,c===!0){var r=!0;q=!1}else te(c)&&(r=c.leading,p="maxWait"in c&&og(+c.maxWait||0,b),q="trailing"in c?c.trailing:q);return g.cancel=d,g}function Zd(a){return qb(a,1,arguments,1)}function $d(a,b){return qb(a,b,arguments,2)}function _d(){var a=arguments,b=a.length;if(!b)return function(){};if(!ab(a,se))throw new Nf(N);return function(){for(var c=0,d=a[c].apply(this,arguments);++c<b;)d=a[c].call(this,d);return d}}function ae(){var a=arguments,b=a.length-1;if(0>b)return function(){};if(!ab(a,se))throw new Nf(N);return function(){for(var c=b,d=a[c].apply(this,arguments);c--;)d=a[c].call(this,d);return d}}function be(a,b){if(!se(a)||b&&!se(b))throw new Nf(N);var c=function(){var d=c.cache,e=b?b.apply(this,arguments):arguments[0];if(d.has(e))return d.get(e);var f=a.apply(this,arguments);return d.set(e,f),f};return c.cache=new be.Cache,c}function ce(a){if(!se(a))throw new Nf(N);return function(){return!a.apply(this,arguments)}}function de(a){return Sd(a,2)}function ee(a){var b=Qb(arguments,1),c=p(b,ee.placeholder);return ic(a,C,null,b,c)}function fe(a){var b=Qb(arguments,1),c=p(b,fe.placeholder);return ic(a,D,null,b,c)}function ge(a){var b=xb(arguments,!1,!1,1);return ic(a,E,null,null,null,b)}function he(a,b,c){var d=!0,e=!0;if(!se(a))throw new Nf(N);return c===!1?d=!1:te(c)&&(d="leading"in c?!!c.leading:d,e="trailing"in c?!!c.trailing:e),La.leading=d,La.maxWait=+b,La.trailing=e,Yd(a,b,La)}function ie(a,b){return b=null==b?uf:b,ic(b,C,null,[a],[])}function je(a,b,c,d){return"boolean"!=typeof b&&null!=b&&(d=c,c=vc(a,b,d)?null:b,b=!1),c="function"==typeof c&&Xb(c,d,1),pb(a,b,c)}function ke(a,b,c){return b="function"==typeof b&&Xb(b,c,1),pb(a,!0,b)}function le(a){var b=n(a)?a.length:v;return wc(b)&&Vf.call(a)==P||!1}function me(a){return a===!0||a===!1||n(a)&&Vf.call(a)==R||!1}function ne(a){return n(a)&&Vf.call(a)==S||!1}function oe(a){return a&&1===a.nodeType&&n(a)&&Vf.call(a).indexOf("Element")>-1||!1}function pe(a){if(null==a)return!0;var b=a.length;return wc(b)&&(Pg(a)||Ae(a)||le(a)||n(a)&&se(a.splice))?!b:!Tg(a).length}function qe(a,b,c,d){if(c="function"==typeof c&&Xb(c,d,3),!c&&xc(a)&&xc(b))return a===b;var e=c?c(a,b):v;return"undefined"==typeof e?Fb(a,b,c):!!e}function re(a){return n(a)&&"string"==typeof a.message&&Vf.call(a)==T||!1}function se(a){return"function"==typeof a||!1}function te(a){var b=typeof a;return"function"==b||a&&"object"==b||!1}function ue(a,b,c,d){var e=Tg(b),f=e.length;if(c="function"==typeof c&&Xb(c,d,3),!c&&1==f){var g=e[0],h=b[g];if(xc(h))return null!=a&&h===a[g]&&Tf.call(a,g)}for(var i=Ef(f),j=Ef(f);f--;)h=i[f]=b[e[f]],j[f]=xc(h);return Hb(a,e,i,j,c)}function ve(a){return ye(a)&&a!=+a}function we(a){return null==a?!1:Vf.call(a)==U?Xf.test(Rf.call(a)):n(a)&&ya.test(a)||!1}function xe(a){return null===a}function ye(a){return"number"==typeof a||n(a)&&Vf.call(a)==W||!1}function ze(a){return n(a)&&Vf.call(a)==Y||!1}function Ae(a){return"string"==typeof a||n(a)&&Vf.call(a)==$||!1}function Be(a){return n(a)&&wc(a.length)&&Ja[Vf.call(a)]||!1}function Ce(a){return"undefined"==typeof a}function De(a){var b=a?a.length:0;return wc(b)?b?Za(a):[]:Xe(a)}function Ee(a){return mb(a,Qe(a))}function Fe(a,b,c){var d=Dg(a);return c&&vc(a,b,c)&&(b=null),b?mb(b,d,Tg(b)):d}function Ge(a){if(null==a)return a;var b=Za(arguments);return b.push(ib),Sg.apply(v,b)}function He(a,b,c){return b=nc(b,c,3),wb(a,b,Bb,!0)}function Ie(a,b,c){return b=nc(b,c,3),wb(a,b,Cb,!0)}function Je(a,b,c){return("function"!=typeof b||"undefined"!=typeof c)&&(b=Xb(b,c,3)),yb(a,b,Qe)}function Ke(a,b,c){return b=Xb(b,c,3),zb(a,b,Qe)}function Le(a,b,c){return("function"!=typeof b||"undefined"!=typeof c)&&(b=Xb(b,c,3)),Bb(a,b)}function Me(a,b,c){return b=Xb(b,c,3),zb(a,b,Tg)}function Ne(a){return Db(a,Qe(a))}function Oe(a,b){return a?Tf.call(a,b):!1}function Pe(a,b,c){c&&vc(a,b,c)&&(b=null);for(var d=-1,e=Tg(a),f=e.length,g={};++d<f;){var h=e[d],i=a[h];b?Tf.call(g,i)?g[i].push(h):g[i]=[h]:g[i]=h}return g}function Qe(a){if(null==a)return[];te(a)||(a=Kf(a));var b=a.length;b=b&&wc(b)&&(Pg(a)||Cg.nonEnumArgs&&le(a))&&b||0;for(var c=a.constructor,d=-1,e="function"==typeof c&&c.prototype==a,f=Ef(b),g=b>0;++d<b;)f[d]=d+"";for(var h in a)g&&uc(h,b)||"constructor"==h&&(e||!Tf.call(a,h))||f.push(h);return f}function Re(a,b,c){var d={};return b=nc(b,c,3),Bb(a,function(a,c,e){d[c]=b(a,c,e)}),d}function Se(a,b,c){if(null==a)return{};if("function"!=typeof b){var d=cb(xb(arguments,!1,!1,1),Mf);return zc(a,rb(Qe(a),d))}return b=Xb(b,c,3),Ac(a,function(a,c,d){return!b(a,c,d)})}function Te(a){for(var b=-1,c=Tg(a),d=c.length,e=Ef(d);++b<d;){var f=c[b];e[b]=[f,a[f]]}return e}function Ue(a,b,c){return null==a?{}:"function"==typeof b?Ac(a,Xb(b,c,3)):zc(a,xb(arguments,!1,!1,1))}function Ve(a,b,c){var d=null==a?v:a[b];return"undefined"==typeof d&&(d=c),se(d)?d.call(a):d}function We(a,b,c,d){var e=Pg(a)||Be(a);if(b=nc(b,d,4),null==c)if(e||te(a)){var f=a.constructor;c=e?Pg(a)?new f:[]:Dg("function"==typeof f&&f.prototype)}else c={};return(e?$a:Bb)(a,function(a,d,e){return b(c,a,d,e)}),c}function Xe(a){return Tb(a,Tg(a))}function Ye(a){return Tb(a,Qe(a))}function Ze(a,b,c){c&&vc(a,b,c)&&(b=c=null);var d=null==a,e=null==b;if(null==c&&(e&&"boolean"==typeof a?(c=a,a=1):"boolean"==typeof b&&(c=b,e=!0)),d&&e&&(b=1,e=!1),a=+a||0,e?(b=a,a=0):b=+b||0,c||a%1||b%1){var f=tg();return pg(a+f*(b-a+parseFloat("1e-"+((f+"").length-1))),b)}return Ob(a,b)}function $e(a){return a=d(a),a&&a.charAt(0).toUpperCase()+a.slice(1)}function _e(a){return a=d(a),a&&a.replace(za,j)}function af(a,b,c){a=d(a),b+="";var e=a.length;return c=("undefined"==typeof c?e:pg(0>c?0:+c||0,e))-b.length,c>=0&&a.indexOf(b,c)==c}function bf(a){return a=d(a),a&&qa.test(a)?a.replace(oa,k):a}function cf(a){return a=d(a),a&&Ca.test(a)?a.replace(Ba,"\\$&"):a}function df(a,b,c){a=d(a),b=+b;var e=a.length;if(e>=b||!mg(b))return a;var f=(b-e)/2,g=ag(f),h=$f(f);return c=gc("",h,c),c.slice(0,g)+a+c}function ef(a,b,c){return a=d(a),a&&gc(a,b,c)+a}function ff(a,b,c){return a=d(a),a&&a+gc(a,b,c)}function gf(a,b,c){return c&&vc(a,b,c)&&(b=0),sg(a,b)}function hf(a,b){var c="";if(a=d(a),b=+b,1>b||!a||!mg(b))return c;do b%2&&(c+=a),b=ag(b/2),a+=a;while(b);return c}function jf(a,b,c){return a=d(a),c=null==c?0:pg(0>c?0:+c||0,a.length),a.lastIndexOf(b,c)==c}function kf(a,b,c){var e=V.templateSettings;c&&vc(a,b,c)&&(b=c=null),a=d(a),b=kb(kb({},c||b),e,jb);
var f,g,h=kb(kb({},b.imports),e.imports,jb),i=Tg(h),j=Tb(h,i),k=0,m=b.interpolate||Aa,n="__p += '",o=Lf((b.escape||Aa).source+"|"+m.source+"|"+(m===ta?ua:Aa).source+"|"+(b.evaluate||Aa).source+"|$","g"),p="//# sourceURL="+("sourceURL"in b?b.sourceURL:"lodash.templateSources["+ ++Ia+"]")+"\n";a.replace(o,function(b,c,d,e,h,i){return d||(d=e),n+=a.slice(k,i).replace(Ea,l),c&&(f=!0,n+="' +\n__e("+c+") +\n'"),h&&(g=!0,n+="';\n"+h+";\n__p += '"),d&&(n+="' +\n((__t = ("+d+")) == null ? '' : __t) +\n'"),k=i+b.length,b}),n+="';\n";var q=b.variable;q||(n="with (obj) {\n"+n+"\n}\n"),n=(g?n.replace(ka,""):n).replace(la,"$1").replace(ma,"$1;"),n="function("+(q||"obj")+") {\n"+(q?"":"obj || (obj = {});\n")+"var __t, __p = ''"+(f?", __e = _.escape":"")+(g?", __j = Array.prototype.join;\nfunction print() { __p += __j.call(arguments, '') }\n":";\n")+n+"return __p\n}";var r=rf(function(){return Hf(i,p+"return "+n).apply(v,j)});if(r.source=n,re(r))throw r;return r}function lf(a,b,c){var e=a;return(a=d(a))?(c?vc(e,b,c):null==b)?a.slice(r(a),s(a)+1):(b=d(b),a.slice(f(a,b),g(a,b)+1)):a}function mf(a,b,c){var e=a;return a=d(a),a?(c?vc(e,b,c):null==b)?a.slice(r(a)):a.slice(f(a,d(b))):a}function nf(a,b,c){var e=a;return a=d(a),a?(c?vc(e,b,c):null==b)?a.slice(0,s(a)+1):a.slice(0,g(a,d(b))+1):a}function of(a,b,c){c&&vc(a,b,c)&&(b=null);var e=G,f=H;if(null!=b)if(te(b)){var g="separator"in b?b.separator:g;e="length"in b?+b.length||0:e,f="omission"in b?d(b.omission):f}else e=+b||0;if(a=d(a),e>=a.length)return a;var h=e-f.length;if(1>h)return f;var i=a.slice(0,h);if(null==g)return i+f;if(ze(g)){if(a.slice(h).search(g)){var j,k,l=a.slice(0,h);for(g.global||(g=Lf(g.source,(va.exec(g)||"")+"g")),g.lastIndex=0;j=g.exec(l);)k=j.index;i=i.slice(0,null==k?h:k)}}else if(a.indexOf(g,h)!=h){var m=i.lastIndexOf(g);m>-1&&(i=i.slice(0,m))}return i+f}function pf(a){return a=d(a),a&&pa.test(a)?a.replace(na,t):a}function qf(a,b,c){return c&&vc(a,b,c)&&(b=null),a=d(a),a.match(b||Fa)||[]}function rf(a){try{return a()}catch(b){return re(b)?b:Gf(b)}}function sf(a,b,c){return c&&vc(a,b,c)&&(b=null),ob(a,b)}function tf(a){return function(){return a}}function uf(a){return a}function vf(a){return Jb(a,!0)}function wf(a,b,c){if(null==c){var d=te(b),e=d&&Tg(b),f=e&&e.length&&Db(b,e);(f?f.length:d)||(f=!1,c=b,b=a,a=this)}f||(f=Db(b,Tg(b)));var g=!0,h=-1,i=se(a),j=f.length;c===!1?g=!1:te(c)&&"chain"in c&&(g=c.chain);for(;++h<j;){var k=f[h],l=b[k];a[k]=l,i&&(a.prototype[k]=function(b){return function(){var c=this.__chain__;if(g||c){var d=a(this.__wrapped__);return(d.__actions__=Za(this.__actions__)).push({func:b,args:arguments,thisArg:a}),d.__chain__=c,d}var e=[this.value()];return cg.apply(e,arguments),b.apply(a,e)}}(l))}return a}function xf(){return o._=Wf,this}function yf(){}function zf(a){return Mb(a+"")}function Af(a){return function(b){return null==a?v:a[b]}}function Bf(a,b,c){c&&vc(a,b,c)&&(b=c=null),a=+a||0,c=null==c?1:+c||0,null==b?(b=a,a=0):b=+b||0;for(var d=-1,e=og($f((b-a)/(c||1)),0),f=Ef(e);++d<e;)f[d]=a,a+=c;return f}function Cf(a,b,c){if(a=+a,1>a||!mg(a))return[];var d=-1,e=Ef(pg(a,wg));for(b=Xb(b,c,1);++d<a;)wg>d?e[d]=b(d):b(d);return e}function Df(a){var b=++Uf;return d(a)+b}o=o?Wa.defaults(Ra.Object(),o,Wa.pick(Ra,Ha)):Ra;var Ef=o.Array,Ff=o.Date,Gf=o.Error,Hf=o.Function,If=o.Math,Jf=o.Number,Kf=o.Object,Lf=o.RegExp,Mf=o.String,Nf=o.TypeError,Of=Ef.prototype,Pf=Kf.prototype,Qf=(Qf=o.window)&&Qf.document,Rf=Hf.prototype.toString,Sf=Mb("length"),Tf=Pf.hasOwnProperty,Uf=0,Vf=Pf.toString,Wf=o._,Xf=Lf("^"+cf(Vf).replace(/toString|(function).*?(?=\\\()| for .+?(?=\\\])/g,"$1.*?")+"$"),Yf=we(Yf=o.ArrayBuffer)&&Yf,Zf=we(Zf=Yf&&new Yf(0).slice)&&Zf,$f=If.ceil,_f=o.clearTimeout,ag=If.floor,bg=we(bg=Kf.getPrototypeOf)&&bg,cg=Of.push,dg=Pf.propertyIsEnumerable,eg=we(eg=o.Set)&&eg,fg=o.setTimeout,gg=Of.splice,hg=we(hg=o.Uint8Array)&&hg,ig=(Of.unshift,we(ig=o.WeakMap)&&ig),jg=function(){try{var a=we(a=o.Float64Array)&&a,b=new a(new Yf(10),0,1)&&a}catch(c){}return b}(),kg=we(kg=Ef.isArray)&&kg,lg=we(lg=Kf.create)&&lg,mg=o.isFinite,ng=we(ng=Kf.keys)&&ng,og=If.max,pg=If.min,qg=we(qg=Ff.now)&&qg,rg=we(rg=Jf.isFinite)&&rg,sg=o.parseInt,tg=If.random,ug=Jf.NEGATIVE_INFINITY,vg=Jf.POSITIVE_INFINITY,wg=If.pow(2,32)-1,xg=wg-1,yg=wg>>>1,zg=jg?jg.BYTES_PER_ELEMENT:0,Ag=If.pow(2,53)-1,Bg=ig&&new ig,Cg=V.support={};!function(a){Cg.funcDecomp=!we(o.WinRTError)&&Da.test(u),Cg.funcNames="string"==typeof Hf.name;try{Cg.dom=11===Qf.createDocumentFragment().nodeType}catch(b){Cg.dom=!1}try{Cg.nonEnumArgs=!dg.call(arguments,1)}catch(b){Cg.nonEnumArgs=!0}}(0,0),V.templateSettings={escape:ra,evaluate:sa,interpolate:ta,variable:"",imports:{_:V}};var Dg=function(){function a(){}return function(b){if(te(b)){a.prototype=b;var c=new a;a.prototype=null}return c||o.Object()}}(),Eg=Bg?function(a,b){return Bg.set(a,b),a}:uf;Zf||(Yb=Yf&&hg?function(a){var b=a.byteLength,c=jg?ag(b/zg):0,d=c*zg,e=new Yf(b);if(c){var f=new jg(e,0,c);f.set(new jg(a,0,c))}return b!=d&&(f=new hg(e,d),f.set(new hg(a,d))),e}:tf(null));var Fg=lg&&eg?function(a){return new Va(a)}:tf(null),Gg=Bg?function(a){return Bg.get(a)}:yf,Hg=function(){var a=0,b=0;return function(c,d){var e=Og(),f=J-(e-b);if(b=e,f>0){if(++a>=I)return c}else a=0;return Eg(c,d)}}(),Ig=_b(function(a,b,c){Tf.call(a,c)?++a[c]:a[c]=1}),Jg=_b(function(a,b,c){Tf.call(a,c)?a[c].push(b):a[c]=[b]}),Kg=_b(function(a,b,c){a[c]=b}),Lg=ec(db),Mg=ec(eb,!0),Ng=_b(function(a,b,c){a[c?0:1].push(b)},function(){return[[],[]]}),Og=qg||function(){return(new Ff).getTime()},Pg=kg||function(a){return n(a)&&wc(a.length)&&Vf.call(a)==Q||!1};Cg.dom||(oe=function(a){return a&&1===a.nodeType&&n(a)&&!Rg(a)||!1});var Qg=rg||function(a){return"number"==typeof a&&mg(a)};(se(/x/)||hg&&!se(hg))&&(se=function(a){return Vf.call(a)==U});var Rg=bg?function(a){if(!a||Vf.call(a)!=X)return!1;var b=a.valueOf,c=we(b)&&(c=bg(b))&&bg(c);return c?a==c||bg(a)==c:Cc(a)}:Cc,Sg=ac(kb),Tg=ng?function(a){if(a)var b=a.constructor,c=a.length;return"function"==typeof b&&b.prototype===a||"function"!=typeof a&&c&&wc(c)?Dc(a):te(a)?ng(a):[]}:Dc,Ug=ac(Kb),Vg=cc(function(a,b,c){return b=b.toLowerCase(),c?a+b.charAt(0).toUpperCase()+b.slice(1):b}),Wg=cc(function(a,b,c){return a+(c?"-":"")+b.toLowerCase()});8!=sg(Ga+"08")&&(gf=function(a,b,c){return(c?vc(a,b,c):null==b)?b=0:b&&(b=+b),a=lf(a),sg(a,b||(xa.test(a)?16:10))});var Xg=cc(function(a,b,c){return a+(c?"_":"")+b.toLowerCase()});return Z.prototype=V.prototype,Pa.prototype["delete"]=Qa,Pa.prototype.get=Sa,Pa.prototype.has=Ta,Pa.prototype.set=Ua,Va.prototype.push=Ya,be.Cache=Pa,V.after=Qd,V.ary=Rd,V.assign=Sg,V.at=ud,V.before=Sd,V.bind=Td,V.bindAll=Ud,V.bindKey=Vd,V.callback=sf,V.chain=nd,V.chunk=Gc,V.compact=Hc,V.constant=tf,V.countBy=Ig,V.create=Fe,V.curry=Wd,V.curryRight=Xd,V.debounce=Yd,V.defaults=Ge,V.defer=Zd,V.delay=$d,V.difference=Ic,V.drop=Jc,V.dropRight=Kc,V.dropRightWhile=Lc,V.dropWhile=Mc,V.filter=xd,V.flatten=Qc,V.flattenDeep=Rc,V.flow=_d,V.flowRight=ae,V.forEach=Bd,V.forEachRight=Cd,V.forIn=Je,V.forInRight=Ke,V.forOwn=Le,V.forOwnRight=Me,V.functions=Ne,V.groupBy=Jg,V.indexBy=Kg,V.initial=Tc,V.intersection=Uc,V.invert=Pe,V.invoke=Dd,V.keys=Tg,V.keysIn=Qe,V.map=Ed,V.mapValues=Re,V.matches=vf,V.memoize=be,V.merge=Ug,V.mixin=wf,V.negate=ce,V.omit=Se,V.once=de,V.pairs=Te,V.partial=ee,V.partialRight=fe,V.partition=Ng,V.pick=Ue,V.pluck=Fd,V.property=zf,V.propertyOf=Af,V.pull=Xc,V.pullAt=Yc,V.range=Bf,V.rearg=ge,V.reject=Id,V.remove=Zc,V.rest=$c,V.shuffle=Kd,V.slice=_c,V.sortBy=Nd,V.sortByAll=Od,V.take=cd,V.takeRight=dd,V.takeRightWhile=ed,V.takeWhile=fd,V.tap=od,V.throttle=he,V.thru=pd,V.times=Cf,V.toArray=De,V.toPlainObject=Ee,V.transform=We,V.union=gd,V.uniq=hd,V.unzip=id,V.values=Xe,V.valuesIn=Ye,V.where=Pd,V.without=jd,V.wrap=ie,V.xor=kd,V.zip=ld,V.zipObject=md,V.backflow=ae,V.collect=Ed,V.compose=ae,V.each=Bd,V.eachRight=Cd,V.extend=Sg,V.iteratee=sf,V.methods=Ne,V.object=md,V.select=xd,V.tail=$c,V.unique=hd,wf(V,V),V.attempt=rf,V.camelCase=Vg,V.capitalize=$e,V.clone=je,V.cloneDeep=ke,V.deburr=_e,V.endsWith=af,V.escape=bf,V.escapeRegExp=cf,V.every=wd,V.find=yd,V.findIndex=Nc,V.findKey=He,V.findLast=zd,V.findLastIndex=Oc,V.findLastKey=Ie,V.findWhere=Ad,V.first=Pc,V.has=Oe,V.identity=uf,V.includes=vd,V.indexOf=Sc,V.isArguments=le,V.isArray=Pg,V.isBoolean=me,V.isDate=ne,V.isElement=oe,V.isEmpty=pe,V.isEqual=qe,V.isError=re,V.isFinite=Qg,V.isFunction=se,V.isMatch=ue,V.isNaN=ve,V.isNative=we,V.isNull=xe,V.isNumber=ye,V.isObject=te,V.isPlainObject=Rg,V.isRegExp=ze,V.isString=Ae,V.isTypedArray=Be,V.isUndefined=Ce,V.kebabCase=Wg,V.last=Vc,V.lastIndexOf=Wc,V.max=Lg,V.min=Mg,V.noConflict=xf,V.noop=yf,V.now=Og,V.pad=df,V.padLeft=ef,V.padRight=ff,V.parseInt=gf,V.random=Ze,V.reduce=Gd,V.reduceRight=Hd,V.repeat=hf,V.result=Ve,V.runInContext=u,V.size=Ld,V.snakeCase=Xg,V.some=Md,V.sortedIndex=ad,V.sortedLastIndex=bd,V.startsWith=jf,V.template=kf,V.trim=lf,V.trimLeft=mf,V.trimRight=nf,V.trunc=of,V.unescape=pf,V.uniqueId=Df,V.words=qf,V.all=wd,V.any=Md,V.contains=vd,V.detect=yd,V.foldl=Gd,V.foldr=Hd,V.head=Pc,V.include=vd,V.inject=Gd,wf(V,function(){var a={};return Bb(V,function(b,c){V.prototype[c]||(a[c]=b)}),a}(),!1),V.sample=Jd,V.prototype.sample=function(a){return this.__chain__||null!=a?this.thru(function(b){return Jd(b,a)}):Jd(this.value())},V.VERSION=w,$a(["bind","bindKey","curry","curryRight","partial","partialRight"],function(a){V[a].placeholder=V}),$a(["filter","map","takeWhile"],function(a,b){var c=b==K;_.prototype[a]=function(a,d){var e=this.clone(),f=e.filtered,g=e.iteratees||(e.iteratees=[]);return e.filtered=f||c||b==M&&e.dir<0,g.push({iteratee:nc(a,d,3),type:b}),e}}),$a(["drop","take"],function(a,b){var c=a+"Count",d=a+"While";_.prototype[a]=function(d){d=null==d?1:og(+d||0,0);var e=this.clone();if(e.filtered){var f=e[c];e[c]=b?pg(f,d):f+d}else{var g=e.views||(e.views=[]);g.push({size:d,type:a+(e.dir<0?"Right":"")})}return e},_.prototype[a+"Right"]=function(b){return this.reverse()[a](b).reverse()},_.prototype[a+"RightWhile"]=function(a,b){return this.reverse()[d](a,b).reverse()}}),$a(["first","last"],function(a,b){var c="take"+(b?"Right":"");_.prototype[a]=function(){return this[c](1).value()[0]}}),$a(["initial","rest"],function(a,b){var c="drop"+(b?"":"Right");_.prototype[a]=function(){return this[c](1)}}),$a(["pluck","where"],function(a,b){var c=b?"filter":"map",d=b?vf:zf;_.prototype[a]=function(a){return this[c](d(a))}}),_.prototype.dropWhile=function(a,b){var c,d,e=this.dir<0;return a=nc(a,b,3),this.filter(function(b,f,g){return c=c&&(e?d>f:f>d),d=f,c||(c=!a(b,f,g))})},_.prototype.reject=function(a,b){return a=nc(a,b,3),this.filter(function(b,c,d){return!a(b,c,d)})},_.prototype.slice=function(a,b){a=null==a?0:+a||0;var c=0>a?this.takeRight(-a):this.drop(a);return"undefined"!=typeof b&&(b=+b||0,c=0>b?c.dropRight(-b):c.take(b-a)),c},Bb(_.prototype,function(a,b){var c=/^(?:first|last)$/.test(b);V.prototype[b]=function(){var d=this.__wrapped__,e=arguments,f=this.__chain__,g=!!this.__actions__.length,h=d instanceof _,i=h&&!g;if(c&&!f)return i?a.call(d):V[b](this.value());var j=function(a){var c=[a];return cg.apply(c,e),V[b].apply(V,c)};if(h||Pg(d)){var k=i?d:new _(this),l=a.apply(k,e);if(!c&&(g||l.actions)){var m=l.actions||(l.actions=[]);m.push({func:pd,args:[j],thisArg:V})}return new Z(l,f)}return this.thru(j)}}),$a(["concat","join","pop","push","shift","sort","splice","unshift"],function(a){var b=Of[a],c=/^(?:push|sort|unshift)$/.test(a)?"tap":"thru",d=/^(?:join|pop|shift)$/.test(a);V.prototype[a]=function(){var a=arguments;return d&&!this.__chain__?b.apply(this.value(),a):this[c](function(c){return b.apply(c,a)})}}),_.prototype.clone=Ma,_.prototype.reverse=Na,_.prototype.value=Oa,V.prototype.chain=qd,V.prototype.reverse=rd,V.prototype.toString=sd,V.prototype.toJSON=V.prototype.valueOf=V.prototype.value=td,V.prototype.collect=V.prototype.map,V.prototype.head=V.prototype.first,V.prototype.select=V.prototype.filter,V.prototype.tail=V.prototype.rest,V}var v,w="3.0.0",x=1,y=2,z=4,A=8,B=16,C=32,D=64,E=128,F=256,G=30,H="...",I=150,J=16,K=0,L=1,M=2,N="Expected a function",O="__lodash_placeholder__",P="[object Arguments]",Q="[object Array]",R="[object Boolean]",S="[object Date]",T="[object Error]",U="[object Function]",V="[object Map]",W="[object Number]",X="[object Object]",Y="[object RegExp]",Z="[object Set]",$="[object String]",_="[object WeakMap]",aa="[object ArrayBuffer]",ba="[object Float32Array]",ca="[object Float64Array]",da="[object Int8Array]",ea="[object Int16Array]",fa="[object Int32Array]",ga="[object Uint8Array]",ha="[object Uint8ClampedArray]",ia="[object Uint16Array]",ja="[object Uint32Array]",ka=/\b__p \+= '';/g,la=/\b(__p \+=) '' \+/g,ma=/(__e\(.*?\)|\b__t\)) \+\n'';/g,na=/&(?:amp|lt|gt|quot|#39|#96);/g,oa=/[&<>"'`]/g,pa=RegExp(na.source),qa=RegExp(oa.source),ra=/<%-([\s\S]+?)%>/g,sa=/<%([\s\S]+?)%>/g,ta=/<%=([\s\S]+?)%>/g,ua=/\$\{([^\\}]*(?:\\.[^\\}]*)*)\}/g,va=/\w*$/,wa=/^\s*function[ \n\r\t]+\w/,xa=/^0[xX]/,ya=/^\[object .+?Constructor\]$/,za=/[\xc0-\xd6\xd8-\xde\xdf-\xf6\xf8-\xff]/g,Aa=/($^)/,Ba=/[.*+?^${}()|[\]\/\\]/g,Ca=RegExp(Ba.source),Da=/\bthis\b/,Ea=/['\n\r\u2028\u2029\\]/g,Fa=function(){var a="[A-Z\\xc0-\\xd6\\xd8-\\xde]",b="[a-z\\xdf-\\xf6\\xf8-\\xff]+";return RegExp(a+"{2,}(?="+a+b+")|"+a+"?"+b+"|"+a+"+|[0-9]+","g")}(),Ga=" 	\f \ufeff\n\r\u2028\u2029 ᠎             　",Ha=["Array","ArrayBuffer","Date","Error","Float32Array","Float64Array","Function","Int8Array","Int16Array","Int32Array","Math","Number","Object","RegExp","Set","String","_","clearTimeout","document","isFinite","parseInt","setTimeout","TypeError","Uint8Array","Uint8ClampedArray","Uint16Array","Uint32Array","WeakMap","window","WinRTError"],Ia=-1,Ja={};Ja[ba]=Ja[ca]=Ja[da]=Ja[ea]=Ja[fa]=Ja[ga]=Ja[ha]=Ja[ia]=Ja[ja]=!0,Ja[P]=Ja[Q]=Ja[aa]=Ja[R]=Ja[S]=Ja[T]=Ja[U]=Ja[V]=Ja[W]=Ja[X]=Ja[Y]=Ja[Z]=Ja[$]=Ja[_]=!1;var Ka={};Ka[P]=Ka[Q]=Ka[aa]=Ka[R]=Ka[S]=Ka[ba]=Ka[ca]=Ka[da]=Ka[ea]=Ka[fa]=Ka[W]=Ka[X]=Ka[Y]=Ka[$]=Ka[ga]=Ka[ha]=Ka[ia]=Ka[ja]=!0,Ka[T]=Ka[U]=Ka[V]=Ka[Z]=Ka[_]=!1;var La={leading:!1,maxWait:0,trailing:!1},Ma={"À":"A","Á":"A","Â":"A","Ã":"A","Ä":"A","Å":"A","à":"a","á":"a","â":"a","ã":"a","ä":"a","å":"a","Ç":"C","ç":"c","Ð":"D","ð":"d","È":"E","É":"E","Ê":"E","Ë":"E","è":"e","é":"e","ê":"e","ë":"e","Ì":"I","Í":"I","Î":"I","Ï":"I","ì":"i","í":"i","î":"i","ï":"i","Ñ":"N","ñ":"n","Ò":"O","Ó":"O","Ô":"O","Õ":"O","Ö":"O","Ø":"O","ò":"o","ó":"o","ô":"o","õ":"o","ö":"o","ø":"o","Ù":"U","Ú":"U","Û":"U","Ü":"U","ù":"u","ú":"u","û":"u","ü":"u","Ý":"Y","ý":"y","ÿ":"y","Æ":"Ae","æ":"ae","Þ":"Th","þ":"th","ß":"ss"},Na={"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;","`":"&#96;"},Oa={"&amp;":"&","&lt;":"<","&gt;":">","&quot;":'"',"&#39;":"'","&#96;":"`"},Pa={"function":!0,object:!0},Qa={"\\":"\\","'":"'","\n":"n","\r":"r","\u2028":"u2028","\u2029":"u2029"},Ra=Pa[typeof window]&&window!==(this&&this.window)?window:this,Sa=Pa[typeof exports]&&exports&&!exports.nodeType&&exports,Ta=Pa[typeof module]&&module&&!module.nodeType&&module,Ua=Sa&&Ta&&"object"==typeof global&&global;!Ua||Ua.global!==Ua&&Ua.window!==Ua&&Ua.self!==Ua||(Ra=Ua);var Va=Ta&&Ta.exports===Sa&&Sa,Wa=u();"function"==typeof define&&"object"==typeof define.amd&&define.amd?(Ra._=Wa,define(function(){return Wa})):Sa&&Ta?Va?(Ta.exports=Wa)._=Wa:Sa._=Wa:Ra._=Wa}.call(this),function(a,b){if("function"==typeof define&&define.amd)define(["../../.","jquery","exports"],function(c,d,e){a.Backbone=b(a,e,c,d)});else if("undefined"!=typeof exports){var c=require("underscore");b(a,exports,c)}else a.Backbone=b(a,{},a._,a.jQuery||a.Zepto||a.ender||a.$)}(this,function(a,b,c,d){var e=a.Backbone,f=[],g=(f.push,f.slice);f.splice;b.VERSION="1.1.2",b.$=d,b.noConflict=function(){return a.Backbone=e,this},b.emulateHTTP=!1,b.emulateJSON=!1;var h=b.Events={on:function(a,b,c){if(!j(this,"on",a,[b,c])||!b)return this;this._events||(this._events={});var d=this._events[a]||(this._events[a]=[]);return d.push({callback:b,context:c,ctx:c||this}),this},once:function(a,b,d){if(!j(this,"once",a,[b,d])||!b)return this;var e=this,f=c.once(function(){e.off(a,f),b.apply(this,arguments)});return f._callback=b,this.on(a,f,d)},off:function(a,b,d){var e,f,g,h,i,k,l,m;if(!this._events||!j(this,"off",a,[b,d]))return this;if(!a&&!b&&!d)return this._events=void 0,this;for(h=a?[a]:c.keys(this._events),i=0,k=h.length;k>i;i++)if(a=h[i],g=this._events[a]){if(this._events[a]=e=[],b||d)for(l=0,m=g.length;m>l;l++)f=g[l],(b&&b!==f.callback&&b!==f.callback._callback||d&&d!==f.context)&&e.push(f);e.length||delete this._events[a]}return this},trigger:function(a){if(!this._events)return this;var b=g.call(arguments,1);if(!j(this,"trigger",a,b))return this;var c=this._events[a],d=this._events.all;return c&&k(c,b),d&&k(d,arguments),this},stopListening:function(a,b,d){var e=this._listeningTo;if(!e)return this;var f=!b&&!d;d||"object"!=typeof b||(d=this),a&&((e={})[a._listenId]=a);for(var g in e)a=e[g],a.off(b,d,this),(f||c.isEmpty(a._events))&&delete this._listeningTo[g];return this}},i=/\s+/,j=function(a,b,c,d){if(!c)return!0;if("object"==typeof c){for(var e in c)a[b].apply(a,[e,c[e]].concat(d));return!1}if(i.test(c)){for(var f=c.split(i),g=0,h=f.length;h>g;g++)a[b].apply(a,[f[g]].concat(d));return!1}return!0},k=function(a,b){var c,d=-1,e=a.length,f=b[0],g=b[1],h=b[2];switch(b.length){case 0:for(;++d<e;)(c=a[d]).callback.call(c.ctx);return;case 1:for(;++d<e;)(c=a[d]).callback.call(c.ctx,f);return;case 2:for(;++d<e;)(c=a[d]).callback.call(c.ctx,f,g);return;case 3:for(;++d<e;)(c=a[d]).callback.call(c.ctx,f,g,h);return;default:for(;++d<e;)(c=a[d]).callback.apply(c.ctx,b);return}},l={listenTo:"on",listenToOnce:"once"};c.each(l,function(a,b){h[b]=function(b,d,e){var f=this._listeningTo||(this._listeningTo={}),g=b._listenId||(b._listenId=c.uniqueId("l"));return f[g]=b,e||"object"!=typeof d||(e=this),b[a](d,e,this),this}}),h.bind=h.on,h.unbind=h.off,c.extend(b,h);var m=b.Model=function(a,b){var d=a||{};b||(b={}),this.cid=c.uniqueId("c"),this.attributes={},b.collection&&(this.collection=b.collection),b.parse&&(d=this.parse(d,b)||{}),d=c.defaults({},d,c.result(this,"defaults")),this.set(d,b),this.changed={},this.initialize.apply(this,arguments)};c.extend(m.prototype,h,{changed:null,validationError:null,idAttribute:"id",initialize:function(){},toJSON:function(a){return c.clone(this.attributes)},sync:function(){return b.sync.apply(this,arguments)},get:function(a){return this.attributes[a]},escape:function(a){return c.escape(this.get(a))},has:function(a){return null!=this.get(a)},set:function(a,b,d){var e,f,g,h,i,j,k,l;if(null==a)return this;if("object"==typeof a?(f=a,d=b):(f={})[a]=b,d||(d={}),!this._validate(f,d))return!1;g=d.unset,i=d.silent,h=[],j=this._changing,this._changing=!0,j||(this._previousAttributes=c.clone(this.attributes),this.changed={}),l=this.attributes,k=this._previousAttributes,this.idAttribute in f&&(this.id=f[this.idAttribute]);for(e in f)b=f[e],c.isEqual(l[e],b)||h.push(e),c.isEqual(k[e],b)?delete this.changed[e]:this.changed[e]=b,g?delete l[e]:l[e]=b;if(!i){h.length&&(this._pending=d);for(var m=0,n=h.length;n>m;m++)this.trigger("change:"+h[m],this,l[h[m]],d)}if(j)return this;if(!i)for(;this._pending;)d=this._pending,this._pending=!1,this.trigger("change",this,d);return this._pending=!1,this._changing=!1,this},unset:function(a,b){return this.set(a,void 0,c.extend({},b,{unset:!0}))},clear:function(a){var b={};for(var d in this.attributes)b[d]=void 0;return this.set(b,c.extend({},a,{unset:!0}))},hasChanged:function(a){return null==a?!c.isEmpty(this.changed):c.has(this.changed,a)},changedAttributes:function(a){if(!a)return this.hasChanged()?c.clone(this.changed):!1;var b,d=!1,e=this._changing?this._previousAttributes:this.attributes;for(var f in a)c.isEqual(e[f],b=a[f])||((d||(d={}))[f]=b);return d},previous:function(a){return null!=a&&this._previousAttributes?this._previousAttributes[a]:null},previousAttributes:function(){return c.clone(this._previousAttributes)},fetch:function(a){a=a?c.clone(a):{},void 0===a.parse&&(a.parse=!0);var b=this,d=a.success;return a.success=function(c){return b.set(b.parse(c,a),a)?(d&&d(b,c,a),void b.trigger("sync",b,c,a)):!1},L(this,a),this.sync("read",this,a)},save:function(a,b,d){var e,f,g,h=this.attributes;if(null==a||"object"==typeof a?(e=a,d=b):(e={})[a]=b,d=c.extend({validate:!0},d),e&&!d.wait){if(!this.set(e,d))return!1}else if(!this._validate(e,d))return!1;e&&d.wait&&(this.attributes=c.extend({},h,e)),void 0===d.parse&&(d.parse=!0);var i=this,j=d.success;return d.success=function(a){i.attributes=h;var b=i.parse(a,d);return d.wait&&(b=c.extend(e||{},b)),c.isObject(b)&&!i.set(b,d)?!1:(j&&j(i,a,d),void i.trigger("sync",i,a,d))},L(this,d),f=this.isNew()?"create":d.patch?"patch":"update","patch"===f&&(d.attrs=e),g=this.sync(f,this,d),e&&d.wait&&(this.attributes=h),g},destroy:function(a){a=a?c.clone(a):{};var b=this,d=a.success,e=function(){b.trigger("destroy",b,b.collection,a)};if(a.success=function(c){(a.wait||b.isNew())&&e(),d&&d(b,c,a),b.isNew()||b.trigger("sync",b,c,a)},this.isNew())return a.success(),!1;L(this,a);var f=this.sync("delete",this,a);return a.wait||e(),f},url:function(){var a=c.result(this,"urlRoot")||c.result(this.collection,"url")||K();return this.isNew()?a:a.replace(/([^\/])$/,"$1/")+encodeURIComponent(this.id)},parse:function(a,b){return a},clone:function(){return new this.constructor(this.attributes)},isNew:function(){return!this.has(this.idAttribute)},isValid:function(a){return this._validate({},c.extend(a||{},{validate:!0}))},_validate:function(a,b){if(!b.validate||!this.validate)return!0;a=c.extend({},this.attributes,a);var d=this.validationError=this.validate(a,b)||null;return d?(this.trigger("invalid",this,d,c.extend(b,{validationError:d})),!1):!0}});var n=["keys","values","pairs","invert","pick","omit"];c.each(n,function(a){m.prototype[a]=function(){var b=g.call(arguments);return b.unshift(this.attributes),c[a].apply(c,b)}});var o=b.Collection=function(a,b){b||(b={}),b.model&&(this.model=b.model),void 0!==b.comparator&&(this.comparator=b.comparator),this._reset(),this.initialize.apply(this,arguments),a&&this.reset(a,c.extend({silent:!0},b))},p={add:!0,remove:!0,merge:!0},q={add:!0,remove:!1};c.extend(o.prototype,h,{model:m,initialize:function(){},toJSON:function(a){return this.map(function(b){return b.toJSON(a)})},sync:function(){return b.sync.apply(this,arguments)},add:function(a,b){return this.set(a,c.extend({merge:!1},b,q))},remove:function(a,b){var d=!c.isArray(a);a=d?[a]:c.clone(a),b||(b={});var e,f,g,h;for(e=0,f=a.length;f>e;e++)h=a[e]=this.get(a[e]),h&&(delete this._byId[h.id],delete this._byId[h.cid],g=this.indexOf(h),this.models.splice(g,1),this.length--,b.silent||(b.index=g,h.trigger("remove",h,this,b)),this._removeReference(h,b));return d?a[0]:a},set:function(a,b){b=c.defaults({},b,p),b.parse&&(a=this.parse(a,b));var d=!c.isArray(a);a=d?a?[a]:[]:c.clone(a);var e,f,g,h,i,j,k,l=b.at,n=this.model,o=this.comparator&&null==l&&b.sort!==!1,q=c.isString(this.comparator)?this.comparator:null,r=[],s=[],t={},u=b.add,v=b.merge,w=b.remove,x=!o&&u&&w?[]:!1;for(e=0,f=a.length;f>e;e++){if(i=a[e]||{},g=i instanceof m?h=i:i[n.prototype.idAttribute||"id"],j=this.get(g))w&&(t[j.cid]=!0),v&&(i=i===h?h.attributes:i,b.parse&&(i=j.parse(i,b)),j.set(i,b),o&&!k&&j.hasChanged(q)&&(k=!0)),a[e]=j;else if(u){if(h=a[e]=this._prepareModel(i,b),!h)continue;r.push(h),this._addReference(h,b)}h=j||h,!x||!h.isNew()&&t[h.id]||x.push(h),t[h.id]=!0}if(w){for(e=0,f=this.length;f>e;++e)t[(h=this.models[e]).cid]||s.push(h);s.length&&this.remove(s,b)}if(r.length||x&&x.length)if(o&&(k=!0),this.length+=r.length,null!=l)for(e=0,f=r.length;f>e;e++)this.models.splice(l+e,0,r[e]);else{x&&(this.models.length=0);var y=x||r;for(e=0,f=y.length;f>e;e++)this.models.push(y[e])}if(k&&this.sort({silent:!0}),!b.silent){for(e=0,f=r.length;f>e;e++)(h=r[e]).trigger("add",h,this,b);(k||x&&x.length)&&this.trigger("sort",this,b)}return d?a[0]:a},reset:function(a,b){b||(b={});for(var d=0,e=this.models.length;e>d;d++)this._removeReference(this.models[d],b);return b.previousModels=this.models,this._reset(),a=this.add(a,c.extend({silent:!0},b)),b.silent||this.trigger("reset",this,b),a},push:function(a,b){return this.add(a,c.extend({at:this.length},b))},pop:function(a){var b=this.at(this.length-1);return this.remove(b,a),b},unshift:function(a,b){return this.add(a,c.extend({at:0},b))},shift:function(a){var b=this.at(0);return this.remove(b,a),b},slice:function(){return g.apply(this.models,arguments)},get:function(a){return null==a?void 0:this._byId[a]||this._byId[a.id]||this._byId[a.cid]},at:function(a){return this.models[a]},where:function(a,b){return c.isEmpty(a)?b?void 0:[]:this[b?"find":"filter"](function(b){for(var c in a)if(a[c]!==b.get(c))return!1;return!0})},findWhere:function(a){return this.where(a,!0)},sort:function(a){if(!this.comparator)throw new Error("Cannot sort a set without a comparator");return a||(a={}),c.isString(this.comparator)||1===this.comparator.length?this.models=this.sortBy(this.comparator,this):this.models.sort(c.bind(this.comparator,this)),a.silent||this.trigger("sort",this,a),this},pluck:function(a){return c.invoke(this.models,"get",a)},fetch:function(a){a=a?c.clone(a):{},void 0===a.parse&&(a.parse=!0);var b=a.success,d=this;return a.success=function(c){var e=a.reset?"reset":"set";d[e](c,a),b&&b(d,c,a),d.trigger("sync",d,c,a)},L(this,a),this.sync("read",this,a)},create:function(a,b){if(b=b?c.clone(b):{},!(a=this._prepareModel(a,b)))return!1;b.wait||this.add(a,b);var d=this,e=b.success;return b.success=function(a,c){b.wait&&d.add(a,b),e&&e(a,c,b)},a.save(null,b),a},parse:function(a,b){return a},clone:function(){return new this.constructor(this.models)},_reset:function(){this.length=0,this.models=[],this._byId={}},_prepareModel:function(a,b){if(a instanceof m)return a;b=b?c.clone(b):{},b.collection=this;var d=new this.model(a,b);return d.validationError?(this.trigger("invalid",this,d.validationError,b),!1):d},_addReference:function(a,b){this._byId[a.cid]=a,null!=a.id&&(this._byId[a.id]=a),a.collection||(a.collection=this),a.on("all",this._onModelEvent,this)},_removeReference:function(a,b){this===a.collection&&delete a.collection,a.off("all",this._onModelEvent,this)},_onModelEvent:function(a,b,c,d){("add"!==a&&"remove"!==a||c===this)&&("destroy"===a&&this.remove(b,d),b&&a==="change:"+b.idAttribute&&(delete this._byId[b.previous(b.idAttribute)],null!=b.id&&(this._byId[b.id]=b)),this.trigger.apply(this,arguments))}});var r=["forEach","each","map","collect","reduce","foldl","inject","reduceRight","foldr","find","detect","filter","select","reject","every","all","some","any","include","contains","invoke","max","min","toArray","size","first","head","take","initial","rest","tail","drop","last","without","difference","indexOf","shuffle","lastIndexOf","isEmpty","chain","sample"];c.each(r,function(a){o.prototype[a]=function(){var b=g.call(arguments);return b.unshift(this.models),c[a].apply(c,b)}});var s=["groupBy","countBy","sortBy","indexBy"];c.each(s,function(a){o.prototype[a]=function(b,d){var e=c.isFunction(b)?b:function(a){return a.get(b)};return c[a](this.models,e,d)}});var t=b.View=function(a){this.cid=c.uniqueId("view"),a||(a={}),c.extend(this,c.pick(a,v)),this._ensureElement(),this.initialize.apply(this,arguments),this.delegateEvents()},u=/^(\S+)\s*(.*)$/,v=["model","collection","el","id","attributes","className","tagName","events"];c.extend(t.prototype,h,{tagName:"div",$:function(a){return this.$el.find(a)},initialize:function(){},render:function(){return this},remove:function(){return this.$el.remove(),this.stopListening(),this},setElement:function(a,c){return this.$el&&this.undelegateEvents(),this.$el=a instanceof b.$?a:b.$(a),this.el=this.$el[0],c!==!1&&this.delegateEvents(),this},delegateEvents:function(a){if(!a&&!(a=c.result(this,"events")))return this;this.undelegateEvents();for(var b in a){var d=a[b];if(c.isFunction(d)||(d=this[a[b]]),d){var e=b.match(u),f=e[1],g=e[2];d=c.bind(d,this),f+=".delegateEvents"+this.cid,""===g?this.$el.on(f,d):this.$el.on(f,g,d)}}return this},undelegateEvents:function(){return this.$el.off(".delegateEvents"+this.cid),this},_ensureElement:function(){if(this.el)this.setElement(c.result(this,"el"),!1);else{var a=c.extend({},c.result(this,"attributes"));this.id&&(a.id=c.result(this,"id")),this.className&&(a["class"]=c.result(this,"className"));var d=b.$("<"+c.result(this,"tagName")+">").attr(a);this.setElement(d,!1)}}}),b.sync=function(a,d,e){var f=x[a];c.defaults(e||(e={}),{emulateHTTP:b.emulateHTTP,emulateJSON:b.emulateJSON});var g={type:f,dataType:"json"};if(e.url||(g.url=c.result(d,"url")||K()),null!=e.data||!d||"create"!==a&&"update"!==a&&"patch"!==a||(g.contentType="application/json",g.data=JSON.stringify(e.attrs||d.toJSON(e))),e.emulateJSON&&(g.contentType="application/x-www-form-urlencoded",g.data=g.data?{model:g.data}:{}),e.emulateHTTP&&("PUT"===f||"DELETE"===f||"PATCH"===f)){g.type="POST",e.emulateJSON&&(g.data._method=f);var h=e.beforeSend;e.beforeSend=function(a){return a.setRequestHeader("X-HTTP-Method-Override",f),h?h.apply(this,arguments):void 0}}"GET"===g.type||e.emulateJSON||(g.processData=!1),"PATCH"===g.type&&w&&(g.xhr=function(){return new ActiveXObject("Microsoft.XMLHTTP")});var i=e.xhr=b.ajax(c.extend(g,e));return d.trigger("request",d,i,e),i};var w=!("undefined"==typeof window||!window.ActiveXObject||window.XMLHttpRequest&&(new XMLHttpRequest).dispatchEvent),x={create:"POST",update:"PUT",patch:"PATCH","delete":"DELETE",read:"GET"};b.ajax=function(){return b.$.ajax.apply(b.$,arguments)};var y=b.Router=function(a){a||(a={}),a.routes&&(this.routes=a.routes),this._bindRoutes(),this.initialize.apply(this,arguments)},z=/\((.*?)\)/g,A=/(\(\?)?:\w+/g,B=/\*\w+/g,C=/[\-{}\[\]+?.,\\\^$|#\s]/g;c.extend(y.prototype,h,{initialize:function(){},route:function(a,d,e){c.isRegExp(a)||(a=this._routeToRegExp(a)),c.isFunction(d)&&(e=d,d=""),e||(e=this[d]);var f=this;return b.history.route(a,function(c){var g=f._extractParameters(a,c);f.execute(e,g),f.trigger.apply(f,["route:"+d].concat(g)),f.trigger("route",d,g),b.history.trigger("route",f,d,g)}),this},execute:function(a,b){a&&a.apply(this,b)},navigate:function(a,c){return b.history.navigate(a,c),this},_bindRoutes:function(){if(this.routes){this.routes=c.result(this,"routes");for(var a,b=c.keys(this.routes);null!=(a=b.pop());)this.route(a,this.routes[a])}},_routeToRegExp:function(a){return a=a.replace(C,"\\$&").replace(z,"(?:$1)?").replace(A,function(a,b){return b?a:"([^/?]+)"}).replace(B,"([^?]*?)"),new RegExp("^"+a+"(?:\\?([\\s\\S]*))?$")},_extractParameters:function(a,b){var d=a.exec(b).slice(1);return c.map(d,function(a,b){return b===d.length-1?a||null:a?decodeURIComponent(a):null})}});var D=b.History=function(){this.handlers=[],c.bindAll(this,"checkUrl"),"undefined"!=typeof window&&(this.location=window.location,this.history=window.history)},E=/^[#\/]|\s+$/g,F=/^\/+|\/+$/g,G=/msie [\w.]+/,H=/\/$/,I=/#.*$/;D.started=!1,c.extend(D.prototype,h,{interval:50,atRoot:function(){return this.location.pathname.replace(/[^\/]$/,"$&/")===this.root},getHash:function(a){var b=(a||this).location.href.match(/#(.*)$/);return b?b[1]:""},getFragment:function(a,b){if(null==a)if(this._hasPushState||!this._wantsHashChange||b){a=decodeURI(this.location.pathname+this.location.search);var c=this.root.replace(H,"");a.indexOf(c)||(a=a.slice(c.length))}else a=this.getHash();return a.replace(E,"")},start:function(a){if(D.started)throw new Error("Backbone.history has already been started");D.started=!0,this.options=c.extend({root:"/"},this.options,a),this.root=this.options.root,this._wantsHashChange=this.options.hashChange!==!1,this._wantsPushState=!!this.options.pushState,this._hasPushState=!!(this.options.pushState&&this.history&&this.history.pushState);var d=this.getFragment(),e=document.documentMode,f=G.exec(navigator.userAgent.toLowerCase())&&(!e||7>=e);if(this.root=("/"+this.root+"/").replace(F,"/"),f&&this._wantsHashChange){var g=b.$('<iframe src="javascript:0" tabindex="-1">');this.iframe=g.hide().appendTo("body")[0].contentWindow,this.navigate(d);
}this._hasPushState?b.$(window).on("popstate",this.checkUrl):this._wantsHashChange&&"onhashchange"in window&&!f?b.$(window).on("hashchange",this.checkUrl):this._wantsHashChange&&(this._checkUrlInterval=setInterval(this.checkUrl,this.interval)),this.fragment=d;var h=this.location;if(this._wantsHashChange&&this._wantsPushState){if(!this._hasPushState&&!this.atRoot())return this.fragment=this.getFragment(null,!0),this.location.replace(this.root+"#"+this.fragment),!0;this._hasPushState&&this.atRoot()&&h.hash&&(this.fragment=this.getHash().replace(E,""),this.history.replaceState({},document.title,this.root+this.fragment))}return this.options.silent?void 0:this.loadUrl()},stop:function(){b.$(window).off("popstate",this.checkUrl).off("hashchange",this.checkUrl),this._checkUrlInterval&&clearInterval(this._checkUrlInterval),D.started=!1},route:function(a,b){this.handlers.unshift({route:a,callback:b})},checkUrl:function(a){var b=this.getFragment();return b===this.fragment&&this.iframe&&(b=this.getFragment(this.getHash(this.iframe))),b===this.fragment?!1:(this.iframe&&this.navigate(b),void this.loadUrl())},loadUrl:function(a){return a=this.fragment=this.getFragment(a),c.any(this.handlers,function(b){return b.route.test(a)?(b.callback(a),!0):void 0})},navigate:function(a,b){if(!D.started)return!1;b&&b!==!0||(b={trigger:!!b});var c=this.root+(a=this.getFragment(a||""));if(a=a.replace(I,""),this.fragment!==a){if(this.fragment=a,""===a&&"/"!==c&&(c=c.slice(0,-1)),this._hasPushState)this.history[b.replace?"replaceState":"pushState"]({},document.title,c);else{if(!this._wantsHashChange)return this.location.assign(c);this._updateHash(this.location,a,b.replace),this.iframe&&a!==this.getFragment(this.getHash(this.iframe))&&(b.replace||this.iframe.document.open().close(),this._updateHash(this.iframe.location,a,b.replace))}return b.trigger?this.loadUrl(a):void 0}},_updateHash:function(a,b,c){if(c){var d=a.href.replace(/(javascript:|#).*$/,"");a.replace(d+"#"+b)}else a.hash="#"+b}}),b.history=new D;var J=function(a,b){var d,e=this;d=a&&c.has(a,"constructor")?a.constructor:function(){return e.apply(this,arguments)},c.extend(d,e,b);var f=function(){this.constructor=d};return f.prototype=e.prototype,d.prototype=new f,a&&c.extend(d.prototype,a),d.__super__=e.prototype,d};m.extend=o.extend=y.extend=t.extend=D.extend=J;var K=function(){throw new Error('A "url" property or function must be specified')},L=function(a,b){var c=b.error;b.error=function(d){c&&c(a,d,b),a.trigger("error",a,d,b)}};return b}),"object"!=typeof JSON&&(JSON={}),function(){"use strict";function f(a){return 10>a?"0"+a:a}function quote(a){return escapable.lastIndex=0,escapable.test(a)?'"'+a.replace(escapable,function(a){var b=meta[a];return"string"==typeof b?b:"\\u"+("0000"+a.charCodeAt(0).toString(16)).slice(-4)})+'"':'"'+a+'"'}function str(a,b){var c,d,e,f,g,h=gap,i=b[a];switch(i&&"object"==typeof i&&"function"==typeof i.toJSON&&(i=i.toJSON(a)),"function"==typeof rep&&(i=rep.call(b,a,i)),typeof i){case"string":return quote(i);case"number":return isFinite(i)?String(i):"null";case"boolean":case"null":return String(i);case"object":if(!i)return"null";if(gap+=indent,g=[],"[object Array]"===Object.prototype.toString.apply(i)){for(f=i.length,c=0;f>c;c+=1)g[c]=str(c,i)||"null";return e=0===g.length?"[]":gap?"[\n"+gap+g.join(",\n"+gap)+"\n"+h+"]":"["+g.join(",")+"]",gap=h,e}if(rep&&"object"==typeof rep)for(f=rep.length,c=0;f>c;c+=1)"string"==typeof rep[c]&&(d=rep[c],e=str(d,i),e&&g.push(quote(d)+(gap?": ":":")+e));else for(d in i)Object.prototype.hasOwnProperty.call(i,d)&&(e=str(d,i),e&&g.push(quote(d)+(gap?": ":":")+e));return e=0===g.length?"{}":gap?"{\n"+gap+g.join(",\n"+gap)+"\n"+h+"}":"{"+g.join(",")+"}",gap=h,e}}"function"!=typeof Date.prototype.toJSON&&(Date.prototype.toJSON=function(){return isFinite(this.valueOf())?this.getUTCFullYear()+"-"+f(this.getUTCMonth()+1)+"-"+f(this.getUTCDate())+"T"+f(this.getUTCHours())+":"+f(this.getUTCMinutes())+":"+f(this.getUTCSeconds())+"Z":null},String.prototype.toJSON=Number.prototype.toJSON=Boolean.prototype.toJSON=function(){return this.valueOf()});var cx,escapable,gap,indent,meta,rep;"function"!=typeof JSON.stringify&&(escapable=/[\\\"\x00-\x1f\x7f-\x9f\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g,meta={"\b":"\\b","	":"\\t","\n":"\\n","\f":"\\f","\r":"\\r",'"':'\\"',"\\":"\\\\"},JSON.stringify=function(a,b,c){var d;if(gap="",indent="","number"==typeof c)for(d=0;c>d;d+=1)indent+=" ";else"string"==typeof c&&(indent=c);if(rep=b,b&&"function"!=typeof b&&("object"!=typeof b||"number"!=typeof b.length))throw new Error("JSON.stringify");return str("",{"":a})}),"function"!=typeof JSON.parse&&(cx=/[\u0000\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g,JSON.parse=function(text,reviver){function walk(a,b){var c,d,e=a[b];if(e&&"object"==typeof e)for(c in e)Object.prototype.hasOwnProperty.call(e,c)&&(d=walk(e,c),void 0!==d?e[c]=d:delete e[c]);return reviver.call(a,b,e)}var j;if(text=String(text),cx.lastIndex=0,cx.test(text)&&(text=text.replace(cx,function(a){return"\\u"+("0000"+a.charCodeAt(0).toString(16)).slice(-4)})),/^[\],:{}\s]*$/.test(text.replace(/\\(?:["\\\/bfnrt]|u[0-9a-fA-F]{4})/g,"@").replace(/"[^"\\\n\r]*"|true|false|null|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?/g,"]").replace(/(?:^|:|,)(?:\s*\[)+/g,"")))return j=eval("("+text+")"),"function"==typeof reviver?walk({"":j},""):j;throw new SyntaxError("JSON.parse")})}(),function(a){"function"==typeof require&&"undefined"!=typeof module&&module.exports?module.exports=a(require("underscore")):"function"==typeof define?define(["underscore"],a):this.Cocktail=a(_)}(function(a){var b={};b.mixins={},b.mixin=function(c){var d=a.chain(arguments).toArray().rest().flatten().value(),e=c.prototype||c,f={};return a.each(d,function(c){a.isString(c)&&(c=b.mixins[c]),a.each(c,function(b,c){if(a.isFunction(b)){if(e[c]===b)return;e[c]&&(f[c]=f.hasOwnProperty(c)?f[c]:[e[c]],f[c].push(b)),e[c]=b}else a.isArray(b)?e[c]=a.union(b,e[c]||[]):a.isObject(b)?e[c]=a.extend({},b,e[c]||{}):c in e||(e[c]=b)})}),a.each(f,function(b,c){e[c]=function(){var c,d=this,e=arguments;return a.each(b,function(b){var f=a.isFunction(b)?b.apply(d,e):b;c="undefined"==typeof f?c:f}),c}}),c};var c;return b.patch=function(d){c=d.Model.extend;var e=function(a,d){var e=c.call(this,a,d),f=e.prototype.mixins;return f&&e.prototype.hasOwnProperty("mixins")&&b.mixin(e,f),e};a.each([d.Model,d.Collection,d.Router,d.View],function(c){c.mixin=function(){b.mixin(this,a.toArray(arguments))},c.extend=e})},b.unpatch=function(b){a.each([b.Model,b.Collection,b.Router,b.View],function(a){a.mixin=void 0,a.extend=c})},b}),function(a,b){if("object"==typeof exports)module.exports=b(require("underscore"),require("backbone"));else if("function"==typeof define&&define.amd)define(["underscore","backbone"],b);else{for(var c="FilteredCollection",d=c.split("."),e=a,f=0;f<d.length-1;f++)void 0===e[d[f]]&&(e[d[f]]={}),e=e[d[f]];e[d[d.length-1]]=b(a._,a.Backbone)}}(this,function(a,b){function c(c){return{underscore:a,backbone:b}[c]}var d=function(a){function b(a){var c=b.cache[a];if(!c){var d={};c=b.cache[a]={id:a,exports:d},b.modules[a].call(d,c,d)}return c.exports}return b.cache=[],b.modules=[function(a,d){function e(){this._filterResultCache={}}function f(a){for(var b in this._filterResultCache)this._filterResultCache.hasOwnProperty(b)&&delete this._filterResultCache[b][a]}function g(a,b){this._filters[a]&&f.call(this,a),this._filters[a]=b,this.trigger("filtered:add",a)}function h(a){delete this._filters[a],f.call(this,a),this.trigger("filtered:remove",a)}function i(a){this._filterResultCache[a.cid]||(this._filterResultCache[a.cid]={});var b=this._filterResultCache[a.cid];for(var c in this._filters)if(this._filters.hasOwnProperty(c)&&(b.hasOwnProperty(c)||(b[c]=this._filters[c].fn(a)),!b[c]))return!1;return!0}function j(){var a=[];this._superset&&(a=this._superset.filter(p.bind(i,this))),this._collection.reset(a),this.length=this._collection.length}function k(a){if(this._filterResultCache[a.cid]={},i.call(this,a)){if(!this._collection.get(a.cid)){for(var b=this.superset().indexOf(a),c=null,d=b-1;d>=0;d-=1)if(this.contains(this.superset().at(d))){c=this.indexOf(this.superset().at(d))+1;break}c=c||0,this._collection.add(a,{at:c})}}else this._collection.get(a.cid)&&this._collection.remove(a);this.length=this._collection.length}function l(a){this._filterResultCache[a.cid]={},i.call(this,a)||this._collection.get(a.cid)&&this._collection.remove(a)}function m(a,b,c){"change:"===a.slice(0,7)&&l.call(this,arguments[1])}function n(a){this.contains(a)&&this._collection.remove(a),this.length=this._collection.length}function o(a){this._superset=a,this._collection=new q.Collection(a.toArray()),r(this._collection,this),this.resetFilters(),this.listenTo(this._superset,"reset sort",j),this.listenTo(this._superset,"add change",k),this.listenTo(this._superset,"remove",n),this.listenTo(this._superset,"all",m)}var p=c("underscore"),q=c("backbone"),r=b(1),s=b(2),t={defaultFilterName:"__default",filterBy:function(a,b){return b||(b=a,a=this.defaultFilterName),g.call(this,a,s(b)),j.call(this),this},removeFilter:function(a){return a||(a=this.defaultFilterName),h.call(this,a),j.call(this),this},resetFilters:function(){return this._filters={},e.call(this),this.trigger("filtered:reset"),j.call(this),this},superset:function(){return this._superset},refilter:function(a){return"object"==typeof a&&a.cid?k.call(this,a):(e.call(this),j.call(this)),this},getFilters:function(){return p.keys(this._filters)},hasFilter:function(a){return p.contains(this.getFilters(),a)},destroy:function(){this.stopListening(),this._collection.reset([]),this._superset=this._collection,this.length=0,this.trigger("filtered:destroy")}};p.extend(o.prototype,t,q.Events),a.exports=o},function(a,b){function d(a,b){function c(){b.length=a.length}function d(c){var d=e.toArray(arguments),f="change"===c||"change:"===c.slice(0,7);"reset"===c&&(b.models=a.models),e.contains(h,c)?(e.contains(["add","remove","destroy"],c)?d[2]=b:e.contains(["reset","sort"],c)&&(d[1]=b),b.trigger.apply(this,d)):f&&b.contains(d[1])&&b.trigger.apply(this,d)}var i={};return e.each(e.functions(f.Collection.prototype),function(b){e.contains(g,b)||(i[b]=function(){return a[b].apply(a,arguments)})}),e.extend(b,f.Events,i),b.listenTo(a,"all",c),b.listenTo(a,"all",d),b.models=a.models,c(),b}var e=c("underscore"),f=c("backbone"),g=["_onModelEvent","_prepareModel","_removeReference","_reset","add","initialize","sync","remove","reset","set","push","pop","unshift","shift","sort","parse","fetch","create","model","off","on","listenTo","listenToOnce","bind","trigger","once","stopListening"],h=["add","remove","reset","sort","destroy","sync","request","error"];a.exports=d},function(a,b){function d(a,b){return function(c){return c.get(a)===b}}function e(a,b){return function(c){return b(c.get(a))}}function f(a,b){return i.isArray(b)||(b=null),{fn:a,keys:b}}function g(a){var b=i.keys(a),c=i.map(b,function(b){var c=a[b];return i.isFunction(c)?e(b,c):d(b,c)}),g=function(a){for(var b=0;b<c.length;b++)if(!c[b](a))return!1;return!0};return f(g,b)}function h(a,b){return i.isFunction(a)?f(a,b):i.isObject(a)?g(a):void 0}var i=c("underscore");a.exports=h}],b(0)}();return d}),function(a,b){if("function"==typeof define&&define.amd)define(["backbone","underscore"],function(a,c){return b(a,c)});else if("undefined"!=typeof exports){var c=require("backbone"),d=require("underscore");module.exports=b(c,d)}else b(a.Backbone,a._)}(this,function(a,b){"use strict";var c=a.ChildViewContainer;return a.ChildViewContainer=function(a,b){var c=function(a){this._views={},this._indexByModel={},this._indexByCustom={},this._updateLength(),b.each(a,this.add,this)};b.extend(c.prototype,{add:function(a,b){var c=a.cid;return this._views[c]=a,a.model&&(this._indexByModel[a.model.cid]=c),b&&(this._indexByCustom[b]=c),this._updateLength(),this},findByModel:function(a){return this.findByModelCid(a.cid)},findByModelCid:function(a){var b=this._indexByModel[a];return this.findByCid(b)},findByCustom:function(a){var b=this._indexByCustom[a];return this.findByCid(b)},findByIndex:function(a){return b.values(this._views)[a]},findByCid:function(a){return this._views[a]},remove:function(a){var c=a.cid;return a.model&&delete this._indexByModel[a.model.cid],b.any(this._indexByCustom,function(a,b){return a===c?(delete this._indexByCustom[b],!0):void 0},this),delete this._views[c],this._updateLength(),this},call:function(a){this.apply(a,b.tail(arguments))},apply:function(a,c){b.each(this._views,function(d){b.isFunction(d[a])&&d[a].apply(d,c||[])})},_updateLength:function(){this.length=b.size(this._views)}});var d=["forEach","each","map","find","detect","filter","select","reject","every","all","some","any","include","contains","invoke","toArray","first","initial","rest","last","without","isEmpty","pluck","reduce"];return b.each(d,function(a){c.prototype[a]=function(){var c=b.values(this._views),d=[c].concat(b.toArray(arguments));return b[a].apply(b,d)}}),c}(a,b),a.ChildViewContainer.VERSION="0.1.6",a.ChildViewContainer.noConflict=function(){return a.ChildViewContainer=c,this},a.ChildViewContainer}),function(a,b){"function"==typeof define&&define.amd?define(["underscore","backbone","jquery"],function(c,d,e){return a.Backbone=b(c,d,e)}):"undefined"!=typeof exports&&"undefined"!=typeof require?module.exports=b(require("underscore"),require("backbone"),require("jquery")):a.Backbone=b(a._,a.Backbone,a.jQuery)}(this,function(a,b,c){function d(b,d){if(b&&a.isObject(b)){if(a.isFunction(b.getCacheKey))return b.getCacheKey(d);b=d&&d.url?d.url:a.isFunction(b.url)?b.url():b.url}else if(a.isFunction(b))return b(d);return d&&d.data?"string"==typeof d.data?b+"?"+d.data:b+"?"+c.param(d.data):b}function e(a,c,d){c=c||{};var e=b.fetchCache.getCacheKey(a,c),f=!1,g=c.lastSync||(new Date).getTime(),h=!1;e&&c.cache!==!1&&(c.cache||c.prefill)&&(c.expires!==!1&&(f=(new Date).getTime()+1e3*(c.expires||300)),c.prefillExpires!==!1&&(h=(new Date).getTime()+1e3*(c.prefillExpires||300)),b.fetchCache._cache[e]={expires:f,lastSync:g,prefillExpires:h,value:d},b.fetchCache.setLocalStorage())}function f(c,e){return a.isFunction(c)?c=c():c&&a.isObject(c)&&(c=d(c,e)),b.fetchCache._cache[c]}function g(a,b){return f(a).lastSync}function h(c,e){a.isFunction(c)?c=c():c&&a.isObject(c)&&(c=d(c,e)),delete b.fetchCache._cache[c],b.fetchCache.setLocalStorage()}function i(){if(m&&b.fetchCache.localStorage)try{localStorage.setItem(b.fetchCache.getLocalStorageKey(),JSON.stringify(b.fetchCache._cache))}catch(a){var c=a.code||a.number||a.message;if(22!==c&&1014!==c)throw a;this._deleteCacheWithPriority()}}function j(){if(m&&b.fetchCache.localStorage){var a=localStorage.getItem(b.fetchCache.getLocalStorageKey())||"{}";b.fetchCache._cache=JSON.parse(a)}}function k(a){return window.setTimeout(a,0)}var l={modelFetch:b.Model.prototype.fetch,modelSync:b.Model.prototype.sync,collectionFetch:b.Collection.prototype.fetch},m=function(){var a="undefined"!=typeof window.localStorage;if(a)try{localStorage.setItem("test_support","test_support"),localStorage.removeItem("test_support")}catch(b){a=!1}return a}();return b.fetchCache=b.fetchCache||{},b.fetchCache._cache=b.fetchCache._cache||{},b.fetchCache.enabled=!0,b.fetchCache.priorityFn=function(a,b){return a&&a.expires&&b&&b.expires?a.expires-b.expires:a},b.fetchCache._prioritize=function(){var b=a.values(this._cache).sort(this.priorityFn),c=a.indexOf(a.values(this._cache),b[0]);return a.keys(this._cache)[c]},b.fetchCache._deleteCacheWithPriority=function(){b.fetchCache._cache[this._prioritize()]=null,delete b.fetchCache._cache[this._prioritize()],b.fetchCache.setLocalStorage()},b.fetchCache.getLocalStorageKey=function(){return"backboneCache"},"undefined"==typeof b.fetchCache.localStorage&&(b.fetchCache.localStorage=!0),b.Model.prototype.fetch=function(d){function e(){return d.prefill&&(!d.prefillExpires||m)}function g(){d.parse&&(n=p.parse(n,d)),p.set(n,d),a.isFunction(d.prefillSuccess)&&d.prefillSuccess(p,n,d),p.trigger("cachesync",p,n,d),p.trigger("sync",p,n,d),e()?o.notify(p):(a.isFunction(d.success)&&d.success(p,n,d),o.resolve(p))}if(!b.fetchCache.enabled)return l.modelFetch.apply(this,arguments);d=a.defaults(d||{},{parse:!0});var h=b.fetchCache.getCacheKey(this,d),i=f(h),j=!1,m=!1,n=!1,o=new c.Deferred,p=this;if(i&&(j=i.expires,j=j&&i.expires<(new Date).getTime(),m=i.prefillExpires,m=m&&i.prefillExpires<(new Date).getTime(),n=i.value),!j&&(d.cache||d.prefill)&&n&&(null==d.async&&(d.async=!0),d.async?k(g):g(),!e()))return o;var q=l.modelFetch.apply(this,arguments);return q.done(a.bind(o.resolve,this,this)).done(a.bind(b.fetchCache.setCache,null,this,d)).fail(a.bind(o.reject,this,this)),o.abort=q.abort,o},b.Model.prototype.sync=function(a,c,d){if("read"===a||!b.fetchCache.enabled)return l.modelSync.apply(this,arguments);var e,f,g=c.collection,i=[];for(i.push(b.fetchCache.getCacheKey(c,d)),g&&i.push(b.fetchCache.getCacheKey(g)),e=0,f=i.length;f>e;e++)h(i[e]);return l.modelSync.apply(this,arguments)},b.Collection.prototype.fetch=function(d){function e(){return d.prefill&&(!d.prefillExpires||m)}function g(){p[d.reset?"reset":"set"](n,d),a.isFunction(d.prefillSuccess)&&d.prefillSuccess(p),p.trigger("cachesync",p,n,d),p.trigger("sync",p,n,d),e()?o.notify(p):(a.isFunction(d.success)&&d.success(p,n,d),o.resolve(p))}if(!b.fetchCache.enabled)return l.collectionFetch.apply(this,arguments);d=a.defaults(d||{},{parse:!0});var h=b.fetchCache.getCacheKey(this,d),i=f(h),j=!1,m=!1,n=!1,o=new c.Deferred,p=this;if(i&&(j=i.expires,j=j&&i.expires<(new Date).getTime(),m=i.prefillExpires,m=m&&i.prefillExpires<(new Date).getTime(),n=i.value),!j&&(d.cache||d.prefill)&&n&&(null==d.async&&(d.async=!0),d.async?k(g):g(),!e()))return o;var q=l.collectionFetch.apply(this,arguments);return q.done(a.bind(o.resolve,this,this)).done(a.bind(b.fetchCache.setCache,null,this,d)).fail(a.bind(o.reject,this,this)),o.abort=q.abort,o},j(),b.fetchCache._superMethods=l,b.fetchCache.setCache=e,b.fetchCache.getCache=f,b.fetchCache.getCacheKey=d,b.fetchCache.getLastSync=g,b.fetchCache.clearItem=h,b.fetchCache.setLocalStorage=i,b.fetchCache.getLocalStorage=j,b}),function(a,b){"object"==typeof exports&&"function"==typeof require?module.exports=b(require("backbone")):"function"==typeof define&&define.amd?define(["backbone"],function(c){return b(c||a.Backbone)}):b(Backbone)}(this,function(a){function b(){return(65536*(1+Math.random())|0).toString(16).substring(1)}function c(){return b()+b()+"-"+b()+"-"+b()+"-"+b()+"-"+b()+b()+b()}function d(a){return a===Object(a)}function e(a,b){for(var c=a.length;c--;)if(a[c]===b)return!0;return!1}function f(a,b){for(var c in b)a[c]=b[c];return a}function g(a,b){if(null==a)return void 0;var c=a[b];return"function"==typeof c?a[b]():c}return a.LocalStorage=window.Store=function(a,b){if(!this.localStorage)throw"Backbone.localStorage: Environment does not support localStorage.";this.name=a,this.serializer=b||{serialize:function(a){return d(a)?JSON.stringify(a):a},deserialize:function(a){return a&&JSON.parse(a)}};var c=this.localStorage().getItem(this.name);this.records=c&&c.split(",")||[]},f(a.LocalStorage.prototype,{save:function(){this.localStorage().setItem(this.name,this.records.join(","))},create:function(a){return a.id||0===a.id||(a.id=c(),a.set(a.idAttribute,a.id)),this.localStorage().setItem(this._itemName(a.id),this.serializer.serialize(a)),this.records.push(a.id.toString()),this.save(),this.find(a)},update:function(a){this.localStorage().setItem(this._itemName(a.id),this.serializer.serialize(a));var b=a.id.toString();return e(this.records,b)||(this.records.push(b),this.save()),this.find(a)},find:function(a){return this.serializer.deserialize(this.localStorage().getItem(this._itemName(a.id)))},findAll:function(){for(var a,b,c=[],d=0;d<this.records.length;d++)a=this.records[d],b=this.serializer.deserialize(this.localStorage().getItem(this._itemName(a))),null!=b&&c.push(b);return c},destroy:function(a){this.localStorage().removeItem(this._itemName(a.id));for(var b=a.id.toString(),c=0;c<this.records.length;c++)this.records[c]===b&&this.records.splice(c,1);return this.save(),a},localStorage:function(){return localStorage},_clear:function(){var a=this.localStorage(),b=new RegExp("^"+this.name+"-");a.removeItem(this.name);for(var c in a)b.test(c)&&a.removeItem(c);this.records.length=0},_storageSize:function(){return this.localStorage().length},_itemName:function(a){return this.name+"-"+a}}),a.LocalStorage.sync=window.Store.sync=a.localSync=function(b,c,d){var e,f,h=g(c,"localStorage")||g(c.collection,"localStorage"),i=a.$?a.$.Deferred&&a.$.Deferred():a.Deferred&&a.Deferred();try{switch(b){case"read":e=void 0!=c.id?h.find(c):h.findAll();break;case"create":e=h.create(c);break;case"update":e=h.update(c);break;case"delete":e=h.destroy(c)}}catch(j){f=22===j.code&&0===h._storageSize()?"Private browsing is unsupported":j.message}return e?(d&&d.success&&("0.9.10"===a.VERSION?d.success(c,e,d):d.success(e)),i&&i.resolve(e)):(f=f?f:"Record Not Found",d&&d.error&&("0.9.10"===a.VERSION?d.error(c,f,d):d.error(f)),i&&i.reject(f)),d&&d.complete&&d.complete(e),i&&i.promise()},a.ajaxSync=a.sync,a.getSyncMethod=function(b,c){var d=c&&c.ajaxSync;return d||!g(b,"localStorage")&&!g(b.collection,"localStorage")?a.ajaxSync:a.localSync},a.sync=function(b,c,d){return a.getSyncMethod(c,d).apply(this,[b,c,d])},a.LocalStorage}),function(a,b){if("function"==typeof define&&define.amd)define(["backbone","underscore"],function(c,d){return a.Marionette=a.Mn=b(a,c,d)});else if("undefined"!=typeof exports){var c=require("backbone"),d=require("underscore");module.exports=b(a,c,d)}else a.Marionette=a.Mn=b(a,a.Backbone,a._)}(this,function(a,b,c){"use strict";!function(a,b){var c=a.ChildViewContainer;return a.ChildViewContainer=function(a,b){var c=function(a){this._views={},this._indexByModel={},this._indexByCustom={},this._updateLength(),b.each(a,this.add,this)};b.extend(c.prototype,{add:function(a,b){var c=a.cid;return this._views[c]=a,a.model&&(this._indexByModel[a.model.cid]=c),b&&(this._indexByCustom[b]=c),this._updateLength(),this},findByModel:function(a){return this.findByModelCid(a.cid)},findByModelCid:function(a){var b=this._indexByModel[a];return this.findByCid(b)},findByCustom:function(a){var b=this._indexByCustom[a];return this.findByCid(b)},findByIndex:function(a){return b.values(this._views)[a]},findByCid:function(a){return this._views[a]},remove:function(a){var c=a.cid;return a.model&&delete this._indexByModel[a.model.cid],b.any(this._indexByCustom,function(a,b){return a===c?(delete this._indexByCustom[b],!0):void 0},this),delete this._views[c],this._updateLength(),this},call:function(a){this.apply(a,b.tail(arguments))},apply:function(a,c){b.each(this._views,function(d){b.isFunction(d[a])&&d[a].apply(d,c||[])})},_updateLength:function(){this.length=b.size(this._views)}});var d=["forEach","each","map","find","detect","filter","select","reject","every","all","some","any","include","contains","invoke","toArray","first","initial","rest","last","without","isEmpty","pluck"];return b.each(d,function(a){c.prototype[a]=function(){var c=b.values(this._views),d=[c].concat(b.toArray(arguments));return b[a].apply(b,d)}}),c}(a,b),a.ChildViewContainer.VERSION="0.1.5",a.ChildViewContainer.noConflict=function(){return a.ChildViewContainer=c,this},a.ChildViewContainer}(b,c),function(a,b){var c=a.Wreqr,d=a.Wreqr={};return a.Wreqr.VERSION="1.3.1",a.Wreqr.noConflict=function(){return a.Wreqr=c,this},d.Handlers=function(a,b){var c=function(a){this.options=a,this._wreqrHandlers={},b.isFunction(this.initialize)&&this.initialize(a)};return c.extend=a.Model.extend,b.extend(c.prototype,a.Events,{setHandlers:function(a){b.each(a,function(a,c){var d=null;b.isObject(a)&&!b.isFunction(a)&&(d=a.context,a=a.callback),this.setHandler(c,a,d)},this)},setHandler:function(a,b,c){var d={callback:b,context:c};this._wreqrHandlers[a]=d,this.trigger("handler:add",a,b,c)},hasHandler:function(a){return!!this._wreqrHandlers[a]},getHandler:function(a){var b=this._wreqrHandlers[a];if(b)return function(){var a=Array.prototype.slice.apply(arguments);return b.callback.apply(b.context,a)}},removeHandler:function(a){delete this._wreqrHandlers[a]},removeAllHandlers:function(){this._wreqrHandlers={}}}),c}(a,b),d.CommandStorage=function(){var c=function(a){this.options=a,this._commands={},b.isFunction(this.initialize)&&this.initialize(a)};return b.extend(c.prototype,a.Events,{getCommands:function(a){var b=this._commands[a];return b||(b={command:a,instances:[]},this._commands[a]=b),b},addCommand:function(a,b){var c=this.getCommands(a);c.instances.push(b)},clearCommands:function(a){var b=this.getCommands(a);b.instances=[]}}),c}(),d.Commands=function(a){return a.Handlers.extend({storageType:a.CommandStorage,constructor:function(b){this.options=b||{},this._initializeStorage(this.options),this.on("handler:add",this._executeCommands,this);var c=Array.prototype.slice.call(arguments);a.Handlers.prototype.constructor.apply(this,c)},execute:function(a,b){a=arguments[0],b=Array.prototype.slice.call(arguments,1),this.hasHandler(a)?this.getHandler(a).apply(this,b):this.storage.addCommand(a,b)},_executeCommands:function(a,c,d){var e=this.storage.getCommands(a);b.each(e.instances,function(a){c.apply(d,a)}),this.storage.clearCommands(a)},_initializeStorage:function(a){var c,d=a.storageType||this.storageType;c=b.isFunction(d)?new d:d,this.storage=c}})}(d),d.RequestResponse=function(a){return a.Handlers.extend({request:function(){var a=arguments[0],b=Array.prototype.slice.call(arguments,1);return this.hasHandler(a)?this.getHandler(a).apply(this,b):void 0}})}(d),d.EventAggregator=function(a,b){var c=function(){};return c.extend=a.Model.extend,b.extend(c.prototype,a.Events),c}(a,b),d.Channel=function(c){var d=function(b){this.vent=new a.Wreqr.EventAggregator,this.reqres=new a.Wreqr.RequestResponse,this.commands=new a.Wreqr.Commands,this.channelName=b};return b.extend(d.prototype,{reset:function(){return this.vent.off(),this.vent.stopListening(),this.reqres.removeAllHandlers(),this.commands.removeAllHandlers(),this},connectEvents:function(a,b){return this._connect("vent",a,b),this},connectCommands:function(a,b){return this._connect("commands",a,b),this},connectRequests:function(a,b){return this._connect("reqres",a,b),this},_connect:function(a,c,d){if(c){d=d||this;var e="vent"===a?"on":"setHandler";b.each(c,function(c,f){this[a][e](f,b.bind(c,d))},this)}}}),d}(d),d.radio=function(a){var c=function(){this._channels={},this.vent={},this.commands={},this.reqres={},this._proxyMethods()};b.extend(c.prototype,{channel:function(a){if(!a)throw new Error("Channel must receive a name");return this._getChannel(a)},_getChannel:function(b){var c=this._channels[b];return c||(c=new a.Channel(b),this._channels[b]=c),c},_proxyMethods:function(){b.each(["vent","commands","reqres"],function(a){b.each(d[a],function(b){this[a][b]=e(this,a,b)},this)},this)}});var d={vent:["on","off","trigger","once","stopListening","listenTo","listenToOnce"],commands:["execute","setHandler","setHandlers","removeHandler","removeAllHandlers"],reqres:["request","setHandler","setHandlers","removeHandler","removeAllHandlers"]},e=function(a,b,c){return function(d){var e=a._getChannel(d)[b],f=Array.prototype.slice.call(arguments,1);return e[c].apply(e,f)}};return new c}(d),a.Wreqr}(b,c);var d=a.Marionette,e=b.Marionette={};e.VERSION="2.3.1",e.noConflict=function(){return a.Marionette=d,this},b.Marionette=e,e.Deferred=b.$.Deferred,e.extend=b.Model.extend,e.isNodeAttached=function(a){return b.$.contains(document.documentElement,a)},e.getOption=function(a,b){return a&&b?a.options&&void 0!==a.options[b]?a.options[b]:a[b]:void 0},e.proxyGetOption=function(a){return e.getOption(this,a)},e._getValue=function(a,b,d){return c.isFunction(a)&&(a=a.apply(b,d)),a},e.normalizeMethods=function(a){return c.reduce(a,function(a,b,d){return c.isFunction(b)||(b=this[b]),b&&(a[d]=b),a},{},this)},e.normalizeUIString=function(a,b){return a.replace(/@ui\.[a-zA-Z_$0-9]*/g,function(a){return b[a.slice(4)]})},e.normalizeUIKeys=function(a,b){return c.reduce(a,function(a,c,d){var f=e.normalizeUIString(d,b);return a[f]=c,a},{})},e.normalizeUIValues=function(a,b){return c.each(a,function(d,f){c.isString(d)&&(a[f]=e.normalizeUIString(d,b))}),a},e.actAsCollection=function(a,b){var d=["forEach","each","map","find","detect","filter","select","reject","every","all","some","any","include","contains","invoke","toArray","first","initial","rest","last","without","isEmpty","pluck"];c.each(d,function(d){a[d]=function(){var a=c.values(c.result(this,b)),e=[a].concat(c.toArray(arguments));return c[d].apply(c,e)}})};var f=e.deprecate=function(a,b){c.isObject(a)&&(a=a.prev+" is going to be removed in the future. Please use "+a.next+" instead."+(a.url?" See: "+a.url:"")),void 0!==b&&b||f._cache[a]||(f._warn("Deprecation warning: "+a),f._cache[a]=!0)};f._warn="undefined"!=typeof console&&(console.warn||console.log)||function(){},f._cache={},e._triggerMethod=function(){function a(a,b,c){return c.toUpperCase()}var b=/(^|:)(\w)/gi;return function(d,e,f){var g=arguments.length<3;g&&(f=e,e=f[0]);var h,i="on"+e.replace(b,a),j=d[i];return c.isFunction(j)&&(h=j.apply(d,g?c.rest(f):f)),c.isFunction(d.trigger)&&(g+f.length>1?d.trigger.apply(d,g?f:[e].concat(c.rest(f,0))):d.trigger(e)),h}}(),e.triggerMethod=function(a){return e._triggerMethod(this,arguments)},e.triggerMethodOn=function(a){var b=c.isFunction(a.triggerMethod)?a.triggerMethod:e.triggerMethod;return b.apply(a,c.rest(arguments))},e.MonitorDOMRefresh=function(a){function b(){a._isShown=!0,f()}function d(){a._isRendered=!0,f()}function f(){a._isShown&&a._isRendered&&e.isNodeAttached(a.el)&&c.isFunction(a.triggerMethod)&&a.triggerMethod("dom:refresh")}a.on({show:b,render:d})},function(a){function b(b,d,e,f){var g=f.split(/\s+/);c.each(g,function(c){var f=b[c];if(!f)throw new a.Error('Method "'+c+'" was configured as an event handler, but does not exist.');b.listenTo(d,e,f)})}function d(a,b,c,d){a.listenTo(b,c,d)}function e(a,b,d,e){var f=e.split(/\s+/);c.each(f,function(c){var e=a[c];a.stopListening(b,d,e)})}function f(a,b,c,d){a.stopListening(b,c,d)}function g(b,d,e,f,g){if(d&&e){if(!c.isObject(e))throw new a.Error({message:"Bindings must be an object or function.",url:"marionette.functions.html#marionettebindentityevents"});e=a._getValue(e,b),c.each(e,function(a,e){c.isFunction(a)?f(b,d,e,a):g(b,d,e,a)})}}a.bindEntityEvents=function(a,c,e){g(a,c,e,d,b)},a.unbindEntityEvents=function(a,b,c){g(a,b,c,f,e)},a.proxyBindEntityEvents=function(b,c){return a.bindEntityEvents(this,b,c)},a.proxyUnbindEntityEvents=function(b,c){return a.unbindEntityEvents(this,b,c)}}(e);var g=["description","fileName","lineNumber","name","message","number"];return e.Error=e.extend.call(Error,{urlRoot:"http://marionettejs.com/docs/v"+e.VERSION+"/",constructor:function(a,b){c.isObject(a)?(b=a,a=b.message):b||(b={});var d=Error.call(this,a);c.extend(this,c.pick(d,g),c.pick(b,g)),this.captureStackTrace(),b.url&&(this.url=this.urlRoot+b.url)},captureStackTrace:function(){Error.captureStackTrace&&Error.captureStackTrace(this,e.Error)},toString:function(){return this.name+": "+this.message+(this.url?" See: "+this.url:"")}}),e.Error.extend=e.extend,e.Callbacks=function(){this._deferred=e.Deferred(),this._callbacks=[]},c.extend(e.Callbacks.prototype,{add:function(a,b){var d=c.result(this._deferred,"promise");this._callbacks.push({cb:a,ctx:b}),d.then(function(c){b&&(c.context=b),a.call(c.context,c.options)})},run:function(a,b){this._deferred.resolve({options:a,context:b})},reset:function(){var a=this._callbacks;this._deferred=e.Deferred(),this._callbacks=[],c.each(a,function(a){this.add(a.cb,a.ctx)},this)}}),e.Controller=function(a){this.options=a||{},c.isFunction(this.initialize)&&this.initialize(this.options)},e.Controller.extend=e.extend,c.extend(e.Controller.prototype,b.Events,{destroy:function(){return e._triggerMethod(this,"before:destroy",arguments),
e._triggerMethod(this,"destroy",arguments),this.stopListening(),this.off(),this},triggerMethod:e.triggerMethod,getOption:e.proxyGetOption}),e.Object=function(a){this.options=c.extend({},c.result(this,"options"),a),this.initialize.apply(this,arguments)},e.Object.extend=e.extend,c.extend(e.Object.prototype,b.Events,{initialize:function(){},destroy:function(){this.triggerMethod("before:destroy"),this.triggerMethod("destroy"),this.stopListening()},triggerMethod:e.triggerMethod,getOption:e.proxyGetOption,bindEntityEvents:e.proxyBindEntityEvents,unbindEntityEvents:e.proxyUnbindEntityEvents}),e.Region=e.Object.extend({constructor:function(a){if(this.options=a||{},this.el=this.getOption("el"),this.el=this.el instanceof b.$?this.el[0]:this.el,!this.el)throw new e.Error({name:"NoElError",message:'An "el" must be specified for a region.'});this.$el=this.getEl(this.el),e.Object.call(this,a)},show:function(a,b){if(this._ensureElement()){this._ensureViewIsIntact(a);var c=b||{},d=a!==this.currentView,f=!!c.preventDestroy,g=!!c.forceShow,h=!!this.currentView,i=d&&!f,j=d||g;if(h&&this.triggerMethod("before:swapOut",this.currentView,this,b),this.currentView&&delete this.currentView._parent,i?this.empty():h&&j&&this.currentView.off("destroy",this.empty,this),j){a.once("destroy",this.empty,this),a.render(),a._parent=this,h&&this.triggerMethod("before:swap",a,this,b),this.triggerMethod("before:show",a,this,b),e.triggerMethodOn(a,"before:show",a,this,b),h&&this.triggerMethod("swapOut",this.currentView,this,b);var k=e.isNodeAttached(this.el),l=[],m=c.triggerBeforeAttach||this.triggerBeforeAttach,n=c.triggerAttach||this.triggerAttach;return k&&m&&(l=this._displayedViews(a),this._triggerAttach(l,"before:")),this.attachHtml(a),this.currentView=a,k&&n&&(l=this._displayedViews(a),this._triggerAttach(l)),h&&this.triggerMethod("swap",a,this,b),this.triggerMethod("show",a,this,b),e.triggerMethodOn(a,"show",a,this,b),this}return this}},triggerBeforeAttach:!0,triggerAttach:!0,_triggerAttach:function(a,b){var d=(b||"")+"attach";c.each(a,function(a){e.triggerMethodOn(a,d,a,this)},this)},_displayedViews:function(a){return c.union([a],c.result(a,"_getNestedViews")||[])},_ensureElement:function(){if(c.isObject(this.el)||(this.$el=this.getEl(this.el),this.el=this.$el[0]),!this.$el||0===this.$el.length){if(this.getOption("allowMissingEl"))return!1;throw new e.Error('An "el" '+this.$el.selector+" must exist in DOM")}return!0},_ensureViewIsIntact:function(a){if(!a)throw new e.Error({name:"ViewNotValid",message:"The view passed is undefined and therefore invalid. You must pass a view instance to show."});if(a.isDestroyed)throw new e.Error({name:"ViewDestroyedError",message:'View (cid: "'+a.cid+'") has already been destroyed and cannot be used.'})},getEl:function(a){return b.$(a,e._getValue(this.options.parentEl,this))},attachHtml:function(a){this.$el.contents().detach(),this.el.appendChild(a.el)},empty:function(){var a=this.currentView;if(a)return a.off("destroy",this.empty,this),this.triggerMethod("before:empty",a),this._destroyView(),this.triggerMethod("empty",a),delete this.currentView,this},_destroyView:function(){var a=this.currentView;a.destroy&&!a.isDestroyed?a.destroy():a.remove&&(a.remove(),a.isDestroyed=!0)},attachView:function(a){return this.currentView=a,this},hasView:function(){return!!this.currentView},reset:function(){return this.empty(),this.$el&&(this.el=this.$el.selector),delete this.$el,this}},{buildRegion:function(a,b){if(c.isString(a))return this._buildRegionFromSelector(a,b);if(a.selector||a.el||a.regionClass)return this._buildRegionFromObject(a,b);if(c.isFunction(a))return this._buildRegionFromRegionClass(a);throw new e.Error({message:"Improper region configuration type.",url:"marionette.region.html#region-configuration-types"})},_buildRegionFromSelector:function(a,b){return new b({el:a})},_buildRegionFromObject:function(a,b){var d=a.regionClass||b,e=c.omit(a,"selector","regionClass");return a.selector&&!e.el&&(e.el=a.selector),new d(e)},_buildRegionFromRegionClass:function(a){return new a}}),e.RegionManager=e.Controller.extend({constructor:function(a){this._regions={},e.Controller.call(this,a),this.addRegions(this.getOption("regions"))},addRegions:function(a,b){return a=e._getValue(a,this,arguments),c.reduce(a,function(a,d,e){return c.isString(d)&&(d={selector:d}),d.selector&&(d=c.defaults({},d,b)),a[e]=this.addRegion(e,d),a},{},this)},addRegion:function(a,b){var c;return c=b instanceof e.Region?b:e.Region.buildRegion(b,e.Region),this.triggerMethod("before:add:region",a,c),c._parent=this,this._store(a,c),this.triggerMethod("add:region",a,c),c},get:function(a){return this._regions[a]},getRegions:function(){return c.clone(this._regions)},removeRegion:function(a){var b=this._regions[a];return this._remove(a,b),b},removeRegions:function(){var a=this.getRegions();return c.each(this._regions,function(a,b){this._remove(b,a)},this),a},emptyRegions:function(){var a=this.getRegions();return c.invoke(a,"empty"),a},destroy:function(){return this.removeRegions(),e.Controller.prototype.destroy.apply(this,arguments)},_store:function(a,b){this._regions[a]=b,this._setLength()},_remove:function(a,b){this.triggerMethod("before:remove:region",a,b),b.empty(),b.stopListening(),delete b._parent,delete this._regions[a],this._setLength(),this.triggerMethod("remove:region",a,b)},_setLength:function(){this.length=c.size(this._regions)}}),e.actAsCollection(e.RegionManager.prototype,"_regions"),e.TemplateCache=function(a){this.templateId=a},c.extend(e.TemplateCache,{templateCaches:{},get:function(a){var b=this.templateCaches[a];return b||(b=new e.TemplateCache(a),this.templateCaches[a]=b),b.load()},clear:function(){var a,b=c.toArray(arguments),d=b.length;if(d>0)for(a=0;d>a;a++)delete this.templateCaches[b[a]];else this.templateCaches={}}}),c.extend(e.TemplateCache.prototype,{load:function(){if(this.compiledTemplate)return this.compiledTemplate;var a=this.loadTemplate(this.templateId);return this.compiledTemplate=this.compileTemplate(a),this.compiledTemplate},loadTemplate:function(a){var c=b.$(a).html();if(!c||0===c.length)throw new e.Error({name:"NoTemplateError",message:'Could not find template: "'+a+'"'});return c},compileTemplate:function(a){return c.template(a)}}),e.Renderer={render:function(a,b){if(!a)throw new e.Error({name:"TemplateNotFoundError",message:"Cannot render the template since its false, null or undefined."});var d=c.isFunction(a)?a:e.TemplateCache.get(a);return d(b)}},e.View=b.View.extend({isDestroyed:!1,constructor:function(a){c.bindAll(this,"render"),a=e._getValue(a,this),this.options=c.extend({},c.result(this,"options"),a),this._behaviors=e.Behaviors(this),b.View.apply(this,arguments),e.MonitorDOMRefresh(this),this.on("show",this.onShowCalled)},getTemplate:function(){return this.getOption("template")},serializeModel:function(a){return a.toJSON.apply(a,c.rest(arguments))},mixinTemplateHelpers:function(a){a=a||{};var b=this.getOption("templateHelpers");return b=e._getValue(b,this),c.extend(a,b)},normalizeUIKeys:function(a){var b=c.result(this,"_uiBindings");return e.normalizeUIKeys(a,b||c.result(this,"ui"))},normalizeUIValues:function(a){var b=c.result(this,"ui"),d=c.result(this,"_uiBindings");return e.normalizeUIValues(a,d||b)},configureTriggers:function(){if(this.triggers){var a=this.normalizeUIKeys(c.result(this,"triggers"));return c.reduce(a,function(a,b,c){return a[c]=this._buildViewTrigger(b),a},{},this)}},delegateEvents:function(a){return this._delegateDOMEvents(a),this.bindEntityEvents(this.model,this.getOption("modelEvents")),this.bindEntityEvents(this.collection,this.getOption("collectionEvents")),c.each(this._behaviors,function(a){a.bindEntityEvents(this.model,a.getOption("modelEvents")),a.bindEntityEvents(this.collection,a.getOption("collectionEvents"))},this),this},_delegateDOMEvents:function(a){var d=e._getValue(a||this.events,this);d=this.normalizeUIKeys(d),c.isUndefined(a)&&(this.events=d);var f={},g=c.result(this,"behaviorEvents")||{},h=this.configureTriggers(),i=c.result(this,"behaviorTriggers")||{};c.extend(f,g,d,h,i),b.View.prototype.delegateEvents.call(this,f)},undelegateEvents:function(){return b.View.prototype.undelegateEvents.apply(this,arguments),this.unbindEntityEvents(this.model,this.getOption("modelEvents")),this.unbindEntityEvents(this.collection,this.getOption("collectionEvents")),c.each(this._behaviors,function(a){a.unbindEntityEvents(this.model,a.getOption("modelEvents")),a.unbindEntityEvents(this.collection,a.getOption("collectionEvents"))},this),this},onShowCalled:function(){},_ensureViewIsIntact:function(){if(this.isDestroyed)throw new e.Error({name:"ViewDestroyedError",message:'View (cid: "'+this.cid+'") has already been destroyed and cannot be used.'})},destroy:function(){if(!this.isDestroyed){var a=c.toArray(arguments);return this.triggerMethod.apply(this,["before:destroy"].concat(a)),this.isDestroyed=!0,this.triggerMethod.apply(this,["destroy"].concat(a)),this.unbindUIElements(),this.remove(),c.invoke(this._behaviors,"destroy",a),this}},bindUIElements:function(){this._bindUIElements(),c.invoke(this._behaviors,this._bindUIElements)},_bindUIElements:function(){if(this.ui){this._uiBindings||(this._uiBindings=this.ui);var a=c.result(this,"_uiBindings");this.ui={},c.each(a,function(a,b){this.ui[b]=this.$(a)},this)}},unbindUIElements:function(){this._unbindUIElements(),c.invoke(this._behaviors,this._unbindUIElements)},_unbindUIElements:function(){this.ui&&this._uiBindings&&(c.each(this.ui,function(a,b){delete this.ui[b]},this),this.ui=this._uiBindings,delete this._uiBindings)},_buildViewTrigger:function(a){var b=c.isObject(a),d=c.defaults({},b?a:{},{preventDefault:!0,stopPropagation:!0}),e=b?d.event:a;return function(a){a&&(a.preventDefault&&d.preventDefault&&a.preventDefault(),a.stopPropagation&&d.stopPropagation&&a.stopPropagation());var b={view:this,model:this.model,collection:this.collection};this.triggerMethod(e,b)}},setElement:function(){var a=b.View.prototype.setElement.apply(this,arguments);return c.invoke(this._behaviors,"proxyViewProperties",this),a},triggerMethod:function(){for(var a=e._triggerMethod,b=a(this,arguments),c=this._behaviors,d=0,f=c&&c.length;f>d;d++)a(c[d],arguments);return b},_getImmediateChildren:function(){return[]},_getNestedViews:function(){var a=this._getImmediateChildren();return a.length?c.reduce(a,function(a,b){return b._getNestedViews?a.concat(b._getNestedViews()):a},a):a},normalizeMethods:e.normalizeMethods,getOption:e.proxyGetOption,bindEntityEvents:e.proxyBindEntityEvents,unbindEntityEvents:e.proxyUnbindEntityEvents}),e.ItemView=e.View.extend({constructor:function(){e.View.apply(this,arguments)},serializeData:function(){if(!this.model&&!this.collection)return{};var a=[this.model||this.collection];return arguments.length&&a.push.apply(a,arguments),this.model?this.serializeModel.apply(this,a):{items:this.serializeCollection.apply(this,a)}},serializeCollection:function(a){return a.toJSON.apply(a,c.rest(arguments))},render:function(){return this._ensureViewIsIntact(),this.triggerMethod("before:render",this),this._renderTemplate(),this.bindUIElements(),this.triggerMethod("render",this),this},_renderTemplate:function(){var a=this.getTemplate();if(a!==!1){if(!a)throw new e.Error({name:"UndefinedTemplateError",message:"Cannot render the template since it is null or undefined."});var b=this.serializeData();b=this.mixinTemplateHelpers(b);var c=e.Renderer.render(a,b,this);return this.attachElContent(c),this}},attachElContent:function(a){return this.$el.html(a),this}}),e.CollectionView=e.View.extend({childViewEventPrefix:"childview",constructor:function(a){var b=a||{};c.isUndefined(this.sort)&&(this.sort=c.isUndefined(b.sort)?!0:b.sort),this.once("render",this._initialEvents),this._initChildViewStorage(),e.View.apply(this,arguments),this.initRenderBuffer()},initRenderBuffer:function(){this.elBuffer=document.createDocumentFragment(),this._bufferedChildren=[]},startBuffering:function(){this.initRenderBuffer(),this.isBuffering=!0},endBuffering:function(){this.isBuffering=!1,this._triggerBeforeShowBufferedChildren(),this.attachBuffer(this,this.elBuffer),this._triggerShowBufferedChildren(),this.initRenderBuffer()},_triggerBeforeShowBufferedChildren:function(){this._isShown&&c.each(this._bufferedChildren,c.partial(this._triggerMethodOnChild,"before:show"))},_triggerShowBufferedChildren:function(){this._isShown&&(c.each(this._bufferedChildren,c.partial(this._triggerMethodOnChild,"show")),this._bufferedChildren=[])},_triggerMethodOnChild:function(a,b){e.triggerMethodOn(b,a)},_initialEvents:function(){this.collection&&(this.listenTo(this.collection,"add",this._onCollectionAdd),this.listenTo(this.collection,"remove",this._onCollectionRemove),this.listenTo(this.collection,"reset",this.render),this.sort&&this.listenTo(this.collection,"sort",this._sortViews))},_onCollectionAdd:function(a){this.destroyEmptyView();var b=this.getChildView(a),c=this.collection.indexOf(a);this.addChild(a,b,c)},_onCollectionRemove:function(a){var b=this.children.findByModel(a);this.removeChildView(b),this.checkEmpty()},onShowCalled:function(){this.children.each(c.partial(this._triggerMethodOnChild,"show"))},render:function(){return this._ensureViewIsIntact(),this.triggerMethod("before:render",this),this._renderChildren(),this.triggerMethod("render",this),this},resortView:function(){this.render()},_sortViews:function(){var a=this.collection.find(function(a,b){var c=this.children.findByModel(a);return!c||c._index!==b},this);a&&this.resortView()},_emptyViewIndex:-1,_renderChildren:function(){this.destroyEmptyView(),this.destroyChildren(),this.isEmpty(this.collection)?this.showEmptyView():(this.triggerMethod("before:render:collection",this),this.startBuffering(),this.showCollection(),this.endBuffering(),this.triggerMethod("render:collection",this))},showCollection:function(){var a;this.collection.each(function(b,c){a=this.getChildView(b),this.addChild(b,a,c)},this)},showEmptyView:function(){var a=this.getEmptyView();if(a&&!this._showingEmptyView){this.triggerMethod("before:render:empty"),this._showingEmptyView=!0;var c=new b.Model;this.addEmptyView(c,a),this.triggerMethod("render:empty")}},destroyEmptyView:function(){this._showingEmptyView&&(this.triggerMethod("before:remove:empty"),this.destroyChildren(),delete this._showingEmptyView,this.triggerMethod("remove:empty"))},getEmptyView:function(){return this.getOption("emptyView")},addEmptyView:function(a,b){var d=this.getOption("emptyViewOptions")||this.getOption("childViewOptions");c.isFunction(d)&&(d=d.call(this,a,this._emptyViewIndex));var f=this.buildChildView(a,b,d);f._parent=this,this.proxyChildEvents(f),this._isShown&&e.triggerMethodOn(f,"before:show"),this.children.add(f),this.renderChildView(f,this._emptyViewIndex),this._isShown&&e.triggerMethodOn(f,"show")},getChildView:function(a){var b=this.getOption("childView");if(!b)throw new e.Error({name:"NoChildViewError",message:'A "childView" must be specified'});return b},addChild:function(a,b,c){var d=this.getOption("childViewOptions");d=e._getValue(d,this,[a,c]);var f=this.buildChildView(a,b,d);return this._updateIndices(f,!0,c),this._addChildView(f,c),f._parent=this,f},_updateIndices:function(a,b,c){this.sort&&(b&&(a._index=c),this.children.each(function(c){c._index>=a._index&&(c._index+=b?1:-1)}))},_addChildView:function(a,b){this.proxyChildEvents(a),this.triggerMethod("before:add:child",a),this.children.add(a),this.renderChildView(a,b),this._isShown&&!this.isBuffering&&e.triggerMethodOn(a,"show"),this.triggerMethod("add:child",a)},renderChildView:function(a,b){return a.render(),this.attachHtml(this,a,b),a},buildChildView:function(a,b,d){var e=c.extend({model:a},d);return new b(e)},removeChildView:function(a){return a&&(this.triggerMethod("before:remove:child",a),a.destroy?a.destroy():a.remove&&a.remove(),delete a._parent,this.stopListening(a),this.children.remove(a),this.triggerMethod("remove:child",a),this._updateIndices(a,!1)),a},isEmpty:function(){return!this.collection||0===this.collection.length},checkEmpty:function(){this.isEmpty(this.collection)&&this.showEmptyView()},attachBuffer:function(a,b){a.$el.append(b)},attachHtml:function(a,b,c){a.isBuffering?(a.elBuffer.appendChild(b.el),a._bufferedChildren.push(b)):a._insertBefore(b,c)||a._insertAfter(b)},_insertBefore:function(a,b){var c,d=this.sort&&b<this.children.length-1;return d&&(c=this.children.find(function(a){return a._index===b+1})),c?(c.$el.before(a.el),!0):!1},_insertAfter:function(a){this.$el.append(a.el)},_initChildViewStorage:function(){this.children=new b.ChildViewContainer},destroy:function(){return this.isDestroyed?void 0:(this.triggerMethod("before:destroy:collection"),this.destroyChildren(),this.triggerMethod("destroy:collection"),e.View.prototype.destroy.apply(this,arguments))},destroyChildren:function(){var a=this.children.map(c.identity);return this.children.each(this.removeChildView,this),this.checkEmpty(),a},proxyChildEvents:function(a){var b=this.getOption("childViewEventPrefix");this.listenTo(a,"all",function(){var d=c.toArray(arguments),e=d[0],f=this.normalizeMethods(c.result(this,"childEvents"));d[0]=b+":"+e,d.splice(1,0,a),"undefined"!=typeof f&&c.isFunction(f[e])&&f[e].apply(this,d.slice(1)),this.triggerMethod.apply(this,d)},this)},_getImmediateChildren:function(){return c.values(this.children._views)}}),e.CompositeView=e.CollectionView.extend({constructor:function(){e.CollectionView.apply(this,arguments)},_initialEvents:function(){this.collection&&(this.listenTo(this.collection,"add",this._onCollectionAdd),this.listenTo(this.collection,"remove",this._onCollectionRemove),this.listenTo(this.collection,"reset",this._renderChildren),this.sort&&this.listenTo(this.collection,"sort",this._sortViews))},getChildView:function(a){var b=this.getOption("childView")||this.constructor;return b},serializeData:function(){var a={};return this.model&&(a=c.partial(this.serializeModel,this.model).apply(this,arguments)),a},render:function(){return this._ensureViewIsIntact(),this.isRendered=!0,this.resetChildViewContainer(),this.triggerMethod("before:render",this),this._renderTemplate(),this._renderChildren(),this.triggerMethod("render",this),this},_renderChildren:function(){this.isRendered&&e.CollectionView.prototype._renderChildren.call(this)},_renderTemplate:function(){var a={};a=this.serializeData(),a=this.mixinTemplateHelpers(a),this.triggerMethod("before:render:template");var b=this.getTemplate(),c=e.Renderer.render(b,a,this);this.attachElContent(c),this.bindUIElements(),this.triggerMethod("render:template")},attachElContent:function(a){return this.$el.html(a),this},attachBuffer:function(a,b){var c=this.getChildViewContainer(a);c.append(b)},_insertAfter:function(a){var b=this.getChildViewContainer(this,a);b.append(a.el)},getChildViewContainer:function(a,b){if("$childViewContainer"in a)return a.$childViewContainer;var c,d=e.getOption(a,"childViewContainer");if(d){var f=e._getValue(d,a);if(c="@"===f.charAt(0)&&a.ui?a.ui[f.substr(4)]:a.$(f),c.length<=0)throw new e.Error({name:"ChildViewContainerMissingError",message:'The specified "childViewContainer" was not found: '+a.childViewContainer})}else c=a.$el;return a.$childViewContainer=c,c},resetChildViewContainer:function(){this.$childViewContainer&&delete this.$childViewContainer}}),e.LayoutView=e.ItemView.extend({regionClass:e.Region,constructor:function(a){a=a||{},this._firstRender=!0,this._initializeRegions(a),e.ItemView.call(this,a)},render:function(){return this._ensureViewIsIntact(),this._firstRender?this._firstRender=!1:this._reInitializeRegions(),e.ItemView.prototype.render.apply(this,arguments)},destroy:function(){return this.isDestroyed?this:(this.regionManager.destroy(),e.ItemView.prototype.destroy.apply(this,arguments))},addRegion:function(a,b){var c={};return c[a]=b,this._buildRegions(c)[a]},addRegions:function(a){return this.regions=c.extend({},this.regions,a),this._buildRegions(a)},removeRegion:function(a){return delete this.regions[a],this.regionManager.removeRegion(a)},getRegion:function(a){return this.regionManager.get(a)},getRegions:function(){return this.regionManager.getRegions()},_buildRegions:function(a){var b={regionClass:this.getOption("regionClass"),parentEl:c.partial(c.result,this,"el")};return this.regionManager.addRegions(a,b)},_initializeRegions:function(a){var b;this._initRegionManager(),b=e._getValue(this.regions,this,[a])||{};var d=this.getOption.call(a,"regions");d=e._getValue(d,this,[a]),c.extend(b,d),b=this.normalizeUIValues(b),this.addRegions(b)},_reInitializeRegions:function(){this.regionManager.invoke("reset")},getRegionManager:function(){return new e.RegionManager},_initRegionManager:function(){this.regionManager=this.getRegionManager(),this.regionManager._parent=this,this.listenTo(this.regionManager,"before:add:region",function(a){this.triggerMethod("before:add:region",a)}),this.listenTo(this.regionManager,"add:region",function(a,b){this[a]=b,this.triggerMethod("add:region",a,b)}),this.listenTo(this.regionManager,"before:remove:region",function(a){this.triggerMethod("before:remove:region",a)}),this.listenTo(this.regionManager,"remove:region",function(a,b){delete this[a],this.triggerMethod("remove:region",a,b)})},_getImmediateChildren:function(){return c.chain(this.regionManager.getRegions()).pluck("currentView").compact().value()}}),e.Behavior=e.Object.extend({constructor:function(a,b){this.view=b,this.defaults=c.result(this,"defaults")||{},this.options=c.extend({},this.defaults,a),e.Object.apply(this,arguments)},$:function(){return this.view.$.apply(this.view,arguments)},destroy:function(){this.stopListening()},proxyViewProperties:function(a){this.$el=a.$el,this.el=a.el}}),e.Behaviors=function(a,b){function c(a,d){return b.isObject(a.behaviors)?(d=c.parseBehaviors(a,d||b.result(a,"behaviors")),c.wrap(a,d,b.keys(f)),d):{}}function d(a,c){this._view=a,this._viewUI=b.result(a,"ui"),this._behaviors=c,this._triggers={}}var e=/^(\S+)\s*(.*)$/,f={behaviorTriggers:function(a,b){var c=new d(this,b);return c.buildBehaviorTriggers()},behaviorEvents:function(c,d){var f={},g=this._uiBindings||b.result(this,"ui");return b.each(d,function(c,d){var h={},i=b.clone(b.result(c,"events"))||{},j=c._uiBindings||b.result(c,"ui"),k=b.extend({},g,j);i=a.normalizeUIKeys(i,k);var l=0;b.each(i,function(a,f){var g=f.match(e),i=g[1]+"."+[this.cid,d,l++," "].join(""),j=g[2],k=i+j,m=b.isFunction(a)?a:c[a];h[k]=b.bind(m,c)},this),f=b.extend(f,h)},this),f}};return b.extend(c,{behaviorsLookup:function(){throw new a.Error({message:"You must define where your behaviors are stored.",url:"marionette.behaviors.html#behaviorslookup"})},getBehaviorClass:function(b,d){return b.behaviorClass?b.behaviorClass:a._getValue(c.behaviorsLookup,this,[b,d])[d]},parseBehaviors:function(a,d){return b.chain(d).map(function(d,e){var f=c.getBehaviorClass(d,e),g=new f(d,a),h=c.parseBehaviors(a,b.result(g,"behaviors"));return[g].concat(h)}).flatten().value()},wrap:function(a,c,d){b.each(d,function(d){a[d]=b.partial(f[d],a[d],c)})}}),b.extend(d.prototype,{buildBehaviorTriggers:function(){return b.each(this._behaviors,this._buildTriggerHandlersForBehavior,this),this._triggers},_buildTriggerHandlersForBehavior:function(c,d){var e=b.extend({},this._viewUI,b.result(c,"ui")),f=b.clone(b.result(c,"triggers"))||{};f=a.normalizeUIKeys(f,e),b.each(f,b.bind(this._setHandlerForBehavior,this,c,d))},_setHandlerForBehavior:function(a,b,c,d){var e=d.replace(/^\S+/,function(a){return a+".behaviortriggers"+b});this._triggers[e]=this._view._buildViewTrigger(c)}}),c}(e,c),e.AppRouter=b.Router.extend({constructor:function(a){this.options=a||{},b.Router.apply(this,arguments);var c=this.getOption("appRoutes"),d=this._getController();this.processAppRoutes(d,c),this.on("route",this._processOnRoute,this)},appRoute:function(a,b){var c=this._getController();this._addAppRoute(c,a,b)},_processOnRoute:function(a,b){if(c.isFunction(this.onRoute)){var d=c.invert(this.getOption("appRoutes"))[a];this.onRoute(a,d,b)}},processAppRoutes:function(a,b){if(b){var d=c.keys(b).reverse();c.each(d,function(c){this._addAppRoute(a,c,b[c])},this)}},_getController:function(){return this.getOption("controller")},_addAppRoute:function(a,b,d){var f=a[d];if(!f)throw new e.Error('Method "'+d+'" was not found on the controller');this.route(b,d,c.bind(f,a))},getOption:e.proxyGetOption,triggerMethod:e.triggerMethod,bindEntityEvents:e.proxyBindEntityEvents,unbindEntityEvents:e.proxyUnbindEntityEvents}),e.Application=e.Object.extend({constructor:function(a){this._initializeRegions(a),this._initCallbacks=new e.Callbacks,this.submodules={},c.extend(this,a),this._initChannel(),e.Object.call(this,a)},execute:function(){this.commands.execute.apply(this.commands,arguments)},request:function(){return this.reqres.request.apply(this.reqres,arguments)},addInitializer:function(a){this._initCallbacks.add(a)},start:function(a){this.triggerMethod("before:start",a),this._initCallbacks.run(a,this),this.triggerMethod("start",a)},addRegions:function(a){return this._regionManager.addRegions(a)},emptyRegions:function(){return this._regionManager.emptyRegions()},removeRegion:function(a){return this._regionManager.removeRegion(a)},getRegion:function(a){return this._regionManager.get(a)},getRegions:function(){return this._regionManager.getRegions()},module:function(a,b){var d=e.Module.getClass(b),f=c.toArray(arguments);return f.unshift(this),d.create.apply(d,f)},getRegionManager:function(){return new e.RegionManager},_initializeRegions:function(a){var b=c.isFunction(this.regions)?this.regions(a):this.regions||{};this._initRegionManager();var d=e.getOption(a,"regions");return c.isFunction(d)&&(d=d.call(this,a)),c.extend(b,d),this.addRegions(b),this},_initRegionManager:function(){this._regionManager=this.getRegionManager(),this._regionManager._parent=this,this.listenTo(this._regionManager,"before:add:region",function(){e._triggerMethod(this,"before:add:region",arguments)}),this.listenTo(this._regionManager,"add:region",function(a,b){this[a]=b,e._triggerMethod(this,"add:region",arguments)}),this.listenTo(this._regionManager,"before:remove:region",function(){e._triggerMethod(this,"before:remove:region",arguments)}),this.listenTo(this._regionManager,"remove:region",function(a){delete this[a],e._triggerMethod(this,"remove:region",arguments)})},_initChannel:function(){this.channelName=c.result(this,"channelName")||"global",this.channel=c.result(this,"channel")||b.Wreqr.radio.channel(this.channelName),this.vent=c.result(this,"vent")||this.channel.vent,this.commands=c.result(this,"commands")||this.channel.commands,this.reqres=c.result(this,"reqres")||this.channel.reqres}}),e.Module=function(a,b,d){this.moduleName=a,this.options=c.extend({},this.options,d),this.initialize=d.initialize||this.initialize,this.submodules={},this._setupInitializersAndFinalizers(),this.app=b,c.isFunction(this.initialize)&&this.initialize(a,b,this.options)},e.Module.extend=e.extend,c.extend(e.Module.prototype,b.Events,{startWithParent:!0,initialize:function(){},addInitializer:function(a){this._initializerCallbacks.add(a)},addFinalizer:function(a){this._finalizerCallbacks.add(a)},start:function(a){this._isInitialized||(c.each(this.submodules,function(b){b.startWithParent&&b.start(a)}),this.triggerMethod("before:start",a),this._initializerCallbacks.run(a,this),this._isInitialized=!0,this.triggerMethod("start",a))},stop:function(){this._isInitialized&&(this._isInitialized=!1,this.triggerMethod("before:stop"),c.invoke(this.submodules,"stop"),this._finalizerCallbacks.run(void 0,this),this._initializerCallbacks.reset(),this._finalizerCallbacks.reset(),this.triggerMethod("stop"))},addDefinition:function(a,b){this._runModuleDefinition(a,b)},_runModuleDefinition:function(a,d){if(a){var f=c.flatten([this,this.app,b,e,b.$,c,d]);a.apply(this,f)}},_setupInitializersAndFinalizers:function(){this._initializerCallbacks=new e.Callbacks,this._finalizerCallbacks=new e.Callbacks},triggerMethod:e.triggerMethod}),c.extend(e.Module,{create:function(a,b,d){var e=a,f=c.rest(arguments,3);b=b.split(".");var g=b.length,h=[];return h[g-1]=d,c.each(b,function(b,c){var g=e;e=this._getModule(g,b,a,d),this._addModuleDefinition(g,e,h[c],f)},this),e},_getModule:function(a,b,d,e,f){var g=c.extend({},e),h=this.getClass(e),i=a[b];return i||(i=new h(b,d,g),a[b]=i,a.submodules[b]=i),i},getClass:function(a){var b=e.Module;return a?a.prototype instanceof b?a:a.moduleClass||b:b},_addModuleDefinition:function(a,b,c,d){var e=this._getDefine(c),f=this._getStartWithParent(c,b);e&&b.addDefinition(e,d),this._addStartWithParent(a,b,f)},_getStartWithParent:function(a,b){var d;return c.isFunction(a)&&a.prototype instanceof e.Module?(d=b.constructor.prototype.startWithParent,c.isUndefined(d)?!0:d):c.isObject(a)?(d=a.startWithParent,c.isUndefined(d)?!0:d):!0},_getDefine:function(a){return!c.isFunction(a)||a.prototype instanceof e.Module?c.isObject(a)?a.define:null:a},_addStartWithParent:function(a,b,c){b.startWithParent=b.startWithParent&&c,b.startWithParent&&!b.startWithParentIsConfigured&&(b.startWithParentIsConfigured=!0,a.addInitializer(function(a){b.startWithParent&&b.start(a)}))}}),e}),function(a){"function"==typeof define&&define.amd?define(["backbone","underscore"],a):"object"==typeof exports?module.exports=a(require("backbone"),require("underscore")):a(window.Backbone,window._)}(function(a,b){var c=a.Router.prototype.route,d=function(){};b.extend(a.Router.prototype,{before:d,after:d,route:function(a,e,f){f||(f=this[e]);var g=b.bind(function(){var c,e=[a,b.toArray(arguments)];if(c=b.isFunction(this.before)?this.before:"undefined"!=typeof this.before[a]?this.before[a]:d,c.apply(this,e)!==!1){f&&f.apply(this,arguments);var g;g=b.isFunction(this.after)?this.after:"undefined"!=typeof this.after[a]?this.after[a]:d,g.apply(this,e)}},this);return c.call(this,a,e,g)}})}),function(a,b,c,d,e,f,g){"use strict";"object"==typeof d?e.exports=f(c("underscore"),c("backbone"),c("jquery")):"function"==typeof b&&b.amd?b(["underscore","backbone","jquery"],function(b,c,d){return b=b===g?a._:b,c=c===g?a.Backbone:c,d=d===g?a.$:d,a.Backbone=f(b,c,d)}):a.returnExportsGlobal=f(a._,a.Backbone,a.$)}(this,this.define,this.require,this.exports,this.module,function(a,b,c,d){"use strict";var e=function(b){this.options=b!==d?b:{},this.namespaceDelimiter=b!==d&&b.namespaceDelimiter!==d?b.namespaceDelimiter:this.namespaceDelimiter,this.contentType=b!==d&&b.contentType!==d?b.contentType:this.contentType,a.bindAll(this)},f=b.Model.prototype.constructor,g=b.sync,h={};return e.prototype={options:{},charset:"iso-8859-1",namespace:"",namespaceDelimiter:"/",contentType:"application/json",url:null,responseID:null,exceptions:{404:{code:-1,message:"404"},500:{code:-2,message:"500"},typeMissmatch:{code:-3,message:"Type missmatch"},badResponseId:{code:-4,message:"Bad response ID"},noResponse:{code:-5,message:"No response"},noDefError:{code:-6,message:"No error defined"},renderError:function(a,b){return{code:b!==d?-7:b,message:a?"No error defined":a}}},onSuccess:function(b,c,e){if(a.isFunction(b)===!0){if(null===e||e===d)return this.handleExceptions(this.exceptions.noResponse),this;null!==e&&c!==String(e.id)&&this.handleExceptions(this.exceptions.badResponseId),b.apply(this,[e.result,e.error])}else this.onError(e)},onError:function(a,b){return null===b||b===d?(this.handleExceptions(this.exceptions.noResponse),this):void(null!==b.error&&d!==b.error?this.handleExceptions(b.error):this.handleExceptions(this.exceptions.noDefError))},query:function(b,e,f){var g=String((new Date).getTime()),h=null;return this.responseID=g,h=a.isArray(e)&&a.isString(b)?c.ajax({contentType:this.contentType+"; charset="+this.charset,type:"POST",dataType:"json",url:this.url,data:JSON.stringify({jsonrpc:"2.0",method:this.namespace+this.namespaceDelimiter+b,id:g,params:e}),statusCode:{404:a.bind(function(){this.handleExceptions(this.exceptions[404])},this),500:a.bind(function(){this.handleExceptions(this.exceptions[500])},this)},success:a.bind(function(a,b,c){null!==a&&a.error!==d?this.onError(f,a,b,c):this.onSuccess(f,g,a,b,c);
},this),error:a.bind(function(a,b,c){404!==a.status&&500!==a.status&&this.onError(f,a,b,c)},this)}):this.handleExceptions(this.exceptions.typeMissmatch)},checkMethods:function(c,e,f,g,i,j,k){var l=null,m=!1,n=null,o=[],p={},q=null;return g="delete"===g?"remove":g,a.isArray(f.methods[g])||a.isFunction(f.methods[g])?(a.isFunction(f.methods[g])?(a.isString(h[f.get("_rpcId")])||a.each(h[f.get("_rpcId")],function(a,b){f.get(b)!==a&&(p[b]=!0)}),h[f.get("_rpcId")]=f.toJSON(),l=a.bind(f.methods[g],f)(p,i)):l=f.methods[g],a.isArray(l[0])&&(m=!0),m!==!0?(q=a.clone(l),n=q.shift(),q.length>0?a.each(q,function(c){""===c?o.push(""):f instanceof b.Collection?f[c]!==d?a.isFunction(f[c])?o.push(f[c]()):o.push(f[c]):i[c]!==d&&o.push(i[c]):f.get(c)!==d?o.push(f.get(c)):i[c]!==d&&o.push(i[c])}):o=[],c(n,o,j,k)):(a.each(l,function(b){var d=a.clone(b);return n=null,o=[],n=d.shift(),a.each(d,function(a){o.push(f.get(a))}),c(n,o,j,k)}),null)):this.handleExceptions(this.exceptions.typeMissmatch)},invoke:function(c,e,f){var g={success:function(b){e.trigger("called:"+c,e,b),f!==d&&a.isFunction(f.success)&&f.success(e,b)},error:function(b,e){b.trigger("error",b,e),b.trigger("error:"+c,b,e),f!==d&&a.isFunction(f.error)&&f.error(b,e)}};return b.sync(c,e,g),this},defaultExceptionHandler:function(a){throw"Error code: "+a.code+" - message: "+a.message},handleExceptions:function(b){var c=a.isFunction(this.options.exceptionHandler)?this.options.exceptionHandler:this.defaultExceptionHandler;return c.call(this,b),this}},b.Rpc=e,b.Model=b.Model.extend({constructor:function(b){this.rpc!==d&&a.isFunction(this.rpc.invoke)===!0&&this.methods!==d&&a.each(this.methods,a.bind(function(b,c){1!=={read:1,create:1,remove:1,update:1}[c]&&(this[c]=a.bind(function(a){return this.rpc.invoke(c,this,a),this},this))},this)),f.apply(this,arguments)}}),b.sync=function(c){var e=null,f=function(g,i,j){var k=function(c,e){return null!==e&&e!==d?(j.error(i,e),this):(i instanceof b.Collection&&c!==d&&null!==c&&("object"==typeof c[0]?a.each(c,function(b,d){b._rpcId=a.uniqueId("rpc_"),c[d]=b,h[b._rpcId]=b}):a.each(c,function(a,b){h[b]=a})),i instanceof b.Model&&c!==d&&null!==c&&(c._rpcId=a.uniqueId("rpc_"),h[c._rpcId]=c),(c===d||null===c)&&(c=[]),i.parsers!==d&&i.parsers[g]!==d&&a.isFunction(i.parsers[g])&&i.parsers[g].apply(i,[c]),void j.success(c))},l=function(a){j.error(i,a)};if(i.rpc instanceof c){if(e=i.rpc,e.url=a.isFunction(i.url)?i.url():i.url,a.isString(i.namespace)===!0&&(e.namespace=i.namespace),i.methods===d)throw"Backbone.Rpc Error: No Method(s) given!";return"object"!=typeof i.params&&(i.params={}),e.checkMethods(e.query,i.params,i,g,j,k,l)}return f.previous.apply(i,arguments)};return f.previous=g,f}(e),b}),function(a,b){if("function"==typeof define&&define.amd)define(["underscore","backbone","jquery"],function(a,c,d){return b(a,c,d)});else if("undefined"!=typeof exports){var c=require("underscore"),d=require("backbone"),e=require("jquery");module.exports=b(c,d,e)}else b(a._,a.Backbone,a.jQuery)}(this,function(a,b,c){"use strict";var d=b.Syphon,e=b.Syphon={};e.VERSION="0.5.0",e.noConflict=function(){return b.Syphon=d,this},e.ignoredTypes=["button","submit","reset","fieldset"],e.serialize=function(b,d){var e={},h=i(d),k=f(b,h);return a.each(k,function(a){var b=c(a),d=g(b),f=h.keyExtractors.get(d),i=f(b),k=h.inputReaders.get(d),l=k(b),m=h.keyAssignmentValidators.get(d);if(m(b,i,l)){var n=h.keySplitter(i);e=j(e,n,l)}}),e},e.deserialize=function(b,d,e){var h=i(e),j=f(b,h),l=k(h,d);a.each(j,function(a){var b=c(a),d=g(b),e=h.keyExtractors.get(d),f=e(b),i=h.inputWriters.get(d),j=l[f];i(b,j)})};var f=function(b,d){var e=h(b),f=e.elements;return f=a.reject(f,function(b){var e,f=g(b),h=d.keyExtractors.get(f),i=h(c(b)),j=a.include(d.ignoredTypes,f),k=a.include(d.include,i),l=a.include(d.exclude,i);return e=k?!1:d.include?!0:l||j})},g=function(a){var b,d=c(a),e=d[0].tagName,f=e;return"input"===e.toLowerCase()&&(b=d.attr("type"),f=b?b:"text"),f.toLowerCase()},h=function(b){return a.isUndefined(b.$el)&&"form"===b.tagName.toLowerCase()?b:b.$el.is("form")?b.el:b.$("form")[0]},i=function(b){var c=a.clone(b)||{};return c.ignoredTypes=a.clone(e.ignoredTypes),c.inputReaders=c.inputReaders||e.InputReaders,c.inputWriters=c.inputWriters||e.InputWriters,c.keyExtractors=c.keyExtractors||e.KeyExtractors,c.keySplitter=c.keySplitter||e.KeySplitter,c.keyJoiner=c.keyJoiner||e.KeyJoiner,c.keyAssignmentValidators=c.keyAssignmentValidators||e.KeyAssignmentValidators,c},j=function(b,c,d){if(!c)return b;var e=c.shift();return b[e]||(b[e]=a.isArray(e)?[]:{}),0===c.length&&(a.isArray(b[e])?b[e].push(d):b[e]=d),c.length>0&&j(b[e],c,d),b},k=function(b,c,d){var e={};return a.each(c,function(c,f){var g={};d&&(f=b.keyJoiner(d,f)),a.isArray(c)?(f+="[]",g[f]=c):a.isObject(c)?g=k(b,c,f):g[f]=c,a.extend(e,g)}),e},l=e.TypeRegistry=function(){this.registeredTypes={}};l.extend=b.Model.extend,a.extend(l.prototype,{get:function(a){return this.registeredTypes[a]||this.registeredTypes["default"]},register:function(a,b){this.registeredTypes[a]=b},registerDefault:function(a){this.registeredTypes["default"]=a},unregister:function(a){this.registeredTypes[a]&&delete this.registeredTypes[a]}});var m=e.KeyExtractorSet=l.extend(),n=e.KeyExtractors=new m;n.registerDefault(function(a){return a.prop("name")||""});var o=e.InputReaderSet=l.extend(),p=e.InputReaders=new o;p.registerDefault(function(a){return a.val()}),p.register("checkbox",function(a){return a.prop("checked")});var q=e.InputWriterSet=l.extend(),r=e.InputWriters=new q;r.registerDefault(function(a,b){a.val(b)}),r.register("checkbox",function(a,b){a.prop("checked",b)}),r.register("radio",function(a,b){a.prop("checked",a.val()===b.toString())});var s=e.KeyAssignmentValidatorSet=l.extend(),t=e.KeyAssignmentValidators=new s;return t.registerDefault(function(){return!0}),t.register("radio",function(a,b,c){return a.prop("checked")}),e.KeySplitter=function(a){var b,c=a.match(/[^\[\]]+/g);return a.indexOf("[]")===a.length-2&&(b=c.pop(),c.push([b])),c},e.KeyJoiner=function(a,b){return a+"["+b+"]"},b.Syphon}),function(a,b){if("function"==typeof define&&define.amd)define(["backbone","underscore"],function(a,c){return b(a,c)});else if("undefined"!=typeof exports){var c=require("backbone"),d=require("underscore");module.exports=b(c,d)}else b(a.Backbone,a._)}(this,function(a,b){"use strict";var c=a.Wreqr,d=a.Wreqr={};return a.Wreqr.VERSION="1.3.2",a.Wreqr.noConflict=function(){return a.Wreqr=c,this},d.Handlers=function(a,b){var c=function(a){this.options=a,this._wreqrHandlers={},b.isFunction(this.initialize)&&this.initialize(a)};return c.extend=a.Model.extend,b.extend(c.prototype,a.Events,{setHandlers:function(a){b.each(a,function(a,c){var d=null;b.isObject(a)&&!b.isFunction(a)&&(d=a.context,a=a.callback),this.setHandler(c,a,d)},this)},setHandler:function(a,b,c){var d={callback:b,context:c};this._wreqrHandlers[a]=d,this.trigger("handler:add",a,b,c)},hasHandler:function(a){return!!this._wreqrHandlers[a]},getHandler:function(a){var b=this._wreqrHandlers[a];if(b)return function(){return b.callback.apply(b.context,arguments)}},removeHandler:function(a){delete this._wreqrHandlers[a]},removeAllHandlers:function(){this._wreqrHandlers={}}}),c}(a,b),d.CommandStorage=function(){var c=function(a){this.options=a,this._commands={},b.isFunction(this.initialize)&&this.initialize(a)};return b.extend(c.prototype,a.Events,{getCommands:function(a){var b=this._commands[a];return b||(b={command:a,instances:[]},this._commands[a]=b),b},addCommand:function(a,b){var c=this.getCommands(a);c.instances.push(b)},clearCommands:function(a){var b=this.getCommands(a);b.instances=[]}}),c}(),d.Commands=function(a,b){return a.Handlers.extend({storageType:a.CommandStorage,constructor:function(b){this.options=b||{},this._initializeStorage(this.options),this.on("handler:add",this._executeCommands,this),a.Handlers.prototype.constructor.apply(this,arguments)},execute:function(a){a=arguments[0];var c=b.rest(arguments);this.hasHandler(a)?this.getHandler(a).apply(this,c):this.storage.addCommand(a,c)},_executeCommands:function(a,c,d){var e=this.storage.getCommands(a);b.each(e.instances,function(a){c.apply(d,a)}),this.storage.clearCommands(a)},_initializeStorage:function(a){var c,d=a.storageType||this.storageType;c=b.isFunction(d)?new d:d,this.storage=c}})}(d,b),d.RequestResponse=function(a,b){return a.Handlers.extend({request:function(a){return this.hasHandler(a)?this.getHandler(a).apply(this,b.rest(arguments)):void 0}})}(d,b),d.EventAggregator=function(a,b){var c=function(){};return c.extend=a.Model.extend,b.extend(c.prototype,a.Events),c}(a,b),d.Channel=function(c){var d=function(b){this.vent=new a.Wreqr.EventAggregator,this.reqres=new a.Wreqr.RequestResponse,this.commands=new a.Wreqr.Commands,this.channelName=b};return b.extend(d.prototype,{reset:function(){return this.vent.off(),this.vent.stopListening(),this.reqres.removeAllHandlers(),this.commands.removeAllHandlers(),this},connectEvents:function(a,b){return this._connect("vent",a,b),this},connectCommands:function(a,b){return this._connect("commands",a,b),this},connectRequests:function(a,b){return this._connect("reqres",a,b),this},_connect:function(a,c,d){if(c){d=d||this;var e="vent"===a?"on":"setHandler";b.each(c,function(c,f){this[a][e](f,b.bind(c,d))},this)}}}),d}(d),d.radio=function(a,b){var c=function(){this._channels={},this.vent={},this.commands={},this.reqres={},this._proxyMethods()};b.extend(c.prototype,{channel:function(a){if(!a)throw new Error("Channel must receive a name");return this._getChannel(a)},_getChannel:function(b){var c=this._channels[b];return c||(c=new a.Channel(b),this._channels[b]=c),c},_proxyMethods:function(){b.each(["vent","commands","reqres"],function(a){b.each(d[a],function(b){this[a][b]=e(this,a,b)},this)},this)}});var d={vent:["on","off","trigger","once","stopListening","listenTo","listenToOnce"],commands:["execute","setHandler","setHandlers","removeHandler","removeAllHandlers"],reqres:["request","setHandler","setHandlers","removeHandler","removeAllHandlers"]},e=function(a,c,d){return function(e){var f=a._getChannel(e)[c];return f[d].apply(f,b.rest(arguments))}};return new c}(d,b),a.Wreqr});var Base64={_keyStr:"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=",encode:function(a){var b,c,d,e,f,g,h,i="",j=0;for(a=Base64._utf8_encode(a);j<a.length;)b=a.charCodeAt(j++),c=a.charCodeAt(j++),d=a.charCodeAt(j++),e=b>>2,f=(3&b)<<4|c>>4,g=(15&c)<<2|d>>6,h=63&d,isNaN(c)?g=h=64:isNaN(d)&&(h=64),i=i+this._keyStr.charAt(e)+this._keyStr.charAt(f)+this._keyStr.charAt(g)+this._keyStr.charAt(h);return i},decode:function(a){var b,c,d,e,f,g,h,i="",j=0;for(a=a.replace(/[^A-Za-z0-9\+\/\=]/g,"");j<a.length;)e=this._keyStr.indexOf(a.charAt(j++)),f=this._keyStr.indexOf(a.charAt(j++)),g=this._keyStr.indexOf(a.charAt(j++)),h=this._keyStr.indexOf(a.charAt(j++)),b=e<<2|f>>4,c=(15&f)<<4|g>>2,d=(3&g)<<6|h,i+=String.fromCharCode(b),64!=g&&(i+=String.fromCharCode(c)),64!=h&&(i+=String.fromCharCode(d));return i=Base64._utf8_decode(i)},_utf8_encode:function(a){a=a.replace(/\r\n/g,"\n");for(var b="",c=0;c<a.length;c++){var d=a.charCodeAt(c);128>d?b+=String.fromCharCode(d):d>127&&2048>d?(b+=String.fromCharCode(d>>6|192),b+=String.fromCharCode(63&d|128)):(b+=String.fromCharCode(d>>12|224),b+=String.fromCharCode(d>>6&63|128),b+=String.fromCharCode(63&d|128))}return b},_utf8_decode:function(a){for(var b="",c=0,d=c1=c2=0;c<a.length;)d=a.charCodeAt(c),128>d?(b+=String.fromCharCode(d),c++):d>191&&224>d?(c2=a.charCodeAt(c+1),b+=String.fromCharCode((31&d)<<6|63&c2),c+=2):(c2=a.charCodeAt(c+1),c3=a.charCodeAt(c+2),b+=String.fromCharCode((15&d)<<12|(63&c2)<<6|63&c3),c+=3);return b}};!function(a){var b={onBeforeRender:function(){this._isRendering=!0},onRender:function(){this.footer?this.footerElement=this.$el.find(this.footer)[0]:this.footerElement=null,delete this._isRendering},appendHtml:function(a,b,c){function d(b){for(var c=1,d=e(b+1);!d&&b+c+1<a.collection.length-1;)c+=1,d=e(b+c);return d}function e(b){if(!(b>=a.collection.length)){var c=a.children.findByModel(a.collection.at(b));return c}}var f=this.footerElement,g=a.itemViewContainer||a.el,h=a.itemViewContainer?$(a.itemViewContainer):a.$el;if(this._isRendering)return void(f?b.$el.insertBefore(f):h.append(b.el));var i;return 0===c?(i=d(0),void(i?b.$el.insertBefore(i.el):f?b.$el.insertBefore(f):b.$el.appendTo(g))):c==a.collection.length-1?void(f?b.$el.insertBefore(f):b.$el.appendTo(g)):(i=e(c-1),void(i?b.$el.insertAfter(i.$el):(i=d(c),i?b.$el.insertBefore(i.el):f?b.$el.insertBefore(f):b.$el.appendTo(g))))}};return a.SortedMixin=b,"function"==typeof define&&define.amd&&define([],function(){return b}),b}(window),function(a,b){function c(a){return l.PF.compile(a||"nplurals=2; plural=(n != 1);")}function d(a,b){this._key=a,this._i18n=b}var e=Array.prototype,f=Object.prototype,g=e.slice,h=f.hasOwnProperty,i=e.forEach,j={},k={forEach:function(a,b,c){var d,e,f;if(null!==a)if(i&&a.forEach===i)a.forEach(b,c);else if(a.length===+a.length){for(d=0,e=a.length;e>d;d++)if(d in a&&b.call(c,a[d],d,a)===j)return}else for(f in a)if(h.call(a,f)&&b.call(c,a[f],f,a)===j)return},extend:function(a){return this.forEach(g.call(arguments,1),function(b){for(var c in b)a[c]=b[c]}),a}},l=function(a){if(this.defaults={locale_data:{messages:{"":{domain:"messages",lang:"en",plural_forms:"nplurals=2; plural=(n != 1);"}}},domain:"messages",debug:!1},this.options=k.extend({},this.defaults,a),this.textdomain(this.options.domain),a.domain&&!this.options.locale_data[this.options.domain])throw new Error("Text domain set to non-existent domain: `"+a.domain+"`")};l.context_delimiter=String.fromCharCode(4),k.extend(d.prototype,{onDomain:function(a){return this._domain=a,this},withContext:function(a){return this._context=a,this},ifPlural:function(a,b){return this._val=a,this._pkey=b,this},fetch:function(a){return"[object Array]"!={}.toString.call(a)&&(a=[].slice.call(arguments,0)),(a&&a.length?l.sprintf:function(a){return a})(this._i18n.dcnpgettext(this._domain,this._context,this._key,this._pkey,this._val),a)}}),k.extend(l.prototype,{translate:function(a){return new d(a,this)},textdomain:function(a){return a?void(this._textdomain=a):this._textdomain},gettext:function(a){return this.dcnpgettext.call(this,b,b,a)},dgettext:function(a,c){return this.dcnpgettext.call(this,a,b,c)},dcgettext:function(a,c){return this.dcnpgettext.call(this,a,b,c)},ngettext:function(a,c,d){return this.dcnpgettext.call(this,b,b,a,c,d)},dngettext:function(a,c,d,e){return this.dcnpgettext.call(this,a,b,c,d,e)},dcngettext:function(a,c,d,e){return this.dcnpgettext.call(this,a,b,c,d,e)},pgettext:function(a,c){return this.dcnpgettext.call(this,b,a,c)},dpgettext:function(a,b,c){return this.dcnpgettext.call(this,a,b,c)},dcpgettext:function(a,b,c){return this.dcnpgettext.call(this,a,b,c)},npgettext:function(a,c,d,e){return this.dcnpgettext.call(this,b,a,c,d,e)},dnpgettext:function(a,b,c,d,e){return this.dcnpgettext.call(this,a,b,c,d,e)},dcnpgettext:function(a,b,d,e,f){e=e||d,a=a||this._textdomain;var g;if(!this.options)return g=new l,g.dcnpgettext.call(g,void 0,void 0,d,e,f);if(!this.options.locale_data)throw new Error("No locale data provided.");if(!this.options.locale_data[a])throw new Error("Domain `"+a+"` was not found.");if(!this.options.locale_data[a][""])throw new Error("No locale meta information provided.");if(!d)throw new Error("No translation key found.");var h,i,j,k=b?b+l.context_delimiter+d:d,m=this.options.locale_data,n=m[a],o=(m.messages||this.defaults.locale_data.messages)[""],p=n[""].plural_forms||n[""]["Plural-Forms"]||n[""]["plural-forms"]||o.plural_forms||o["Plural-Forms"]||o["plural-forms"];if(void 0===f)j=0;else{if("number"!=typeof f&&(f=parseInt(f,10),isNaN(f)))throw new Error("The number that was passed in is not a number.");j=c(p)(f)}if(!n)throw new Error("No domain named `"+a+"` could be found.");return h=n[k],!h||j>h.length?(this.options.missing_key_callback&&this.options.missing_key_callback(k,a),i=[d,e],this.options.debug===!0&&console.log(i[c(p)(f)]),i[c()(f)]):(i=h[j],i?i:(i=[d,e],i[c()(f)]))}});var m=function(){function a(a){return Object.prototype.toString.call(a).slice(8,-1).toLowerCase()}function b(a,b){for(var c=[];b>0;c[--b]=a);return c.join("")}var c=function(){return c.cache.hasOwnProperty(arguments[0])||(c.cache[arguments[0]]=c.parse(arguments[0])),c.format.call(null,c.cache[arguments[0]],arguments)};return c.format=function(c,d){var e,f,g,h,i,j,k,l=1,n=c.length,o="",p=[];for(f=0;n>f;f++)if(o=a(c[f]),"string"===o)p.push(c[f]);else if("array"===o){if(h=c[f],h[2])for(e=d[l],g=0;g<h[2].length;g++){if(!e.hasOwnProperty(h[2][g]))throw m('[sprintf] property "%s" does not exist',h[2][g]);e=e[h[2][g]]}else e=h[1]?d[h[1]]:d[l++];if(/[^s]/.test(h[8])&&"number"!=a(e))throw m("[sprintf] expecting number but found %s",a(e));switch(("undefined"==typeof e||null===e)&&(e=""),h[8]){case"b":e=e.toString(2);break;case"c":e=String.fromCharCode(e);break;case"d":e=parseInt(e,10);break;case"e":e=h[7]?e.toExponential(h[7]):e.toExponential();break;case"f":e=h[7]?parseFloat(e).toFixed(h[7]):parseFloat(e);break;case"o":e=e.toString(8);break;case"s":e=(e=String(e))&&h[7]?e.substring(0,h[7]):e;break;case"u":e=Math.abs(e);break;case"x":e=e.toString(16);break;case"X":e=e.toString(16).toUpperCase()}e=/[def]/.test(h[8])&&h[3]&&e>=0?"+"+e:e,j=h[4]?"0"==h[4]?"0":h[4].charAt(1):" ",k=h[6]-String(e).length,i=h[6]?b(j,k):"",p.push(h[5]?e+i:i+e)}return p.join("")},c.cache={},c.parse=function(a){for(var b=a,c=[],d=[],e=0;b;){if(null!==(c=/^[^\x25]+/.exec(b)))d.push(c[0]);else if(null!==(c=/^\x25{2}/.exec(b)))d.push("%");else{if(null===(c=/^\x25(?:([1-9]\d*)\$|\(([^\)]+)\))?(\+)?(0|'[^$])?(-)?(\d+)?(?:\.(\d+))?([b-fosuxX])/.exec(b)))throw"[sprintf] huh?";if(c[2]){e|=1;var f=[],g=c[2],h=[];if(null===(h=/^([a-z_][a-z_\d]*)/i.exec(g)))throw"[sprintf] huh?";for(f.push(h[1]);""!==(g=g.substring(h[0].length));)if(null!==(h=/^\.([a-z_][a-z_\d]*)/i.exec(g)))f.push(h[1]);else{if(null===(h=/^\[(\d+)\]/.exec(g)))throw"[sprintf] huh?";f.push(h[1])}c[2]=f}else e|=2;if(3===e)throw"[sprintf] mixing positional and named placeholders is not (yet) supported";d.push(c)}b=b.substring(c[0].length)}return d},c}(),n=function(a,b){return b.unshift(a),m.apply(null,b)};l.parse_plural=function(a,b){return a=a.replace(/n/g,b),l.parse_expression(a)},l.sprintf=function(a,b){return"[object Array]"=={}.toString.call(b)?n(a,[].slice.call(b)):m.apply(this,[].slice.call(arguments))},l.prototype.sprintf=function(){return l.sprintf.apply(this,arguments)},l.PF={},l.PF.parse=function(a){var b=l.PF.extractPluralExpr(a);return l.PF.parser.parse.call(l.PF.parser,b)},l.PF.compile=function(a){function b(a){return a===!0?1:a?a:0}var c=l.PF.parse(a);return function(a){return b(l.PF.interpreter(c)(a))}},l.PF.interpreter=function(a){return function(b){switch(a.type){case"GROUP":return l.PF.interpreter(a.expr)(b);case"TERNARY":return l.PF.interpreter(a.expr)(b)?l.PF.interpreter(a.truthy)(b):l.PF.interpreter(a.falsey)(b);case"OR":return l.PF.interpreter(a.left)(b)||l.PF.interpreter(a.right)(b);case"AND":return l.PF.interpreter(a.left)(b)&&l.PF.interpreter(a.right)(b);case"LT":return l.PF.interpreter(a.left)(b)<l.PF.interpreter(a.right)(b);case"GT":return l.PF.interpreter(a.left)(b)>l.PF.interpreter(a.right)(b);case"LTE":return l.PF.interpreter(a.left)(b)<=l.PF.interpreter(a.right)(b);case"GTE":return l.PF.interpreter(a.left)(b)>=l.PF.interpreter(a.right)(b);case"EQ":return l.PF.interpreter(a.left)(b)==l.PF.interpreter(a.right)(b);case"NEQ":return l.PF.interpreter(a.left)(b)!=l.PF.interpreter(a.right)(b);case"MOD":return l.PF.interpreter(a.left)(b)%l.PF.interpreter(a.right)(b);case"VAR":return b;case"NUM":return a.val;default:throw new Error("Invalid Token found.")}}},l.PF.extractPluralExpr=function(a){a=a.replace(/^\s\s*/,"").replace(/\s\s*$/,""),/;\s*$/.test(a)||(a=a.concat(";"));var b,c=/nplurals\=(\d+);/,d=/plural\=(.*);/,e=a.match(c),f={};if(!(e.length>1))throw new Error("nplurals not found in plural_forms string: "+a);if(f.nplurals=e[1],a=a.replace(c,""),b=a.match(d),!(b&&b.length>1))throw new Error("`plural` expression not found: "+a);return b[1]},l.PF.parser=function(){var a={trace:function(){},yy:{},symbols_:{error:2,expressions:3,e:4,EOF:5,"?":6,":":7,"||":8,"&&":9,"<":10,"<=":11,">":12,">=":13,"!=":14,"==":15,"%":16,"(":17,")":18,n:19,NUMBER:20,$accept:0,$end:1},terminals_:{2:"error",5:"EOF",6:"?",7:":",8:"||",9:"&&",10:"<",11:"<=",12:">",13:">=",14:"!=",15:"==",16:"%",17:"(",18:")",19:"n",20:"NUMBER"},productions_:[0,[3,2],[4,5],[4,3],[4,3],[4,3],[4,3],[4,3],[4,3],[4,3],[4,3],[4,3],[4,3],[4,1],[4,1]],performAction:function(a,b,c,d,e,f,g){var h=f.length-1;switch(e){case 1:return{type:"GROUP",expr:f[h-1]};case 2:this.$={type:"TERNARY",expr:f[h-4],truthy:f[h-2],falsey:f[h]};break;case 3:this.$={type:"OR",left:f[h-2],right:f[h]};break;case 4:this.$={type:"AND",left:f[h-2],right:f[h]};break;case 5:this.$={type:"LT",left:f[h-2],right:f[h]};break;case 6:this.$={type:"LTE",left:f[h-2],right:f[h]};break;case 7:this.$={type:"GT",left:f[h-2],right:f[h]};break;case 8:this.$={type:"GTE",left:f[h-2],right:f[h]};break;case 9:this.$={type:"NEQ",left:f[h-2],right:f[h]};break;case 10:this.$={type:"EQ",left:f[h-2],right:f[h]};break;case 11:this.$={type:"MOD",left:f[h-2],right:f[h]};break;case 12:this.$={type:"GROUP",expr:f[h-1]};break;case 13:this.$={type:"VAR"};break;case 14:this.$={type:"NUM",val:Number(a)}}},table:[{3:1,4:2,17:[1,3],19:[1,4],20:[1,5]},{1:[3]},{5:[1,6],6:[1,7],8:[1,8],9:[1,9],10:[1,10],11:[1,11],12:[1,12],13:[1,13],14:[1,14],15:[1,15],16:[1,16]},{4:17,17:[1,3],19:[1,4],20:[1,5]},{5:[2,13],6:[2,13],7:[2,13],8:[2,13],9:[2,13],10:[2,13],11:[2,13],12:[2,13],13:[2,13],14:[2,13],15:[2,13],16:[2,13],18:[2,13]},{5:[2,14],6:[2,14],7:[2,14],8:[2,14],9:[2,14],10:[2,14],11:[2,14],12:[2,14],13:[2,14],14:[2,14],15:[2,14],16:[2,14],18:[2,14]},{1:[2,1]},{4:18,17:[1,3],19:[1,4],20:[1,5]},{4:19,17:[1,3],19:[1,4],20:[1,5]},{4:20,17:[1,3],19:[1,4],20:[1,5]},{4:21,17:[1,3],19:[1,4],20:[1,5]},{4:22,17:[1,3],19:[1,4],20:[1,5]},{4:23,17:[1,3],19:[1,4],20:[1,5]},{4:24,17:[1,3],19:[1,4],20:[1,5]},{4:25,17:[1,3],19:[1,4],20:[1,5]},{4:26,17:[1,3],19:[1,4],20:[1,5]},{4:27,17:[1,3],19:[1,4],20:[1,5]},{6:[1,7],8:[1,8],9:[1,9],10:[1,10],11:[1,11],12:[1,12],13:[1,13],14:[1,14],15:[1,15],16:[1,16],18:[1,28]},{6:[1,7],7:[1,29],8:[1,8],9:[1,9],10:[1,10],11:[1,11],12:[1,12],13:[1,13],14:[1,14],15:[1,15],16:[1,16]},{5:[2,3],6:[2,3],7:[2,3],8:[2,3],9:[1,9],10:[1,10],11:[1,11],12:[1,12],13:[1,13],14:[1,14],15:[1,15],16:[1,16],18:[2,3]},{5:[2,4],6:[2,4],7:[2,4],8:[2,4],9:[2,4],10:[1,10],11:[1,11],12:[1,12],13:[1,13],14:[1,14],15:[1,15],16:[1,16],18:[2,4]},{5:[2,5],6:[2,5],7:[2,5],8:[2,5],9:[2,5],10:[2,5],11:[2,5],12:[2,5],13:[2,5],14:[2,5],15:[2,5],16:[1,16],18:[2,5]},{5:[2,6],6:[2,6],7:[2,6],8:[2,6],9:[2,6],10:[2,6],11:[2,6],12:[2,6],13:[2,6],14:[2,6],15:[2,6],16:[1,16],18:[2,6]},{5:[2,7],6:[2,7],7:[2,7],8:[2,7],9:[2,7],10:[2,7],11:[2,7],12:[2,7],13:[2,7],14:[2,7],15:[2,7],16:[1,16],18:[2,7]},{5:[2,8],6:[2,8],7:[2,8],8:[2,8],9:[2,8],10:[2,8],11:[2,8],12:[2,8],13:[2,8],14:[2,8],15:[2,8],16:[1,16],18:[2,8]},{5:[2,9],6:[2,9],7:[2,9],8:[2,9],9:[2,9],10:[2,9],11:[2,9],12:[2,9],13:[2,9],14:[2,9],15:[2,9],16:[1,16],18:[2,9]},{5:[2,10],6:[2,10],7:[2,10],8:[2,10],9:[2,10],10:[2,10],11:[2,10],12:[2,10],13:[2,10],14:[2,10],15:[2,10],16:[1,16],18:[2,10]},{5:[2,11],6:[2,11],7:[2,11],8:[2,11],9:[2,11],10:[2,11],11:[2,11],12:[2,11],13:[2,11],14:[2,11],15:[2,11],16:[2,11],18:[2,11]},{5:[2,12],6:[2,12],7:[2,12],8:[2,12],9:[2,12],10:[2,12],11:[2,12],12:[2,12],13:[2,12],14:[2,12],15:[2,12],16:[2,12],18:[2,12]},{4:30,17:[1,3],19:[1,4],20:[1,5]},{5:[2,2],6:[1,7],7:[2,2],8:[1,8],9:[1,9],10:[1,10],11:[1,11],12:[1,12],13:[1,13],14:[1,14],15:[1,15],16:[1,16],18:[2,2]}],defaultActions:{6:[2,1]},parseError:function(a,b){throw new Error(a)},parse:function(a){function b(a){e.length=e.length-2*a,f.length=f.length-a,g.length=g.length-a}function c(){var a;return a=d.lexer.lex()||1,"number"!=typeof a&&(a=d.symbols_[a]||a),a}var d=this,e=[0],f=[null],g=[],h=this.table,i="",j=0,k=0,l=0,m=2,n=1;this.lexer.setInput(a),this.lexer.yy=this.yy,this.yy.lexer=this.lexer,"undefined"==typeof this.lexer.yylloc&&(this.lexer.yylloc={});var o=this.lexer.yylloc;g.push(o),"function"==typeof this.yy.parseError&&(this.parseError=this.yy.parseError);for(var p,q,r,s,t,u,v,w,x,y={};;){if(r=e[e.length-1],this.defaultActions[r]?s=this.defaultActions[r]:(null==p&&(p=c()),s=h[r]&&h[r][p]),"undefined"==typeof s||!s.length||!s[0]){if(!l){x=[];for(u in h[r])this.terminals_[u]&&u>2&&x.push("'"+this.terminals_[u]+"'");var z="";z=this.lexer.showPosition?"Parse error on line "+(j+1)+":\n"+this.lexer.showPosition()+"\nExpecting "+x.join(", ")+", got '"+this.terminals_[p]+"'":"Parse error on line "+(j+1)+": Unexpected "+(1==p?"end of input":"'"+(this.terminals_[p]||p)+"'"),this.parseError(z,{text:this.lexer.match,token:this.terminals_[p]||p,line:this.lexer.yylineno,loc:o,expected:x})}if(3==l){if(p==n)throw new Error(z||"Parsing halted.");k=this.lexer.yyleng,i=this.lexer.yytext,j=this.lexer.yylineno,o=this.lexer.yylloc,p=c()}for(;;){if(m.toString()in h[r])break;if(0==r)throw new Error(z||"Parsing halted.");b(1),r=e[e.length-1]}q=p,p=m,r=e[e.length-1],s=h[r]&&h[r][m],l=3}if(s[0]instanceof Array&&s.length>1)throw new Error("Parse Error: multiple actions possible at state: "+r+", token: "+p);switch(s[0]){case 1:e.push(p),f.push(this.lexer.yytext),g.push(this.lexer.yylloc),e.push(s[1]),p=null,q?(p=q,q=null):(k=this.lexer.yyleng,i=this.lexer.yytext,j=this.lexer.yylineno,o=this.lexer.yylloc,l>0&&l--);break;case 2:if(v=this.productions_[s[1]][1],y.$=f[f.length-v],y._$={first_line:g[g.length-(v||1)].first_line,last_line:g[g.length-1].last_line,first_column:g[g.length-(v||1)].first_column,last_column:g[g.length-1].last_column},t=this.performAction.call(y,i,k,j,this.yy,s[1],f,g),"undefined"!=typeof t)return t;v&&(e=e.slice(0,-1*v*2),f=f.slice(0,-1*v),g=g.slice(0,-1*v)),e.push(this.productions_[s[1]][0]),f.push(y.$),g.push(y._$),w=h[e[e.length-2]][e[e.length-1]],e.push(w);break;case 3:return!0}}return!0}},b=function(){var a={EOF:1,parseError:function(a,b){if(!this.yy.parseError)throw new Error(a);this.yy.parseError(a,b)},setInput:function(a){return this._input=a,this._more=this._less=this.done=!1,this.yylineno=this.yyleng=0,this.yytext=this.matched=this.match="",this.conditionStack=["INITIAL"],this.yylloc={first_line:1,first_column:0,last_line:1,last_column:0},this},input:function(){var a=this._input[0];this.yytext+=a,this.yyleng++,this.match+=a,this.matched+=a;var b=a.match(/\n/);return b&&this.yylineno++,this._input=this._input.slice(1),a},unput:function(a){return this._input=a+this._input,this},more:function(){return this._more=!0,this},pastInput:function(){var a=this.matched.substr(0,this.matched.length-this.match.length);return(a.length>20?"...":"")+a.substr(-20).replace(/\n/g,"")},upcomingInput:function(){var a=this.match;return a.length<20&&(a+=this._input.substr(0,20-a.length)),(a.substr(0,20)+(a.length>20?"...":"")).replace(/\n/g,"")},showPosition:function(){var a=this.pastInput(),b=new Array(a.length+1).join("-");return a+this.upcomingInput()+"\n"+b+"^"},next:function(){if(this.done)return this.EOF;this._input||(this.done=!0);var a,b,c;this._more||(this.yytext="",this.match="");for(var d=this._currentRules(),e=0;e<d.length;e++)if(b=this._input.match(this.rules[d[e]]))return c=b[0].match(/\n.*/g),c&&(this.yylineno+=c.length),this.yylloc={first_line:this.yylloc.last_line,last_line:this.yylineno+1,first_column:this.yylloc.last_column,last_column:c?c[c.length-1].length-1:this.yylloc.last_column+b[0].length},this.yytext+=b[0],this.match+=b[0],this.matches=b,this.yyleng=this.yytext.length,this._more=!1,this._input=this._input.slice(b[0].length),this.matched+=b[0],a=this.performAction.call(this,this.yy,this,d[e],this.conditionStack[this.conditionStack.length-1]),a?a:void 0;return""===this._input?this.EOF:void this.parseError("Lexical error on line "+(this.yylineno+1)+". Unrecognized text.\n"+this.showPosition(),{text:"",token:null,line:this.yylineno})},lex:function(){var a=this.next();return"undefined"!=typeof a?a:this.lex()},begin:function(a){this.conditionStack.push(a)},popState:function(){return this.conditionStack.pop()},_currentRules:function(){return this.conditions[this.conditionStack[this.conditionStack.length-1]].rules},topState:function(){return this.conditionStack[this.conditionStack.length-2]},pushState:function(a){this.begin(a)}};return a.performAction=function(a,b,c,d){switch(c){case 0:break;case 1:return 20;case 2:return 19;case 3:return 8;case 4:return 9;case 5:return 6;case 6:return 7;case 7:return 11;case 8:return 13;case 9:return 10;case 10:return 12;case 11:return 14;case 12:return 15;case 13:return 16;case 14:return 17;case 15:return 18;case 16:return 5;case 17:return"INVALID"}},a.rules=[/^\s+/,/^[0-9]+(\.[0-9]+)?\b/,/^n\b/,/^\|\|/,/^&&/,/^\?/,/^:/,/^<=/,/^>=/,/^</,/^>/,/^!=/,/^==/,/^%/,/^\(/,/^\)/,/^$/,/^./],a.conditions={INITIAL:{rules:[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17],inclusive:!0}},a}();return a.lexer=b,a}(),"undefined"!=typeof exports?("undefined"!=typeof module&&module.exports&&(exports=module.exports=l),exports.Jed=l):("function"==typeof define&&define.amd&&define(function(){return l}),a.Jed=l)}(this),function(a){function b(a){return{jsonrpc:"2.0",method:a.method||"",params:a.params||{},id:g++}}function c(c){var d=a.isArray(c)?c.map(b):b(c);return JSON.stringify(d)}function d(a){return a.sort(e)}function e(a,b){return a.id<b.id?-1:1}function f(b,e){var f=new a.Deferred;e=e||{};var g=e.success||h,i=e.error||h;delete e.success,delete e.error;var j=a.isArray(b),k=a.extend({url:(j?b[0].url:b.url)||a.jsonrpc.defaultUrl,contentType:"application/json",dataType:"text",dataFilter:function(a,b){return JSON.parse(a)},type:"POST",processData:!1,data:c(b),success:function(a){if(j){var b=d(a);return g(b),void f.resolve(b)}if(a.hasOwnProperty("error"))return i(a.error),void f.reject(a.error);if(a.hasOwnProperty("result"))return g(a.result),void f.resolve(a.result);throw"Invalid response returned"},error:function(a,b,c){var d=null;if("timeout"===c)d={status:b,code:-32e3,message:"Request Timeout",data:null};else try{var e=JSON.parse(a.responseText);d=e.error}catch(g){d={status:b,code:-32603,message:c,data:a.responseText}}i(d),f.reject(d)}},e);return a.ajax(k),f.promise()}var g=1,h=function(){};a.extend({jsonrpc:f}),a.jsonrpc.defaultUrl="/jsonrpc"}(jQuery),!function(a,b,c,d){var e=a(b);a.fn.lazyload=function(f){function g(){var b=0;i.each(function(){var c=a(this);if(!j.skip_invisible||c.is(":visible"))if(a.abovethetop(this,j)||a.leftofbegin(this,j));else if(a.belowthefold(this,j)||a.rightoffold(this,j)){if(++b>j.failure_limit)return!1}else c.trigger("appear"),b=0})}var h,i=this,j={threshold:0,failure_limit:0,event:"scroll",effect:"show",container:b,data_attribute:"original",skip_invisible:!0,appear:null,load:null,placeholder:"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsQAAA7EAZUrDhsAAAANSURBVBhXYzh8+PB/AAffA0nNPuCLAAAAAElFTkSuQmCC"};return f&&(d!==f.failurelimit&&(f.failure_limit=f.failurelimit,delete f.failurelimit),d!==f.effectspeed&&(f.effect_speed=f.effectspeed,delete f.effectspeed),a.extend(j,f)),h=j.container===d||j.container===b?e:a(j.container),0===j.event.indexOf("scroll")&&h.bind(j.event,function(){return g()}),this.each(function(){var b=this,c=a(b);b.loaded=!1,(c.attr("src")===d||c.attr("src")===!1)&&c.is("img")&&c.attr("src",j.placeholder),c.one("appear",function(){if(!this.loaded){if(j.appear){var d=i.length;j.appear.call(b,d,j)}a("<img />").bind("load",function(){var d=c.attr("data-"+j.data_attribute);c.hide(),c.is("img")?c.attr("src",d):c.css("background-image","url('"+d+"')"),c[j.effect](j.effect_speed),b.loaded=!0;var e=a.grep(i,function(a){return!a.loaded});if(i=a(e),j.load){var f=i.length;j.load.call(b,f,j)}}).attr("src",c.attr("data-"+j.data_attribute))}}),0!==j.event.indexOf("scroll")&&c.bind(j.event,function(){
b.loaded||c.trigger("appear")})}),e.bind("resize",function(){g()}),/(?:iphone|ipod|ipad).*os 5/gi.test(navigator.appVersion)&&e.bind("pageshow",function(b){b.originalEvent&&b.originalEvent.persisted&&i.each(function(){a(this).trigger("appear")})}),a(c).ready(function(){g()}),this},a.belowthefold=function(c,f){var g;return g=f.container===d||f.container===b?(b.innerHeight?b.innerHeight:e.height())+e.scrollTop():a(f.container).offset().top+a(f.container).height(),g<=a(c).offset().top-f.threshold},a.rightoffold=function(c,f){var g;return g=f.container===d||f.container===b?e.width()+e.scrollLeft():a(f.container).offset().left+a(f.container).width(),g<=a(c).offset().left-f.threshold},a.abovethetop=function(c,f){var g;return g=f.container===d||f.container===b?e.scrollTop():a(f.container).offset().top,g>=a(c).offset().top+f.threshold+a(c).height()},a.leftofbegin=function(c,f){var g;return g=f.container===d||f.container===b?e.scrollLeft():a(f.container).offset().left,g>=a(c).offset().left+f.threshold+a(c).width()},a.inviewport=function(b,c){return!(a.rightoffold(b,c)||a.leftofbegin(b,c)||a.belowthefold(b,c)||a.abovethetop(b,c))},a.extend(a.expr[":"],{"below-the-fold":function(b){return a.belowthefold(b,{threshold:0})},"above-the-top":function(b){return!a.belowthefold(b,{threshold:0})},"right-of-screen":function(b){return a.rightoffold(b,{threshold:0})},"left-of-screen":function(b){return!a.rightoffold(b,{threshold:0})},"in-viewport":function(b){return a.inviewport(b,{threshold:0})},"above-the-fold":function(b){return!a.belowthefold(b,{threshold:0})},"right-of-fold":function(b){return a.rightoffold(b,{threshold:0})},"left-of-fold":function(b){return!a.rightoffold(b,{threshold:0})}})}(jQuery,window,document),function(a){"use strict";function b(a,b){return Math.round(a/b)*b}function c(a){return"number"==typeof a&&!isNaN(a)&&isFinite(a)}function d(a){var b=Math.pow(10,7);return Number((Math.round(a*b)/b).toFixed(7))}function e(a,b,c){a.addClass(b),setTimeout(function(){a.removeClass(b)},c)}function f(a){return Math.max(Math.min(a,100),0)}function g(b){return a.isArray(b)?b:[b]}function h(a){var b=a.split(".");return b.length>1?b[1].length:0}function i(a,b){return 100/(b-a)}function j(a,b){return 100*b/(a[1]-a[0])}function k(a,b){return j(a,a[0]<0?b+Math.abs(a[0]):b-a[0])}function l(a,b){return b*(a[1]-a[0])/100+a[0]}function m(a,b){for(var c=1;a>=b[c];)c+=1;return c}function n(a,b,c){if(c>=a.slice(-1)[0])return 100;var d,e,f,g,h=m(c,a);return d=a[h-1],e=a[h],f=b[h-1],g=b[h],f+k([d,e],c)/i(f,g)}function o(a,b,c){if(c>=100)return a.slice(-1)[0];var d,e,f,g,h=m(c,b);return d=a[h-1],e=a[h],f=b[h-1],g=b[h],l([d,e],(c-f)*i(f,g))}function p(a,c,d,e){if(100===e)return e;var f,g,h=m(e,a);return d?(f=a[h-1],g=a[h],e-f>(g-f)/2?g:f):c[h-1]?a[h-1]+b(e-a[h-1],c[h-1]):e}function q(a,b,d){var e;if("number"==typeof b&&(b=[b]),"[object Array]"!==Object.prototype.toString.call(b))throw new Error("noUiSlider: 'range' contains invalid value.");if(e="min"===a?0:"max"===a?100:parseFloat(a),!c(e)||!c(b[0]))throw new Error("noUiSlider: 'range' value isn't numeric.");d.xPct.push(e),d.xVal.push(b[0]),e?d.xSteps.push(isNaN(b[1])?!1:b[1]):isNaN(b[1])||(d.xSteps[0]=b[1])}function r(a,b,c){return b?void(c.xSteps[a]=j([c.xVal[a],c.xVal[a+1]],b)/i(c.xPct[a],c.xPct[a+1])):!0}function s(a,b,c,d){this.xPct=[],this.xVal=[],this.xSteps=[d||!1],this.xNumSteps=[!1],this.snap=b,this.direction=c;var e,f=[];for(e in a)a.hasOwnProperty(e)&&f.push([a[e],e]);for(f.sort(function(a,b){return a[0]-b[0]}),e=0;e<f.length;e++)q(f[e][1],f[e][0],this);for(this.xNumSteps=this.xSteps.slice(0),e=0;e<this.xNumSteps.length;e++)r(e,this.xNumSteps[e],this)}function t(a,b){if(!c(b))throw new Error("noUiSlider: 'step' is not numeric.");a.singleStep=b}function u(b,c){if("object"!=typeof c||a.isArray(c))throw new Error("noUiSlider: 'range' is not an object.");if(void 0===c.min||void 0===c.max)throw new Error("noUiSlider: Missing 'min' or 'max' in 'range'.");b.spectrum=new s(c,b.snap,b.dir,b.singleStep)}function v(b,c){if(c=g(c),!a.isArray(c)||!c.length||c.length>2)throw new Error("noUiSlider: 'start' option is incorrect.");b.handles=c.length,b.start=c}function w(a,b){if(a.snap=b,"boolean"!=typeof b)throw new Error("noUiSlider: 'snap' option must be a boolean.")}function x(a,b){if(a.animate=b,"boolean"!=typeof b)throw new Error("noUiSlider: 'animate' option must be a boolean.")}function y(a,b){if("lower"===b&&1===a.handles)a.connect=1;else if("upper"===b&&1===a.handles)a.connect=2;else if(b===!0&&2===a.handles)a.connect=3;else{if(b!==!1)throw new Error("noUiSlider: 'connect' option doesn't match handle count.");a.connect=0}}function z(a,b){switch(b){case"horizontal":a.ort=0;break;case"vertical":a.ort=1;break;default:throw new Error("noUiSlider: 'orientation' option is invalid.")}}function A(a,b){if(!c(b))throw new Error("noUiSlider: 'margin' option must be numeric.");if(a.margin=a.spectrum.getMargin(b),!a.margin)throw new Error("noUiSlider: 'margin' option is only supported on linear sliders.")}function B(a,b){if(!c(b))throw new Error("noUiSlider: 'limit' option must be numeric.");if(a.limit=a.spectrum.getMargin(b),!a.limit)throw new Error("noUiSlider: 'limit' option is only supported on linear sliders.")}function C(a,b){switch(b){case"ltr":a.dir=0;break;case"rtl":a.dir=1,a.connect=[0,2,1,3][a.connect];break;default:throw new Error("noUiSlider: 'direction' option was not recognized.")}}function D(a,b){if("string"!=typeof b)throw new Error("noUiSlider: 'behaviour' must be a string containing options.");var c=b.indexOf("tap")>=0,d=b.indexOf("drag")>=0,e=b.indexOf("fixed")>=0,f=b.indexOf("snap")>=0;a.events={tap:c||f,drag:d,fixed:e,snap:f}}function E(a,b){if(a.format=b,"function"==typeof b.to&&"function"==typeof b.from)return!0;throw new Error("noUiSlider: 'format' requires 'to' and 'from' methods.")}function F(b){var c,d={margin:0,limit:0,animate:!0,format:V};return c={step:{r:!1,t:t},start:{r:!0,t:v},connect:{r:!0,t:y},direction:{r:!0,t:C},snap:{r:!1,t:w},animate:{r:!1,t:x},range:{r:!0,t:u},orientation:{r:!1,t:z},margin:{r:!1,t:A},limit:{r:!1,t:B},behaviour:{r:!0,t:D},format:{r:!1,t:E}},b=a.extend({connect:!1,direction:"ltr",behaviour:"tap",orientation:"horizontal"},b),a.each(c,function(a,c){if(void 0===b[a]){if(c.r)throw new Error("noUiSlider: '"+a+"' is required.");return!0}c.t(d,b[a])}),d.style=d.ort?"top":"left",d}function G(a,b,c){var d=a+b[0],e=a+b[1];return c?(0>d&&(e+=Math.abs(d)),e>100&&(d-=e-100),[f(d),f(e)]):[d,e]}function H(a){a.preventDefault();var b,c,d=0===a.type.indexOf("touch"),e=0===a.type.indexOf("mouse"),f=0===a.type.indexOf("pointer"),g=a;return 0===a.type.indexOf("MSPointer")&&(f=!0),a.originalEvent&&(a=a.originalEvent),d&&(b=a.changedTouches[0].pageX,c=a.changedTouches[0].pageY),(e||f)&&(f||void 0!==window.pageXOffset||(window.pageXOffset=document.documentElement.scrollLeft,window.pageYOffset=document.documentElement.scrollTop),b=a.clientX+window.pageXOffset,c=a.clientY+window.pageYOffset),g.points=[b,c],g.cursor=e,g}function I(b,c){var d=a("<div><div/></div>").addClass(U[2]),e=["-lower","-upper"];return b&&e.reverse(),d.children().addClass(U[3]+" "+U[3]+e[c]),d}function J(a,b,c){switch(a){case 1:b.addClass(U[7]),c[0].addClass(U[6]);break;case 3:c[1].addClass(U[6]);case 2:c[0].addClass(U[7]);case 0:b.addClass(U[6])}}function K(a,b,c){var d,e=[];for(d=0;a>d;d+=1)e.push(I(b,d).appendTo(c));return e}function L(b,c,d){return d.addClass([U[0],U[8+b],U[4+c]].join(" ")),a("<div/>").appendTo(d).addClass(U[1])}function M(b,c,d){function i(){return C[["width","height"][c.ort]]()}function j(a){var b,c=[E.val()];for(b=0;b<a.length;b+=1)E.trigger(a[b],c)}function k(a){return 1===a.length?a[0]:c.dir?a.reverse():a}function l(a){return function(b,c){E.val([a?null:c,a?c:null],!0)}}function m(b){var c=a.inArray(b,N);E[0].linkAPI&&E[0].linkAPI[b]&&E[0].linkAPI[b].change(M[c],D[c].children(),E)}function n(b,d){var e=a.inArray(b,N);return d&&d.appendTo(D[e].children()),c.dir&&c.handles>1&&(e=1===e?0:1),l(e)}function o(){var a,b;for(a=0;a<N.length;a+=1)this.linkAPI&&this.linkAPI[b=N[a]]&&this.linkAPI[b].reconfirm(b)}function p(a,b,d,e){return a=a.replace(/\s/g,S+" ")+S,b.on(a,function(a){return E.attr("disabled")?!1:E.hasClass(U[14])?!1:(a=H(a),a.calcPoint=a.points[c.ort],void d(a,e))})}function q(a,b){var c,d=b.handles||D,e=!1,f=100*(a.calcPoint-b.start)/i(),g=d[0][0]!==D[0][0]?1:0;c=G(f,b.positions,d.length>1),e=v(d[0],c[g],1===d.length),d.length>1&&(e=v(d[1],c[g?0:1],!1)||e),e&&j(["slide"])}function r(b){a("."+U[15]).removeClass(U[15]),b.cursor&&a("body").css("cursor","").off(S),Q.off(S),E.removeClass(U[12]),j(["set","change"])}function s(b,c){1===c.handles.length&&c.handles[0].children().addClass(U[15]),b.stopPropagation(),p(T.move,Q,q,{start:b.calcPoint,handles:c.handles,positions:[F[0],F[D.length-1]]}),p(T.end,Q,r,null),b.cursor&&(a("body").css("cursor",a(b.target).css("cursor")),D.length>1&&E.addClass(U[12]),a("body").on("selectstart"+S,!1))}function t(b){var d,f=b.calcPoint,g=0;b.stopPropagation(),a.each(D,function(){g+=this.offset()[c.style]}),g=g/2>f||1===D.length?0:1,f-=C.offset()[c.style],d=100*f/i(),c.events.snap||e(E,U[14],300),v(D[g],d),j(["slide","set","change"]),c.events.snap&&s(b,{handles:[D[g]]})}function u(a){var b,c;if(!a.fixed)for(b=0;b<D.length;b+=1)p(T.start,D[b].children(),s,{handles:[D[b]]});a.tap&&p(T.start,C,t,{handles:D}),a.drag&&(c=C.find("."+U[7]).addClass(U[10]),a.fixed&&(c=c.add(C.children().not(c).children())),p(T.start,c,s,{handles:D}))}function v(a,b,d){var e=a[0]!==D[0][0]?1:0,g=F[0]+c.margin,h=F[1]-c.margin,i=F[0]+c.limit,j=F[1]-c.limit;return D.length>1&&(b=e?Math.max(b,g):Math.min(b,h)),d!==!1&&c.limit&&D.length>1&&(b=e?Math.min(b,i):Math.max(b,j)),b=I.getStep(b),b=f(parseFloat(b.toFixed(7))),b===F[e]?!1:(a.css(c.style,b+"%"),a.is(":first-child")&&a.toggleClass(U[17],b>50),F[e]=b,M[e]=I.fromStepping(b),m(N[e]),!0)}function w(a,b){var d,e,f;for(c.limit&&(a+=1),d=0;a>d;d+=1)e=d%2,f=b[e],null!==f&&f!==!1&&("number"==typeof f&&(f=String(f)),f=c.format.from(f),(f===!1||isNaN(f)||v(D[e],I.toStepping(f),d===3-c.dir)===!1)&&m(N[e]))}function x(a){if(E[0].LinkIsEmitting)return this;var b,d=g(a);return c.dir&&c.handles>1&&d.reverse(),c.animate&&-1!==F[0]&&e(E,U[14],300),b=D.length>1?3:1,1===d.length&&(b=1),w(b,d),j(["set"]),this}function y(){var a,b=[];for(a=0;a<c.handles;a+=1)b[a]=c.format.to(M[a]);return k(b)}function z(){return a(this).off(S).removeClass(U.join(" ")).empty(),delete this.LinkUpdate,delete this.LinkConfirm,delete this.LinkDefaultFormatter,delete this.LinkDefaultFlag,delete this.reappend,delete this.vGet,delete this.vSet,delete this.getCurrentStep,delete this.getInfo,delete this.destroy,d}function A(){var b=a.map(F,function(a,b){var c=I.getApplicableStep(a),d=h(String(c[2])),e=M[b],f=100===a?null:c[2],g=Number((e-c[2]).toFixed(d)),i=0===a?null:g>=c[1]?c[2]:c[0]||!1;return[[i,f]]});return k(b)}function B(){return d}var C,D,E=a(b),F=[-1,-1],I=c.spectrum,M=[],N=["lower","upper"].slice(0,c.handles);if(c.dir&&N.reverse(),b.LinkUpdate=m,b.LinkConfirm=n,b.LinkDefaultFormatter=c.format,b.LinkDefaultFlag="lower",b.reappend=o,E.hasClass(U[0]))throw new Error("Slider was already initialized.");C=L(c.dir,c.ort,E),D=K(c.handles,c.dir,C),J(c.connect,E,D),u(c.events),b.vSet=x,b.vGet=y,b.destroy=z,b.getCurrentStep=A,b.getOriginalOptions=B,b.getInfo=function(){return[I,c.style,c.ort]},E.val(c.start)}function N(a){var b=F(a,this);return this.each(function(){M(this,b,a)})}function O(b){return this.each(function(){if(!this.destroy)return void a(this).noUiSlider(b);var c=a(this).val(),d=this.destroy(),e=a.extend({},d,b);a(this).noUiSlider(e),this.reappend(),d.start===e.start&&a(this).val(c)})}function P(){return this[0][arguments.length?"vSet":"vGet"].apply(this[0],arguments)}var Q=a(document),R=a.fn.val,S=".nui",T=window.navigator.pointerEnabled?{start:"pointerdown",move:"pointermove",end:"pointerup"}:window.navigator.msPointerEnabled?{start:"MSPointerDown",move:"MSPointerMove",end:"MSPointerUp"}:{start:"mousedown touchstart",move:"mousemove touchmove",end:"mouseup touchend"},U=["noUi-target","noUi-base","noUi-origin","noUi-handle","noUi-horizontal","noUi-vertical","noUi-background","noUi-connect","noUi-ltr","noUi-rtl","noUi-dragable","","noUi-state-drag","","noUi-state-tap","noUi-active","","noUi-stacking"];s.prototype.getMargin=function(a){return 2===this.xPct.length?j(this.xVal,a):!1},s.prototype.toStepping=function(a){return a=n(this.xVal,this.xPct,a),this.direction&&(a=100-a),a},s.prototype.fromStepping=function(a){return this.direction&&(a=100-a),d(o(this.xVal,this.xPct,a))},s.prototype.getStep=function(a){return this.direction&&(a=100-a),a=p(this.xPct,this.xSteps,this.snap,a),this.direction&&(a=100-a),a},s.prototype.getApplicableStep=function(a){var b=m(a,this.xPct),c=100===a?2:1;return[this.xNumSteps[b-2],this.xVal[b-c],this.xNumSteps[b-c]]},s.prototype.convert=function(a){return this.getStep(this.toStepping(a))};var V={to:function(a){return a.toFixed(2)},from:Number};a.fn.val=function(b){function c(a){return a.hasClass(U[0])?P:R}if(!arguments.length){var d=a(this[0]);return c(d).call(d)}var e=a.isFunction(b);return this.each(function(d){var f=b,g=a(this);e&&(f=b.call(this,d,g.val())),c(g).call(g,f)})},a.fn.noUiSlider=function(a,b){switch(a){case"step":return this[0].getCurrentStep();case"options":return this[0].getOriginalOptions()}return(b?O:N).call(this,a)}}(window.jQuery||window.Zepto),function(a){"use strict";"object"==typeof exports?a(require("jquery")):"function"==typeof define&&define.amd?define(["jquery"],a):a(jQuery)}(function(a){"use strict";var b=function(a){if(a=a||"once","string"!=typeof a)throw new Error("The jQuery Once id parameter must be a string");return a};a.fn.once=function(c){var d="jquery-once-"+b(c);return this.filter(function(){return a(this).data(d)!==!0}).data(d,!0)},a.fn.removeOnce=function(a){return this.findOnce(a).removeData("jquery-once-"+b(a))},a.fn.findOnce=function(c){var d="jquery-once-"+b(c);return this.filter(function(){return a(this).data(d)===!0})}}),function(a){var b=a(window);a.fn.visible=function(a,c,d){if(!(this.length<1)){var e=this.length>1?this.eq(0):this,f=e.get(0),g=b.width(),h=b.height(),d=d?d:"both",i=c===!0?f.offsetWidth*f.offsetHeight:!0;if("function"==typeof f.getBoundingClientRect){var j=f.getBoundingClientRect(),k=j.top>=0&&j.top<h,l=j.bottom>0&&j.bottom<=h,m=j.left>=0&&j.left<g,n=j.right>0&&j.right<=g,o=a?k||l:k&&l,p=a?m||n:m&&n;if("both"===d)return i&&o&&p;if("vertical"===d)return i&&o;if("horizontal"===d)return i&&p}else{var q=b.scrollTop(),r=q+h,s=b.scrollLeft(),t=s+g,u=e.offset(),v=u.top,w=v+e.height(),x=u.left,y=x+e.width(),z=a===!0?w:v,A=a===!0?v:w,B=a===!0?y:x,C=a===!0?x:y;if("both"===d)return!!i&&r>=A&&z>=q&&t>=C&&B>=s;if("vertical"===d)return!!i&&r>=A&&z>=q;if("horizontal"===d)return!!i&&t>=C&&B>=s}}}}(jQuery),window.JST||(window.JST={});var prettyPrint=function(){var a={el:function(b,c){var d,e=document.createElement(b);if(c=a.merge({},c),c&&c.style){c.style;a.applyCSS(e,c.style),delete c.style}for(d in c)c.hasOwnProperty(d)&&(e[d]=c[d]);return e},applyCSS:function(a,b){for(var c in b)if(b.hasOwnProperty(c))try{a.style[c]=b[c]}catch(d){}},txt:function(a){return document.createTextNode(a)},row:function(b,c,d){d=d||"td";var e,f=a.count(b,null)+1,g=a.el("tr"),h={style:a.getStyles(d,c),colSpan:f,onmouseover:function(){var b=this.parentNode.childNodes;a.forEach(b,function(b){"td"===b.nodeName.toLowerCase()&&a.applyCSS(b,a.getStyles("td_hover",c))})},onmouseout:function(){var b=this.parentNode.childNodes;a.forEach(b,function(b){"td"===b.nodeName.toLowerCase()&&a.applyCSS(b,a.getStyles("td",c))})}};return a.forEach(b,function(b){null!==b&&(e=a.el(d,h),b.nodeType?e.appendChild(b):e.innerHTML=a.shorten(b.toString()),g.appendChild(e))}),g},hRow:function(b,c){return a.row(b,c,"th")},table:function(b,c){b=b||[];var d={thead:{style:a.getStyles("thead",c)},tbody:{style:a.getStyles("tbody",c)},table:{style:a.getStyles("table",c)}},e=a.el("table",d.table),f=a.el("thead",d.thead),g=a.el("tbody",d.tbody);return b.length&&(e.appendChild(f),f.appendChild(a.hRow(b,c))),e.appendChild(g),{node:e,tbody:g,thead:f,appendChild:function(a){this.tbody.appendChild(a)},addRow:function(b,d,e){return this.appendChild(a.row.call(a,b,d||c,e)),this}}},shorten:function(a){var b=40;return a=a.replace(/^\s\s*|\s\s*$|\n/g,""),a.length>b?a.substring(0,b-1)+"...":a},htmlentities:function(a){return a.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;")},merge:function(b,c){"object"!=typeof b&&(b={});for(var d in c)if(c.hasOwnProperty(d)){var e=c[d];if("object"==typeof e){b[d]=a.merge(b[d],e);continue}b[d]=e}for(var f=2,g=arguments.length;g>f;f++)a.merge(b,arguments[f]);return b},count:function(a,b){for(var c=0,d=0,e=a.length;e>d;d++)a[d]===b&&c++;return c},thead:function(a){return a.getElementsByTagName("thead")[0]},forEach:function(a,b,c){c||(c=b);for(var d=a.length,e=-1;++e<d&&c(a[e],e,a)!==!1;);return!0},type:function(a){try{if(null===a)return"null";if(void 0===a)return"undefined";var b=Object.prototype.toString.call(a).match(/\s(.+?)\]/)[1].toLowerCase();return a.nodeType?1===a.nodeType?"domelement":"domnode":/^(string|number|array|regexp|function|date|boolean)$/.test(b)?b:"object"==typeof a?a.jquery&&"string"==typeof a.jquery?"jquery":"object":a===window||a===document?"object":"default"}catch(c){return"default"}},within:function(a){return{is:function(b){for(var c in a)if(a[c]===b)return c;return""}}},common:{circRef:function(c,d,e){return a.expander("[POINTS BACK TO <strong>"+d+"</strong>]","Click to show this item anyway",function(){this.parentNode.appendChild(b(c,{maxDepth:1}))})},depthReached:function(c,d){return a.expander("[DEPTH REACHED]","Click to show this item anyway",function(){try{this.parentNode.appendChild(b(c,{maxDepth:1}))}catch(d){this.parentNode.appendChild(a.table(["ERROR OCCURED DURING OBJECT RETRIEVAL"],"error").addRow([d.message]).node)}})}},getStyles:function(c,d){return d=b.settings.styles[d]||{},a.merge({},b.settings.styles["default"][c],d[c])},expander:function(b,c,d){return a.el("a",{innerHTML:a.shorten(b)+' <b style="visibility:hidden;">[+]</b>',title:c,onmouseover:function(){this.getElementsByTagName("b")[0].style.visibility="visible"},onmouseout:function(){this.getElementsByTagName("b")[0].style.visibility="hidden"},onclick:function(){return this.style.display="none",d.call(this),!1},style:{cursor:"pointer"}})},stringify:function(b){var c,d=a.type(b),e=!0;if("array"===d)return c="[",a.forEach(b,function(b,d){c+=(0===d?"":", ")+a.stringify(b)}),c+"]";if("object"==typeof b){c="{";for(var f in b)b.hasOwnProperty(f)&&(c+=(e?"":", ")+f+":"+a.stringify(b[f]),e=!1);return c+"}"}return"regexp"===d?"/"+b.source+"/":"string"===d?'"'+b.replace(/"/g,'\\"')+'"':b.toString()},headerGradient:function(){var a=document.createElement("canvas");if(!a.getContext)return"";var b=a.getContext("2d");a.height=30,a.width=1;var c=b.createLinearGradient(0,0,0,30);c.addColorStop(0,"rgba(0,0,0,0)"),c.addColorStop(1,"rgba(0,0,0,0.25)"),b.fillStyle=c,b.fillRect(0,0,1,30);var d=a.toDataURL&&a.toDataURL();return"url("+(d||"")+")"}()},b=function(c,d){d=d||{};var e=a.merge({},b.config,d),f=a.el("div"),g=(b.config,0),h={},i=!1;b.settings=e;var j={string:function(b){return a.txt('"'+a.shorten(b.replace(/"/g,'\\"'))+'"')},number:function(b){return a.txt(b)},regexp:function(b){var c=a.table(["RegExp",null],"regexp"),d=a.table(),f=a.expander("/"+b.source+"/","Click to show more",function(){this.parentNode.appendChild(c.node)});return d.addRow(["g",b.global]).addRow(["i",b.ignoreCase]).addRow(["m",b.multiline]),c.addRow(["source","/"+b.source+"/"]).addRow(["flags",d.node]).addRow(["lastIndex",b.lastIndex]),e.expanded?c.node:f},domelement:function(b,c){var d=a.table(["DOMElement",null],"domelement"),f=["id","className","innerHTML","src","href"],g=b.nodeName||"";return d.addRow(["tag","&lt;"+g.toLowerCase()+"&gt;"]),a.forEach(f,function(c){b[c]&&d.addRow([c,a.htmlentities(b[c])])}),e.expanded?d.node:a.expander("DOMElement ("+g.toLowerCase()+")","Click to show more",function(){this.parentNode.appendChild(d.node)})},domnode:function(b){var c=a.table(["DOMNode",null],"domelement"),d=a.htmlentities((b.data||"UNDEFINED").replace(/\n/g,"\\n"));return c.addRow(["nodeType",b.nodeType+" ("+b.nodeName+")"]).addRow(["data",d]),e.expanded?c.node:a.expander("DOMNode","Click to show more",function(){this.parentNode.appendChild(c.node)})},jquery:function(a,b,c){return j.array(a,b,c,!0)},object:function(b,c,d){var f=a.within(h).is(b);if(f)return a.common.circRef(b,f,e);if(h[d||"TOP"]=b,c===e.maxDepth)return a.common.depthReached(b,e);var g=a.table(["Object",null],"object"),k=!0;for(var l in b)if(!b.hasOwnProperty||b.hasOwnProperty(l)){var m=b[l],n=a.type(m);k=!1;try{g.addRow([l,j[n](m,c+1,l)],n)}catch(o){window.console&&window.console.log&&console.log(o.message)}}k?g.addRow(["<small>[empty]</small>"]):g.thead.appendChild(a.hRow(["key","value"],"colHeader"));var p=e.expanded||i?g.node:a.expander(a.stringify(b),"Click to show more",function(){this.parentNode.appendChild(g.node)});return i=!0,p},array:function(b,c,d,f){var g=a.within(h).is(b);if(g)return a.common.circRef(b,g);if(h[d||"TOP"]=b,c===e.maxDepth)return a.common.depthReached(b);var i=f?"jQuery":"Array",k=a.table([i+"("+b.length+")",null],f?"jquery":i.toLowerCase()),l=!0,m=0;return f&&k.addRow(["selector",b.selector]),a.forEach(b,function(d,f){return e.maxArray>=0&&++m>e.maxArray?(k.addRow([f+".."+(b.length-1),j[a.type(d)]("...",c+1,f)]),!1):(l=!1,void k.addRow([f,j[a.type(d)](d,c+1,f)]))}),f||(l?k.addRow(["<small>[empty]</small>"]):k.thead.appendChild(a.hRow(["index","value"],"colHeader"))),e.expanded?k.node:a.expander(a.stringify(b),"Click to show more",function(){this.parentNode.appendChild(k.node)})},"function":function(b,c,d){var f=a.within(h).is(b);if(f)return a.common.circRef(b,f);h[d||"TOP"]=b;var g=a.table(["Function",null],"function"),i=(a.table(["Arguments"]),b.toString().match(/\((.+?)\)/)),j=b.toString().match(/\(.*?\)\s+?\{?([\S\s]+)/)[1].replace(/\}?$/,"");return g.addRow(["arguments",i?i[1].replace(/[^\w_,\s]/g,""):"<small>[none/native]</small>"]).addRow(["body",j]),e.expanded?g.node:a.expander("function(){...}","Click to see more about this function.",function(){this.parentNode.appendChild(g.node)})},date:function(b){var c=a.table(["Date",null],"date"),d=b.toString().split(/\s/);return c.addRow(["Time",d[4]]).addRow(["Date",d.slice(0,4).join("-")]),e.expanded?c.node:a.expander("Date (timestamp): "+ +b,"Click to see a little more info about this date",function(){this.parentNode.appendChild(c.node)})},"boolean":function(b){return a.txt(b.toString().toUpperCase())},undefined:function(){return a.txt("UNDEFINED")},"null":function(){return a.txt("NULL")},"default":function(){return a.txt("prettyPrint: TypeNotFound Error")}};return f.appendChild(j[e.forceObject?"object":a.type(c)](c,g)),f};return b.config={expanded:!0,forceObject:!1,maxDepth:3,maxArray:-1,styles:{array:{th:{backgroundColor:"#A4C18B",color:"white"}},"function":{th:{backgroundColor:"#D82525"}},regexp:{th:{backgroundColor:"#E2F3FB",color:"#000"}},object:{th:{backgroundColor:"#8DA3AD"}},jquery:{th:{backgroundColor:"#FBF315"}},error:{th:{backgroundColor:"red",color:"yellow"}},domelement:{th:{backgroundColor:"#F3801E"}},date:{th:{backgroundColor:"#A725D8"}},colHeader:{th:{backgroundColor:"#EEE",color:"#aaa",textTransform:"uppercase",fontSize:"80%",padding:"2px 5px"}},"default":{table:{borderCollapse:"collapse",width:"100%"},td:{padding:"5px",fontSize:"12px",backgroundColor:"rgba(255,255,255,0.5)",color:"#222",border:"1px solid #ddd",verticalAlign:"top",fontFamily:'"Consolas","Lucida Console",Courier,mono',whiteSpace:"nowrap"},td_hover:{},th:{padding:"5px",fontSize:"12px",backgroundColor:"#222",color:"#EEE",textAlign:"left",border:"1px solid #ddd",verticalAlign:"top",fontFamily:'"Consolas","Lucida Console",Courier,mono',backgroundRepeat:"repeat-x"}}}},b}();!function(a){"use strict";"function"==typeof define&&define.amd?define(a):"undefined"!=typeof module&&"undefined"!=typeof module.exports?module.exports=a():"undefined"!=typeof Package?Sortable=a():window.Sortable=a()}(function(){"use strict";function a(a,b){this.el=a,this.options=b=b||{};var d={group:Math.random(),sort:!0,disabled:!1,store:null,handle:null,scroll:!0,scrollSensitivity:30,scrollSpeed:10,draggable:/[uo]l/i.test(a.nodeName)?"li":">*",ghostClass:"sortable-ghost",ignore:"a, img",filter:null,animation:0,setData:function(a,b){a.setData("Text",b.textContent)},dropBubble:!1,dragoverBubble:!1};for(var e in d)!(e in b)&&(b[e]=d[e]);var g=b.group;g&&"object"==typeof g||(g=b.group={name:g}),["pull","put"].forEach(function(a){a in g||(g[a]=!0)}),M.forEach(function(d){b[d]=c(this,b[d]||N),f(a,d.substr(2).toLowerCase(),b[d])},this),b.groups=" "+g.name+(g.put.join?" "+g.put.join(" "):"")+" ",a[F]=b;for(var h in this)"_"===h.charAt(0)&&(this[h]=c(this,this[h]));f(a,"mousedown",this._onTapStart),f(a,"touchstart",this._onTapStart),f(a,"dragover",this),f(a,"dragenter",this),Q.push(this._onDragOver),b.store&&this.sort(b.store.get(this))}function b(a){s&&s.state!==a&&(i(s,"display",a?"none":""),!a&&s.state&&t.insertBefore(s,q),s.state=a)}function c(a,b){var c=P.call(arguments,2);return b.bind?b.bind.apply(b,[a].concat(c)):function(){return b.apply(a,c.concat(P.call(arguments)))}}function d(a,b,c){if(a){c=c||H,b=b.split(".");var d=b.shift().toUpperCase(),e=new RegExp("\\s("+b.join("|")+")\\s","g");do if(">*"===d&&a.parentNode===c||(""===d||a.nodeName.toUpperCase()==d)&&(!b.length||((" "+a.className+" ").match(e)||[]).length==b.length))return a;while(a!==c&&(a=a.parentNode))}return null}function e(a){a.dataTransfer.dropEffect="move",a.preventDefault()}function f(a,b,c){a.addEventListener(b,c,!1)}function g(a,b,c){a.removeEventListener(b,c,!1)}function h(a,b,c){if(a)if(a.classList)a.classList[c?"add":"remove"](b);else{var d=(" "+a.className+" ").replace(/\s+/g," ").replace(" "+b+" ","");a.className=d+(c?" "+b:"")}}function i(a,b,c){var d=a&&a.style;if(d){if(void 0===c)return H.defaultView&&H.defaultView.getComputedStyle?c=H.defaultView.getComputedStyle(a,""):a.currentStyle&&(c=a.currentStyle),void 0===b?c:c[b];b in d||(b="-webkit-"+b),d[b]=c+("string"==typeof c?"":"px")}}function j(a,b,c){if(a){var d=a.getElementsByTagName(b),e=0,f=d.length;if(c)for(;f>e;e++)c(d[e],e);return d}return[]}function k(a){a.draggable=!1}function l(){K=!1}function m(a,b){var c=a.lastElementChild,d=c.getBoundingClientRect();return b.clientY-(d.top+d.height)>5&&c}function n(a){for(var b=a.tagName+a.className+a.src+a.href+a.textContent,c=b.length,d=0;c--;)d+=b.charCodeAt(c);return d.toString(36)}function o(a){for(var b=0;a&&(a=a.previousElementSibling);)"TEMPLATE"!==a.nodeName.toUpperCase()&&b++;return b}function p(a,b){var c,d;return function(){void 0===c&&(c=arguments,d=this,setTimeout(function(){1===c.length?a.call(d,c[0]):a.apply(d,c),c=void 0},b))}}var q,r,s,t,u,v,w,x,y,z,A,B,C,D,E={},F="Sortable"+(new Date).getTime(),G=window,H=G.document,I=G.parseInt,J=!!("draggable"in H.createElement("div")),K=!1,L=function(a,b,c,d,e,f){var g=H.createEvent("Event");g.initEvent(b,!0,!0),g.item=c||a,g.from=d||a,g.clone=s,g.oldIndex=e,g.newIndex=f,a.dispatchEvent(g)},M="onAdd onUpdate onRemove onStart onEnd onFilter onSort".split(" "),N=function(){},O=Math.abs,P=[].slice,Q=[],R=p(function(a,b,c){if(c&&b.scroll){var d,e,f,g,h=b.scrollSensitivity,i=b.scrollSpeed,j=a.clientX,k=a.clientY,l=window.innerWidth,m=window.innerHeight;if(w!==c&&(v=b.scroll,w=c,v===!0)){v=c;do if(v.offsetWidth<v.scrollWidth||v.offsetHeight<v.scrollHeight)break;while(v=v.parentNode)}v&&(d=v,e=v.getBoundingClientRect(),f=(O(e.right-j)<=h)-(O(e.left-j)<=h),g=(O(e.bottom-k)<=h)-(O(e.top-k)<=h)),f||g||(f=(h>=l-j)-(h>=j),g=(h>=m-k)-(h>=k),(f||g)&&(d=G)),(E.vx!==f||E.vy!==g||E.el!==d)&&(E.el=d,E.vx=f,E.vy=g,clearInterval(E.pid),d&&(E.pid=setInterval(function(){d===G?G.scrollTo(G.scrollX+f*i,G.scrollY+g*i):(g&&(d.scrollTop+=g*i),f&&(d.scrollLeft+=f*i))},24)))}},30);return a.prototype={constructor:a,_dragStarted:function(){t&&q&&(h(q,this.options.ghostClass,!0),a.active=this,L(t,"start",q,t,z))},_onTapStart:function(a){var b=a.type,c=a.touches&&a.touches[0],e=(c||a).target,g=e,h=this.options,i=this.el,l=h.filter;if(!("mousedown"===b&&0!==a.button||h.disabled)&&(e=d(e,h.draggable,i))){if(z=o(e),"function"==typeof l){if(l.call(this,a,e,this))return L(g,"filter",e,i,z),void a.preventDefault()}else if(l&&(l=l.split(",").some(function(a){return a=d(g,a.trim(),i),a?(L(a,"filter",e,i,z),!0):void 0})))return void a.preventDefault();if((!h.handle||d(g,h.handle,i))&&e&&!q&&e.parentNode===i){C=a,t=this.el,q=e,u=q.nextSibling,B=this.options.group,q.draggable=!0,h.ignore.split(",").forEach(function(a){j(e,a.trim(),k)}),c&&(C={target:e,clientX:c.clientX,clientY:c.clientY},this._onDragStart(C,"touch"),a.preventDefault()),f(H,"mouseup",this._onDrop),f(H,"touchend",this._onDrop),f(H,"touchcancel",this._onDrop),f(q,"dragend",this),f(t,"dragstart",this._onDragStart),J||this._onDragStart(C,!0);try{H.selection?H.selection.empty():window.getSelection().removeAllRanges()}catch(m){}}}},_emulateDragOver:function(){if(D){i(r,"display","none");var a=H.elementFromPoint(D.clientX,D.clientY),b=a,c=" "+this.options.group.name,d=Q.length;if(b)do{if(b[F]&&b[F].groups.indexOf(c)>-1){for(;d--;)Q[d]({clientX:D.clientX,clientY:D.clientY,target:a,rootEl:b});break}a=b}while(b=b.parentNode);i(r,"display","")}},_onTouchMove:function(a){if(C){var b=a.touches?a.touches[0]:a,c=b.clientX-C.clientX,d=b.clientY-C.clientY,e=a.touches?"translate3d("+c+"px,"+d+"px,0)":"translate("+c+"px,"+d+"px)";D=b,i(r,"webkitTransform",e),i(r,"mozTransform",e),i(r,"msTransform",e),i(r,"transform",e),a.preventDefault()}},_onDragStart:function(a,b){var c=a.dataTransfer,d=this.options;if(this._offUpEvents(),"clone"==B.pull&&(s=q.cloneNode(!0),i(s,"display","none"),t.insertBefore(s,q)),b){var e,g=q.getBoundingClientRect(),h=i(q);r=q.cloneNode(!0),i(r,"top",g.top-I(h.marginTop,10)),i(r,"left",g.left-I(h.marginLeft,10)),i(r,"width",g.width),i(r,"height",g.height),i(r,"opacity","0.8"),i(r,"position","fixed"),i(r,"zIndex","100000"),t.appendChild(r),e=r.getBoundingClientRect(),i(r,"width",2*g.width-e.width),i(r,"height",2*g.height-e.height),"touch"===b?(f(H,"touchmove",this._onTouchMove),f(H,"touchend",this._onDrop),f(H,"touchcancel",this._onDrop)):(f(H,"mousemove",this._onTouchMove),f(H,"mouseup",this._onDrop)),this._loopId=setInterval(this._emulateDragOver,150)}else c&&(c.effectAllowed="move",d.setData&&d.setData.call(this,c,q)),f(H,"drop",this);setTimeout(this._dragStarted,0)},_onDragOver:function(a){var c,e,f,g=this.el,h=this.options,j=h.group,k=j.put,n=B===j,o=h.sort;if(q&&(void 0!==a.preventDefault&&(a.preventDefault(),!h.dragoverBubble&&a.stopPropagation()),B&&!h.disabled&&(n?o||(f=!t.contains(q)):B.pull&&k&&(B.name===j.name||k.indexOf&&~k.indexOf(B.name)))&&(void 0===a.rootEl||a.rootEl===this.el))){if(R(a,h,this.el),K)return;if(c=d(a.target,h.draggable,g),e=q.getBoundingClientRect(),f)return b(!0),void(s||u?t.insertBefore(q,s||u):o||t.appendChild(q));if(0===g.children.length||g.children[0]===r||g===a.target&&(c=m(g,a))){if(c){if(c.animated)return;v=c.getBoundingClientRect()}b(n),g.appendChild(q),this._animate(e,q),c&&this._animate(v,c)}else if(c&&!c.animated&&c!==q&&void 0!==c.parentNode[F]){x!==c&&(x=c,y=i(c));var p,v=c.getBoundingClientRect(),w=v.right-v.left,z=v.bottom-v.top,A=/left|right|inline/.test(y.cssFloat+y.display),C=c.offsetWidth>q.offsetWidth,D=c.offsetHeight>q.offsetHeight,E=(A?(a.clientX-v.left)/w:(a.clientY-v.top)/z)>.5,G=c.nextElementSibling;K=!0,setTimeout(l,30),b(n),p=A?c.previousElementSibling===q&&!C||E&&C:G!==q&&!D||E&&D,p&&!G?g.appendChild(q):c.parentNode.insertBefore(q,p?G:c),this._animate(e,q),this._animate(v,c)}}},_animate:function(a,b){var c=this.options.animation;if(c){var d=b.getBoundingClientRect();i(b,"transition","none"),i(b,"transform","translate3d("+(a.left-d.left)+"px,"+(a.top-d.top)+"px,0)"),b.offsetWidth,i(b,"transition","all "+c+"ms"),i(b,"transform","translate3d(0,0,0)"),
clearTimeout(b.animated),b.animated=setTimeout(function(){i(b,"transition",""),i(b,"transform",""),b.animated=!1},c)}},_offUpEvents:function(){g(H,"mouseup",this._onDrop),g(H,"touchmove",this._onTouchMove),g(H,"touchend",this._onDrop),g(H,"touchcancel",this._onDrop)},_onDrop:function(b){var c=this.el,d=this.options;clearInterval(this._loopId),clearInterval(E.pid),g(H,"drop",this),g(H,"mousemove",this._onTouchMove),g(c,"dragstart",this._onDragStart),this._offUpEvents(),b&&(b.preventDefault(),!d.dropBubble&&b.stopPropagation(),r&&r.parentNode.removeChild(r),q&&(g(q,"dragend",this),k(q),h(q,this.options.ghostClass,!1),t!==q.parentNode?(A=o(q),L(q.parentNode,"sort",q,t,z,A),L(t,"sort",q,t,z,A),L(q,"add",q,t,z,A),L(t,"remove",q,t,z,A)):(s&&s.parentNode.removeChild(s),q.nextSibling!==u&&(A=o(q),L(t,"update",q,t,z,A),L(t,"sort",q,t,z,A))),a.active&&L(t,"end",q,t,z,A)),t=q=r=u=s=v=w=C=D=x=y=B=a.active=null,this.save())},handleEvent:function(a){var b=a.type;"dragover"===b||"dragenter"===b?(this._onDragOver(a),e(a)):("drop"===b||"dragend"===b)&&this._onDrop(a)},toArray:function(){for(var a,b=[],c=this.el.children,e=0,f=c.length;f>e;e++)a=c[e],d(a,this.options.draggable,this.el)&&b.push(a.getAttribute("data-id")||n(a));return b},sort:function(a){var b={},c=this.el;this.toArray().forEach(function(a,e){var f=c.children[e];d(f,this.options.draggable,c)&&(b[a]=f)},this),a.forEach(function(a){b[a]&&(c.removeChild(b[a]),c.appendChild(b[a]))})},save:function(){var a=this.options.store;a&&a.set(this)},closest:function(a,b){return d(a,b||this.options.draggable,this.el)},option:function(a,b){var c=this.options;return void 0===b?c[a]:void(c[a]=b)},destroy:function(){var a=this.el,b=this.options;M.forEach(function(c){g(a,c.substr(2).toLowerCase(),b[c])}),g(a,"mousedown",this._onTapStart),g(a,"touchstart",this._onTapStart),g(a,"dragover",this),g(a,"dragenter",this),Array.prototype.forEach.call(a.querySelectorAll("[draggable]"),function(a){a.removeAttribute("draggable")}),Q.splice(Q.indexOf(this._onDragOver),1),this._onDrop(),this.el=null}},a.utils={on:f,off:g,css:i,find:j,bind:c,is:function(a,b){return!!d(a,b,a)},throttle:p,closest:d,toggleClass:h,dispatchEvent:L,index:o},a.version="1.1.1",a.create=function(b,c){return new a(b,c)},a}),function(a){"use strict";"function"==typeof define&&define.amd?define(["jquery"],a):a(jQuery)}(function(a){"use strict";a.fn.sortable=function(b){var c;return this.each(function(){var d=a(this),e=d.data("sortable");if(e||!(b instanceof Object)&&b||(e=new Sortable(this,b),d.data("sortable",e)),e){if("widget"===b)return e;"destroy"===b?(e.destroy(),d.removeData("sortable")):b in e&&(c=e[e].apply(e,[].slice.call(arguments,1)))}}),void 0===c?this:c}});var XBBCODE=function(){function a(){i=[];var a,b,c;for(a in h)if(h.hasOwnProperty(a)){for("*"===a?i.push("\\"+a):(i.push(a),h[a].noParse&&u.push(a)),h[a].validChildLookup={},h[a].validParentLookup={},h[a].restrictParentsTo=h[a].restrictParentsTo||[],h[a].restrictChildrenTo=h[a].restrictChildrenTo||[],c=h[a].restrictChildrenTo.length,b=0;c>b;b++)h[a].validChildLookup[h[a].restrictChildrenTo[b]]=!0;for(c=h[a].restrictParentsTo.length,b=0;c>b;b++)h[a].validParentLookup[h[a].restrictParentsTo[b]]=!0}j=new RegExp("<bbcl=([0-9]+) ("+i.join("|")+")([ =][^>]*?)?>((?:.|[\\r\\n])*?)<bbcl=\\1 /\\2>","gi"),k=new RegExp("\\[("+i.join("|")+")([ =][^\\]]*?)?\\]([^\\[]*?)\\[/\\1\\]","gi"),l=new RegExp("\\[("+u.join("|")+")([ =][^\\]]*?)?\\]([\\s\\S]*?)\\[/\\1\\]","gi"),function(){for(var a=[],b=0;b<i.length;b++)"\\*"!==i[b]&&a.push("/"+i[b]);m=new RegExp("(\\[)((?:"+i.join("|")+")(?:[ =][^\\]]*?)?)(\\])","gi"),n=new RegExp("(\\[)("+a.join("|")+")(\\])","gi")}()}function b(a,c,d,e,f,g,k){k=k||[],d++;var l,m,n,o,p=new RegExp("(<bbcl="+d+" )("+i.join("|")+")([ =>])","gi"),q=new RegExp("(<bbcl="+d+" )("+i.join("|")+")([ =>])","i"),r=g.match(p)||[],s=h[a]||{};for(p.lastIndex=0,r||(g=""),n=0;n<r.length;n++)q.lastIndex=0,o=r[n].match(q)[2].toLowerCase(),s&&s.restrictChildrenTo&&s.restrictChildrenTo.length>0&&(s.validChildLookup[o]||(m='The tag "'+o+'" is not allowed as a child of the tag "'+a+'".',k.push(m))),l=h[o]||{},l.restrictParentsTo.length>0&&(l.validParentLookup[a]||(m='The tag "'+a+'" is not allowed as a parent of the tag "'+o+'".',k.push(m)));return g=g.replace(j,function(a,c,d,e,f){return k=b(d.toLowerCase(),a,c,d,e,f,k),a}),k}function c(a){return a=a.replace(/\<([^\>][^\>]*?)\>/gi,function(a,b){var c=b.match(/^bbcl=([0-9]+) /);return null===c?"<bbcl=0 "+b+">":"<"+b.replace(/^(bbcl=)([0-9]+)/,function(a,b,c){return b+(parseInt(c,10)+1)})+">"})}function d(a){return a.replace(/<bbcl=[0-9]+ \/\*>/gi,"").replace(/<bbcl=[0-9]+ /gi,"&#91;").replace(/>/gi,"&#93;")}function e(a){var b=a.text;return b=b.replace(j,v)}function f(a){for(a=a.replace(/\[(?!\*[ =\]]|list([ =][^\]]*)?\]|\/list[\]])/gi,"<"),a=a.replace(/\[(?=list([ =][^\]]*)?\]|\/list[\]])/gi,">");a!==(a=a.replace(/>list([ =][^\]]*)?\]([^>]*?)(>\/list])/gi,function(a,b,c){for(var d=a;d!==(d=d.replace(/\[\*\]([^\[]*?)(\[\*\]|>\/list])/i,function(a,b,c){return c=">/list]"===c.toLowerCase()?"</*]</list]":"</*][*]","<*]"+b+c})););return d=d.replace(/>/g,"<")})););return a=a.replace(/</g,"[")}function g(a){for(;a!==(a=a.replace(k,function(a,b,d,e){return a=a.replace(/\[/g,"<"),a=a.replace(/\]/g,">"),c(a)})););return a}var h,i,j,k,l,m,n,o={},p=/^(?:https?|file|c):(?:\/{1,3}|\\{1})[-a-zA-Z0-9:;@#%&()~_?\+=\/\\\.]*$/,q=/^(?:aliceblue|antiquewhite|aqua|aquamarine|azure|beige|bisque|black|blanchedalmond|blue|blueviolet|brown|burlywood|cadetblue|chartreuse|chocolate|coral|cornflowerblue|cornsilk|crimson|cyan|darkblue|darkcyan|darkgoldenrod|darkgray|darkgreen|darkkhaki|darkmagenta|darkolivegreen|darkorange|darkorchid|darkred|darksalmon|darkseagreen|darkslateblue|darkslategray|darkturquoise|darkviolet|deeppink|deepskyblue|dimgray|dodgerblue|firebrick|floralwhite|forestgreen|fuchsia|gainsboro|ghostwhite|gold|goldenrod|gray|green|greenyellow|honeydew|hotpink|indianred|indigo|ivory|khaki|lavender|lavenderblush|lawngreen|lemonchiffon|lightblue|lightcoral|lightcyan|lightgoldenrodyellow|lightgray|lightgreen|lightpink|lightsalmon|lightseagreen|lightskyblue|lightslategray|lightsteelblue|lightyellow|lime|limegreen|linen|magenta|maroon|mediumaquamarine|mediumblue|mediumorchid|mediumpurple|mediumseagreen|mediumslateblue|mediumspringgreen|mediumturquoise|mediumvioletred|midnightblue|mintcream|mistyrose|moccasin|navajowhite|navy|oldlace|olive|olivedrab|orange|orangered|orchid|palegoldenrod|palegreen|paleturquoise|palevioletred|papayawhip|peachpuff|peru|pink|plum|powderblue|purple|red|rosybrown|royalblue|saddlebrown|salmon|sandybrown|seagreen|seashell|sienna|silver|skyblue|slateblue|slategray|snow|springgreen|steelblue|tan|teal|thistle|tomato|turquoise|violet|wheat|white|whitesmoke|yellow|yellowgreen)$/,r=/^#?[a-fA-F0-9]{6}$/,s=/[^\s@]+@[^\s@]+\.[^\s@]+/,t=/^([a-z][a-z0-9_]+|"[a-z][a-z0-9_\s]+")$/i,u=[];h={b:{openTag:function(a,b){return'<span class="xbbcode-b">'},closeTag:function(a,b){return"</span>"}},bbcode:{openTag:function(a,b){return""},closeTag:function(a,b){return""}},center:{openTag:function(a,b){return'<span class="xbbcode-center">'},closeTag:function(a,b){return"</span>"}},code:{openTag:function(a,b){return'<span class="xbbcode-code">'},closeTag:function(a,b){return"</span>"},noParse:!0},color:{openTag:function(a,b){var c=a.substr(1).toLowerCase()||"black";return q.lastIndex=0,r.lastIndex=0,q.test(c)||(r.test(c)?"#"!==c.substr(0,1)&&(c="#"+c):c="black"),'<span style="color:'+c+'">'},closeTag:function(a,b){return"</span>"}},email:{openTag:function(a,b){var c;return c=a?a.substr(1):b.replace(/<.*?>/g,""),s.lastIndex=0,s.test(c)?'<a href="mailto:'+c+'">':"<a>"},closeTag:function(a,b){return"</a>"}},face:{openTag:function(a,b){var c=a.substr(1)||"inherit";return t.lastIndex=0,t.test(c)||(c="inherit"),'<span style="font-family:'+c+'">'},closeTag:function(a,b){return"</span>"}},font:{openTag:function(a,b){var c=a.substr(1)||"inherit";return t.lastIndex=0,t.test(c)||(c="inherit"),'<span style="font-family:'+c+'">'},closeTag:function(a,b){return"</span>"}},i:{openTag:function(a,b){return'<span class="xbbcode-i">'},closeTag:function(a,b){return"</span>"}},img:{openTag:function(a,b){var c=b;return p.lastIndex=0,p.test(c)||(c=""),'<img src="'+c+'" />'},closeTag:function(a,b){return""},displayContent:!1},justify:{openTag:function(a,b){return'<span class="xbbcode-justify">'},closeTag:function(a,b){return"</span>"}},large:{openTag:function(a,b){var a=a||"",c=a.substr(1)||"inherit";return q.lastIndex=0,r.lastIndex=0,q.test(c)||(r.test(c)?"#"!==c.substr(0,1)&&(c="#"+c):c="inherit"),'<span class="xbbcode-size-36" style="color:'+c+'">'},closeTag:function(a,b){return"</span>"}},left:{openTag:function(a,b){return'<span class="xbbcode-left">'},closeTag:function(a,b){return"</span>"}},li:{openTag:function(a,b){return"<li>"},closeTag:function(a,b){return"</li>"},restrictParentsTo:["list","ul","ol"]},list:{openTag:function(a,b){return"<ul>"},closeTag:function(a,b){return"</ul>"},restrictChildrenTo:["*","li"]},noparse:{openTag:function(a,b){return""},closeTag:function(a,b){return""},noParse:!0},ol:{openTag:function(a,b){return"<ol>"},closeTag:function(a,b){return"</ol>"},restrictChildrenTo:["*","li"]},php:{openTag:function(a,b){return'<span class="xbbcode-code">'},closeTag:function(a,b){return"</span>"},noParse:!0},quote:{openTag:function(a,b){return'<blockquote class="xbbcode-blockquote">'},closeTag:function(a,b){return"</blockquote>"}},right:{openTag:function(a,b){return'<span class="xbbcode-right">'},closeTag:function(a,b){return"</span>"}},s:{openTag:function(a,b){return'<span class="xbbcode-s">'},closeTag:function(a,b){return"</span>"}},size:{openTag:function(a,b){var c=parseInt(a.substr(1),10)||0;return(4>c||c>40)&&(c=14),'<span class="xbbcode-size-'+c+'">'},closeTag:function(a,b){return"</span>"}},small:{openTag:function(a,b){var a=a||"",c=a.substr(1)||"inherit";return q.lastIndex=0,r.lastIndex=0,q.test(c)||(r.test(c)?"#"!==c.substr(0,1)&&(c="#"+c):c="inherit"),'<span class="xbbcode-size-10" style="color:'+c+'">'},closeTag:function(a,b){return"</span>"}},sub:{openTag:function(a,b){return"<sub>"},closeTag:function(a,b){return"</sub>"}},sup:{openTag:function(a,b){return"<sup>"},closeTag:function(a,b){return"</sup>"}},table:{openTag:function(a,b){return'<table class="xbbcode-table">'},closeTag:function(a,b){return"</table>"},restrictChildrenTo:["tbody","thead","tfoot","tr"]},tbody:{openTag:function(a,b){return"<tbody>"},closeTag:function(a,b){return"</tbody>"},restrictChildrenTo:["tr"],restrictParentsTo:["table"]},tfoot:{openTag:function(a,b){return"<tfoot>"},closeTag:function(a,b){return"</tfoot>"},restrictChildrenTo:["tr"],restrictParentsTo:["table"]},thead:{openTag:function(a,b){return'<thead class="xbbcode-thead">'},closeTag:function(a,b){return"</thead>"},restrictChildrenTo:["tr"],restrictParentsTo:["table"]},td:{openTag:function(a,b){return'<td class="xbbcode-td">'},closeTag:function(a,b){return"</td>"},restrictParentsTo:["tr"]},th:{openTag:function(a,b){return'<th class="xbbcode-th">'},closeTag:function(a,b){return"</th>"},restrictParentsTo:["tr"]},tr:{openTag:function(a,b){return'<tr class="xbbcode-tr">'},closeTag:function(a,b){return"</tr>"},restrictChildrenTo:["td","th"],restrictParentsTo:["table","tbody","tfoot","thead"]},u:{openTag:function(a,b){return'<span class="xbbcode-u">'},closeTag:function(a,b){return"</span>"}},ul:{openTag:function(a,b){return"<ul>"},closeTag:function(a,b){return"</ul>"},restrictChildrenTo:["*","li"]},url:{openTag:function(a,b){var c;return c=a?a.substr(1):b.replace(/<.*?>/g,""),p.lastIndex=0,p.test(c)||(c="#"),'<a href="'+c+'">'},closeTag:function(a,b){return"</a>"}},"*":{openTag:function(a,b){return"<li>"},closeTag:function(a,b){return"</li>"},restrictParentsTo:["list","ul","ol"]}},a();var v=function(a,b,c,e,f){c=c.toLowerCase();var g=h[c].noParse?d(f):f.replace(j,v),i=h[c].openTag(e,g),k=h[c].closeTag(e,g);return h[c].displayContent===!1&&(g=""),i+g+k};return o.tags=function(){return h},o.addTags=function(b){var c;for(c in b)h[c]=b[c];a()},o.process=function(a){var c={html:"",error:!1},d=[];for(a.text=a.text.replace(/</g,"&lt;"),a.text=a.text.replace(/>/g,"&gt;"),a.text=a.text.replace(m,function(a,b,c,d){return"<"+c+">"}),a.text=a.text.replace(n,function(a,b,c,d){return"<"+c+">"}),a.text=a.text.replace(/\[/g,"&#91;"),a.text=a.text.replace(/\]/g,"&#93;"),a.text=a.text.replace(/</g,"["),a.text=a.text.replace(/>/g,"]");a.text!==(a.text=a.text.replace(l,function(a,b,c,d){return d=d.replace(/\[/g,"&#91;"),d=d.replace(/\]/g,"&#93;"),c=c||"",d=d||"","["+b+c+"]"+d+"[/"+b+"]"})););return a.text=f(a.text),a.text=g(a.text),d=b("bbcode",a.text,-1,"","",a.text),c.html=e(a),(-1!==c.html.indexOf("[")||-1!==c.html.indexOf("]"))&&d.push("Some tags appear to be misaligned."),a.removeMisalignedTags&&(c.html=c.html.replace(/\[.*?\]/g,"")),a.addInLineBreaks&&(c.html='<div style="white-space:pre;">'+c.html+"</div>"),c.html=c.html.replace("&#91;","["),c.html=c.html.replace("&#93;","]"),c.error=0!==d.length,c.errorQueue=d,c},o}();+function(a){"use strict";function b(b){return this.each(function(){var d=a(this),e=d.data("bs.affix"),f="object"==typeof b&&b;e||d.data("bs.affix",e=new c(this,f)),"string"==typeof b&&e[b]()})}var c=function(b,d){this.options=a.extend({},c.DEFAULTS,d),this.$target=a(this.options.target).on("scroll.bs.affix.data-api",a.proxy(this.checkPosition,this)).on("click.bs.affix.data-api",a.proxy(this.checkPositionWithEventLoop,this)),this.$element=a(b),this.affixed=this.unpin=this.pinnedOffset=null,this.checkPosition()};c.VERSION="3.3.1",c.RESET="affix affix-top affix-bottom",c.DEFAULTS={offset:0,target:window},c.prototype.getState=function(a,b,c,d){var e=this.$target.scrollTop(),f=this.$element.offset(),g=this.$target.height();if(null!=c&&"top"==this.affixed)return c>e?"top":!1;if("bottom"==this.affixed)return null!=c?e+this.unpin<=f.top?!1:"bottom":a-d>=e+g?!1:"bottom";var h=null==this.affixed,i=h?e:f.top,j=h?g:b;return null!=c&&c>=i?"top":null!=d&&i+j>=a-d?"bottom":!1},c.prototype.getPinnedOffset=function(){if(this.pinnedOffset)return this.pinnedOffset;this.$element.removeClass(c.RESET).addClass("affix");var a=this.$target.scrollTop(),b=this.$element.offset();return this.pinnedOffset=b.top-a},c.prototype.checkPositionWithEventLoop=function(){setTimeout(a.proxy(this.checkPosition,this),1)},c.prototype.checkPosition=function(){if(this.$element.is(":visible")){var b=this.$element.height(),d=this.options.offset,e=d.top,f=d.bottom,g=a("body").height();"object"!=typeof d&&(f=e=d),"function"==typeof e&&(e=d.top(this.$element)),"function"==typeof f&&(f=d.bottom(this.$element));var h=this.getState(g,b,e,f);if(this.affixed!=h){null!=this.unpin&&this.$element.css("top","");var i="affix"+(h?"-"+h:""),j=a.Event(i+".bs.affix");if(this.$element.trigger(j),j.isDefaultPrevented())return;this.affixed=h,this.unpin="bottom"==h?this.getPinnedOffset():null,this.$element.removeClass(c.RESET).addClass(i).trigger(i.replace("affix","affixed")+".bs.affix")}"bottom"==h&&this.$element.offset({top:g-b-f})}};var d=a.fn.affix;a.fn.affix=b,a.fn.affix.Constructor=c,a.fn.affix.noConflict=function(){return a.fn.affix=d,this},a(window).on("load",function(){a('[data-spy="affix"]').each(function(){var c=a(this),d=c.data();d.offset=d.offset||{},null!=d.offsetBottom&&(d.offset.bottom=d.offsetBottom),null!=d.offsetTop&&(d.offset.top=d.offsetTop),b.call(c,d)})})}(jQuery),+function(a){"use strict";function b(b){return this.each(function(){var c=a(this),e=c.data("bs.alert");e||c.data("bs.alert",e=new d(this)),"string"==typeof b&&e[b].call(c)})}var c='[data-dismiss="alert"]',d=function(b){a(b).on("click",c,this.close)};d.VERSION="3.3.1",d.TRANSITION_DURATION=150,d.prototype.close=function(b){function c(){g.detach().trigger("closed.bs.alert").remove()}var e=a(this),f=e.attr("data-target");f||(f=e.attr("href"),f=f&&f.replace(/.*(?=#[^\s]*$)/,""));var g=a(f);b&&b.preventDefault(),g.length||(g=e.closest(".alert")),g.trigger(b=a.Event("close.bs.alert")),b.isDefaultPrevented()||(g.removeClass("in"),a.support.transition&&g.hasClass("fade")?g.one("bsTransitionEnd",c).emulateTransitionEnd(d.TRANSITION_DURATION):c())};var e=a.fn.alert;a.fn.alert=b,a.fn.alert.Constructor=d,a.fn.alert.noConflict=function(){return a.fn.alert=e,this},a(document).on("click.bs.alert.data-api",c,d.prototype.close)}(jQuery),+function(a){"use strict";function b(b){return this.each(function(){var d=a(this),e=d.data("bs.button"),f="object"==typeof b&&b;e||d.data("bs.button",e=new c(this,f)),"toggle"==b?e.toggle():b&&e.setState(b)})}var c=function(b,d){this.$element=a(b),this.options=a.extend({},c.DEFAULTS,d),this.isLoading=!1};c.VERSION="3.3.1",c.DEFAULTS={loadingText:"loading..."},c.prototype.setState=function(b){var c="disabled",d=this.$element,e=d.is("input")?"val":"html",f=d.data();b+="Text",null==f.resetText&&d.data("resetText",d[e]()),setTimeout(a.proxy(function(){d[e](null==f[b]?this.options[b]:f[b]),"loadingText"==b?(this.isLoading=!0,d.addClass(c).attr(c,c)):this.isLoading&&(this.isLoading=!1,d.removeClass(c).removeAttr(c))},this),0)},c.prototype.toggle=function(){var a=!0,b=this.$element.closest('[data-toggle="buttons"]');if(b.length){var c=this.$element.find("input");"radio"==c.prop("type")&&(c.prop("checked")&&this.$element.hasClass("active")?a=!1:b.find(".active").removeClass("active")),a&&c.prop("checked",!this.$element.hasClass("active")).trigger("change")}else this.$element.attr("aria-pressed",!this.$element.hasClass("active"));a&&this.$element.toggleClass("active")};var d=a.fn.button;a.fn.button=b,a.fn.button.Constructor=c,a.fn.button.noConflict=function(){return a.fn.button=d,this},a(document).on("click.bs.button.data-api",'[data-toggle^="button"]',function(c){var d=a(c.target);d.hasClass("btn")||(d=d.closest(".btn")),b.call(d,"toggle"),c.preventDefault()}).on("focus.bs.button.data-api blur.bs.button.data-api",'[data-toggle^="button"]',function(b){a(b.target).closest(".btn").toggleClass("focus",/^focus(in)?$/.test(b.type))})}(jQuery),+function(a){"use strict";function b(b){return this.each(function(){var d=a(this),e=d.data("bs.carousel"),f=a.extend({},c.DEFAULTS,d.data(),"object"==typeof b&&b),g="string"==typeof b?b:f.slide;e||d.data("bs.carousel",e=new c(this,f)),"number"==typeof b?e.to(b):g?e[g]():f.interval&&e.pause().cycle()})}var c=function(b,c){this.$element=a(b),this.$indicators=this.$element.find(".carousel-indicators"),this.options=c,this.paused=this.sliding=this.interval=this.$active=this.$items=null,this.options.keyboard&&this.$element.on("keydown.bs.carousel",a.proxy(this.keydown,this)),"hover"==this.options.pause&&!("ontouchstart"in document.documentElement)&&this.$element.on("mouseenter.bs.carousel",a.proxy(this.pause,this)).on("mouseleave.bs.carousel",a.proxy(this.cycle,this))};c.VERSION="3.3.1",c.TRANSITION_DURATION=600,c.DEFAULTS={interval:5e3,pause:"hover",wrap:!0,keyboard:!0},c.prototype.keydown=function(a){if(!/input|textarea/i.test(a.target.tagName)){switch(a.which){case 37:this.prev();break;case 39:this.next();break;default:return}a.preventDefault()}},c.prototype.cycle=function(b){return b||(this.paused=!1),this.interval&&clearInterval(this.interval),this.options.interval&&!this.paused&&(this.interval=setInterval(a.proxy(this.next,this),this.options.interval)),this},c.prototype.getItemIndex=function(a){return this.$items=a.parent().children(".item"),this.$items.index(a||this.$active)},c.prototype.getItemForDirection=function(a,b){var c="prev"==a?-1:1,d=this.getItemIndex(b),e=(d+c)%this.$items.length;return this.$items.eq(e)},c.prototype.to=function(a){var b=this,c=this.getItemIndex(this.$active=this.$element.find(".item.active"));return a>this.$items.length-1||0>a?void 0:this.sliding?this.$element.one("slid.bs.carousel",function(){b.to(a)}):c==a?this.pause().cycle():this.slide(a>c?"next":"prev",this.$items.eq(a))},c.prototype.pause=function(b){return b||(this.paused=!0),this.$element.find(".next, .prev").length&&a.support.transition&&(this.$element.trigger(a.support.transition.end),this.cycle(!0)),this.interval=clearInterval(this.interval),this},c.prototype.next=function(){return this.sliding?void 0:this.slide("next")},c.prototype.prev=function(){return this.sliding?void 0:this.slide("prev")},c.prototype.slide=function(b,d){var e=this.$element.find(".item.active"),f=d||this.getItemForDirection(b,e),g=this.interval,h="next"==b?"left":"right",i="next"==b?"first":"last",j=this;if(!f.length){if(!this.options.wrap)return;f=this.$element.find(".item")[i]()}if(f.hasClass("active"))return this.sliding=!1;var k=f[0],l=a.Event("slide.bs.carousel",{relatedTarget:k,direction:h});if(this.$element.trigger(l),!l.isDefaultPrevented()){if(this.sliding=!0,g&&this.pause(),this.$indicators.length){this.$indicators.find(".active").removeClass("active");var m=a(this.$indicators.children()[this.getItemIndex(f)]);m&&m.addClass("active")}var n=a.Event("slid.bs.carousel",{relatedTarget:k,direction:h});return a.support.transition&&this.$element.hasClass("slide")?(f.addClass(b),f[0].offsetWidth,e.addClass(h),f.addClass(h),e.one("bsTransitionEnd",function(){f.removeClass([b,h].join(" ")).addClass("active"),e.removeClass(["active",h].join(" ")),j.sliding=!1,setTimeout(function(){j.$element.trigger(n)},0)}).emulateTransitionEnd(c.TRANSITION_DURATION)):(e.removeClass("active"),f.addClass("active"),this.sliding=!1,this.$element.trigger(n)),g&&this.cycle(),this}};var d=a.fn.carousel;a.fn.carousel=b,a.fn.carousel.Constructor=c,a.fn.carousel.noConflict=function(){return a.fn.carousel=d,this};var e=function(c){var d,e=a(this),f=a(e.attr("data-target")||(d=e.attr("href"))&&d.replace(/.*(?=#[^\s]+$)/,""));if(f.hasClass("carousel")){var g=a.extend({},f.data(),e.data()),h=e.attr("data-slide-to");h&&(g.interval=!1),b.call(f,g),h&&f.data("bs.carousel").to(h),c.preventDefault()}};a(document).on("click.bs.carousel.data-api","[data-slide]",e).on("click.bs.carousel.data-api","[data-slide-to]",e),a(window).on("load",function(){a('[data-ride="carousel"]').each(function(){var c=a(this);b.call(c,c.data())})})}(jQuery),+function(a){"use strict";function b(b){var c,d=b.attr("data-target")||(c=b.attr("href"))&&c.replace(/.*(?=#[^\s]+$)/,"");return a(d)}function c(b){return this.each(function(){var c=a(this),e=c.data("bs.collapse"),f=a.extend({},d.DEFAULTS,c.data(),"object"==typeof b&&b);!e&&f.toggle&&"show"==b&&(f.toggle=!1),e||c.data("bs.collapse",e=new d(this,f)),"string"==typeof b&&e[b]()})}var d=function(b,c){this.$element=a(b),this.options=a.extend({},d.DEFAULTS,c),this.$trigger=a(this.options.trigger).filter('[href="#'+b.id+'"], [data-target="#'+b.id+'"]'),this.transitioning=null,this.options.parent?this.$parent=this.getParent():this.addAriaAndCollapsedClass(this.$element,this.$trigger),this.options.toggle&&this.toggle()};d.VERSION="3.3.1",d.TRANSITION_DURATION=350,d.DEFAULTS={toggle:!0,trigger:'[data-toggle="collapse"]'},d.prototype.dimension=function(){var a=this.$element.hasClass("width");return a?"width":"height"},d.prototype.show=function(){if(!this.transitioning&&!this.$element.hasClass("in")){var b,e=this.$parent&&this.$parent.find("> .panel").children(".in, .collapsing");if(!(e&&e.length&&(b=e.data("bs.collapse"),b&&b.transitioning))){var f=a.Event("show.bs.collapse");if(this.$element.trigger(f),!f.isDefaultPrevented()){e&&e.length&&(c.call(e,"hide"),b||e.data("bs.collapse",null));var g=this.dimension();this.$element.removeClass("collapse").addClass("collapsing")[g](0).attr("aria-expanded",!0),this.$trigger.removeClass("collapsed").attr("aria-expanded",!0),this.transitioning=1;var h=function(){this.$element.removeClass("collapsing").addClass("collapse in")[g](""),this.transitioning=0,this.$element.trigger("shown.bs.collapse")};if(!a.support.transition)return h.call(this);var i=a.camelCase(["scroll",g].join("-"));this.$element.one("bsTransitionEnd",a.proxy(h,this)).emulateTransitionEnd(d.TRANSITION_DURATION)[g](this.$element[0][i])}}}},d.prototype.hide=function(){if(!this.transitioning&&this.$element.hasClass("in")){var b=a.Event("hide.bs.collapse");if(this.$element.trigger(b),!b.isDefaultPrevented()){var c=this.dimension();this.$element[c](this.$element[c]())[0].offsetHeight,this.$element.addClass("collapsing").removeClass("collapse in").attr("aria-expanded",!1),this.$trigger.addClass("collapsed").attr("aria-expanded",!1),this.transitioning=1;var e=function(){this.transitioning=0,this.$element.removeClass("collapsing").addClass("collapse").trigger("hidden.bs.collapse")};return a.support.transition?void this.$element[c](0).one("bsTransitionEnd",a.proxy(e,this)).emulateTransitionEnd(d.TRANSITION_DURATION):e.call(this)}}},d.prototype.toggle=function(){this[this.$element.hasClass("in")?"hide":"show"]()},d.prototype.getParent=function(){return a(this.options.parent).find('[data-toggle="collapse"][data-parent="'+this.options.parent+'"]').each(a.proxy(function(c,d){var e=a(d);this.addAriaAndCollapsedClass(b(e),e)},this)).end()},d.prototype.addAriaAndCollapsedClass=function(a,b){var c=a.hasClass("in");a.attr("aria-expanded",c),b.toggleClass("collapsed",!c).attr("aria-expanded",c)};var e=a.fn.collapse;a.fn.collapse=c,a.fn.collapse.Constructor=d,a.fn.collapse.noConflict=function(){return a.fn.collapse=e,this},a(document).on("click.bs.collapse.data-api",'[data-toggle="collapse"]',function(d){var e=a(this);e.attr("data-target")||d.preventDefault();var f=b(e),g=f.data("bs.collapse"),h=g?"toggle":a.extend({},e.data(),{trigger:this});c.call(f,h)})}(jQuery),+function(a){"use strict";function b(b){b&&3===b.which||(a(e).remove(),a(f).each(function(){var d=a(this),e=c(d),f={relatedTarget:this};e.hasClass("open")&&(e.trigger(b=a.Event("hide.bs.dropdown",f)),b.isDefaultPrevented()||(d.attr("aria-expanded","false"),e.removeClass("open").trigger("hidden.bs.dropdown",f)))}))}function c(b){var c=b.attr("data-target");c||(c=b.attr("href"),c=c&&/#[A-Za-z]/.test(c)&&c.replace(/.*(?=#[^\s]*$)/,""));var d=c&&a(c);return d&&d.length?d:b.parent()}function d(b){return this.each(function(){var c=a(this),d=c.data("bs.dropdown");d||c.data("bs.dropdown",d=new g(this)),"string"==typeof b&&d[b].call(c)})}var e=".dropdown-backdrop",f='[data-toggle="dropdown"]',g=function(b){a(b).on("click.bs.dropdown",this.toggle)};g.VERSION="3.3.1",g.prototype.toggle=function(d){var e=a(this);if(!e.is(".disabled, :disabled")){var f=c(e),g=f.hasClass("open");if(b(),!g){"ontouchstart"in document.documentElement&&!f.closest(".navbar-nav").length&&a('<div class="dropdown-backdrop"/>').insertAfter(a(this)).on("click",b);var h={relatedTarget:this};if(f.trigger(d=a.Event("show.bs.dropdown",h)),d.isDefaultPrevented())return;e.trigger("focus").attr("aria-expanded","true"),f.toggleClass("open").trigger("shown.bs.dropdown",h)}return!1}},g.prototype.keydown=function(b){if(/(38|40|27|32)/.test(b.which)&&!/input|textarea/i.test(b.target.tagName)){var d=a(this);if(b.preventDefault(),b.stopPropagation(),!d.is(".disabled, :disabled")){var e=c(d),g=e.hasClass("open");if(!g&&27!=b.which||g&&27==b.which)return 27==b.which&&e.find(f).trigger("focus"),d.trigger("click");var h=" li:not(.divider):visible a",i=e.find('[role="menu"]'+h+', [role="listbox"]'+h);if(i.length){var j=i.index(b.target);38==b.which&&j>0&&j--,40==b.which&&j<i.length-1&&j++,~j||(j=0),i.eq(j).trigger("focus")}}}};var h=a.fn.dropdown;a.fn.dropdown=d,a.fn.dropdown.Constructor=g,a.fn.dropdown.noConflict=function(){return a.fn.dropdown=h,this},a(document).on("click.bs.dropdown.data-api",b).on("click.bs.dropdown.data-api",".dropdown form",function(a){a.stopPropagation()}).on("click.bs.dropdown.data-api",f,g.prototype.toggle).on("keydown.bs.dropdown.data-api",f,g.prototype.keydown).on("keydown.bs.dropdown.data-api",'[role="menu"]',g.prototype.keydown).on("keydown.bs.dropdown.data-api",'[role="listbox"]',g.prototype.keydown)}(jQuery),+function(a){"use strict";function b(b){return this.each(function(){var d=a(this),e=d.data("bs.tab");e||d.data("bs.tab",e=new c(this)),"string"==typeof b&&e[b]()})}var c=function(b){this.element=a(b)};c.VERSION="3.3.1",c.TRANSITION_DURATION=150,c.prototype.show=function(){var b=this.element,c=b.closest("ul:not(.dropdown-menu)"),d=b.data("target");if(d||(d=b.attr("href"),d=d&&d.replace(/.*(?=#[^\s]*$)/,"")),!b.parent("li").hasClass("active")){var e=c.find(".active:last a"),f=a.Event("hide.bs.tab",{relatedTarget:b[0]}),g=a.Event("show.bs.tab",{relatedTarget:e[0]});if(e.trigger(f),b.trigger(g),!g.isDefaultPrevented()&&!f.isDefaultPrevented()){var h=a(d);this.activate(b.closest("li"),c),this.activate(h,h.parent(),function(){e.trigger({type:"hidden.bs.tab",relatedTarget:b[0]}),b.trigger({type:"shown.bs.tab",relatedTarget:e[0]})})}}},c.prototype.activate=function(b,d,e){function f(){g.removeClass("active").find("> .dropdown-menu > .active").removeClass("active").end().find('[data-toggle="tab"]').attr("aria-expanded",!1),b.addClass("active").find('[data-toggle="tab"]').attr("aria-expanded",!0),h?(b[0].offsetWidth,b.addClass("in")):b.removeClass("fade"),b.parent(".dropdown-menu")&&b.closest("li.dropdown").addClass("active").end().find('[data-toggle="tab"]').attr("aria-expanded",!0),e&&e()}var g=d.find("> .active"),h=e&&a.support.transition&&(g.length&&g.hasClass("fade")||!!d.find("> .fade").length);g.length&&h?g.one("bsTransitionEnd",f).emulateTransitionEnd(c.TRANSITION_DURATION):f(),g.removeClass("in")};var d=a.fn.tab;a.fn.tab=b,a.fn.tab.Constructor=c,a.fn.tab.noConflict=function(){return a.fn.tab=d,this};var e=function(c){c.preventDefault(),b.call(a(this),"show")};a(document).on("click.bs.tab.data-api",'[data-toggle="tab"]',e).on("click.bs.tab.data-api",'[data-toggle="pill"]',e)}(jQuery),+function(a){"use strict";function b(){var a=document.createElement("bootstrap"),b={WebkitTransition:"webkitTransitionEnd",MozTransition:"transitionend",OTransition:"oTransitionEnd otransitionend",transition:"transitionend"};for(var c in b)if(void 0!==a.style[c])return{end:b[c]};return!1}a.fn.emulateTransitionEnd=function(b){var c=!1,d=this;a(this).one("bsTransitionEnd",function(){c=!0});var e=function(){c||a(d).trigger(a.support.transition.end)};return setTimeout(e,b),this},a(function(){a.support.transition=b(),a.support.transition&&(a.event.special.bsTransitionEnd={bindType:a.support.transition.end,delegateType:a.support.transition.end,handle:function(b){return a(b.target).is(this)?b.handleObj.handler.apply(this,arguments):void 0}})})}(jQuery),+function(a){"use strict";function b(c,d){var e=a.proxy(this.process,this);this.$body=a("body"),this.$scrollElement=a(a(c).is("body")?window:c),this.options=a.extend({},b.DEFAULTS,d),this.selector=(this.options.target||"")+" .nav li > a",this.offsets=[],this.targets=[],this.activeTarget=null,this.scrollHeight=0,this.$scrollElement.on("scroll.bs.scrollspy",e),this.refresh(),this.process()}function c(c){return this.each(function(){var d=a(this),e=d.data("bs.scrollspy"),f="object"==typeof c&&c;e||d.data("bs.scrollspy",e=new b(this,f)),"string"==typeof c&&e[c]()})}b.VERSION="3.3.1",b.DEFAULTS={offset:10},b.prototype.getScrollHeight=function(){return this.$scrollElement[0].scrollHeight||Math.max(this.$body[0].scrollHeight,document.documentElement.scrollHeight)},b.prototype.refresh=function(){var b="offset",c=0;a.isWindow(this.$scrollElement[0])||(b="position",c=this.$scrollElement.scrollTop()),this.offsets=[],this.targets=[],this.scrollHeight=this.getScrollHeight();var d=this;this.$body.find(this.selector).map(function(){var d=a(this),e=d.data("target")||d.attr("href"),f=/^#./.test(e)&&a(e);return f&&f.length&&f.is(":visible")&&[[f[b]().top+c,e]]||null}).sort(function(a,b){return a[0]-b[0]}).each(function(){d.offsets.push(this[0]),d.targets.push(this[1])})},b.prototype.process=function(){var a,b=this.$scrollElement.scrollTop()+this.options.offset,c=this.getScrollHeight(),d=this.options.offset+c-this.$scrollElement.height(),e=this.offsets,f=this.targets,g=this.activeTarget;if(this.scrollHeight!=c&&this.refresh(),b>=d)return g!=(a=f[f.length-1])&&this.activate(a);if(g&&b<e[0])return this.activeTarget=null,this.clear();for(a=e.length;a--;)g!=f[a]&&b>=e[a]&&(!e[a+1]||b<=e[a+1])&&this.activate(f[a]);
},b.prototype.activate=function(b){this.activeTarget=b,this.clear();var c=this.selector+'[data-target="'+b+'"],'+this.selector+'[href="'+b+'"]',d=a(c).parents("li").addClass("active");d.parent(".dropdown-menu").length&&(d=d.closest("li.dropdown").addClass("active")),d.trigger("activate.bs.scrollspy")},b.prototype.clear=function(){a(this.selector).parentsUntil(this.options.target,".active").removeClass("active")};var d=a.fn.scrollspy;a.fn.scrollspy=c,a.fn.scrollspy.Constructor=b,a.fn.scrollspy.noConflict=function(){return a.fn.scrollspy=d,this},a(window).on("load.bs.scrollspy.data-api",function(){a('[data-spy="scroll"]').each(function(){var b=a(this);c.call(b,b.data())})})}(jQuery),+function(a){"use strict";function b(b,d){return this.each(function(){var e=a(this),f=e.data("bs.modal"),g=a.extend({},c.DEFAULTS,e.data(),"object"==typeof b&&b);f||e.data("bs.modal",f=new c(this,g)),"string"==typeof b?f[b](d):g.show&&f.show(d)})}var c=function(b,c){this.options=c,this.$body=a(document.body),this.$element=a(b),this.$backdrop=this.isShown=null,this.scrollbarWidth=0,this.options.remote&&this.$element.find(".modal-content").load(this.options.remote,a.proxy(function(){this.$element.trigger("loaded.bs.modal")},this))};c.VERSION="3.3.1",c.TRANSITION_DURATION=300,c.BACKDROP_TRANSITION_DURATION=150,c.DEFAULTS={backdrop:!0,keyboard:!0,show:!0},c.prototype.toggle=function(a){return this.isShown?this.hide():this.show(a)},c.prototype.show=function(b){var d=this,e=a.Event("show.bs.modal",{relatedTarget:b});this.$element.trigger(e),this.isShown||e.isDefaultPrevented()||(this.isShown=!0,this.checkScrollbar(),this.setScrollbar(),this.$body.addClass("modal-open"),this.escape(),this.resize(),this.$element.on("click.dismiss.bs.modal",'[data-dismiss="modal"]',a.proxy(this.hide,this)),this.backdrop(function(){var e=a.support.transition&&d.$element.hasClass("fade");d.$element.parent().length||d.$element.appendTo(d.$body),d.$element.show().scrollTop(0),d.options.backdrop&&d.adjustBackdrop(),d.adjustDialog(),e&&d.$element[0].offsetWidth,d.$element.addClass("in").attr("aria-hidden",!1),d.enforceFocus();var f=a.Event("shown.bs.modal",{relatedTarget:b});e?d.$element.find(".modal-dialog").one("bsTransitionEnd",function(){d.$element.trigger("focus").trigger(f)}).emulateTransitionEnd(c.TRANSITION_DURATION):d.$element.trigger("focus").trigger(f)}))},c.prototype.hide=function(b){b&&b.preventDefault(),b=a.Event("hide.bs.modal"),this.$element.trigger(b),this.isShown&&!b.isDefaultPrevented()&&(this.isShown=!1,this.escape(),this.resize(),a(document).off("focusin.bs.modal"),this.$element.removeClass("in").attr("aria-hidden",!0).off("click.dismiss.bs.modal"),a.support.transition&&this.$element.hasClass("fade")?this.$element.one("bsTransitionEnd",a.proxy(this.hideModal,this)).emulateTransitionEnd(c.TRANSITION_DURATION):this.hideModal())},c.prototype.enforceFocus=function(){a(document).off("focusin.bs.modal").on("focusin.bs.modal",a.proxy(function(a){this.$element[0]===a.target||this.$element.has(a.target).length||this.$element.trigger("focus")},this))},c.prototype.escape=function(){this.isShown&&this.options.keyboard?this.$element.on("keydown.dismiss.bs.modal",a.proxy(function(a){27==a.which&&this.hide()},this)):this.isShown||this.$element.off("keydown.dismiss.bs.modal")},c.prototype.resize=function(){this.isShown?a(window).on("resize.bs.modal",a.proxy(this.handleUpdate,this)):a(window).off("resize.bs.modal")},c.prototype.hideModal=function(){var a=this;this.$element.hide(),this.backdrop(function(){a.$body.removeClass("modal-open"),a.resetAdjustments(),a.resetScrollbar(),a.$element.trigger("hidden.bs.modal")})},c.prototype.removeBackdrop=function(){this.$backdrop&&this.$backdrop.remove(),this.$backdrop=null},c.prototype.backdrop=function(b){var d=this,e=this.$element.hasClass("fade")?"fade":"";if(this.isShown&&this.options.backdrop){var f=a.support.transition&&e;if(this.$backdrop=a('<div class="modal-backdrop '+e+'" />').prependTo(this.$element).on("click.dismiss.bs.modal",a.proxy(function(a){a.target===a.currentTarget&&("static"==this.options.backdrop?this.$element[0].focus.call(this.$element[0]):this.hide.call(this))},this)),f&&this.$backdrop[0].offsetWidth,this.$backdrop.addClass("in"),!b)return;f?this.$backdrop.one("bsTransitionEnd",b).emulateTransitionEnd(c.BACKDROP_TRANSITION_DURATION):b()}else if(!this.isShown&&this.$backdrop){this.$backdrop.removeClass("in");var g=function(){d.removeBackdrop(),b&&b()};a.support.transition&&this.$element.hasClass("fade")?this.$backdrop.one("bsTransitionEnd",g).emulateTransitionEnd(c.BACKDROP_TRANSITION_DURATION):g()}else b&&b()},c.prototype.handleUpdate=function(){this.options.backdrop&&this.adjustBackdrop(),this.adjustDialog()},c.prototype.adjustBackdrop=function(){this.$backdrop.css("height",0).css("height",this.$element[0].scrollHeight)},c.prototype.adjustDialog=function(){var a=this.$element[0].scrollHeight>document.documentElement.clientHeight;this.$element.css({paddingLeft:!this.bodyIsOverflowing&&a?this.scrollbarWidth:"",paddingRight:this.bodyIsOverflowing&&!a?this.scrollbarWidth:""})},c.prototype.resetAdjustments=function(){this.$element.css({paddingLeft:"",paddingRight:""})},c.prototype.checkScrollbar=function(){this.bodyIsOverflowing=document.body.scrollHeight>document.documentElement.clientHeight,this.scrollbarWidth=this.measureScrollbar()},c.prototype.setScrollbar=function(){var a=parseInt(this.$body.css("padding-right")||0,10);this.bodyIsOverflowing&&this.$body.css("padding-right",a+this.scrollbarWidth)},c.prototype.resetScrollbar=function(){this.$body.css("padding-right","")},c.prototype.measureScrollbar=function(){var a=document.createElement("div");a.className="modal-scrollbar-measure",this.$body.append(a);var b=a.offsetWidth-a.clientWidth;return this.$body[0].removeChild(a),b};var d=a.fn.modal;a.fn.modal=b,a.fn.modal.Constructor=c,a.fn.modal.noConflict=function(){return a.fn.modal=d,this},a(document).on("click.bs.modal.data-api",'[data-toggle="modal"]',function(c){var d=a(this),e=d.attr("href"),f=a(d.attr("data-target")||e&&e.replace(/.*(?=#[^\s]+$)/,"")),g=f.data("bs.modal")?"toggle":a.extend({remote:!/#/.test(e)&&e},f.data(),d.data());d.is("a")&&c.preventDefault(),f.one("show.bs.modal",function(a){a.isDefaultPrevented()||f.one("hidden.bs.modal",function(){d.is(":visible")&&d.trigger("focus")})}),b.call(f,g,this)})}(jQuery),+function(a){"use strict";function b(b){return this.each(function(){var d=a(this),e=d.data("bs.tooltip"),f="object"==typeof b&&b,g=f&&f.selector;(e||"destroy"!=b)&&(g?(e||d.data("bs.tooltip",e={}),e[g]||(e[g]=new c(this,f))):e||d.data("bs.tooltip",e=new c(this,f)),"string"==typeof b&&e[b]())})}var c=function(a,b){this.type=this.options=this.enabled=this.timeout=this.hoverState=this.$element=null,this.init("tooltip",a,b)};c.VERSION="3.3.1",c.TRANSITION_DURATION=150,c.DEFAULTS={animation:!0,placement:"top",selector:!1,template:'<div class="tooltip" role="tooltip"><div class="tooltip-arrow"></div><div class="tooltip-inner"></div></div>',trigger:"hover focus",title:"",delay:0,html:!1,container:!1,viewport:{selector:"body",padding:0}},c.prototype.init=function(b,c,d){this.enabled=!0,this.type=b,this.$element=a(c),this.options=this.getOptions(d),this.$viewport=this.options.viewport&&a(this.options.viewport.selector||this.options.viewport);for(var e=this.options.trigger.split(" "),f=e.length;f--;){var g=e[f];if("click"==g)this.$element.on("click."+this.type,this.options.selector,a.proxy(this.toggle,this));else if("manual"!=g){var h="hover"==g?"mouseenter":"focusin",i="hover"==g?"mouseleave":"focusout";this.$element.on(h+"."+this.type,this.options.selector,a.proxy(this.enter,this)),this.$element.on(i+"."+this.type,this.options.selector,a.proxy(this.leave,this))}}this.options.selector?this._options=a.extend({},this.options,{trigger:"manual",selector:""}):this.fixTitle()},c.prototype.getDefaults=function(){return c.DEFAULTS},c.prototype.getOptions=function(b){return b=a.extend({},this.getDefaults(),this.$element.data(),b),b.delay&&"number"==typeof b.delay&&(b.delay={show:b.delay,hide:b.delay}),b},c.prototype.getDelegateOptions=function(){var b={},c=this.getDefaults();return this._options&&a.each(this._options,function(a,d){c[a]!=d&&(b[a]=d)}),b},c.prototype.enter=function(b){var c=b instanceof this.constructor?b:a(b.currentTarget).data("bs."+this.type);return c&&c.$tip&&c.$tip.is(":visible")?void(c.hoverState="in"):(c||(c=new this.constructor(b.currentTarget,this.getDelegateOptions()),a(b.currentTarget).data("bs."+this.type,c)),clearTimeout(c.timeout),c.hoverState="in",c.options.delay&&c.options.delay.show?void(c.timeout=setTimeout(function(){"in"==c.hoverState&&c.show()},c.options.delay.show)):c.show())},c.prototype.leave=function(b){var c=b instanceof this.constructor?b:a(b.currentTarget).data("bs."+this.type);return c||(c=new this.constructor(b.currentTarget,this.getDelegateOptions()),a(b.currentTarget).data("bs."+this.type,c)),clearTimeout(c.timeout),c.hoverState="out",c.options.delay&&c.options.delay.hide?void(c.timeout=setTimeout(function(){"out"==c.hoverState&&c.hide()},c.options.delay.hide)):c.hide()},c.prototype.show=function(){var b=a.Event("show.bs."+this.type);if(this.hasContent()&&this.enabled){this.$element.trigger(b);var d=a.contains(this.$element[0].ownerDocument.documentElement,this.$element[0]);if(b.isDefaultPrevented()||!d)return;var e=this,f=this.tip(),g=this.getUID(this.type);this.setContent(),f.attr("id",g),this.$element.attr("aria-describedby",g),this.options.animation&&f.addClass("fade");var h="function"==typeof this.options.placement?this.options.placement.call(this,f[0],this.$element[0]):this.options.placement,i=/\s?auto?\s?/i,j=i.test(h);j&&(h=h.replace(i,"")||"top"),f.detach().css({top:0,left:0,display:"block"}).addClass(h).data("bs."+this.type,this),this.options.container?f.appendTo(this.options.container):f.insertAfter(this.$element);var k=this.getPosition(),l=f[0].offsetWidth,m=f[0].offsetHeight;if(j){var n=h,o=this.options.container?a(this.options.container):this.$element.parent(),p=this.getPosition(o);h="bottom"==h&&k.bottom+m>p.bottom?"top":"top"==h&&k.top-m<p.top?"bottom":"right"==h&&k.right+l>p.width?"left":"left"==h&&k.left-l<p.left?"right":h,f.removeClass(n).addClass(h)}var q=this.getCalculatedOffset(h,k,l,m);this.applyPlacement(q,h);var r=function(){var a=e.hoverState;e.$element.trigger("shown.bs."+e.type),e.hoverState=null,"out"==a&&e.leave(e)};a.support.transition&&this.$tip.hasClass("fade")?f.one("bsTransitionEnd",r).emulateTransitionEnd(c.TRANSITION_DURATION):r()}},c.prototype.applyPlacement=function(b,c){var d=this.tip(),e=d[0].offsetWidth,f=d[0].offsetHeight,g=parseInt(d.css("margin-top"),10),h=parseInt(d.css("margin-left"),10);isNaN(g)&&(g=0),isNaN(h)&&(h=0),b.top=b.top+g,b.left=b.left+h,a.offset.setOffset(d[0],a.extend({using:function(a){d.css({top:Math.round(a.top),left:Math.round(a.left)})}},b),0),d.addClass("in");var i=d[0].offsetWidth,j=d[0].offsetHeight;"top"==c&&j!=f&&(b.top=b.top+f-j);var k=this.getViewportAdjustedDelta(c,b,i,j);k.left?b.left+=k.left:b.top+=k.top;var l=/top|bottom/.test(c),m=l?2*k.left-e+i:2*k.top-f+j,n=l?"offsetWidth":"offsetHeight";d.offset(b),this.replaceArrow(m,d[0][n],l)},c.prototype.replaceArrow=function(a,b,c){this.arrow().css(c?"left":"top",50*(1-a/b)+"%").css(c?"top":"left","")},c.prototype.setContent=function(){var a=this.tip(),b=this.getTitle();a.find(".tooltip-inner")[this.options.html?"html":"text"](b),a.removeClass("fade in top bottom left right")},c.prototype.hide=function(b){function d(){"in"!=e.hoverState&&f.detach(),e.$element.removeAttr("aria-describedby").trigger("hidden.bs."+e.type),b&&b()}var e=this,f=this.tip(),g=a.Event("hide.bs."+this.type);return this.$element.trigger(g),g.isDefaultPrevented()?void 0:(f.removeClass("in"),a.support.transition&&this.$tip.hasClass("fade")?f.one("bsTransitionEnd",d).emulateTransitionEnd(c.TRANSITION_DURATION):d(),this.hoverState=null,this)},c.prototype.fixTitle=function(){var a=this.$element;(a.attr("title")||"string"!=typeof a.attr("data-original-title"))&&a.attr("data-original-title",a.attr("title")||"").attr("title","")},c.prototype.hasContent=function(){return this.getTitle()},c.prototype.getPosition=function(b){b=b||this.$element;var c=b[0],d="BODY"==c.tagName,e=c.getBoundingClientRect();null==e.width&&(e=a.extend({},e,{width:e.right-e.left,height:e.bottom-e.top}));var f=d?{top:0,left:0}:b.offset(),g={scroll:d?document.documentElement.scrollTop||document.body.scrollTop:b.scrollTop()},h=d?{width:a(window).width(),height:a(window).height()}:null;return a.extend({},e,g,h,f)},c.prototype.getCalculatedOffset=function(a,b,c,d){return"bottom"==a?{top:b.top+b.height,left:b.left+b.width/2-c/2}:"top"==a?{top:b.top-d,left:b.left+b.width/2-c/2}:"left"==a?{top:b.top+b.height/2-d/2,left:b.left-c}:{top:b.top+b.height/2-d/2,left:b.left+b.width}},c.prototype.getViewportAdjustedDelta=function(a,b,c,d){var e={top:0,left:0};if(!this.$viewport)return e;var f=this.options.viewport&&this.options.viewport.padding||0,g=this.getPosition(this.$viewport);if(/right|left/.test(a)){var h=b.top-f-g.scroll,i=b.top+f-g.scroll+d;h<g.top?e.top=g.top-h:i>g.top+g.height&&(e.top=g.top+g.height-i)}else{var j=b.left-f,k=b.left+f+c;j<g.left?e.left=g.left-j:k>g.width&&(e.left=g.left+g.width-k)}return e},c.prototype.getTitle=function(){var a,b=this.$element,c=this.options;return a=b.attr("data-original-title")||("function"==typeof c.title?c.title.call(b[0]):c.title)},c.prototype.getUID=function(a){do a+=~~(1e6*Math.random());while(document.getElementById(a));return a},c.prototype.tip=function(){return this.$tip=this.$tip||a(this.options.template)},c.prototype.arrow=function(){return this.$arrow=this.$arrow||this.tip().find(".tooltip-arrow")},c.prototype.enable=function(){this.enabled=!0},c.prototype.disable=function(){this.enabled=!1},c.prototype.toggleEnabled=function(){this.enabled=!this.enabled},c.prototype.toggle=function(b){var c=this;b&&(c=a(b.currentTarget).data("bs."+this.type),c||(c=new this.constructor(b.currentTarget,this.getDelegateOptions()),a(b.currentTarget).data("bs."+this.type,c))),c.tip().hasClass("in")?c.leave(c):c.enter(c)},c.prototype.destroy=function(){var a=this;clearTimeout(this.timeout),this.hide(function(){a.$element.off("."+a.type).removeData("bs."+a.type)})};var d=a.fn.tooltip;a.fn.tooltip=b,a.fn.tooltip.Constructor=c,a.fn.tooltip.noConflict=function(){return a.fn.tooltip=d,this}}(jQuery),+function(a){"use strict";function b(b){return this.each(function(){var d=a(this),e=d.data("bs.popover"),f="object"==typeof b&&b,g=f&&f.selector;(e||"destroy"!=b)&&(g?(e||d.data("bs.popover",e={}),e[g]||(e[g]=new c(this,f))):e||d.data("bs.popover",e=new c(this,f)),"string"==typeof b&&e[b]())})}var c=function(a,b){this.init("popover",a,b)};if(!a.fn.tooltip)throw new Error("Popover requires tooltip.js");c.VERSION="3.3.1",c.DEFAULTS=a.extend({},a.fn.tooltip.Constructor.DEFAULTS,{placement:"right",trigger:"click",content:"",template:'<div class="popover" role="tooltip"><div class="arrow"></div><h3 class="popover-title"></h3><div class="popover-content"></div></div>'}),c.prototype=a.extend({},a.fn.tooltip.Constructor.prototype),c.prototype.constructor=c,c.prototype.getDefaults=function(){return c.DEFAULTS},c.prototype.setContent=function(){var a=this.tip(),b=this.getTitle(),c=this.getContent();a.find(".popover-title")[this.options.html?"html":"text"](b),a.find(".popover-content").children().detach().end()[this.options.html?"string"==typeof c?"html":"append":"text"](c),a.removeClass("fade top bottom left right in"),a.find(".popover-title").html()||a.find(".popover-title").hide()},c.prototype.hasContent=function(){return this.getTitle()||this.getContent()},c.prototype.getContent=function(){var a=this.$element,b=this.options;return a.attr("data-content")||("function"==typeof b.content?b.content.call(a[0]):b.content)},c.prototype.arrow=function(){return this.$arrow=this.$arrow||this.tip().find(".arrow")},c.prototype.tip=function(){return this.$tip||(this.$tip=a(this.options.template)),this.$tip};var d=a.fn.popover;a.fn.popover=b,a.fn.popover.Constructor=c,a.fn.popover.noConflict=function(){return a.fn.popover=d,this}}(jQuery),function(a){function b(a){return"undefined"==typeof a.which?!0:"number"==typeof a.which&&a.which>0?!a.ctrlKey&&!a.metaKey&&!a.altKey&&8!=a.which:!1}a.expr[":"].notmdproc=function(b){return a(b).data("mdproc")?!1:!0},a.material={options:{input:!0,ripples:!0,checkbox:!0,togglebutton:!0,radio:!0,arrive:!0,autofill:!0,withRipples:[".btn:not(.btn-link)",".card-image",".navbar a:not(.withoutripple)",".dropdown-menu a",".nav-tabs a:not(.withoutripple)",".withripple"].join(","),inputElements:"input.form-control, textarea.form-control, select.form-control",checkboxElements:".checkbox > label > input[type=checkbox]",togglebuttonElements:".togglebutton > label > input[type=checkbox]",radioElements:".radio > label > input[type=radio]"},checkbox:function(b){a(b?b:this.options.checkboxElements).filter(":notmdproc").data("mdproc",!0).after("<span class=ripple></span><span class=check></span>")},togglebutton:function(b){a(b?b:this.options.togglebuttonElements).filter(":notmdproc").data("mdproc",!0).after("<span class=toggle></span>")},radio:function(b){a(b?b:this.options.radioElements).filter(":notmdproc").data("mdproc",!0).after("<span class=circle></span><span class=check></span>")},input:function(c){a(c?c:this.options.inputElements).filter(":notmdproc").data("mdproc",!0).each(function(){var b=a(this);if(b.wrap("<div class=form-control-wrapper></div>"),b.after("<span class=material-input></span>"),b.hasClass("floating-label")){var c=b.attr("placeholder");b.attr("placeholder",null).removeClass("floating-label"),b.after("<div class=floating-label>"+c+"</div>")}if(b.attr("data-hint")&&b.after("<div class=hint>"+b.attr("data-hint")+"</div>"),(null===b.val()||"undefined"==b.val()||""===b.val())&&b.addClass("empty"),b.parent().next().is("[type=file]")){b.parent().addClass("fileinput");var d=b.parent().next().detach();b.after(d)}}),a(document).on("change",".checkbox input[type=checkbox]",function(){a(this).blur()}).on("keydown paste",".form-control",function(c){b(c)&&a(this).removeClass("empty")}).on("keyup change",".form-control",function(){var b=a(this);""===b.val()&&b[0].checkValidity()?b.addClass("empty"):b.removeClass("empty")}).on("focus",".form-control-wrapper.fileinput",function(){a(this).find("input").addClass("focus")}).on("blur",".form-control-wrapper.fileinput",function(){a(this).find("input").removeClass("focus")}).on("change",".form-control-wrapper.fileinput [type=file]",function(){var b="";a.each(a(this)[0].files,function(a,c){console.log(c),b+=c.name+", "}),b=b.substring(0,b.length-2),b?a(this).prev().removeClass("empty"):a(this).prev().addClass("empty"),a(this).prev().val(b)})},ripples:function(b){a(b?b:this.options.withRipples).ripples()},autofill:function(){var b=setInterval(function(){a("input[type!=checkbox]").each(function(){a(this).val()&&a(this).val()!==a(this).attr("value")&&a(this).trigger("change")})},100);setTimeout(function(){clearInterval(b)},1e4);var c;a(document).on("focus","input",function(){var b=a(this).parents("form").find("input").not("[type=file]");c=setInterval(function(){b.each(function(){a(this).val()!==a(this).attr("value")&&a(this).trigger("change")})},100)}).on("blur","input",function(){clearInterval(c)})},init:function(){a.ripples&&this.options.ripples&&this.ripples(),this.options.input&&this.input(),this.options.checkbox&&this.checkbox(),this.options.togglebutton&&this.togglebutton(),this.options.radio&&this.radio(),this.options.autofill&&this.autofill(),document.arrive&&this.options.arrive&&(a(document).arrive(this.options.inputElements,function(){a.material.input(a(this))}),a(document).arrive(this.options.checkboxElements,function(){a.material.checkbox(a(this))}),a(document).arrive(this.options.radioElements,function(){a.material.radio(a(this))}),a(document).arrive(this.options.togglebuttonElements,function(){a.material.togglebutton(a(this))}))}}}(jQuery),function(a,b,c,d){"use strict";function e(b,c){g=this,this.element=a(b),this.options=a.extend({},h,c),this._defaults=h,this._name=f,this.init()}var f="ripples",g=null,h={};e.prototype.init=function(){var c=this.element;c.on("mousedown touchstart",function(d){if(g.isTouch()&&"mousedown"===d.type)return!1;c.find(".ripple-wrapper").length||c.append('<div class="ripple-wrapper"></div>');var e=c.children(".ripple-wrapper"),f=g.getRelY(e,d),h=g.getRelX(e,d);if(f||h){var i=g.getRipplesColor(),j=a("<div></div>");j.addClass("ripple").css({left:h,top:f,"background-color":i}),e.append(j),function(){return b.getComputedStyle(j[0]).opacity}(),g.rippleOn(j),setTimeout(function(){g.rippleEnd(j)},500),c.on("mouseup mouseleave touchend",function(){j.data("mousedown","off"),"off"===j.data("animating")&&g.rippleOut(j)})}})},e.prototype.getNewSize=function(a){var b=this.element;return Math.max(b.outerWidth(),b.outerHeight())/a.outerWidth()*2.5},e.prototype.getRelX=function(a,b){var c=a.offset();return g.isTouch()?(b=b.originalEvent,1!==b.touches.length?b.touches[0].pageX-c.left:!1):b.pageX-c.left},e.prototype.getRelY=function(a,b){var c=a.offset();return g.isTouch()?(b=b.originalEvent,1!==b.touches.length?b.touches[0].pageY-c.top:!1):b.pageY-c.top},e.prototype.getRipplesColor=function(){var a,c=this.element;return a=this.options&&this.options.color?this.options.color:c.data("ripple-color")?c.data("ripple-color"):b.getComputedStyle(c[0]).color},e.prototype.hasTransitionSupport=function(){var a=c.body||c.documentElement,b=a.style,e=b.transition!==d||b.WebkitTransition!==d||b.MozTransition!==d||b.MsTransition!==d||b.OTransition!==d;return e},e.prototype.isTouch=function(){return/Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent)},e.prototype.rippleEnd=function(a){a.data("animating","off"),"off"===a.data("mousedown")&&g.rippleOut(a)},e.prototype.rippleOut=function(a){a.off(),g.hasTransitionSupport()?a.addClass("ripple-out"):a.animate({opacity:0},100,function(){a.trigger("transitionend")}),a.on("transitionend webkitTransitionEnd oTransitionEnd MSTransitionEnd",function(){a.remove()})},e.prototype.rippleOn=function(a){var b=g.getNewSize(a),c=this.element;g.hasTransitionSupport()?a.css({"-ms-transform":"scale("+b+")","-moz-transform":"scale("+b+")","-webkit-transform":"scale("+b+")",transform:"scale("+b+")"}).addClass("ripple-on").data("animating","on").data("mousedown","on"):a.animate({width:2*Math.max(c.outerWidth(),c.outerHeight()),height:2*Math.max(c.outerWidth(),c.outerHeight()),"margin-left":-1*Math.max(c.outerWidth(),c.outerHeight()),"margin-top":-1*Math.max(c.outerWidth(),c.outerHeight()),opacity:.2},500,function(){a.trigger("transitionend")})},a.fn.ripples=function(b){return this.each(function(){a.data(this,"plugin_"+f)||a.data(this,"plugin_"+f,new e(this,b))})}}(jQuery,window,document),function(a){function b(a){return"undefined"!=typeof a&&null!==a?!0:!1}a(document).ready(function(){a("body").append("<div id=snackbar-container/>")}),a(document).on("click","[data-toggle=snackbar]",function(){a(this).snackbar("toggle")}).on("click","#snackbar-container .snackbar",function(){a(this).snackbar("hide")}),a.snackbar=function(c){if(b(c)&&c===Object(c)){var d;d=b(c.id)?a("#"+c.id):a("<div/>").attr("id","snackbar"+Date.now()).attr("class","snackbar");var e=d.hasClass("snackbar-opened");b(c.style)?d.attr("class","snackbar "+c.style):d.attr("class","snackbar"),c.timeout=b(c.timeout)?c.timeout:3e3,b(c.content)&&(d.find(".snackbar-content").length?d.find(".snackbar-content").text(c.content):d.prepend("<span class=snackbar-content>"+c.content+"</span>")),b(c.id)?d.insertAfter("#snackbar-container .snackbar:last-child"):d.appendTo("#snackbar-container"),b(c.action)&&"toggle"==c.action&&(e?c.action="hide":c.action="show");var f=Date.now();d.data("animationId1",f),setTimeout(function(){d.data("animationId1")===f&&(b(c.action)&&"show"!=c.action?b(c.action)&&"hide"==c.action&&d.removeClass("snackbar-opened"):d.addClass("snackbar-opened"))},50);var g=Date.now();return d.data("animationId2",g),0!==c.timeout&&setTimeout(function(){d.data("animationId2")===g&&d.removeClass("snackbar-opened")},c.timeout),d}return!1},a.fn.snackbar=function(c){var d={};if(this.hasClass("snackbar"))return d.id=this.attr("id"),("show"===c||"hide"===c||"toggle"==c)&&(d.action=c),a.snackbar(d);b(c)&&"show"!==c&&"hide"!==c&&"toggle"!=c||(d={content:a(this).attr("data-content"),style:a(this).attr("data-style"),timeout:a(this).attr("data-timeout")}),b(c)&&(d.id=this.attr("data-snackbar-id"),("show"===c||"hide"===c||"toggle"==c)&&(d.action=c));var e=a.snackbar(d);return this.attr("data-snackbar-id",e.attr("id")),e}}(jQuery),function a(b,c,d){function e(g,h){if(!c[g]){if(!b[g]){var i="function"==typeof require&&require;if(!h&&i)return i(g,!0);if(f)return f(g,!0);var j=new Error("Cannot find module '"+g+"'");throw j.code="MODULE_NOT_FOUND",j}var k=c[g]={exports:{}};b[g][0].call(k.exports,function(a){var c=b[g][1][a];return e(c?c:a)},k,k.exports,a,b,c,d)}return c[g].exports}for(var f="function"==typeof require&&require,g=0;g<d.length;g++)e(d[g]);return e}({1:[function(a,b,c){if(!d)var d={map:function(a,b){var c={};return b?a.map(function(a,d){return c.index=d,b.call(c,a)}):a.slice()},naturalOrder:function(a,b){return b>a?-1:a>b?1:0},sum:function(a,b){var c={};return a.reduce(b?function(a,d,e){return c.index=e,a+b.call(c,d)}:function(a,b){return a+b},0)},max:function(a,b){return Math.max.apply(null,b?d.map(a,b):a)}};var e=function(){function a(a,b,c){return(a<<2*j)+(b<<j)+c}function b(a){function b(){c.sort(a),d=!0}var c=[],d=!1;return{push:function(a){c.push(a),d=!1},peek:function(a){return d||b(),void 0===a&&(a=c.length-1),c[a]},pop:function(){return d||b(),c.pop()},size:function(){return c.length},map:function(a){return c.map(a)},debug:function(){return d||b(),c}}}function c(a,b,c,d,e,f,g){var h=this;h.r1=a,h.r2=b,h.g1=c,h.g2=d,h.b1=e,h.b2=f,h.histo=g}function e(){this.vboxes=new b(function(a,b){return d.naturalOrder(a.vbox.count()*a.vbox.volume(),b.vbox.count()*b.vbox.volume())})}function f(b){var c,d,e,f,g=1<<3*j,h=new Array(g);return b.forEach(function(b){d=b[0]>>k,e=b[1]>>k,f=b[2]>>k,c=a(d,e,f),h[c]=(h[c]||0)+1}),h}function g(a,b){var d,e,f,g=1e6,h=0,i=1e6,j=0,l=1e6,m=0;return a.forEach(function(a){d=a[0]>>k,e=a[1]>>k,f=a[2]>>k,g>d?g=d:d>h&&(h=d),i>e?i=e:e>j&&(j=e),l>f?l=f:f>m&&(m=f)}),new c(g,h,i,j,l,m,b)}function h(b,c){function e(a){var b,d,e,f,g,h=a+"1",i=a+"2",k=0;for(j=c[h];j<=c[i];j++)if(p[j]>o/2){for(e=c.copy(),f=c.copy(),b=j-c[h],d=c[i]-j,g=d>=b?Math.min(c[i]-1,~~(j+d/2)):Math.max(c[h],~~(j-1-b/2));!p[g];)g++;for(k=q[g];!k&&p[g-1];)k=q[--g];return e[i]=g,f[h]=e[i]+1,[e,f]}}if(c.count()){var f=c.r2-c.r1+1,g=c.g2-c.g1+1,h=c.b2-c.b1+1,i=d.max([f,g,h]);if(1==c.count())return[c.copy()];var j,k,l,m,n,o=0,p=[],q=[];if(i==f)for(j=c.r1;j<=c.r2;j++){for(m=0,k=c.g1;k<=c.g2;k++)for(l=c.b1;l<=c.b2;l++)n=a(j,k,l),m+=b[n]||0;o+=m,p[j]=o}else if(i==g)for(j=c.g1;j<=c.g2;j++){for(m=0,k=c.r1;k<=c.r2;k++)for(l=c.b1;l<=c.b2;l++)n=a(k,j,l),m+=b[n]||0;o+=m,p[j]=o}else for(j=c.b1;j<=c.b2;j++){for(m=0,k=c.r1;k<=c.r2;k++)for(l=c.g1;l<=c.g2;l++)n=a(k,l,j),m+=b[n]||0;o+=m,p[j]=o}return p.forEach(function(a,b){q[b]=o-a}),e(i==f?"r":i==g?"g":"b")}}function i(a,c){function i(a,b){for(var c,d=1,e=0;l>e;)if(c=a.pop(),c.count()){var f=h(j,c),g=f[0],i=f[1];if(!g)return;if(a.push(g),i&&(a.push(i),d++),d>=b)return;if(e++>l)return}else a.push(c),e++}if(!a.length||2>c||c>256)return!1;var j=f(a),k=0;j.forEach(function(){k++});var n=g(a,j),o=new b(function(a,b){return d.naturalOrder(a.count(),b.count())});o.push(n),i(o,m*c);for(var p=new b(function(a,b){return d.naturalOrder(a.count()*a.volume(),b.count()*b.volume())});o.size();)p.push(o.pop());i(p,c-p.size());for(var q=new e;p.size();)q.push(p.pop());return q}var j=5,k=8-j,l=1e3,m=.75;return c.prototype={volume:function(a){var b=this;return(!b._volume||a)&&(b._volume=(b.r2-b.r1+1)*(b.g2-b.g1+1)*(b.b2-b.b1+1)),b._volume},count:function(b){var c=this,d=c.histo;if(!c._count_set||b){var e,f,g,h=0;for(e=c.r1;e<=c.r2;e++)for(f=c.g1;f<=c.g2;f++)for(g=c.b1;g<=c.b2;g++)index=a(e,f,g),h+=d[index]||0;c._count=h,c._count_set=!0}return c._count},copy:function(){var a=this;return new c(a.r1,a.r2,a.g1,a.g2,a.b1,a.b2,a.histo)},avg:function(b){var c=this,d=c.histo;if(!c._avg||b){var e,f,g,h,i,k=0,l=1<<8-j,m=0,n=0,o=0;for(f=c.r1;f<=c.r2;f++)for(g=c.g1;g<=c.g2;g++)for(h=c.b1;h<=c.b2;h++)i=a(f,g,h),e=d[i]||0,k+=e,m+=e*(f+.5)*l,n+=e*(g+.5)*l,o+=e*(h+.5)*l;k?c._avg=[~~(m/k),~~(n/k),~~(o/k)]:c._avg=[~~(l*(c.r1+c.r2+1)/2),~~(l*(c.g1+c.g2+1)/2),~~(l*(c.b1+c.b2+1)/2)]}return c._avg},contains:function(a){var b=this,c=a[0]>>k;return gval=a[1]>>k,bval=a[2]>>k,c>=b.r1&&c<=b.r2&&gval>=b.g1&&gval<=b.g2&&bval>=b.b1&&bval<=b.b2}},e.prototype={push:function(a){this.vboxes.push({vbox:a,color:a.avg()})},palette:function(){return this.vboxes.map(function(a){return a.color})},size:function(){return this.vboxes.size()},map:function(a){for(var b=this.vboxes,c=0;c<b.size();c++)if(b.peek(c).vbox.contains(a))return b.peek(c).color;return this.nearest(a)},nearest:function(a){for(var b,c,d,e=this.vboxes,f=0;f<e.size();f++)c=Math.sqrt(Math.pow(a[0]-e.peek(f).color[0],2)+Math.pow(a[1]-e.peek(f).color[1],2)+Math.pow(a[2]-e.peek(f).color[2],2)),(b>c||void 0===b)&&(b=c,d=e.peek(f).color);return d},forcebw:function(){var a=this.vboxes;a.sort(function(a,b){return d.naturalOrder(d.sum(a.color),d.sum(b.color))});var b=a[0].color;b[0]<5&&b[1]<5&&b[2]<5&&(a[0].color=[0,0,0]);var c=a.length-1,e=a[c].color;e[0]>251&&e[1]>251&&e[2]>251&&(a[c].color=[255,255,255])}},{quantize:i}}();b.exports=e.quantize},{}],2:[function(a,b,c){(function(){var b,c,d,e=function(a,b){return function(){return a.apply(b,arguments)}},f=[].slice;window.Swatch=c=function(){function a(a,b){this.rgb=a,this.population=b}return a.prototype.hsl=void 0,a.prototype.rgb=void 0,a.prototype.population=1,a.yiq=0,a.prototype.getHsl=function(){return this.hsl?this.hsl:this.hsl=d.rgbToHsl(this.rgb[0],this.rgb[1],this.rgb[2])},a.prototype.getPopulation=function(){return this.population},a.prototype.getRgb=function(){return this.rgb},a.prototype.getHex=function(){return"#"+((1<<24)+(this.rgb[0]<<16)+(this.rgb[1]<<8)+this.rgb[2]).toString(16).slice(1,7)},a.prototype.getTitleTextColor=function(){return this._ensureTextColors(),this.yiq<200?"#fff":"#000"},a.prototype.getBodyTextColor=function(){return this._ensureTextColors(),this.yiq<150?"#fff":"#000"},a.prototype._ensureTextColors=function(){return this.yiq?void 0:this.yiq=(299*this.rgb[0]+587*this.rgb[1]+114*this.rgb[2])/1e3},a}(),window.Vibrant=d=function(){function d(a,d,f){this.swatches=e(this.swatches,this);var g,h,i,j,k,l,m,n,o,p,q,r;for("undefined"==typeof d&&(d=64),"undefined"==typeof f&&(f=5),m=new b(a),n=m.getImageData(),q=n.data,p=m.getPixelCount(),h=[],l=0;p>l;)o=4*l,r=q[o+0],k=q[o+1],i=q[o+2],g=q[o+3],g>=125&&(r>250&&k>250&&i>250||h.push([r,k,i])),l+=f;j=this.quantize(h,d),this._swatches=j.vboxes.map(function(a){return function(a){return new c(a.color,a.vbox.count())}}(this)),this.maxPopulation=this.findMaxPopulation,this.generateVarationColors(),this.generateEmptySwatches(),m.removeCanvas()}return d.prototype.quantize=a("quantize"),d.prototype._swatches=[],d.prototype.TARGET_DARK_LUMA=.26,d.prototype.MAX_DARK_LUMA=.45,d.prototype.MIN_LIGHT_LUMA=.55,d.prototype.TARGET_LIGHT_LUMA=.74,d.prototype.MIN_NORMAL_LUMA=.3,d.prototype.TARGET_NORMAL_LUMA=.5,d.prototype.MAX_NORMAL_LUMA=.7,d.prototype.TARGET_MUTED_SATURATION=.3,d.prototype.MAX_MUTED_SATURATION=.4,d.prototype.TARGET_VIBRANT_SATURATION=1,d.prototype.MIN_VIBRANT_SATURATION=.35,d.prototype.WEIGHT_SATURATION=3,d.prototype.WEIGHT_LUMA=6,d.prototype.WEIGHT_POPULATION=1,d.prototype.VibrantSwatch=void 0,d.prototype.MutedSwatch=void 0,d.prototype.DarkVibrantSwatch=void 0,d.prototype.DarkMutedSwatch=void 0,d.prototype.LightVibrantSwatch=void 0,d.prototype.LightMutedSwatch=void 0,d.prototype.HighestPopulation=0,d.prototype.generateVarationColors=function(){return this.VibrantSwatch=this.findColorVariation(this.TARGET_NORMAL_LUMA,this.MIN_NORMAL_LUMA,this.MAX_NORMAL_LUMA,this.TARGET_VIBRANT_SATURATION,this.MIN_VIBRANT_SATURATION,1),
this.LightVibrantSwatch=this.findColorVariation(this.TARGET_LIGHT_LUMA,this.MIN_LIGHT_LUMA,1,this.TARGET_VIBRANT_SATURATION,this.MIN_VIBRANT_SATURATION,1),this.DarkVibrantSwatch=this.findColorVariation(this.TARGET_DARK_LUMA,0,this.MAX_DARK_LUMA,this.TARGET_VIBRANT_SATURATION,this.MIN_VIBRANT_SATURATION,1),this.MutedSwatch=this.findColorVariation(this.TARGET_NORMAL_LUMA,this.MIN_NORMAL_LUMA,this.MAX_NORMAL_LUMA,this.TARGET_MUTED_SATURATION,0,this.MAX_MUTED_SATURATION),this.LightMutedSwatch=this.findColorVariation(this.TARGET_LIGHT_LUMA,this.MIN_LIGHT_LUMA,1,this.TARGET_MUTED_SATURATION,0,this.MAX_MUTED_SATURATION),this.DarkMutedSwatch=this.findColorVariation(this.TARGET_DARK_LUMA,0,this.MAX_DARK_LUMA,this.TARGET_MUTED_SATURATION,0,this.MAX_MUTED_SATURATION)},d.prototype.generateEmptySwatches=function(){var a;return void 0===this.VibrantSwatch&&void 0!==this.DarkVibrantSwatch&&(a=this.DarkVibrantSwatch.getHsl(),a[2]=this.TARGET_NORMAL_LUMA,this.VibrantSwatch=new c(d.hslToRgb(a[0],a[1],a[2]),0)),void 0===this.DarkVibrantSwatch&&void 0!==this.VibrantSwatch?(a=this.VibrantSwatch.getHsl(),a[2]=this.TARGET_DARK_LUMA,this.DarkVibrantSwatch=new c(d.hslToRgb(a[0],a[1],a[2]),0)):void 0},d.prototype.findMaxPopulation=function(){var a,b,c,d,e;for(c=0,d=this._swatches,a=0,b=d.length;b>a;a++)e=d[a],c=Math.max(c,e.getPopulation());return c},d.prototype.findColorVariation=function(a,b,c,d,e,f){var g,h,i,j,k,l,m,n,o;for(j=void 0,k=0,l=this._swatches,g=0,h=l.length;h>g;g++)n=l[g],m=n.getHsl()[1],i=n.getHsl()[2],m>=e&&f>=m&&i>=b&&c>=i&&!this.isAlreadySelected(n)&&(o=this.createComparisonValue(m,d,i,a,n.getPopulation(),this.HighestPopulation),(void 0===j||o>k)&&(j=n,k=o));return j},d.prototype.createComparisonValue=function(a,b,c,d,e,f){return this.weightedMean(this.invertDiff(a,b),this.WEIGHT_SATURATION,this.invertDiff(c,d),this.WEIGHT_LUMA,e/f,this.WEIGHT_POPULATION)},d.prototype.invertDiff=function(a,b){return 1-Math.abs(a-b)},d.prototype.weightedMean=function(){var a,b,c,d,e,g;for(e=1<=arguments.length?f.call(arguments,0):[],b=0,c=0,a=0;a<e.length;)d=e[a],g=e[a+1],b+=d*g,c+=g,a+=2;return b/c},d.prototype.swatches=function(){return{Vibrant:this.VibrantSwatch,Muted:this.MutedSwatch,DarkVibrant:this.DarkVibrantSwatch,DarkMuted:this.DarkMutedSwatch,LightVibrant:this.LightVibrantSwatch,LightMuted:this.LightMuted}},d.prototype.isAlreadySelected=function(a){return this.VibrantSwatch===a||this.DarkVibrantSwatch===a||this.LightVibrantSwatch===a||this.MutedSwatch===a||this.DarkMutedSwatch===a||this.LightMutedSwatch===a},d.rgbToHsl=function(a,b,c){var d,e,f,g,h,i;if(a/=255,b/=255,c/=255,g=Math.max(a,b,c),h=Math.min(a,b,c),e=void 0,i=void 0,f=(g+h)/2,g===h)e=i=0;else{switch(d=g-h,i=f>.5?d/(2-g-h):d/(g+h),g){case a:e=(b-c)/d+(c>b?6:0);break;case b:e=(c-a)/d+2;break;case c:e=(a-b)/d+4}e/=6}return[e,i,f]},d.hslToRgb=function(a,b,c){var d,e,f,g,h,i;return i=void 0,e=void 0,d=void 0,f=function(a,b,c){return 0>c&&(c+=1),c>1&&(c-=1),1/6>c?a+6*(b-a)*c:.5>c?b:2/3>c?a+(b-a)*(2/3-c)*6:a},0===b?i=e=d=c:(h=.5>c?c*(1+b):c+b-c*b,g=2*c-h,i=f(g,h,a+1/3),e=f(g,h,a),d=f(g,h,a-1/3)),[255*i,255*e,255*d]},d}(),window.CanvasImage=b=function(){function a(a){this.canvas=document.createElement("canvas"),this.context=this.canvas.getContext("2d"),document.body.appendChild(this.canvas),this.width=this.canvas.width=a.width,this.height=this.canvas.height=a.height,this.context.drawImage(a,0,0,this.width,this.height)}return a.prototype.clear=function(){return this.context.clearRect(0,0,this.width,this.height)},a.prototype.update=function(a){return this.context.putImageData(a,0,0)},a.prototype.getPixelCount=function(){return this.width*this.height},a.prototype.getImageData=function(){return this.context.getImageData(0,0,this.width,this.height)},a.prototype.removeCanvas=function(){return this.canvas.parentNode.removeChild(this.canvas)},a}()}).call(this)},{quantize:1}]},{},[2]),function(a,b){"use strict";function c(c,d){function e(a){return qa.preferFlash&&ja&&!qa.ignoreFlash&&qa.flash[a]!==b&&qa.flash[a]}function f(a){return function(b){var c,d=this._s;return d&&d._a?c=a.call(this,b):(d&&d.id?qa._wD(d.id+": Ignoring "+b.type):qa._wD(va+"Ignoring "+b.type),c=null),c}}this.setupOptions={url:c||null,flashVersion:8,debugMode:!0,debugFlash:!1,useConsole:!0,consoleOnly:!0,waitForWindowLoad:!1,bgColor:"#ffffff",useHighPerformance:!1,flashPollingInterval:null,html5PollingInterval:null,flashLoadTimeout:1e3,wmode:null,allowScriptAccess:"always",useFlashBlock:!1,useHTML5Audio:!0,html5Test:/^(probably|maybe)$/i,preferFlash:!1,noSWFCache:!1,idPrefix:"sound"},this.defaultOptions={autoLoad:!1,autoPlay:!1,from:null,loops:1,onid3:null,onload:null,whileloading:null,onplay:null,onpause:null,onresume:null,whileplaying:null,onposition:null,onstop:null,onfailure:null,onfinish:null,multiShot:!0,multiShotEvents:!1,position:null,pan:0,stream:!0,to:null,type:null,usePolicyFile:!1,volume:100},this.flash9Options={isMovieStar:null,usePeakData:!1,useWaveformData:!1,useEQData:!1,onbufferchange:null,ondataerror:null},this.movieStarOptions={bufferTime:3,serverURL:null,onconnect:null,duration:null},this.audioFormats={mp3:{type:['audio/mpeg; codecs="mp3"',"audio/mpeg","audio/mp3","audio/MPA","audio/mpa-robust"],required:!0},mp4:{related:["aac","m4a","m4b"],type:['audio/mp4; codecs="mp4a.40.2"',"audio/aac","audio/x-m4a","audio/MP4A-LATM","audio/mpeg4-generic"],required:!1},ogg:{type:["audio/ogg; codecs=vorbis"],required:!1},opus:{type:["audio/ogg; codecs=opus","audio/opus"],required:!1},wav:{type:['audio/wav; codecs="1"',"audio/wav","audio/wave","audio/x-wav"],required:!1}},this.movieID="sm2-container",this.id=d||"sm2movie",this.debugID="soundmanager-debug",this.debugURLParam=/([#?&])debug=1/i,this.versionNumber="V2.97a.20140901",this.version=null,this.movieURL=null,this.altURL=null,this.swfLoaded=!1,this.enabled=!1,this.oMC=null,this.sounds={},this.soundIDs=[],this.muted=!1,this.didFlashBlock=!1,this.filePattern=null,this.filePatterns={flash8:/\.mp3(\?.*)?$/i,flash9:/\.mp3(\?.*)?$/i},this.features={buffering:!1,peakData:!1,waveformData:!1,eqData:!1,movieStar:!1},this.sandbox={type:null,types:{remote:"remote (domain-based) rules",localWithFile:"local with file access (no internet access)",localWithNetwork:"local with network (internet access only, no local access)",localTrusted:"local, trusted (local+internet access)"},description:null,noRemote:null,noLocal:null},this.html5={usingFlash:null},this.flash={},this.html5Only=!1,this.ignoreFlash=!1;var g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,$,_,aa,ba,ca,da,ea,fa,ga,ha,ia,ja,ka,la,ma,na,oa,pa,qa=this,ra=null,sa=null,ta="soundManager",ua=ta+": ",va="HTML5::",wa=navigator.userAgent,xa=a.location.href.toString(),ya=document,za=[],Aa=!0,Ba=!1,Ca=!1,Da=!1,Ea=!1,Fa=!1,Ga=0,Ha=["log","info","warn","error"],Ia=8,Ja=null,Ka=null,La=!1,Ma=!1,Na=0,Oa=null,Pa=[],Qa=null,Ra=Array.prototype.slice,Sa=!1,Ta=0,Ua=wa.match(/(ipad|iphone|ipod)/i),Va=wa.match(/android/i),Wa=wa.match(/msie/i),Xa=wa.match(/webkit/i),Ya=wa.match(/safari/i)&&!wa.match(/chrome/i),Za=wa.match(/opera/i),$a=wa.match(/(mobile|pre\/|xoom)/i)||Ua||Va,_a=!xa.match(/usehtml5audio/i)&&!xa.match(/sm2\-ignorebadua/i)&&Ya&&!wa.match(/silk/i)&&wa.match(/OS X 10_6_([3-7])/i),ab=a.console!==b&&console.log!==b,bb=ya.hasFocus!==b?ya.hasFocus():null,cb=Ya&&(ya.hasFocus===b||!ya.hasFocus()),db=!cb,eb=/(mp3|mp4|mpa|m4a|m4b)/i,fb=1e3,gb="about:blank",hb="data:audio/wave;base64,/UklGRiYAAABXQVZFZm10IBAAAAABAAEARKwAAIhYAQACABAAZGF0YQIAAAD//w==",ib=ya.location?ya.location.protocol.match(/http/i):null,jb=ib?"":"http://",kb=/^\s*audio\/(?:x-)?(?:mpeg4|aac|flv|mov|mp4||m4v|m4a|m4b|mp4v|3gp|3g2)\s*(?:$|;)/i,lb=["mpeg4","aac","flv","mov","mp4","m4v","f4v","m4a","m4b","mp4v","3gp","3g2"],mb=new RegExp("\\.("+lb.join("|")+")(\\?.*)?$","i");this.mimePattern=/^\s*audio\/(?:x-)?(?:mp(?:eg|3))\s*(?:$|;)/i,this.useAltURL=!ib,R={swfBox:"sm2-object-box",swfDefault:"movieContainer",swfError:"swf_error",swfTimedout:"swf_timedout",swfLoaded:"swf_loaded",swfUnblocked:"swf_unblocked",sm2Debug:"sm2_debug",highPerf:"high_performance",flashDebug:"flash_debug"},this.hasHTML5=function(){try{return Audio!==b&&(Za&&opera!==b&&opera.version()<10?new Audio(null):new Audio).canPlayType!==b}catch(a){return!1}}(),this.setup=function(a){var c=!qa.url;return a!==b&&Da&&Qa&&qa.ok()&&(a.flashVersion!==b||a.url!==b||a.html5Test!==b)&&V(O("setupLate")),q(a),a&&(c&&F&&a.url!==b&&qa.beginDelayedInit(),F||a.url===b||"complete"!==ya.readyState||setTimeout(D,1)),qa},this.ok=function(){return Qa?Da&&!Ea:qa.useHTML5Audio&&qa.hasHTML5},this.supported=this.ok,this.getMovie=function(b){return h(b)||ya[b]||a[b]},this.createSound=function(a,c){function d(){return h=T(h),qa.sounds[h.id]=new g(h),qa.soundIDs.push(h.id),qa.sounds[h.id]}var e,f,h,i=null;if(e=ta+".createSound(): ",f=e+O(Da?"notOK":"notReady"),!Da||!qa.ok())return V(f),!1;if(c!==b&&(a={id:a,url:c}),h=p(a),h.url=$(h.url),void 0===h.id&&(h.id=qa.setupOptions.idPrefix+Ta++),h.id.toString().charAt(0).match(/^[0-9]$/)&&qa._wD(e+O("badID",h.id),2),qa._wD(e+h.id+(h.url?" ("+h.url+")":""),1),W(h.id,!0))return qa._wD(e+h.id+" exists",1),qa.sounds[h.id];if(ba(h))i=d(),qa._wD(h.id+": Using HTML5"),i._setup_html5(h);else{if(qa.html5Only)return qa._wD(h.id+": No HTML5 support for this sound, and no Flash. Exiting."),d();if(qa.html5.usingFlash&&h.url&&h.url.match(/data\:/i))return qa._wD(h.id+": data: URIs not supported via Flash. Exiting."),d();l>8&&(null===h.isMovieStar&&(h.isMovieStar=!!(h.serverURL||(h.type?h.type.match(kb):!1)||h.url&&h.url.match(mb))),h.isMovieStar&&(qa._wD(e+"using MovieStar handling"),h.loops>1&&n("noNSLoop"))),h=U(h,e),i=d(),8===l?sa._createSound(h.id,h.loops||1,h.usePolicyFile):(sa._createSound(h.id,h.url,h.usePeakData,h.useWaveformData,h.useEQData,h.isMovieStar,h.isMovieStar?h.bufferTime:!1,h.loops||1,h.serverURL,h.duration||null,h.autoPlay,!0,h.autoLoad,h.usePolicyFile),h.serverURL||(i.connected=!0,h.onconnect&&h.onconnect.apply(i))),h.serverURL||!h.autoLoad&&!h.autoPlay||i.load(h)}return!h.serverURL&&h.autoPlay&&i.play(),i},this.destroySound=function(a,b){if(!W(a))return!1;var c,d=qa.sounds[a];for(d._iO={},d.stop(),d.unload(),c=0;c<qa.soundIDs.length;c++)if(qa.soundIDs[c]===a){qa.soundIDs.splice(c,1);break}return b||d.destruct(!0),d=null,delete qa.sounds[a],!0},this.load=function(a,b){return W(a)?qa.sounds[a].load(b):!1},this.unload=function(a){return W(a)?qa.sounds[a].unload():!1},this.onPosition=function(a,b,c,d){return W(a)?qa.sounds[a].onposition(b,c,d):!1},this.onposition=this.onPosition,this.clearOnPosition=function(a,b,c){return W(a)?qa.sounds[a].clearOnPosition(b,c):!1},this.play=function(a,b){var c=null,d=b&&!(b instanceof Object);if(!Da||!qa.ok())return V(ta+".play(): "+O(Da?"notOK":"notReady")),!1;if(W(a,d))d&&(b={url:b});else{if(!d)return!1;d&&(b={url:b}),b&&b.url&&(qa._wD(ta+'.play(): Attempting to create "'+a+'"',1),b.id=a,c=qa.createSound(b).play())}return null===c&&(c=qa.sounds[a].play(b)),c},this.start=this.play,this.setPosition=function(a,b){return W(a)?qa.sounds[a].setPosition(b):!1},this.stop=function(a){return W(a)?(qa._wD(ta+".stop("+a+")",1),qa.sounds[a].stop()):!1},this.stopAll=function(){var a;qa._wD(ta+".stopAll()",1);for(a in qa.sounds)qa.sounds.hasOwnProperty(a)&&qa.sounds[a].stop()},this.pause=function(a){return W(a)?qa.sounds[a].pause():!1},this.pauseAll=function(){var a;for(a=qa.soundIDs.length-1;a>=0;a--)qa.sounds[qa.soundIDs[a]].pause()},this.resume=function(a){return W(a)?qa.sounds[a].resume():!1},this.resumeAll=function(){var a;for(a=qa.soundIDs.length-1;a>=0;a--)qa.sounds[qa.soundIDs[a]].resume()},this.togglePause=function(a){return W(a)?qa.sounds[a].togglePause():!1},this.setPan=function(a,b){return W(a)?qa.sounds[a].setPan(b):!1},this.setVolume=function(a,b){return W(a)?qa.sounds[a].setVolume(b):!1},this.mute=function(a){var b=0;if(a instanceof String&&(a=null),a)return W(a)?(qa._wD(ta+'.mute(): Muting "'+a+'"'),qa.sounds[a].mute()):!1;for(qa._wD(ta+".mute(): Muting all sounds"),b=qa.soundIDs.length-1;b>=0;b--)qa.sounds[qa.soundIDs[b]].mute();return qa.muted=!0,!0},this.muteAll=function(){qa.mute()},this.unmute=function(a){var b;if(a instanceof String&&(a=null),a)return W(a)?(qa._wD(ta+'.unmute(): Unmuting "'+a+'"'),qa.sounds[a].unmute()):!1;for(qa._wD(ta+".unmute(): Unmuting all sounds"),b=qa.soundIDs.length-1;b>=0;b--)qa.sounds[qa.soundIDs[b]].unmute();return qa.muted=!1,!0},this.unmuteAll=function(){qa.unmute()},this.toggleMute=function(a){return W(a)?qa.sounds[a].toggleMute():!1},this.getMemoryUse=function(){var a=0;return sa&&8!==l&&(a=parseInt(sa._getMemoryUse(),10)),a},this.disable=function(c){var d;if(c===b&&(c=!1),Ea)return!1;for(Ea=!0,n("shutdown",1),d=qa.soundIDs.length-1;d>=0;d--)L(qa.sounds[qa.soundIDs[d]]);return o(c),ha.remove(a,"load",u),!0},this.canPlayMIME=function(a){var b;return qa.hasHTML5&&(b=ca({type:a})),!b&&Qa&&(b=a&&qa.ok()?!!((l>8?a.match(kb):null)||a.match(qa.mimePattern)):null),b},this.canPlayURL=function(a){var b;return qa.hasHTML5&&(b=ca({url:a})),!b&&Qa&&(b=a&&qa.ok()?!!a.match(qa.filePattern):null),b},this.canPlayLink=function(a){return a.type!==b&&a.type&&qa.canPlayMIME(a.type)?!0:qa.canPlayURL(a.href)},this.getSoundById=function(a,b){if(!a)return null;var c=qa.sounds[a];return c||b||qa._wD(ta+'.getSoundById(): Sound "'+a+'" not found.',2),c},this.onready=function(b,c){var d="onready",e=!1;if("function"!=typeof b)throw O("needFunction",d);return Da&&qa._wD(O("queue",d)),c||(c=a),s(d,b,c),t(),e=!0,e},this.ontimeout=function(b,c){var d="ontimeout",e=!1;if("function"!=typeof b)throw O("needFunction",d);return Da&&qa._wD(O("queue",d)),c||(c=a),s(d,b,c),t({type:d}),e=!0,e},this._writeDebug=function(a,c){var d,e,f="soundmanager-debug";return qa.debugMode?ab&&qa.useConsole&&(c&&"object"==typeof c?console.log(a,c):Ha[c]!==b?console[Ha[c]](a):console.log(a),qa.consoleOnly)?!0:(d=h(f))?(e=ya.createElement("div"),++Ga%2===0&&(e.className="sm2-alt"),c=c===b?0:parseInt(c,10),e.appendChild(ya.createTextNode(a)),c&&(c>=2&&(e.style.fontWeight="bold"),3===c&&(e.style.color="#ff3333")),d.insertBefore(e,d.firstChild),d=null,!0):!1:!1},-1!==xa.indexOf("sm2-debug=alert")&&(this._writeDebug=function(b){a.alert(b)}),this._wD=this._writeDebug,this._debug=function(){var a,b;for(n("currentObj",1),a=0,b=qa.soundIDs.length;b>a;a++)qa.sounds[qa.soundIDs[a]]._debug()},this.reboot=function(b,c){qa.soundIDs.length&&qa._wD("Destroying "+qa.soundIDs.length+" SMSound object"+(1!==qa.soundIDs.length?"s":"")+"...");var d,e,f;for(d=qa.soundIDs.length-1;d>=0;d--)qa.sounds[qa.soundIDs[d]].destruct();if(sa)try{Wa&&(Ka=sa.innerHTML),Ja=sa.parentNode.removeChild(sa)}catch(g){n("badRemove",2)}if(Ka=Ja=Qa=sa=null,qa.enabled=F=Da=La=Ma=Ba=Ca=Ea=Sa=qa.swfLoaded=!1,qa.soundIDs=[],qa.sounds={},Ta=0,b)za=[];else for(d in za)if(za.hasOwnProperty(d))for(e=0,f=za[d].length;f>e;e++)za[d][e].fired=!1;return c||qa._wD(ta+": Rebooting..."),qa.html5={usingFlash:null},qa.flash={},qa.html5Only=!1,qa.ignoreFlash=!1,a.setTimeout(function(){C(),c||qa.beginDelayedInit()},20),qa},this.reset=function(){return n("reset"),qa.reboot(!0,!0)},this.getMoviePercent=function(){return sa&&"PercentLoaded"in sa?sa.PercentLoaded():null},this.beginDelayedInit=function(){Fa=!0,D(),setTimeout(function(){return Ma?!1:(H(),B(),Ma=!0,!0)},20),v()},this.destruct=function(){qa._wD(ta+".destruct()"),qa.disable(!0)},g=function(a){var c,d,e,f,g,h,i,j,k,o,q=this,r=!1,s=[],t=0,u=null;k={duration:null,time:null},this.id=a.id,this.sID=this.id,this.url=a.url,this.options=p(a),this.instanceOptions=this.options,this._iO=this.instanceOptions,this.pan=this.options.pan,this.volume=this.options.volume,this.isHTML5=!1,this._a=null,o=this.url?!1:!0,this.id3={},this._debug=function(){qa._wD(q.id+": Merged options:",q.options)},this.load=function(a){var c,d=null;if(a!==b?q._iO=p(a,q.options):(a=q.options,q._iO=a,u&&u!==q.url&&(n("manURL"),q._iO.url=q.url,q.url=null)),q._iO.url||(q._iO.url=q.url),q._iO.url=$(q._iO.url),q.instanceOptions=q._iO,c=q._iO,qa._wD(q.id+": load ("+c.url+")"),!c.url&&!q.url)return qa._wD(q.id+": load(): url is unassigned. Exiting.",2),q;if(q.isHTML5||8!==l||q.url||c.autoPlay||qa._wD(q.id+": Flash 8 load() limitation: Wait for onload() before calling play().",1),c.url===q.url&&0!==q.readyState&&2!==q.readyState)return n("onURL",1),3===q.readyState&&c.onload&&pa(q,function(){c.onload.apply(q,[!!q.duration])}),q;if(q.loaded=!1,q.readyState=1,q.playState=0,q.id3={},ba(c))d=q._setup_html5(c),d._called_load?qa._wD(q.id+": Ignoring request to load again"):(q._html5_canplay=!1,q.url!==c.url&&(qa._wD(n("manURL")+": "+c.url),q._a.src=c.url,q.setPosition(0)),q._a.autobuffer="auto",q._a.preload="auto",q._a._called_load=!0);else{if(qa.html5Only)return qa._wD(q.id+": No flash support. Exiting."),q;if(q._iO.url&&q._iO.url.match(/data\:/i))return qa._wD(q.id+": data: URIs not supported via Flash. Exiting."),q;try{q.isHTML5=!1,q._iO=U(T(c)),q._iO.autoPlay&&(q._iO.position||q._iO.from)&&(qa._wD(q.id+": Disabling autoPlay because of non-zero offset case"),q._iO.autoPlay=!1),c=q._iO,8===l?sa._load(q.id,c.url,c.stream,c.autoPlay,c.usePolicyFile):sa._load(q.id,c.url,!!c.stream,!!c.autoPlay,c.loops||1,!!c.autoLoad,c.usePolicyFile)}catch(e){n("smError",2),m("onload",!1),I({type:"SMSOUND_LOAD_JS_EXCEPTION",fatal:!0})}}return q.url=c.url,q},this.unload=function(){return 0!==q.readyState&&(qa._wD(q.id+": unload()"),q.isHTML5?(f(),q._a&&(q._a.pause(),u=ea(q._a))):8===l?sa._unload(q.id,gb):sa._unload(q.id),c()),q},this.destruct=function(a){qa._wD(q.id+": Destruct"),q.isHTML5?(f(),q._a&&(q._a.pause(),ea(q._a),Sa||e(),q._a._s=null,q._a=null)):(q._iO.onfailure=null,sa._destroySound(q.id)),a||qa.destroySound(q.id,!0)},this.play=function(a,c){var d,e,f,i,k,m,n,s=!0,t=null;if(d=q.id+": play(): ",c=c===b?!0:c,a||(a={}),q.url&&(q._iO.url=q.url),q._iO=p(q._iO,q.options),q._iO=p(a,q._iO),q._iO.url=$(q._iO.url),q.instanceOptions=q._iO,!q.isHTML5&&q._iO.serverURL&&!q.connected)return q.getAutoPlay()||(qa._wD(d+" Netstream not connected yet - setting autoPlay"),q.setAutoPlay(!0)),q;if(ba(q._iO)&&(q._setup_html5(q._iO),g()),1!==q.playState||q.paused||(e=q._iO.multiShot,e?qa._wD(d+"Already playing (multi-shot)",1):(qa._wD(d+"Already playing (one-shot)",1),q.isHTML5&&q.setPosition(q._iO.position),t=q)),null!==t)return t;if(a.url&&a.url!==q.url&&(q.readyState||q.isHTML5||8!==l||!o?q.load(q._iO):o=!1),q.loaded?qa._wD(d.substr(0,d.lastIndexOf(":"))):0===q.readyState?(qa._wD(d+"Attempting to load"),q.isHTML5||qa.html5Only?q.isHTML5?q.load(q._iO):(qa._wD(d+"Unsupported type. Exiting."),t=q):(q._iO.autoPlay=!0,q.load(q._iO)),q.instanceOptions=q._iO):2===q.readyState?(qa._wD(d+"Could not load - exiting",2),t=q):qa._wD(d+"Loading - attempting to play..."),null!==t)return t;if(!q.isHTML5&&9===l&&q.position>0&&q.position===q.duration&&(qa._wD(d+"Sound at end, resetting to position:0"),a.position=0),q.paused&&q.position>=0&&(!q._iO.serverURL||q.position>0))qa._wD(d+"Resuming from paused state",1),q.resume();else{if(q._iO=p(a,q._iO),(!q.isHTML5&&null!==q._iO.position&&q._iO.position>0||null!==q._iO.from&&q._iO.from>0||null!==q._iO.to)&&0===q.instanceCount&&0===q.playState&&!q._iO.serverURL){if(i=function(){q._iO=p(a,q._iO),q.play(q._iO)},q.isHTML5&&!q._html5_canplay?(qa._wD(d+"Beginning load for non-zero offset case"),q.load({_oncanplay:i}),t=!1):q.isHTML5||q.loaded||q.readyState&&2===q.readyState||(qa._wD(d+"Preloading for non-zero offset case"),q.load({onload:i}),t=!1),null!==t)return t;q._iO=j()}(!q.instanceCount||q._iO.multiShotEvents||q.isHTML5&&q._iO.multiShot&&!Sa||!q.isHTML5&&l>8&&!q.getAutoPlay())&&q.instanceCount++,q._iO.onposition&&0===q.playState&&h(q),q.playState=1,q.paused=!1,q.position=q._iO.position===b||isNaN(q._iO.position)?0:q._iO.position,q.isHTML5||(q._iO=U(T(q._iO))),q._iO.onplay&&c&&(q._iO.onplay.apply(q),r=!0),q.setVolume(q._iO.volume,!0),q.setPan(q._iO.pan,!0),q.isHTML5?q.instanceCount<2?(g(),f=q._setup_html5(),q.setPosition(q._iO.position),f.play()):(qa._wD(q.id+": Cloning Audio() for instance #"+q.instanceCount+"..."),k=new Audio(q._iO.url),m=function(){ha.remove(k,"ended",m),q._onfinish(q),ea(k),k=null},n=function(){ha.remove(k,"canplay",n);try{k.currentTime=q._iO.position/fb}catch(a){V(q.id+": multiShot play() failed to apply position of "+q._iO.position/fb)}k.play()},ha.add(k,"ended",m),void 0!==q._iO.volume&&(k.volume=Math.max(0,Math.min(1,q._iO.volume/100))),q.muted&&(k.muted=!0),q._iO.position?ha.add(k,"canplay",n):k.play()):(s=sa._start(q.id,q._iO.loops||1,9===l?q.position:q.position/fb,q._iO.multiShot||!1),9!==l||s||(qa._wD(d+"No sound hardware, or 32-sound ceiling hit",2),q._iO.onplayerror&&q._iO.onplayerror.apply(q)))}return q},this.start=this.play,this.stop=function(a){var b,c=q._iO;return 1===q.playState&&(qa._wD(q.id+": stop()"),q._onbufferchange(0),q._resetOnPosition(0),q.paused=!1,q.isHTML5||(q.playState=0),i(),c.to&&q.clearOnPosition(c.to),q.isHTML5?q._a&&(b=q.position,q.setPosition(0),q.position=b,q._a.pause(),q.playState=0,q._onTimer(),f()):(sa._stop(q.id,a),c.serverURL&&q.unload()),q.instanceCount=0,q._iO={},c.onstop&&c.onstop.apply(q)),q},this.setAutoPlay=function(a){qa._wD(q.id+": Autoplay turned "+(a?"on":"off")),q._iO.autoPlay=a,q.isHTML5||(sa._setAutoPlay(q.id,a),a&&(q.instanceCount||1!==q.readyState||(q.instanceCount++,qa._wD(q.id+": Incremented instance count to "+q.instanceCount))))},this.getAutoPlay=function(){return q._iO.autoPlay},this.setPosition=function(a){a===b&&(a=0);var c,d,e=q.isHTML5?Math.max(a,0):Math.min(q.duration||q._iO.duration,Math.max(a,0));if(q.position=e,d=q.position/fb,q._resetOnPosition(q.position),q._iO.position=e,q.isHTML5){if(q._a){if(q._html5_canplay){if(q._a.currentTime!==d){qa._wD(q.id+": setPosition("+d+")");try{q._a.currentTime=d,(0===q.playState||q.paused)&&q._a.pause()}catch(f){qa._wD(q.id+": setPosition("+d+") failed: "+f.message,2)}}}else if(d)return qa._wD(q.id+": setPosition("+d+"): Cannot seek yet, sound not ready",2),q;q.paused&&q._onTimer(!0)}}else c=9===l?q.position:d,q.readyState&&2!==q.readyState&&sa._setPosition(q.id,c,q.paused||!q.playState,q._iO.multiShot);return q},this.pause=function(a){return q.paused||0===q.playState&&1!==q.readyState?q:(qa._wD(q.id+": pause()"),q.paused=!0,q.isHTML5?(q._setup_html5().pause(),f()):(a||a===b)&&sa._pause(q.id,q._iO.multiShot),q._iO.onpause&&q._iO.onpause.apply(q),q)},this.resume=function(){var a=q._iO;return q.paused?(qa._wD(q.id+": resume()"),q.paused=!1,q.playState=1,q.isHTML5?(q._setup_html5().play(),g()):(a.isMovieStar&&!a.serverURL&&q.setPosition(q.position),sa._pause(q.id,a.multiShot)),!r&&a.onplay?(a.onplay.apply(q),r=!0):a.onresume&&a.onresume.apply(q),q):q},this.togglePause=function(){return qa._wD(q.id+": togglePause()"),0===q.playState?(q.play({position:9!==l||q.isHTML5?q.position/fb:q.position}),q):(q.paused?q.resume():q.pause(),q)},this.setPan=function(a,c){return a===b&&(a=0),c===b&&(c=!1),q.isHTML5||sa._setPan(q.id,a),q._iO.pan=a,c||(q.pan=a,q.options.pan=a),q},this.setVolume=function(a,c){return a===b&&(a=100),c===b&&(c=!1),q.isHTML5?q._a&&(qa.muted&&!q.muted&&(q.muted=!0,q._a.muted=!0),q._a.volume=Math.max(0,Math.min(1,a/100))):sa._setVolume(q.id,qa.muted&&!q.muted||q.muted?0:a),q._iO.volume=a,c||(q.volume=a,q.options.volume=a),q},this.mute=function(){return q.muted=!0,q.isHTML5?q._a&&(q._a.muted=!0):sa._setVolume(q.id,0),q},this.unmute=function(){q.muted=!1;var a=q._iO.volume!==b;return q.isHTML5?q._a&&(q._a.muted=!1):sa._setVolume(q.id,a?q._iO.volume:q.options.volume),q},this.toggleMute=function(){return q.muted?q.unmute():q.mute()},this.onPosition=function(a,c,d){return s.push({position:parseInt(a,10),method:c,scope:d!==b?d:q,fired:!1}),q},this.onposition=this.onPosition,this.clearOnPosition=function(a,b){var c;if(a=parseInt(a,10),isNaN(a))return!1;for(c=0;c<s.length;c++)a===s[c].position&&(b&&b!==s[c].method||(s[c].fired&&t--,s.splice(c,1)))},this._processOnPosition=function(){var a,b,c=s.length;if(!c||!q.playState||t>=c)return!1;for(a=c-1;a>=0;a--)b=s[a],!b.fired&&q.position>=b.position&&(b.fired=!0,t++,b.method.apply(b.scope,[b.position]),c=s.length);return!0},this._resetOnPosition=function(a){var b,c,d=s.length;if(!d)return!1;for(b=d-1;b>=0;b--)c=s[b],c.fired&&a<=c.position&&(c.fired=!1,t--);return!0},j=function(){var a,b,c=q._iO,d=c.from,e=c.to;return b=function(){qa._wD(q.id+': "To" time of '+e+" reached."),q.clearOnPosition(e,b),q.stop()},a=function(){qa._wD(q.id+': Playing "from" '+d),null===e||isNaN(e)||q.onPosition(e,b)},null===d||isNaN(d)||(c.position=d,c.multiShot=!1,a()),c},h=function(){var a,b=q._iO.onposition;if(b)for(a in b)b.hasOwnProperty(a)&&q.onPosition(parseInt(a,10),b[a])},i=function(){var a,b=q._iO.onposition;if(b)for(a in b)b.hasOwnProperty(a)&&q.clearOnPosition(parseInt(a,10))},g=function(){q.isHTML5&&X(q)},f=function(){q.isHTML5&&Y(q)},c=function(a){a||(s=[],t=0),r=!1,q._hasTimer=null,q._a=null,q._html5_canplay=!1,q.bytesLoaded=null,q.bytesTotal=null,q.duration=q._iO&&q._iO.duration?q._iO.duration:null,q.durationEstimate=null,q.buffered=[],q.eqData=[],q.eqData.left=[],q.eqData.right=[],q.failures=0,q.isBuffering=!1,q.instanceOptions={},q.instanceCount=0,q.loaded=!1,q.metadata={},q.readyState=0,q.muted=!1,q.paused=!1,q.peakData={left:0,right:0},q.waveformData={left:[],right:[]},q.playState=0,q.position=null,q.id3={}},c(),this._onTimer=function(a){var b,c,d=!1,e={};return q._hasTimer||a?(q._a&&(a||(q.playState>0||1===q.readyState)&&!q.paused)&&(b=q._get_html5_duration(),b!==k.duration&&(k.duration=b,q.duration=b,d=!0),q.durationEstimate=q.duration,c=q._a.currentTime*fb||0,c!==k.time&&(k.time=c,d=!0),(d||a)&&q._whileplaying(c,e,e,e,e)),d):void 0},this._get_html5_duration=function(){var a=q._iO,b=q._a&&q._a.duration?q._a.duration*fb:a&&a.duration?a.duration:null,c=b&&!isNaN(b)&&b!==1/0?b:null;return c},this._apply_loop=function(a,b){!a.loop&&b>1&&qa._wD("Note: Native HTML5 looping is infinite.",1),a.loop=b>1?"loop":""},this._setup_html5=function(a){var b,e=p(q._iO,a),f=Sa?ra:q._a,g=decodeURI(e.url);if(Sa?g===decodeURI(ia)&&(b=!0):g===decodeURI(u)&&(b=!0),f){if(f._s)if(Sa)f._s&&f._s.playState&&!b&&f._s.stop();else if(!Sa&&g===decodeURI(u))return q._apply_loop(f,e.loops),f;b||(u&&c(!1),f.src=e.url,q.url=e.url,u=e.url,ia=e.url,f._called_load=!1)}else e.autoLoad||e.autoPlay?(q._a=new Audio(e.url),q._a.load()):q._a=Za&&opera.version()<10?new Audio(null):new Audio,f=q._a,f._called_load=!1,Sa&&(ra=f);return q.isHTML5=!0,q._a=f,f._s=q,d(),q._apply_loop(f,e.loops),e.autoLoad||e.autoPlay?q.load():(f.autobuffer=!1,f.preload="auto"),f},d=function(){function a(a,b,c){return q._a?q._a.addEventListener(a,b,c||!1):null}if(q._a._added_events)return!1;var b;q._a._added_events=!0;for(b in ma)ma.hasOwnProperty(b)&&a(b,ma[b]);return!0},e=function(){function a(a,b,c){return q._a?q._a.removeEventListener(a,b,c||!1):null}var b;qa._wD(q.id+": Removing event listeners"),q._a._added_events=!1;for(b in ma)ma.hasOwnProperty(b)&&a(b,ma[b])},this._onload=function(a){var b,c=!!a||!q.isHTML5&&8===l&&q.duration;return b=q.id+": ",qa._wD(b+(c?"onload()":"Failed to load / invalid sound?"+(q.duration?" -":" Zero-length duration reported.")+" ("+q.url+")"),c?1:2),c||q.isHTML5||(qa.sandbox.noRemote===!0&&qa._wD(b+O("noNet"),1),qa.sandbox.noLocal===!0&&qa._wD(b+O("noLocal"),1)),q.loaded=c,q.readyState=c?3:2,q._onbufferchange(0),q._iO.onload&&pa(q,function(){q._iO.onload.apply(q,[c])}),!0},this._onbufferchange=function(a){return 0===q.playState?!1:a&&q.isBuffering||!a&&!q.isBuffering?!1:(q.isBuffering=1===a,q._iO.onbufferchange&&(qa._wD(q.id+": Buffer state change: "+a),q._iO.onbufferchange.apply(q,[a])),!0)},this._onsuspend=function(){return q._iO.onsuspend&&(qa._wD(q.id+": Playback suspended"),q._iO.onsuspend.apply(q)),!0},this._onfailure=function(a,b,c){q.failures++,qa._wD(q.id+": Failure ("+q.failures+"): "+a),q._iO.onfailure&&1===q.failures?q._iO.onfailure(a,b,c):qa._wD(q.id+": Ignoring failure")},this._onwarning=function(a,b,c){q._iO.onwarning&&q._iO.onwarning(a,b,c)},this._onfinish=function(){var a=q._iO.onfinish;q._onbufferchange(0),q._resetOnPosition(0),q.instanceCount&&(q.instanceCount--,q.instanceCount||(i(),q.playState=0,q.paused=!1,q.instanceCount=0,q.instanceOptions={},q._iO={},f(),q.isHTML5&&(q.position=0)),(!q.instanceCount||q._iO.multiShotEvents)&&a&&(qa._wD(q.id+": onfinish()"),pa(q,function(){a.apply(q)})))},this._whileloading=function(a,b,c,d){var e=q._iO;q.bytesLoaded=a,q.bytesTotal=b,q.duration=Math.floor(c),q.bufferLength=d,q.isHTML5||e.isMovieStar?q.durationEstimate=q.duration:e.duration?q.durationEstimate=q.duration>e.duration?q.duration:e.duration:q.durationEstimate=parseInt(q.bytesTotal/q.bytesLoaded*q.duration,10),q.isHTML5||(q.buffered=[{start:0,end:q.duration}]),(3!==q.readyState||q.isHTML5)&&e.whileloading&&e.whileloading.apply(q)},this._whileplaying=function(a,c,d,e,f){var g,h=q._iO;return isNaN(a)||null===a?!1:(q.position=Math.max(0,a),q._processOnPosition(),!q.isHTML5&&l>8&&(h.usePeakData&&c!==b&&c&&(q.peakData={left:c.leftPeak,right:c.rightPeak}),h.useWaveformData&&d!==b&&d&&(q.waveformData={left:d.split(","),right:e.split(",")}),h.useEQData&&f!==b&&f&&f.leftEQ&&(g=f.leftEQ.split(","),q.eqData=g,q.eqData.left=g,f.rightEQ!==b&&f.rightEQ&&(q.eqData.right=f.rightEQ.split(",")))),1===q.playState&&(q.isHTML5||8!==l||q.position||!q.isBuffering||q._onbufferchange(0),h.whileplaying&&h.whileplaying.apply(q)),!0)},this._oncaptiondata=function(a){qa._wD(q.id+": Caption data received."),q.captiondata=a,q._iO.oncaptiondata&&q._iO.oncaptiondata.apply(q,[a])},this._onmetadata=function(a,b){qa._wD(q.id+": Metadata received.");var c,d,e={};for(c=0,d=a.length;d>c;c++)e[a[c]]=b[c];q.metadata=e,console.log("updated metadata",q.metadata),q._iO.onmetadata&&q._iO.onmetadata.call(q,q.metadata)},this._onid3=function(a,b){qa._wD(q.id+": ID3 data received.");var c,d,e=[];for(c=0,d=a.length;d>c;c++)e[a[c]]=b[c];q.id3=p(q.id3,e),q._iO.onid3&&q._iO.onid3.apply(q)},this._onconnect=function(a){a=1===a,qa._wD(q.id+": "+(a?"Connected.":"Failed to connect? - "+q.url),a?1:2),q.connected=a,a&&(q.failures=0,W(q.id)&&(q.getAutoPlay()?q.play(b,q.getAutoPlay()):q._iO.autoLoad&&q.load()),q._iO.onconnect&&q._iO.onconnect.apply(q,[a]))},this._ondataerror=function(a){q.playState>0&&(qa._wD(q.id+": Data error: "+a),q._iO.ondataerror&&q._iO.ondataerror.apply(q))},this._debug()},G=function(){return ya.body||ya.getElementsByTagName("div")[0]},h=function(a){return ya.getElementById(a)},p=function(a,c){var d,e,f=a||{};d=c===b?qa.defaultOptions:c;for(e in d)d.hasOwnProperty(e)&&f[e]===b&&("object"!=typeof d[e]||null===d[e]?f[e]=d[e]:f[e]=p(f[e],d[e]));return f},pa=function(b,c){b.isHTML5||8!==l?c():a.setTimeout(c,0)},r={onready:1,ontimeout:1,defaultOptions:1,flash9Options:1,movieStarOptions:1},q=function(a,c){var d,e=!0,f=c!==b,g=qa.setupOptions,h=r;if(a===b){e=[];for(d in g)g.hasOwnProperty(d)&&e.push(d);for(d in h)h.hasOwnProperty(d)&&("object"==typeof qa[d]?e.push(d+": {...}"):qa[d]instanceof Function?e.push(d+": function() {...}"):e.push(d));return qa._wD(O("setup",e.join(", "))),!1}for(d in a)if(a.hasOwnProperty(d))if("object"!=typeof a[d]||null===a[d]||a[d]instanceof Array||a[d]instanceof RegExp)f&&h[c]!==b?qa[c][d]=a[d]:g[d]!==b?(qa.setupOptions[d]=a[d],qa[d]=a[d]):h[d]===b?(V(O(qa[d]===b?"setupUndef":"setupError",d),2),e=!1):qa[d]instanceof Function?qa[d].apply(qa,a[d]instanceof Array?a[d]:[a[d]]):qa[d]=a[d];else{if(h[d]!==b)return q(a[d],d);V(O(qa[d]===b?"setupUndef":"setupError",d),2),e=!1}return e},ha=function(){function b(a){var b=Ra.call(a),c=b.length;return f?(b[1]="on"+b[1],c>3&&b.pop()):3===c&&b.push(!1),b}function c(a,b){var c=a.shift(),d=[g[b]];f?c[d](a[0],a[1]):c[d].apply(c,a)}function d(){c(b(arguments),"add")}function e(){c(b(arguments),"remove")}var f=a.attachEvent,g={add:f?"attachEvent":"addEventListener",remove:f?"detachEvent":"removeEventListener"};return{add:d,remove:e}}(),ma={abort:f(function(){qa._wD(this._s.id+": abort")}),canplay:f(function(){var a,c=this._s;if(c._html5_canplay)return!0;if(c._html5_canplay=!0,qa._wD(c.id+": canplay"),c._onbufferchange(0),
a=c._iO.position===b||isNaN(c._iO.position)?null:c._iO.position/fb,this.currentTime!==a){qa._wD(c.id+": canplay: Setting position to "+a);try{this.currentTime=a}catch(d){qa._wD(c.id+": canplay: Setting position of "+a+" failed: "+d.message,2)}}c._iO._oncanplay&&c._iO._oncanplay()}),canplaythrough:f(function(){var a=this._s;a.loaded||(a._onbufferchange(0),a._whileloading(a.bytesLoaded,a.bytesTotal,a._get_html5_duration()),a._onload(!0))}),durationchange:f(function(){var a,b=this._s;a=b._get_html5_duration(),isNaN(a)||a===b.duration||(qa._wD(this._s.id+": durationchange ("+a+")"+(b.duration?", previously "+b.duration:"")),b.durationEstimate=b.duration=a)}),ended:f(function(){var a=this._s;qa._wD(a.id+": ended"),a._onfinish()}),error:f(function(){qa._wD(this._s.id+": HTML5 error, code "+this.error.code),this._s._onload(!1)}),loadeddata:f(function(){var a=this._s;qa._wD(a.id+": loadeddata"),a._loaded||Ya||(a.duration=a._get_html5_duration())}),loadedmetadata:f(function(){qa._wD(this._s.id+": loadedmetadata")}),loadstart:f(function(){qa._wD(this._s.id+": loadstart"),this._s._onbufferchange(1)}),play:f(function(){this._s._onbufferchange(0)}),playing:f(function(){qa._wD(this._s.id+": playing "+String.fromCharCode(9835)),this._s._onbufferchange(0)}),progress:f(function(a){var b,c,d,e=this._s,f=0,g="progress"===a.type,h=a.target.buffered,i=a.loaded||0,j=a.total||1;if(e.buffered=[],h&&h.length){for(b=0,c=h.length;c>b;b++)e.buffered.push({start:h.start(b)*fb,end:h.end(b)*fb});if(f=(h.end(0)-h.start(0))*fb,i=Math.min(1,f/(a.target.duration*fb)),g&&h.length>1){for(d=[],c=h.length,b=0;c>b;b++)d.push(a.target.buffered.start(b)*fb+"-"+a.target.buffered.end(b)*fb);qa._wD(this._s.id+": progress, timeRanges: "+d.join(", "))}g&&!isNaN(i)&&qa._wD(this._s.id+": progress, "+Math.floor(100*i)+"% loaded")}isNaN(i)||(e._whileloading(i,j,e._get_html5_duration()),i&&j&&i===j&&ma.canplaythrough.call(this,a))}),ratechange:f(function(){qa._wD(this._s.id+": ratechange")}),suspend:f(function(a){var b=this._s;qa._wD(this._s.id+": suspend"),ma.progress.call(this,a),b._onsuspend()}),stalled:f(function(){qa._wD(this._s.id+": stalled")}),timeupdate:f(function(){this._s._onTimer()}),waiting:f(function(){var a=this._s;qa._wD(this._s.id+": waiting"),a._onbufferchange(1)})},ba=function(a){var b;return b=a&&(a.type||a.url||a.serverURL)?a.serverURL||a.type&&e(a.type)?!1:a.type?ca({type:a.type}):ca({url:a.url})||qa.html5Only||a.url.match(/data\:/i):!1},ea=function(a){var b;return a&&(b=Ya?gb:qa.html5.canPlayType("audio/wav")?hb:gb,a.src=b,void 0!==a._called_unload&&(a._called_load=!1)),Sa&&(ia=null),b},ca=function(a){if(!qa.useHTML5Audio||!qa.hasHTML5)return!1;var c,d,f,g,h=a.url||null,i=a.type||null,j=qa.audioFormats;if(i&&qa.html5[i]!==b)return qa.html5[i]&&!e(i);if(!da){da=[];for(g in j)j.hasOwnProperty(g)&&(da.push(g),j[g].related&&(da=da.concat(j[g].related)));da=new RegExp("\\.("+da.join("|")+")(\\?.*)?$","i")}return f=h?h.toLowerCase().match(da):null,f&&f.length?f=f[1]:i?(d=i.indexOf(";"),f=(-1!==d?i.substr(0,d):i).substr(6)):c=!1,f&&qa.html5[f]!==b?c=qa.html5[f]&&!e(f):(i="audio/"+f,c=qa.html5.canPlayType({type:i}),qa.html5[f]=c,c=c&&qa.html5[i]&&!e(i)),c},ga=function(){function a(a){var b,c,d=!1,e=!1;if(!g||"function"!=typeof g.canPlayType)return d;if(a instanceof Array){for(f=0,c=a.length;c>f;f++)(qa.html5[a[f]]||g.canPlayType(a[f]).match(qa.html5Test))&&(e=!0,qa.html5[a[f]]=!0,qa.flash[a[f]]=!!a[f].match(eb));d=e}else b=g&&"function"==typeof g.canPlayType?g.canPlayType(a):!1,d=!(!b||!b.match(qa.html5Test));return d}if(!qa.useHTML5Audio||!qa.hasHTML5)return qa.html5.usingFlash=!0,Qa=!0,!1;var c,d,e,f,g=Audio!==b?Za&&opera.version()<10?new Audio(null):new Audio:null,h={};e=qa.audioFormats;for(c in e)if(e.hasOwnProperty(c)&&(d="audio/"+c,h[c]=a(e[c].type),h[d]=h[c],c.match(eb)?(qa.flash[c]=!0,qa.flash[d]=!0):(qa.flash[c]=!1,qa.flash[d]=!1),e[c]&&e[c].related))for(f=e[c].related.length-1;f>=0;f--)h["audio/"+e[c].related[f]]=h[c],qa.html5[e[c].related[f]]=h[c],qa.flash[e[c].related[f]]=h[c];return h.canPlayType=g?a:null,qa.html5=p(qa.html5,h),qa.html5.usingFlash=aa(),Qa=qa.html5.usingFlash,!0},A={notReady:"Unavailable - wait until onready() has fired.",notOK:"Audio support is not available.",domError:ta+"exception caught while appending SWF to DOM.",spcWmode:"Removing wmode, preventing known SWF loading issue(s)",swf404:ua+"Verify that %s is a valid path.",tryDebug:"Try "+ta+".debugFlash = true for more security details (output goes to SWF.)",checkSWF:"See SWF output for more debug info.",localFail:ua+"Non-HTTP page ("+ya.location.protocol+" URL?) Review Flash player security settings for this special case:\nhttp://www.macromedia.com/support/documentation/en/flashplayer/help/settings_manager04.html\nMay need to add/allow path, e.g. c:/sm2/ or /users/me/sm2/",waitFocus:ua+"Special case: Waiting for SWF to load with window focus...",waitForever:ua+"Waiting indefinitely for Flash (will recover if unblocked)...",waitSWF:ua+"Waiting for 100% SWF load...",needFunction:ua+"Function object expected for %s",badID:'Sound ID "%s" should be a string, starting with a non-numeric character',currentObj:ua+"_debug(): Current sound objects",waitOnload:ua+"Waiting for window.onload()",docLoaded:ua+"Document already loaded",onload:ua+"initComplete(): calling soundManager.onload()",onloadOK:ta+".onload() complete",didInit:ua+"init(): Already called?",secNote:"Flash security note: Network/internet URLs will not load due to security restrictions. Access can be configured via Flash Player Global Security Settings Page: http://www.macromedia.com/support/documentation/en/flashplayer/help/settings_manager04.html",badRemove:ua+"Failed to remove Flash node.",shutdown:ta+".disable(): Shutting down",queue:ua+"Queueing %s handler",smError:"SMSound.load(): Exception: JS-Flash communication failed, or JS error.",fbTimeout:"No flash response, applying ."+R.swfTimedout+" CSS...",fbLoaded:"Flash loaded",fbHandler:ua+"flashBlockHandler()",manURL:"SMSound.load(): Using manually-assigned URL",onURL:ta+".load(): current URL already assigned.",badFV:ta+'.flashVersion must be 8 or 9. "%s" is invalid. Reverting to %s.',as2loop:"Note: Setting stream:false so looping can work (flash 8 limitation)",noNSLoop:"Note: Looping not implemented for MovieStar formats",needfl9:"Note: Switching to flash 9, required for MP4 formats.",mfTimeout:"Setting flashLoadTimeout = 0 (infinite) for off-screen, mobile flash case",needFlash:ua+"Fatal error: Flash is needed to play some required formats, but is not available.",gotFocus:ua+"Got window focus.",policy:"Enabling usePolicyFile for data access",setup:ta+".setup(): allowed parameters: %s",setupError:ta+'.setup(): "%s" cannot be assigned with this method.',setupUndef:ta+'.setup(): Could not find option "%s"',setupLate:ta+".setup(): url, flashVersion and html5Test property changes will not take effect until reboot().",noURL:ua+"Flash URL required. Call soundManager.setup({url:...}) to get started.",sm2Loaded:"SoundManager 2: Ready. "+String.fromCharCode(10003),reset:ta+".reset(): Removing event callbacks",mobileUA:"Mobile UA detected, preferring HTML5 by default.",globalHTML5:"Using singleton HTML5 Audio() pattern for this device."},O=function(){var a,b,c,d,e;if(a=Ra.call(arguments),d=a.shift(),e=A&&A[d]?A[d]:"",e&&a&&a.length)for(b=0,c=a.length;c>b;b++)e=e.replace("%s",a[b]);return e},T=function(a){return 8===l&&a.loops>1&&a.stream&&(n("as2loop"),a.stream=!1),a},U=function(a,b){return a&&!a.usePolicyFile&&(a.onid3||a.usePeakData||a.useWaveformData||a.useEQData)&&(qa._wD((b||"")+O("policy")),a.usePolicyFile=!0),a},V=function(a){ab&&console.warn!==b?console.warn(a):qa._wD(a)},i=function(){return!1},L=function(a){var b;for(b in a)a.hasOwnProperty(b)&&"function"==typeof a[b]&&(a[b]=i);b=null},M=function(a){a===b&&(a=!1),(Ea||a)&&qa.disable(a)},N=function(a){var b,c=null;if(a)if(a.match(/\.swf(\?.*)?$/i)){if(c=a.substr(a.toLowerCase().lastIndexOf(".swf?")+4))return a}else a.lastIndexOf("/")!==a.length-1&&(a+="/");return b=(a&&-1!==a.lastIndexOf("/")?a.substr(0,a.lastIndexOf("/")+1):"./")+qa.movieURL,qa.noSWFCache&&(b+="?ts="+(new Date).getTime()),b},y=function(){l=parseInt(qa.flashVersion,10),8!==l&&9!==l&&(qa._wD(O("badFV",l,Ia)),qa.flashVersion=l=Ia);var a=qa.debugMode||qa.debugFlash?"_debug.swf":".swf";qa.useHTML5Audio&&!qa.html5Only&&qa.audioFormats.mp4.required&&9>l&&(qa._wD(O("needfl9")),qa.flashVersion=l=9),qa.version=qa.versionNumber+(qa.html5Only?" (HTML5-only mode)":9===l?" (AS3/Flash 9)":" (AS2/Flash 8)"),l>8?(qa.defaultOptions=p(qa.defaultOptions,qa.flash9Options),qa.features.buffering=!0,qa.defaultOptions=p(qa.defaultOptions,qa.movieStarOptions),qa.filePatterns.flash9=new RegExp("\\.(mp3|"+lb.join("|")+")(\\?.*)?$","i"),qa.features.movieStar=!0):qa.features.movieStar=!1,qa.filePattern=qa.filePatterns[8!==l?"flash9":"flash8"],qa.movieURL=(8===l?"soundmanager2.swf":"soundmanager2_flash9.swf").replace(".swf",a),qa.features.peakData=qa.features.waveformData=qa.features.eqData=l>8},J=function(a,b){return sa?void sa._setPolling(a,b):!1},K=function(){if(qa.debugURLParam.test(xa)&&(qa.debugMode=!0),h(qa.debugID))return!1;var a,b,c,d,e;if(qa.debugMode&&!h(qa.debugID)&&(!ab||!qa.useConsole||!qa.consoleOnly)){a=ya.createElement("div"),a.id=qa.debugID+"-toggle",d={position:"fixed",bottom:"0px",right:"0px",width:"1.2em",height:"1.2em",lineHeight:"1.2em",margin:"2px",textAlign:"center",border:"1px solid #999",cursor:"pointer",background:"#fff",color:"#333",zIndex:10001},a.appendChild(ya.createTextNode("-")),a.onclick=S,a.title="Toggle SM2 debug console",wa.match(/msie 6/i)&&(a.style.position="absolute",a.style.cursor="hand");for(e in d)d.hasOwnProperty(e)&&(a.style[e]=d[e]);if(b=ya.createElement("div"),b.id=qa.debugID,b.style.display=qa.debugMode?"block":"none",qa.debugMode&&!h(a.id)){try{c=G(),c.appendChild(a)}catch(f){throw new Error(O("domError")+" \n"+f.toString())}c.appendChild(b)}}c=null},W=this.getSoundById,n=function(a,b){return a?qa._wD(O(a),b):""},S=function(){var a=h(qa.debugID),b=h(qa.debugID+"-toggle");return a?(Aa?(b.innerHTML="+",a.style.display="none"):(b.innerHTML="-",a.style.display="block"),void(Aa=!Aa)):!1},m=function(c,d,e){if(a.sm2Debugger!==b)try{sm2Debugger.handleEvent(c,d,e)}catch(f){return!1}return!0},Q=function(){var a=[];return qa.debugMode&&a.push(R.sm2Debug),qa.debugFlash&&a.push(R.flashDebug),qa.useHighPerformance&&a.push(R.highPerf),a.join(" ")},P=function(){var a=O("fbHandler"),b=qa.getMoviePercent(),c=R,d={type:"FLASHBLOCK"};return qa.html5Only?!1:void(qa.ok()?(qa.didFlashBlock&&qa._wD(a+": Unblocked"),qa.oMC&&(qa.oMC.className=[Q(),c.swfDefault,c.swfLoaded+(qa.didFlashBlock?" "+c.swfUnblocked:"")].join(" "))):(Qa&&(qa.oMC.className=Q()+" "+c.swfDefault+" "+(null===b?c.swfTimedout:c.swfError),qa._wD(a+": "+O("fbTimeout")+(b?" ("+O("fbLoaded")+")":""))),qa.didFlashBlock=!0,t({type:"ontimeout",ignoreInit:!0,error:d}),I(d)))},s=function(a,c,d){za[a]===b&&(za[a]=[]),za[a].push({method:c,scope:d||null,fired:!1})},t=function(a){if(a||(a={type:qa.ok()?"onready":"ontimeout"}),!Da&&a&&!a.ignoreInit)return!1;if("ontimeout"===a.type&&(qa.ok()||Ea&&!a.ignoreInit))return!1;var b,c,d={success:a&&a.ignoreInit?qa.ok():!Ea},e=a&&a.type?za[a.type]||[]:[],f=[],g=[d],h=Qa&&!qa.ok();for(a.error&&(g[0].error=a.error),b=0,c=e.length;c>b;b++)e[b].fired!==!0&&f.push(e[b]);if(f.length)for(b=0,c=f.length;c>b;b++)f[b].scope?f[b].method.apply(f[b].scope,g):f[b].method.apply(this,g),h||(f[b].fired=!0);return!0},u=function(){a.setTimeout(function(){qa.useFlashBlock&&P(),t(),"function"==typeof qa.onload&&(n("onload",1),qa.onload.apply(a),n("onloadOK",1)),qa.waitForWindowLoad&&ha.add(a,"load",u)},1)},ka=function(){if(ja!==b)return ja;var c,d,e,f=!1,g=navigator,h=g.plugins,i=a.ActiveXObject;if(h&&h.length)d="application/x-shockwave-flash",e=g.mimeTypes,e&&e[d]&&e[d].enabledPlugin&&e[d].enabledPlugin.description&&(f=!0);else if(i!==b&&!wa.match(/MSAppHost/i)){try{c=new i("ShockwaveFlash.ShockwaveFlash")}catch(j){c=null}f=!!c,c=null}return ja=f,f},aa=function(){var a,b,c=qa.audioFormats,d=Ua&&!!wa.match(/os (1|2|3_0|3_1)\s/i);if(d?(qa.hasHTML5=!1,qa.html5Only=!0,qa.oMC&&(qa.oMC.style.display="none")):qa.useHTML5Audio&&(qa.html5&&qa.html5.canPlayType||(qa._wD("SoundManager: No HTML5 Audio() support detected."),qa.hasHTML5=!1),_a&&qa._wD(ua+"Note: Buggy HTML5 Audio in Safari on this OS X release, see https://bugs.webkit.org/show_bug.cgi?id=32159 - "+(ja?"will use flash fallback for MP3/MP4, if available":" would use flash fallback for MP3/MP4, but none detected."),1)),qa.useHTML5Audio&&qa.hasHTML5){_=!0;for(b in c)c.hasOwnProperty(b)&&c[b].required&&(qa.html5.canPlayType(c[b].type)?qa.preferFlash&&(qa.flash[b]||qa.flash[c[b].type])&&(a=!0):(_=!1,a=!0))}return qa.ignoreFlash&&(a=!1,_=!0),qa.html5Only=qa.hasHTML5&&qa.useHTML5Audio&&!a,!qa.html5Only},$=function(a){var b,c,d,e=0;if(a instanceof Array){for(b=0,c=a.length;c>b;b++)if(a[b]instanceof Object){if(qa.canPlayMIME(a[b].type)){e=b;break}}else if(qa.canPlayURL(a[b])){e=b;break}a[e].url&&(a[e]=a[e].url),d=a[e]}else d=a;return d},X=function(a){a._hasTimer||(a._hasTimer=!0,!$a&&qa.html5PollingInterval&&(null===Oa&&0===Na&&(Oa=setInterval(Z,qa.html5PollingInterval)),Na++))},Y=function(a){a._hasTimer&&(a._hasTimer=!1,!$a&&qa.html5PollingInterval&&Na--)},Z=function(){var a;if(null!==Oa&&!Na)return clearInterval(Oa),Oa=null,!1;for(a=qa.soundIDs.length-1;a>=0;a--)qa.sounds[qa.soundIDs[a]].isHTML5&&qa.sounds[qa.soundIDs[a]]._hasTimer&&qa.sounds[qa.soundIDs[a]]._onTimer()},I=function(c){c=c!==b?c:{},"function"==typeof qa.onerror&&qa.onerror.apply(a,[{type:c.type!==b?c.type:null}]),c.fatal!==b&&c.fatal&&qa.disable()},la=function(){if(!_a||!ka())return!1;var a,b,c=qa.audioFormats;for(b in c)if(c.hasOwnProperty(b)&&("mp3"===b||"mp4"===b)&&(qa._wD(ta+": Using flash fallback for "+b+" format"),qa.html5[b]=!1,c[b]&&c[b].related))for(a=c[b].related.length-1;a>=0;a--)qa.html5[c[b].related[a]]=!1},this._setSandboxType=function(a){var c=qa.sandbox;c.type=a,c.description=c.types[c.types[a]!==b?a:"unknown"],"localWithFile"===c.type?(c.noRemote=!0,c.noLocal=!1,n("secNote",2)):"localWithNetwork"===c.type?(c.noRemote=!1,c.noLocal=!0):"localTrusted"===c.type&&(c.noRemote=!1,c.noLocal=!1)},this._externalInterfaceOK=function(a){if(qa.swfLoaded)return!1;var b;return m("swf",!0),m("flashtojs",!0),qa.swfLoaded=!0,cb=!1,_a&&la(),a&&a.replace(/\+dev/i,"")===qa.versionNumber.replace(/\+dev/i,"")?void setTimeout(k,Wa?100:1):(b=ta+': Fatal: JavaScript file build "'+qa.versionNumber+'" does not match Flash SWF build "'+a+'" at '+qa.url+". Ensure both are up-to-date.",setTimeout(function(){throw new Error(b)},0),!1)},H=function(a,c){function d(){var a,b=[],c=[],d=" + ";a="SoundManager "+qa.version+(!qa.html5Only&&qa.useHTML5Audio?qa.hasHTML5?" + HTML5 audio":", no HTML5 audio support":""),qa.html5Only?qa.html5PollingInterval&&b.push("html5PollingInterval ("+qa.html5PollingInterval+"ms)"):(qa.preferFlash&&b.push("preferFlash"),qa.useHighPerformance&&b.push("useHighPerformance"),qa.flashPollingInterval&&b.push("flashPollingInterval ("+qa.flashPollingInterval+"ms)"),qa.html5PollingInterval&&b.push("html5PollingInterval ("+qa.html5PollingInterval+"ms)"),qa.wmode&&b.push("wmode ("+qa.wmode+")"),qa.debugFlash&&b.push("debugFlash"),qa.useFlashBlock&&b.push("flashBlock")),b.length&&(c=c.concat([b.join(d)])),qa._wD(a+(c.length?d+c.join(", "):""),1),na()}function e(a,b){return'<param name="'+a+'" value="'+b+'" />'}if(Ba&&Ca)return!1;if(qa.html5Only)return y(),d(),qa.oMC=h(qa.movieID),k(),Ba=!0,Ca=!0,!1;var f,g,i,j,l,m,n,o,p=c||qa.url,q=qa.altURL||p,r="JS/Flash audio component (SoundManager 2)",s=G(),t=Q(),u=null,v=ya.getElementsByTagName("html")[0];if(u=v&&v.dir&&v.dir.match(/rtl/i),a=a===b?qa.id:a,y(),qa.url=N(ib?p:q),c=qa.url,qa.wmode=!qa.wmode&&qa.useHighPerformance?"transparent":qa.wmode,null!==qa.wmode&&(wa.match(/msie 8/i)||!Wa&&!qa.useHighPerformance)&&navigator.platform.match(/win32|win64/i)&&(Pa.push(A.spcWmode),qa.wmode=null),f={name:a,id:a,src:c,quality:"high",allowScriptAccess:qa.allowScriptAccess,bgcolor:qa.bgColor,pluginspage:jb+"www.macromedia.com/go/getflashplayer",title:r,type:"application/x-shockwave-flash",wmode:qa.wmode,hasPriority:"true"},qa.debugFlash&&(f.FlashVars="debug=1"),qa.wmode||delete f.wmode,Wa)g=ya.createElement("div"),j=['<object id="'+a+'" data="'+c+'" type="'+f.type+'" title="'+f.title+'" classid="clsid:D27CDB6E-AE6D-11cf-96B8-444553540000" codebase="'+jb+'download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=6,0,40,0">',e("movie",c),e("AllowScriptAccess",qa.allowScriptAccess),e("quality",f.quality),qa.wmode?e("wmode",qa.wmode):"",e("bgcolor",qa.bgColor),e("hasPriority","true"),qa.debugFlash?e("FlashVars",f.FlashVars):"","</object>"].join("");else{g=ya.createElement("embed");for(i in f)f.hasOwnProperty(i)&&g.setAttribute(i,f[i])}if(K(),t=Q(),s=G())if(qa.oMC=h(qa.movieID)||ya.createElement("div"),qa.oMC.id)o=qa.oMC.className,qa.oMC.className=(o?o+" ":R.swfDefault)+(t?" "+t:""),qa.oMC.appendChild(g),Wa&&(l=qa.oMC.appendChild(ya.createElement("div")),l.className=R.swfBox,l.innerHTML=j),Ca=!0;else{if(qa.oMC.id=qa.movieID,qa.oMC.className=R.swfDefault+" "+t,m=null,l=null,qa.useFlashBlock||(qa.useHighPerformance?m={position:"fixed",width:"8px",height:"8px",bottom:"0px",left:"0px",overflow:"hidden"}:(m={position:"absolute",width:"6px",height:"6px",top:"-9999px",left:"-9999px"},u&&(m.left=Math.abs(parseInt(m.left,10))+"px"))),Xa&&(qa.oMC.style.zIndex=1e4),!qa.debugFlash)for(n in m)m.hasOwnProperty(n)&&(qa.oMC.style[n]=m[n]);try{Wa||qa.oMC.appendChild(g),s.appendChild(qa.oMC),Wa&&(l=qa.oMC.appendChild(ya.createElement("div")),l.className=R.swfBox,l.innerHTML=j),Ca=!0}catch(w){throw new Error(O("domError")+" \n"+w.toString())}}return Ba=!0,d(),!0},B=function(){return qa.html5Only?(H(),!1):sa?!1:qa.url?(sa=qa.getMovie(qa.id),sa||(Ja?(Wa?qa.oMC.innerHTML=Ka:qa.oMC.appendChild(Ja),Ja=null,Ba=!0):H(qa.id,qa.url),sa=qa.getMovie(qa.id)),"function"==typeof qa.oninitmovie&&setTimeout(qa.oninitmovie,1),oa(),!0):(n("noURL"),!1)},v=function(){setTimeout(w,1e3)},x=function(){a.setTimeout(function(){V(ua+"useFlashBlock is false, 100% HTML5 mode is possible. Rebooting with preferFlash: false..."),qa.setup({preferFlash:!1}).reboot(),qa.didFlashBlock=!0,qa.beginDelayedInit()},1)},w=function(){var b,c=!1;return qa.url?La?!1:(La=!0,ha.remove(a,"load",v),ja&&cb&&!bb?(n("waitFocus"),!1):(Da||(b=qa.getMoviePercent(),b>0&&100>b&&(c=!0)),void setTimeout(function(){return b=qa.getMoviePercent(),c?(La=!1,qa._wD(O("waitSWF")),a.setTimeout(v,1),!1):(Da||(qa._wD(ta+": No Flash response within expected time. Likely causes: "+(0===b?"SWF load failed, ":"")+"Flash blocked or JS-Flash security error."+(qa.debugFlash?" "+O("checkSWF"):""),2),!ib&&b&&(n("localFail",2),qa.debugFlash||n("tryDebug",2)),0===b&&qa._wD(O("swf404",qa.url),1),m("flashtojs",!1," (Check flash security or flash blockers)")),void(!Da&&db&&(null===b?qa.useFlashBlock||0===qa.flashLoadTimeout?(qa.useFlashBlock&&P(),n("waitForever")):!qa.useFlashBlock&&_?x():(n("waitForever"),t({type:"ontimeout",ignoreInit:!0,error:{type:"INIT_FLASHBLOCK"}})):0===qa.flashLoadTimeout?n("waitForever"):!qa.useFlashBlock&&_?x():M(!0))))},qa.flashLoadTimeout))):!1},z=function(){function b(){ha.remove(a,"focus",z)}return bb||!cb?(b(),!0):(db=!0,bb=!0,n("gotFocus"),La=!1,v(),b(),!0)},oa=function(){Pa.length&&(qa._wD("SoundManager 2: "+Pa.join(" "),1),Pa=[])},na=function(){oa();var a,b=[];if(qa.useHTML5Audio&&qa.hasHTML5){for(a in qa.audioFormats)qa.audioFormats.hasOwnProperty(a)&&b.push(a+" = "+qa.html5[a]+(!qa.html5[a]&&Qa&&qa.flash[a]?" (using flash)":qa.preferFlash&&qa.flash[a]&&Qa?" (preferring flash)":qa.html5[a]?"":" ("+(qa.audioFormats[a].required?"required, ":"")+"and no flash support)"));qa._wD("SoundManager 2 HTML5 support: "+b.join(", "),1)}},o=function(b){if(Da)return!1;if(qa.html5Only)return n("sm2Loaded",1),Da=!0,u(),m("onload",!0),!0;var c,d=qa.useFlashBlock&&qa.flashLoadTimeout&&!qa.getMoviePercent(),e=!0;return d||(Da=!0),c={type:!ja&&Qa?"NO_FLASH":"INIT_TIMEOUT"},qa._wD("SoundManager 2 "+(Ea?"failed to load":"loaded")+" ("+(Ea?"Flash security/load error":"OK")+") "+String.fromCharCode(Ea?10006:10003),Ea?2:1),Ea||b?(qa.useFlashBlock&&qa.oMC&&(qa.oMC.className=Q()+" "+(null===qa.getMoviePercent()?R.swfTimedout:R.swfError)),t({type:"ontimeout",error:c,ignoreInit:!0}),m("onload",!1),I(c),e=!1):m("onload",!0),Ea||(qa.waitForWindowLoad&&!Fa?(n("waitOnload"),ha.add(a,"load",u)):(qa.waitForWindowLoad&&Fa&&n("docLoaded"),u())),e},j=function(){var a,c=qa.setupOptions;for(a in c)c.hasOwnProperty(a)&&(qa[a]===b?qa[a]=c[a]:qa[a]!==c[a]&&(qa.setupOptions[a]=qa[a]))},k=function(){function b(){ha.remove(a,"load",qa.beginDelayedInit)}if(Da)return n("didInit"),!1;if(qa.html5Only)return Da||(b(),qa.enabled=!0,o()),!0;B();try{sa._externalInterfaceTest(!1),J(!0,qa.flashPollingInterval||(qa.useHighPerformance?10:50)),qa.debugMode||sa._disableDebug(),qa.enabled=!0,m("jstoflash",!0),qa.html5Only||ha.add(a,"unload",i)}catch(c){return qa._wD("js/flash exception: "+c.toString()),m("jstoflash",!1),I({type:"JS_TO_FLASH_EXCEPTION",fatal:!0}),M(!0),o(),!1}return o(),b(),!0},D=function(){return F?!1:(F=!0,j(),K(),function(){var a="sm2-usehtml5audio=",b="sm2-preferflash=",c=null,d=null,e=xa.toLowerCase();-1!==e.indexOf(a)&&(c="1"===e.charAt(e.indexOf(a)+a.length),ab&&console.log((c?"Enabling ":"Disabling ")+"useHTML5Audio via URL parameter"),qa.setup({useHTML5Audio:c})),-1!==e.indexOf(b)&&(d="1"===e.charAt(e.indexOf(b)+b.length),ab&&console.log((d?"Enabling ":"Disabling ")+"preferFlash via URL parameter"),qa.setup({preferFlash:d}))}(),!ja&&qa.hasHTML5&&(qa._wD("SoundManager 2: No Flash detected"+(qa.useHTML5Audio?". Trying HTML5-only mode.":", enabling HTML5."),1),qa.setup({useHTML5Audio:!0,preferFlash:!1})),ga(),!ja&&Qa&&(Pa.push(A.needFlash),qa.setup({flashLoadTimeout:1})),ya.removeEventListener&&ya.removeEventListener("DOMContentLoaded",D,!1),B(),!0)},fa=function(){return"complete"===ya.readyState&&(D(),ya.detachEvent("onreadystatechange",fa)),!0},E=function(){Fa=!0,D(),ha.remove(a,"load",E)},C=function(){$a&&((!qa.setupOptions.useHTML5Audio||qa.setupOptions.preferFlash)&&Pa.push(A.mobileUA),qa.setupOptions.useHTML5Audio=!0,qa.setupOptions.preferFlash=!1,(Ua||Va&&!wa.match(/android\s2\.3/i))&&(Pa.push(A.globalHTML5),Ua&&(qa.ignoreFlash=!0),Sa=!0))},C(),ka(),ha.add(a,"focus",z),ha.add(a,"load",v),ha.add(a,"load",E),ya.addEventListener?ya.addEventListener("DOMContentLoaded",D,!1):ya.attachEvent?ya.attachEvent("onreadystatechange",fa):(m("onload",!1),I({type:"NO_DOM2_EVENTS",fatal:!0}))}if(!a||!a.document)throw new Error("SoundManager requires a browser with window and document objects.");var d=null;void 0!==a.SM2_DEFER&&SM2_DEFER||(d=new c),"object"==typeof module&&module&&"object"==typeof module.exports?(a.soundManager=d,module.exports.SoundManager=c,module.exports.soundManager=d):"function"==typeof define&&define.amd?define("SoundManager",[],function(){return{SoundManager:c,soundManager:d}}):(a.SoundManager=c,a.soundManager=d)}(window),window.JST["apps/album/landing/tpl/landing.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b("<h3>")),d(t.gettext("Recently added")),d(b('</h3>\n<div class="landing-section region-recently-added"></div>\n<h3>')),d(t.gettext("Recently played")),d(b('</h3>\n<div class="landing-section region-recently-played"></div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/album/show/tpl/album_with_songs.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="album album--with-songs">\n    <div class="region-album-side">\n        <div class="region-album-meta"></div>\n    </div>\n    <div class="region-album-content">\n        <div class="region-album-songs"></div>\n    </div>\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/album/show/tpl/details_meta.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="region-details-title">\n    <h2><span class="title">')),d(this.label),d(b('</span> <span class="sub">')),d(this.year),d(b('</span></h2>\n</div>\n\n\n\n<div class="region-details-meta-below">\n\n    <div class="artist"><a href="#music/artist/')),d(this.artistid),d(b('">')),d(this.artist),d(b("</a></div>\n\n    ")),this.genre.length>0&&(d(b('\n    <div class="album-genres">\n        ')),d(b(helpers.url.filterLinks("music/albums","genre",this.genre))),d(b("\n    </div>\n    "))),d(b('\n\n    <div class="description">')),d(this.description),d(b("</div>\n\n</div>\n"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/artist/show/tpl/details_meta.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="region-details-title">\n    <h2>')),d(this.label),d(b(' <span class="sub">')),d(this.formed),d(b('</span></h2>\n</div>\n\n\n<div class="region-details-meta-below">\n\n    <div class="region-details-subtext">\n        ')),this.genre.length>0&&(d(b('\n        <div class="artist-genres">\n            ')),d(b(helpers.url.filterLinks("music/artists","genre",this.genre))),d(b("\n        </div>\n        "))),d(b('\n    </div>\n\n    <div class="description">')),d(this.description),d(b("</div>\n\n</div>\n"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/browser/list/tpl/back_button.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<i class="mdi thumb"></i><div class="title">')),d(t.gettext("Back")),d(b("</div>"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/browser/list/tpl/file.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="thumb" style="background-image: url(\'')),d(this.thumbnail),d(b('\')"><div class="mdi play"></div></div>\n<div class="title">')),d(b(this.label)),d(b("</div>"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/browser/list/tpl/folder_layout.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="folder-layout">\n    <div class="loading-bar"><div class="inner"><span>')),d(t.gettext("Loading folder...")),d(b('</span></div></div>\n    <div class="path"></div>\n    <div class="folder-container">\n        <div class="files">\n        </div>\n        <div class="folders-pane">\n            <div class="back"></div>\n            <div class="folders">\n                <div class="intro">\n                    <h3><span class="mdi-navigation-arrow-back text-dim"></span> ')),d(t.gettext("Browse files and add-ons")),d(b("</h3>\n                    <p>")),d(t.gettext("This is where you can browse all Kodi content, not just what is in the library. Browse by source or add-on.")),d(b("</p>\n                </div>\n            </div>\n        </div>\n    </div>\n</div>"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/browser/list/tpl/path.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="title">')),d(this.label),d(b("</div>"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/browser/list/tpl/source.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="source source-')),d(this.media),d(b('">\n    ')),d(this.label),d(b("\n</div>"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/browser/list/tpl/source_set.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b("<h3>")),d(this.label),d(b('</h3>\n<ul class="sources"></ul>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/cast/list/tpl/cast.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<a href="#')),d(this.origin),d(b("?cast=")),d(this.name),d(b('" title="')),d(this.name),d(b(" (")),d(this.role),d(b(')">\n    <div class="thumb">\n        <img src="')),d(this.thumbnail),d(b('" />\n    </div>\n    <div class="meta">\n        <strong>')),d(this.name),d(b('</strong>\n        <span title="')),d(this.role),d(b('">')),d(this.role),d(b('</span>\n    </div>\n</a>\n<ul class="actions">\n    <li class="imdb"></li>\n    <li class="google"></li>\n</ul>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/epg/list/tpl/programmes.jst"]=function(a){
var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b("\n<div ")),this.isactive&&d(b(' class="airing" ')),d(b(" ><strong>")),d(this.label),d(b("</strong>")),this.hastimer&&d(b('<span class="hastimer"></span>')),d(b("</div>\n<div><strong>")),d(t.gettext("Start")),d(b(":</strong> ")),d(helpers.global.epgDateTimeToJS(this.starttime).toLocaleString()),d(b("</div>\n<div><strong>")),d(t.gettext("End")),d(b(":</strong> ")),d(helpers.global.epgDateTimeToJS(this.endtime).toLocaleString()),d(b("</div>\n<div><strong> ")),d(t.gettext("Runtime")),d(b(":</strong> ")),d(this.runtime),d(b(" </div>\n<div>")),d(this.plot),d(b('</div>\n<div class="programme-progress"><div class="current-progress" style="width: ')),d(this.progresspercentage),d(b('%" title="')),d(Math.round(this.progresspercentage)),d(b("% ")),d(t.gettext("complete")),d(b('"></div></div>\n'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/external/youtube/tpl/youtube.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<img src="')),d(this.thumbnail),d(b('" class="thumb" />\n<h3>')),d(this.title),d(b('</h3>\n<span class="play-kodi flat-btn action">Play in Kodi</span>\n<span class="play-local flat-btn action">Play in browser</span>\n'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/filter/show/tpl/filter_options.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="options-search-wrapper">\n    <input class="options-search" value="" />\n</div>\n<div class="deselect-all">')),d(t.gettext("Deselect all")),d(b('</div>\n<ul class="selection-list"></ul>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/filter/show/tpl/filters_bar.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<span class="filters-active-all">')),d(this.filters),d(b('</span><i class="remove"></i>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/filter/show/tpl/filters_ui.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="filters-container">\n\n    <div class="filters-current filter-pane">\n        <div class="nav-section"></div>\n\n        <h3 class="open-filters">')),d(t.gettext("Filters")),d(b('<i></i></h3>\n        <div class="filters-active"></div>\n\n        <h3>')),d(t.gettext("Sort")),d(b('</h3>\n        <div class="list sort-options"></div>\n    </div>\n\n    <div class="filters-page filter-pane">\n        <h3 class="close-filters">')),d(t.gettext("Select a filter")),d(b('</h3>\n        <div class="list filters-list"></div>\n    </div>\n\n    <div class="filters-options filter-pane">\n        <h3 class="close-options">')),d(t.gettext("Select an option")),d(b('</h3>\n        <div class="list filter-options-list"></div>\n    </div>\n\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/filter/show/tpl/list_item.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b(this.title))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/help/overview/tpl/overview.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b("<h1>")),d(tr("About Chorus")),d(b("</h1>\n<h2>")),d(tr("Status report")),d(b('</h2>\n<div class="help--overview--report">\n    <ul>\n	<li class="report-chorus-version"><strong>Chorus ')),d(tr("version")),d(b('</strong><span></span></li>\n	<li class="report-kodi-version"><strong>Kodi ')),d(tr("version")),d(b('</strong><span></span></li>\n	<li class="report-websockets"><strong>')),d(tr("Remote control")),d(b('</strong><span></span></li>\n	<li class="report-local-audio"><strong>')),d(tr("Local audio")),d(b('</strong><span></span></li>\n    </ul>\n</div>\n<div class="help--overview--header"></div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/input/remote/tpl/remote_control.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div id="remote-background" class="close-remote"></div>\n<div class="remote kodi-remote">\n    <div class="toggle-visibility"></div>\n    <div class="playing-area">\n\n    </div>\n    <div class="main-controls">\n        <div class="direction">\n            <div class="pad">\n                <div class="ibut mdi-hardware-keyboard-arrow-left left input-button" data-type="Left"></div>\n                <div class="ibut mdi-hardware-keyboard-arrow-up up input-button" data-type="Up"></div>\n                <div class="ibut mdi-hardware-keyboard-arrow-down down input-button" data-type="Down"></div>\n                <div class="ibut mdi-hardware-keyboard-arrow-right right input-button" data-type="Right"></div>\n                <div class="ibut mdi-image-brightness-1 ok input-button" data-type="Select"></div>\n            </div>\n        </div>\n        <div class="buttons">\n            <div class="ibut mdi-action-settings-power power-button"></div>\n            <div class="ibut mdi-navigation-more-vert input-button" data-type="ContextMenu"></div>\n            <div class="ibut mdi-action-info info-button" data-type="Info"></div>\n        </div>\n    </div>\n    <div class="secondary-controls">\n        <div class="ibut mdi-hardware-keyboard-return input-button" data-type="Back"></div>\n        <div class="ibut mdi-av-stop player-button" data-type="Stop"></div>\n        <div class="ibut mdi-maps-store-mall-directory input-button" data-type="Home"></div>\n    </div>\n\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/lab/apiBrowser/tpl/api_browser_landing.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="api-browser--landing page">\n    <h2>')),d(t.gettext("Kodi API browser")),d(b('</h2>\n    <h4><a href="#lab">')),d(t.gettext("Chorus lab")),d(b('</a></h4>\n    <div class="api-browser--content">\n        <p>')),d(t.gettext("This is a tool to test out the api. Select a method then execute it with parameters.")),d(b('</p>\n        <br />\n        <div class="alert alert-dismissable alert-warning">\n            <button type="button" class="close" data-dismiss="alert">×</button>\n            <h4>')),d(t.gettext("Warning")),d(b("</h4>\n            <p>")),d(t.gettext("You could potentially damage your system with this and there are no sanity checks. Use at own risk.")),d(b("<br /></p>\n        </div>\n    </div>\n</div>"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/lab/apiBrowser/tpl/api_method_item.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="api-method--item">\n    <h4 class="method">')),d(this.method),d(b('</h4>\n    <p class="description">')),d(this.description),d(b("</p>\n</div>\n\n"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/lab/apiBrowser/tpl/api_method_list.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="api-methods--list">\n    <p class="search-box"><input type="text" id="api-search" class="api-methods--search" /></p>\n    <ul class="items"></ul>\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/lab/apiBrowser/tpl/api_method_page.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('\n<div class="api-method--info page">\n    <h2 class="method"><a href="http://kodi.wiki/view/JSON-RPC_API/v6#')),d(this.method),d(b('" target="_blank">')),d(this.method),d(b('</a></h2>\n    <p class="description">')),d(this.description),d(b('</p>\n\n</div>\n\n<div class="api-method--execute">\n    <h3>Execute <strong>')),d(this.method),d(b('</strong> with these params:</h3>\n        <textarea class="api-method--params" placeholder=\'Eg. ["arg", "foo", true]\'></textarea>\n        <p class="description">Parameters get parsed by\n            <a href="https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/JSON/parse" target="_blank">JSON.parse</a>.\n            Check the console for response objects, you will get an \'unexpected token\' error if parsing failed.\n            Params should be an array \'[]\' matching below \'Method params\'. Only use double quotes for strings/keys.\n        </p>\n        <p class="description">\n            Eg. [true] or [255, ["born", "formed", "thumbnail"]] or [] or [255]. Brackets required.\n        </p>\n    <p><button class="btn btn-primary" id="send-command">Send Command</button></p>\n\n</div>\n\n<div class="api-method--result" id="api-result"></div>\n\n<h3>Method Params</h3>\n<div class="api-method--params"></div>\n\n<hr />\n\n<h3>Method Returns</h3>\n<div class="api-method--return"></div>\n\n'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/lab/lab/tpl/lab_item.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<a class="lab-item" href="#')),d(this.path),d(b('">\n    <h4>')),d(this.title),d(b("</h4>\n    <p>")),d(this.description),d(b("</p>\n</a>"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/loading/show/tpl/loading_page.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div id="loading-page">\n    <div class="spinner-double-section-far"></div>\n    <h2>')),d(t.gettext("Just a sec...")),d(b("</h2>\n</div>\n\n"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/localPlaylist/list/tpl/playlist.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<span class="item">')),d(b(this.title)),d(b("</span>"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/localPlaylist/list/tpl/playlist_layout.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="local-playlist-header">\n    <h2></h2>\n    <div class="dropdown">\n        <i data-toggle="dropdown"></i>\n        <ul class="dropdown-menu">\n            <li class="play">Play in Kodi</li>\n            <li class="localplay">Play in browser</li>\n            <li class="list">Export list</li>\n            <div class="divider"></div>\n            <li class="clear">Clear playlist</li>\n            <li class="delete">Delete playlist</li>\n        </ul>\n    </div>\n</div>\n<div class="item-container">\n    <div class="empty-content">')),d(t.gettext("Empty playlist, you should probably add something to it?")),d(b("</div>\n</div>"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/localPlaylist/list/tpl/playlist_list.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<h3></h3>\n<ul class="lists"></ul>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/localPlaylist/list/tpl/playlist_sidebar_layout.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="current-lists"></div>\n<div class="new-list">New playlist</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/movie/landing/tpl/landing.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b("<h3>")),d(t.gettext("Recently added")),d(b('</h3>\n<div class="landing-section region-recently-added"></div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/movie/show/tpl/content.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('\n<div class="entity-progress"><div class="current-progress" style="width: ')),d(this.progress),d(b('%" title="')),d(this.progress),d(b("% ")),d(t.gettext("complete")),d(b('"></div></div>\n\n<div class="section-content">\n    <h2>')),d(t.gettext("Synopsis")),d(b("</h2>\n    ")),"youtube"===this.trailer.source&&(d(b('\n        <div class="trailer ')),d(this.trailer.source),d(b('">\n            <img src="')),d(b(this.trailer.img)),d(b('" />\n        </div>\n    '))),d(b("\n    <p>")),d(this.plot),d(b('</p>\n    <ul class="inline-links">\n        <li>')),d(b(helpers.url.imdbUrl(this.imdbnumber,"View on IMDb"))),d(b("</li>\n    </ul>\n</div>\n\n")),this.cast.length>0&&(d(b('\n    <div class="section-content">\n        <h2>')),d(t.gettext("Full cast")),d(b('</h2>\n        <div class="region-cast"></div>\n    </div>\n'))),d(b('\n\n<div class="region-sets section-content"></div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/movie/show/tpl/details_meta.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){var a,c,e,f;if(d(b('<div class="region-details-top">\n    <div class="region-details-title">\n        <h2><span class="title">')),d(this.label),d(b('</span> <span class="sub">')),d(this.year),d(b('</span></h2>\n    </div>\n    <div class="region-details-rating">\n        ')),d(this.rating),d(b(' <i></i>\n    </div>\n</div>\n<div class="region-details-meta-below">\n\n    <div class="region-details-subtext">\n\n        <div class="runtime">\n            ')),d(helpers.global.formatTime(helpers.global.secToTime(this.runtime))),d(b("\n        </div>\n\n        ")),this.genre.length>0&&(d(b('\n        <div class="genres">\n            ')),d(b(helpers.url.filterLinks("movies","genre",this.genre))),d(b("\n        </div>\n        "))),d(b('\n    </div>\n\n    <div class="description">')),d(this.plotoutline),d(b('</div>\n\n    <ul class="people">\n        ')),this.director.length>0&&(d(b("\n            <li><label>")),d(t.ngettext("Director","Directors",this.director.length)),d(b(":</label> <span>")),d(b(helpers.url.filterLinks("movies","director",this.director))),d(b("</span></li>\n        "))),d(b("\n        ")),this.writer.length>0&&(d(b("\n            <li><label>")),d(t.ngettext("Writer","Writers",this.writer.length)),d(b(":</label> <span>")),d(b(helpers.url.filterLinks("movies","writer",this.writer))),d(b("</span></li>\n        "))),d(b("\n        ")),this.cast.length>0&&(d(b("\n            <li><label>")),d(t.gettext("Cast")),d(b(":</label> <span>")),d(b(helpers.url.filterLinks("movies","cast",_.pluck(this.cast,"name")))),d(b("</span></li>\n        "))),d(b('\n    </ul>\n\n    <ul class="streams">\n        <li><label>')),d(t.gettext("Video")),d(b(":</label> <span>")),d(_.pluck(this.streamdetails.video,"label").join(", ")),d(b("</span></li>\n        <li><label>")),d(t.gettext("Audio")),d(b(":</label> <span>")),d(_.pluck(this.streamdetails.audio,"label").join(", ")),d(b("</span></li>\n        ")),this.streamdetails.subtitle.length>0&&""!==this.streamdetails.subtitle[0].label){for(d(b("\n            <li><label>")),d(t.ngettext("Subtitle","Subtitles",this.streamdetails.subtitle.length)),d(b(':</label>\n                <span class="dropdown"><span data-toggle="dropdown">')),d(_.pluck(this.streamdetails.subtitle,"label").join(", ")),d(b('</span>\n                <ul class="dropdown-menu">\n                    ')),e=this.streamdetails.subtitle,a=0,c=e.length;c>a;a++)f=e[a],d(b("\n                        <li>")),d(f.label),d(b("</li>\n                    "));d(b("\n                </ul>\n                </span>\n            </li>\n        "))}d(b('\n    </ul>\n\n    <ul class="inline-links">\n        <li class="btn-flat-play play">')),d(t.gettext("Play")),d(b('</li>\n        <li class="btn-flat-add add">')),d(t.gettext("Queue")),d(b('</li>\n        <li class="btn-flat-stream stream">')),d(t.gettext("Stream")),d(b('</li>\n        <li class="btn-flat-download download">')),d(t.gettext("Download")),d(b("</li>\n    </ul>\n</div>\n"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/movie/show/tpl/set.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="set-collection">\n    <h2 class="set-name"></h2>\n    <div class="collection-items"></div>\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/navMain/show/tpl/navMain.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){var a,c,e,f,g,h,i,j;for(d(b('<div id="nav-header"></div>\n<nav>\n    <ul>\n        ')),i=this.items,c=0,g=i.length;g>c;c++)if(e=i[c],"undefined"!==e.path&&0===e.parent){if(d(b('\n            <li class="')),d(e["class"]),d(b('">\n                <a href="#')),d(e.path),d(b('">\n                    <i class="')),d(e.icon),d(b('"></i>\n                    <span>')),d(e.title),d(b("</span>\n                </a>\n\n                ")),0!==e.children.length){for(d(b("\n                <ul>\n                    ")),j=e.children,f=0,h=j.length;h>f;f++)a=j[f],"undefined"!==a.path&&(d(b('\n                      <li><a href="#')),d(a.path),d(b('">')),d(a.title),d(b("</a></li>\n                    ")));d(b("\n                </ul>\n                "))}d(b("\n            </li>\n        "))}d(b("\n    </ul>\n</nav>"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/navMain/show/tpl/nav_item.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b(this.link))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/navMain/show/tpl/nav_sub.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b("<h3>")),d(this.title),d(b('</h3>\n<ul class="items"></ul>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/player/show/tpl/player.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="player">\n\n    <div class="controls-primary">\n        <div class="controls-primary-buttons">\n            <div class="control control-prev"></div>\n            <div class="control control-play"></div>\n            <div class="control control-next"></div>\n        </div>\n    </div>\n\n    <div class="controls-secondary">\n        <div class="volume slider-bar"></div>\n        <div class="controls-secondary-buttons">\n            <div class="control control-mute"></div>\n            <div class="control control-repeat"></div>\n            <div class="control control-shuffle"></div>\n            <div class="control control-menu"></div>\n        </div>\n    </div>\n\n    <div class="now-playing">\n        <div class="playing-thumb thumb">\n            <div class="mdi remote-toggle"></div>\n        </div>\n        <div class="playing-info">\n            <div class="playing-progress slider-bar"></div>\n            <div class="playing-time">\n                <div class="playing-time-current">0</div>\n                <div class="playing-time-duration">0:00</div>\n            </div>\n            <div class="playing-meta">\n                <div class="playing-title">')),d(t.gettext("Nothing playing")),d(b('</div>\n                <div class="playing-subtitle"></div>\n            </div>\n        </div>\n    </div>\n\n</div>\n'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/playlist/list/tpl/playlist_bar.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="playlist-header">\n    <ul class="player-toggle">\n        <li class="kodi">')),d(t.gettext("Kodi")),d(b('</li>\n        <li class="local">')),d(t.gettext("Local")),d(b('</li>\n    </ul>\n    <div class="playlist-menu dropdown">\n        <i data-toggle="dropdown" class="menu-toggle"></i>\n        <ul class="dropdown-menu pull-right">\n            <li class="dropdown-header">')),d(t.gettext("Current playlist")),d(b('</li>\n            <li><a href="#" class="clear-playlist">')),d(t.gettext("Clear playlist")),d(b('</a></li>\n            <li><a href="#" class="refresh-playlist">')),d(t.gettext("Refresh playlist")),d(b('</a></li>\n            <li class="dropdown-header">')),d(t.gettext("Kodi")),d(b('</li>\n            <li><a href="#" class="party-mode">')),d(t.gettext("Party mode")),d(b(' <i class="mdi-navigation-check"></i></a></li>\n            <li><a href="#" class="save-playlist">')),d(t.gettext("Save Kodi playlist")),d(b('</a></li>\n            </li>\n        </ul>\n    </div>\n</div>\n<div class="playlists-wrapper">\n    <div class="kodi-playlists">\n        <ul class="media-toggle">\n            <li class="audio">')),d(t.gettext("Audio")),d(b('</li>\n            <li class="video">')),d(t.gettext("Video")),d(b('</li>\n        </ul>\n        <div class="kodi-playlist"></div>\n    </div>\n    <div class="local-playlists">\n        <div class="local-playlist"></div>\n    </div>\n</div>\n'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/playlist/list/tpl/playlist_item.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="item-inner item-')),d(this.type),d(b('">\n    <div class="artwork">\n        <div class="thumb" title="')),d(this.label),d(b('" style="background-image: url(\'')),d(this.thumbnail),d(b('\')">\n            <div class="mdi play"></div>\n        </div>\n    </div>\n    <div class="meta">\n        <div class="title"><a href="#')),d(this.url),d(b('" title="')),d(this.label),d(b('">')),d(this.label),d(b("</a></div>\n        ")),null!=this.subtitle&&(d(b('\n        <div class="subtitle">')),d(b(this.subtitle)),d(b("</div>\n        "))),d(b('\n    </div>\n    <div class="remove"></div>\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/playlist/show/tpl/landing.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="playlist-page playlist-page__empty set-page">\n    <h3>')),d(t.gettext("Now playing - Playlists")),d(b("</h3>\n    <p>")),d(t.gettext("Switch between Kodi and local playback via the tabs. You can toggle visibility with the arrow in the top right")),d(b("</p>\n</div>\n"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/pvr/list/tpl/channel.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="play">\n    <div class="thumb">\n        <img src="')),d(this.thumbnail),d(b('" />\n    </div>\n    <div class="meta">\n        <strong>')),d(this.label),d(b("</strong>\n    </div>\n</div>"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/search/list/tpl/search_layout.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);
return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="search-inner">\n    <div class="entity-set entity-set-movie"></div>\n    <div class="entity-set entity-set-tvshow"></div>\n    <div class="entity-set entity-set-artist"></div>\n    <div class="entity-set entity-set-album"></div>\n    <div class="entity-set entity-set-song"></div>\n    <div class="entity-set entity-set-loading"></div>\n    <div class="entity-set entity-set-addons"></div>\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/search/list/tpl/search_set.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<h2 class="set-header"></h2>\n<div class="set-results"></div>\n<div class="more"></div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/search/show/tpl/landing.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="search-page search-page__empty set-page">\n    <h3>')),d(t.gettext("Enter your search above")),d(b("</h3>\n</div>\n"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/settings/show/tpl/settings_sidebar.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="settings-sidebar">\n    <div class="settings-sidebar--section local-nav nav-sub"></div>\n    <div class="settings-sidebar--section kodi-nav"></div>\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/shell/show/tpl/homepage.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div id="homepage"></div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/shell/show/tpl/shell.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div id="shell">\n\n    <a id="logo" href="#"></a>\n\n    <div id="nav-bar"></div>\n\n    <div id="header">\n\n        <h1 id="page-title">\n            <span class="context"></span>\n            <span class="title"></span>\n        </h1>\n\n        <ul class="mobile-menu">\n            <li><a href="#remote" class="mobile-menu--link__remote remote-toggle"><i></i></a></li>\n            <li><a href="#search" class="mobile-menu--link__search"><i></i></a></li>\n            <li><a href="#playlist" class="mobile-menu--link__playlist"><i></i></a></li>\n        </ul>\n\n        <div id="search-region">\n            <input id="search" title="Search">\n            <span id="do-search"></span>\n        </div>\n\n    </div>\n\n    <div id="main">\n\n        <div id="sidebar-one"></div>\n\n        <div id="content">')),d(t.gettext("Loading things...")),d(b('</div>\n\n    </div>\n\n    <div id="sidebar-two">\n        <div class="playlist-toggle-open"></div>\n        <div id="playlist-summary"></div>\n        <div id="playlist-bar"></div>\n    </div>\n\n    <div id="remote"></div>\n\n    <div id="player-wrapper">\n        <footer id="player-kodi"></footer>\n        <footer id="player-local"></footer>\n    </div>\n\n    <div class="player-menu-wrapper">\n        <ul class="player-menu">\n            <li class="video-scan">')),d(t.gettext("Scan video library")),d(b('</li>\n            <li class="audio-scan">')),d(t.gettext("Scan audio library")),d(b('</li>\n            <li class="send-input">')),d(t.gettext("Send text to Kodi")),d(b('</li>\n            <li class="goto-lab">')),d(t.gettext("The lab")),d(b('</li>\n            <li class="about">')),d(t.gettext("About Chorus")),d(b('</li>\n        </ul>\n    </div>\n\n</div>\n\n<div id="fanart"></div>\n<div id="fanart-overlay"></div>\n\n<div id="snackbar-container"></div>\n\n<div class="modal fade" id="modal-window">\n    <div class="modal-dialog">\n        <div class="modal-content">\n            <div class="modal-header">\n                <button type="button" class="close" data-dismiss="modal" aria-label="Close"><span aria-hidden="true">&times;</span></button>\n                <h4 class="modal-title"></h4>\n            </div>\n            <div class="modal-body"></div>\n            <div class="modal-footer"></div>\n        </div>\n    </div>\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/song/list/tpl/song.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<td class="cell-first">\n    <div class="thumb" style="background-image: url(\'')),d(this.thumbnail),d(b('\')">\n    </div>\n    <div class="track">')),d(this.track),d(b('</div>\n    <div class="mdi play"></div>\n</td>\n<td class="cell-label song-title"><span class="crop">')),d(this.label),d(b('</span></td>\n<td class="cell-label song-artist"><a class="crop" href="#music/artist/')),d(this.artistid),d(b('">')),d(this.artist),d(b('</a></td>\n<td class="cell-last">\n    <li class="thumbed-up"></li>\n    <div class="duration">')),d(this.displayDuration),d(b('</div>\n    <ul class="actions">\n        <li class="mdi thumbs"></li>\n        <li class="mdi add"></li>\n        <li class="menu dropdown">\n            <i data-toggle="dropdown" class="mdi"></i>\n            <ul class="dropdown-menu pull-right"></ul>\n        </li>\n    </ul>\n</td>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/thumbs/list/tpl/thumbs_layout.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="thumbs-inner">\n    <div class="entity-set entity-set-movie"></div>\n    <div class="entity-set entity-set-tvshow"></div>\n    <div class="entity-set entity-set-artist"></div>\n    <div class="entity-set entity-set-album"></div>\n    <div class="entity-set entity-set-song"></div>\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/thumbs/list/tpl/thumbs_set.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<h2 class="set-header"></h2>\n<div class="set-results"></div>\n<div class="more"></div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/tvshow/episode/tpl/content.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('\n<div class="entity-progress"><div class="current-progress" style="width: ')),d(this.progress),d(b('%" title="')),d(this.progress),d(b("% ")),d(t.gettext("complete")),d(b('"></div></div>\n\n<div class="section-content">\n    <h2>')),d(t.gettext("Synopsis")),d(b("</h2>\n    <p>")),d(this.plot),d(b("</p>\n</div>\n\n")),this.cast.length>0&&(d(b('\n    <div class="section-content">\n        <h2>')),d(t.gettext("Full Cast")),d(b('</h2>\n        <div class="region-cast"></div>\n    </div>\n'))),d(b("\n"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/tvshow/episode/tpl/details_meta.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){var a,c,e,f;if(d(b('<div class="region-details-top">\n    ')),null!=this.showtitle&&d(b("\n\n    ")),d(b('\n    <div class="region-details-title">\n        <h2>\n            <span class="title">')),d(this.label),d(b('</span>\n            <span class="sub show-title"><a href="#')),d(this.url.split("/",2).join("/")),d(b('">')),d(this.showtitle),d(b('</a></span>\n            <span class="sub">S')),d(this.season),d(b(" E")),d(this.episode),d(b('</span>\n        </h2>\n    </div>\n    <div class="region-details-rating">\n        ')),d(this.rating),d(b(' <i></i>\n    </div>\n</div>\n<div class="region-details-meta-below">\n\n    <div class="region-details-subtext">\n\n        <div class="runtime">\n            ')),d(helpers.global.formatTime(helpers.global.secToTime(this.runtime))),d(b('\n        </div>\n\n    </div>\n\n\n\n\n    <ul class="people">\n        ')),this.director.length>0&&(d(b("\n            <li><label>")),d(t.ngettext("Director","Directors",this.director.length)),d(b(":</label> <span>")),d(b(helpers.url.filterLinks("tvshows","director",this.director))),d(b("</span></li>\n        "))),d(b("\n        ")),this.writer.length>0&&(d(b("\n            <li><label>")),d(t.ngettext("Writer","Writers",this.writer.length)),d(b(":</label> <span>")),d(b(helpers.url.filterLinks("tvshows","writer",this.writer))),d(b("</span></li>\n        "))),d(b("\n        ")),this.cast.length>0&&(d(b("\n            <li><label>")),d(t.gettext("Cast")),d(b(":</label> <span>")),d(b(helpers.url.filterLinks("tvshows","cast",_.pluck(this.cast,"name")))),d(b("</span></li>\n        "))),d(b('\n    </ul>\n\n    <ul class="streams">\n        <li><label>')),d(t.gettext("Video")),d(b(":</label> <span>")),d(_.pluck(this.streamdetails.video,"label").join(", ")),d(b("</span></li>\n        <li><label>")),d(t.gettext("Audio")),d(b(":</label> <span>")),d(_.pluck(this.streamdetails.audio,"label").join(", ")),d(b("</span></li>\n        ")),this.streamdetails.subtitle.length>0&&""!==this.streamdetails.subtitle[0].label){for(d(b("\n            <li><label>")),d(t.ngettext("Subtitle","Subtitles",this.streamdetails.subtitle.length)),d(b(':</label>\n                <span class="dropdown"><span data-toggle="dropdown">')),d(_.first(_.pluck(this.streamdetails.subtitle,"label"))),d(b('</span>\n                <ul class="dropdown-menu">\n                    ')),e=this.streamdetails.subtitle,a=0,c=e.length;c>a;a++)f=e[a],d(b("\n                        <li>")),d(f.label),d(b("</li>\n                    "));d(b("\n                </ul>\n                </span>\n            </li>\n        "))}d(b('\n    </ul>\n\n    <ul class="inline-links">\n        <li class="btn-flat-play play">')),d(t.gettext("Play")),d(b('</li>\n        <li class="btn-flat-add add">')),d(t.gettext("Queue")),d(b('</li>\n        <li class="btn-flat-stream stream">')),d(t.gettext("Stream")),d(b('</li>\n        <li class="btn-flat-download download">')),d(t.gettext("Download")),d(b("</li>\n    </ul>\n</div>\n"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/tvshow/landing/tpl/landing.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b("<h3>")),d(t.gettext("Recently added")),d(b('</h3>\n<div class="landing-section region-recently-added"></div> '))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/tvshow/season/tpl/details_meta.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="region-details-top">\n    <div class="region-details-title">\n        <h2>\n            <span class="title">')),d(t.gettext("Season")),d(b(" ")),d(this.season),d(b('</span>\n            <span class="sub"><a href="#tvshow/')),d(this.tvshowid),d(b('">')),d(this.label),d(b('</a></span>\n        </h2>\n    </div>\n    <div class="region-details-rating">\n        ')),d(this.rating),d(b(' <i></i>\n    </div>\n</div>\n<div class="region-details-meta-below">\n\n    <div class="region-details-subtext">\n        ')),this.genre.length>0&&(d(b('\n        <div class="genres">\n            ')),d(b(helpers.url.filterLinks("tvshows","genre",this.genre))),d(b("\n        </div>\n        "))),d(b('\n    </div>\n\n    <ul class="people">\n        ')),this.cast.length>0&&(d(b("\n        <li><label>")),d(t.gettext("Cast")),d(b(":</label> <span>")),d(b(helpers.url.filterLinks("tvshows","cast",_.pluck(this.cast,"name")))),d(b("</span></li>\n        "))),d(b('\n    </ul>\n\n    <div class="description">')),d(this.plot),d(b("</div>\n\n</div>\n"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["apps/tvshow/show/tpl/details_meta.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="region-details-top">\n    <div class="region-details-title">\n        <h2><span class="title">')),d(this.label),d(b('</span> <span class="sub">')),d(this.year),d(b('</span></h2>\n    </div>\n    <div class="region-details-rating">\n        ')),d(this.rating),d(b(' <i></i>\n    </div>\n</div>\n<div class="region-details-meta-below">\n\n    <div class="region-details-subtext">\n        ')),this.genre.length>0&&(d(b('\n        <div class="genres">\n            ')),d(b(helpers.url.filterLinks("tvshows","genre",this.genre))),d(b("\n        </div>\n        "))),d(b('\n    </div>\n\n    <ul class="people">\n        ')),this.cast.length>0&&(d(b("\n        <li><label>")),d(t.gettext("Cast")),d(b(":</label> <span>")),d(b(helpers.url.filterLinks("tvshows","cast",_.pluck(this.cast,"name")))),d(b("</span></li>\n        "))),d(b('\n    </ul>\n\n    <div class="description">')),d(this.plot),d(b("</div>\n\n</div>\n"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["components/form/tpl/form.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="form-inner">\n	<div class="form-content-region"></div>\n    <footer>\n        <ul class="inline-list">\n            <li>\n                <button type="submit" data-form-button="submit" class="btn btn-primary form-save">Save</button>\n            </li>\n            <li class="response">\n\n            </li>\n        </ul>\n    </footer>\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["components/form/tpl/form_item.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){this.title&&(d(b('\n    <label class="control-label">')),d(b(this.title)),d(b("</label>\n"))),d(b("\n\n")),"markup"===this.type||"button"===this.type?(d(b("\n    ")),d(b(this.element)),d(b("\n"))):(d(b('\n    <div class="element">\n        ')),"checkbox"!==this.type?(d(b("\n            ")),d(b(this.element)),d(b("\n        "))):(d(b('\n            <div class="togglebutton">\n                <label>')),d(b(this.element)),d(b("</label>\n            </div>\n        "))),d(b("\n        ")),this.description&&(d(b('\n        <div class="help-block description">')),d(b(this.description)),d(b("</div>\n        "))),d(b("\n    </div>\n"))),d(b("\n"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["components/form/tpl/form_item_group.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){this.title&&(d(b('\n    <h3 class="group-title">')),this.icon&&(d(b('<i class="')),d(b(this.icon)),d(b('"></i> '))),d(b(this.title)),d(b("</h3>\n"))),d(b('\n<div class="form-items"></div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["views/card/tpl/card.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){var a,c,e;if(d(b('<div class="card-')),d(this.type),d(b('">\n    <div class="artwork">\n        <a href="#')),d(this.url),d(b('" class="thumb" title="')),d(helpers.global.stripTags(this.label)),d(b('" style="background-image: url(\'')),d(this.thumbnail),d(b('\')"></a>\n        <div class="mdi play"></div>\n        ')),("channeltv"===this.type||"channelradio"===this.type)&&d(b('\n          <div class="mdi record"></div>\n        ')),d(b('\n    </div>\n    <div class="meta">\n        <div class="title"><a href="#')),d(this.url),d(b('" title="')),d(helpers.global.stripTags(this.label)),d(b('">')),d(b(this.label)),d(b("</a></div>\n        ")),null!=this.subtitle&&(d(b('\n            <div class="subtitle">')),d(b(this.subtitle)),d(b("</div>\n        "))),d(b("\n    </div>\n    ")),this.actions){d(b('\n        <ul class="actions">\n            ')),c=this.actions;for(a in c)e=c[a],d(b('<li class="mdi ')),d(a),d(b('" title="')),d(e),d(b('"></li>'));d(b("\n        </ul>\n    "))}d(b("\n    ")),this.menu&&d(b('\n        <div class="dropdown">\n            <i data-toggle="dropdown" class="mdi"></i>\n            <ul class="dropdown-menu"></ul>\n        </div>\n    ')),d(b("\n    ")),this.progress&&(d(b('\n        <div class="entity-progress"><div class="current-progress" style="width: ')),d(this.progress),d(b('%" title="')),d(this.progress),d(b("% ")),d(t.gettext("complete")),d(b('"></div></div>\n    '))),d(b('\n    <i class="mdi watched-tick"></i>\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["views/card/tpl/card_placeholder.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b("<i></i>"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["views/empty/tpl/empty_page.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="empty--page">\n    ')),this.title&&(d(b('\n        <h2 class="empty--page-title">')),d(title),d(b("</h2>\n    "))),d(b("\n\n    ")),this.content&&(d(b('\n        <div class="empty--page-content">')),d(this.content),d(b("</div>\n    "))),d(b("\n</div>"))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["views/empty/tpl/empty_results.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="empty-result">\n    <h2>No results found</h2>\n    <p>Have you done a library scan?</p>\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["views/layouts/tpl/layout_details_header.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="layout-container details-header">\n\n    <div class="region-details-side"></div>\n\n    <div class="region-details-meta-wrapper"><div class="region-details-meta">\n\n        <div class="region-details-title"><span class="title"></span> <span class="sub"></span></div>\n\n        <div class="region-details-meta-below">\n            <div class="region-details-subtext"></div>\n            <div class="description"></div>\n        </div>\n\n    </div></div>\n\n    <div class="region-details-fanart"><div class="gradient"></div></div>\n\n</div>\n'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["views/layouts/tpl/layout_with_header.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="layout-container with-header">\n\n    <header class="region-header"></header>\n\n    <section class="region-content"></section>\n\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())},window.JST["views/layouts/tpl/layout_with_sidebar_first.jst"]=function(a){var b=function(a){"undefined"==typeof a&&null==a&&(a="");var b=new String(a);return b.ecoSafe=!0,b};return function(){var a=[],c=this,d=function(b){"undefined"!=typeof b&&null!=b&&a.push(b.ecoSafe?b:c.escape(b))};return function(){d(b('<div class="layout-container with-sidebar-first">\n\n    <div class="region-first-toggle"></div>\n    <section class="region-first"></section>\n\n    <section class="region-content-wrapper">\n        <div class="region-content-top"></div>\n        <div class="region-content"></div>\n    </section>\n\n</div>'))}.call(this),a.join("")}.call(function(){var c,d={escape:function(a){return(""+a).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")},safe:b};for(c in a)d[c]=a[c];return d}())};;var tr,
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; },
  __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __slice = [].slice;

this.helpers = {};

this.config = {
  "static": {
    appTitle: 'Kodi',
    jsonRpcEndpoint: 'jsonrpc',
    socketsHost: location.hostname,
    socketsPort: 9090,
    ajaxTimeout: 5000,
    hashKey: 'kodi',
    defaultPlayer: 'auto',
    ignoreArticle: true,
    pollInterval: 10000,
    reverseProxy: false,
    albumAtristsOnly: true,
    searchIndexCacheExpiry: 24 * 60 * 60,
    collectionCacheExpiry: 7 * 24 * 60 * 60,
    addOnsLoaded: false,
    vibrantHeaders: false,
    lang: "en",
    kodiSettingsLevel: 'standard',
    playlistFocusPlaying: true,
    keyboardControl: 'kodi'
  }
};

this.Kodi = (function(Backbone, Marionette) {
  var App;
  App = new Backbone.Marionette.Application();
  App.addRegions({
    root: "body"
  });
  App.on("before:start", function() {
    return config["static"] = _.extend(config["static"], config.get('app', 'config:local', config["static"]));
  });
  App.vent.on("shell:ready", (function(_this) {
    return function(options) {
      return App.startHistory();
    };
  })(this));
  return App;
})(Backbone, Marionette);

$(document).ready((function(_this) {
  return function() {
    return helpers.translate.init(function() {
      Kodi.start();
      return $.material.init();
    });
  };
})(this));


/*
  Helper to return you to the same scroll position on the last page.
 */

helpers.backscroll = {
  lastPath: '',
  lastScroll: 0,
  setLast: function() {
    this.lastPath = location.hash;
    return this.lastScroll = document.body.scrollTop;
  },
  scrollToLast: function() {
    var scrollPos;
    scrollPos = this.lastPath === location.hash ? this.lastScroll : 0;
    return window.scrollTo(0, scrollPos);
  }
};


/*
  Our cache storage, persists only for app lifycle
  Eg. gets wiped when page reloaded.
 */

helpers.cache = {
  store: {},
  defaultExpiry: 406800
};

helpers.cache.set = function(key, data, expires) {
  if (expires == null) {
    expires = helpers.cache.defaultExpiry;
  }
  helpers.cache.store[key] = {
    data: data,
    expires: expires + helpers.global.time(),
    key: key
  };
  return data;
};

helpers.cache.get = function(key, fallback) {
  if (fallback == null) {
    fallback = false;
  }
  if ((helpers.cache.store[key] != null) && helpers.cache.store[key].expires >= helpers.global.time()) {
    return helpers.cache.store[key].data;
  } else {
    return fallback;
  }
};

helpers.cache.del = function(key) {
  if (helpers.cache.store[key] != null) {
    return delete helpers.cache.store[key];
  }
};

helpers.cache.clear = function() {
  return helpers.cache.store = {};
};

helpers.cache.updateCollection = function(collectionKey, responseKey, modelId, property, value) {
  var i, item, _ref, _results;
  if ((Backbone.fetchCache._cache != null) && (Backbone.fetchCache._cache[collectionKey] != null) && (Backbone.fetchCache._cache[collectionKey].value.result != null)) {
    if (Backbone.fetchCache._cache[collectionKey].value.result[responseKey] != null) {
      _ref = Backbone.fetchCache._cache[collectionKey].value.result[responseKey];
      _results = [];
      for (i in _ref) {
        item = _ref[i];
        if (item.id === modelId) {
          _results.push(Backbone.fetchCache._cache[collectionKey].value.result[responseKey][parseInt(i)][property] = value);
        } else {
          _results.push(void 0);
        }
      }
      return _results;
    }
  }
};


/*
  Config Helpers.
 */

config.get = function(type, id, defaultData, callback) {
  var data;
  if (defaultData == null) {
    defaultData = '';
  }
  data = Kodi.request("config:" + type + ":get", id, defaultData);
  if (callback != null) {
    callback(data);
  }
  return data;
};

config.set = function(type, id, data, callback) {
  var resp;
  resp = Kodi.request("config:" + type + ":set", id, data);
  if (callback != null) {
    callback(resp);
  }
  return resp;
};

config.getLocal = function(id, defaultData, callback) {
  return config.get('static', id, defaultData, callback);
};

config.setLocal = function(id, data, callback) {
  return config.set('static', id, data, callback);
};

config.setLocalApp = function() {
  return config.set('static', id, data, callback);
};

config.preStartGet = function(id, defaultData) {
  var config;
  if (defaultData == null) {
    defaultData = '';
  }
  if ((typeof localStorage !== "undefined" && localStorage !== null) && (localStorage.getItem('config:app-config:local') != null)) {
    config = JSON.parse(localStorage.getItem('config:app-config:local')).data;
    if (config[id] != null) {
      return config[id];
    }
  }
  return defaultData;
};


/*
  Entities mixins, all the common things we do/need on almost every collection

  example of usage:

  collection = new KodiCollection()
    .setEntityType 'collection'
    .setEntityKey 'movie'
    .setEntityFields 'small', ['thumbnail', 'title']
    .setEntityFields 'full', ['fanart', 'genre']
    .setMethod 'VideoLibrary.GetMovies'
    .setArgHelper 'fields'
    .setArgHelper 'limit'
    .setArgHelper 'sort'
    .applySettings()
 */

if (this.KodiMixins == null) {
  this.KodiMixins = {};
}

KodiMixins.Entities = {
  url: function() {
    return helpers.url.baseKodiUrl("Mixins");
  },
  rpc: new Backbone.Rpc({
    useNamedParameters: true,
    namespaceDelimiter: ''
  }),

  /*
    Overrides!
   */

  /*
    Apply all the defined settings.
   */
  applySettings: function() {
    if (this.entityType === 'model') {
      return this.setModelDefaultFields();
    }
  },

  /*
    What kind of entity are we dealing with. collection or model
   */
  entityType: 'model',
  setEntityType: function(type) {
    this.entityType = type;
    return this;
  },

  /*
    Entity Keys, properties that change between the entities
   */
  entityKeys: {
    type: '',
    modelResponseProperty: '',
    collectionResponseProperty: '',
    idProperty: ''
  },
  setEntityKey: function(key, value) {
    this.entityKeys[key] = value;
    return this;
  },
  getEntityKey: function(key) {
    var ret, type;
    type = this.entityKeys.type;
    switch (key) {
      case 'modelResponseProperty':
        ret = this.entityKeys[key] != null ? this.entityKeys[key] : type + 'details';
        break;
      case 'collectionResponseProperty':
        ret = this.entityKeys[key] != null ? this.entityKeys[key] : type + 's';
        break;
      case 'idProperty':
        ret = this.entityKeys[key] != null ? this.entityKeys[key] : type + 'id';
        break;
      default:
        ret = type;
    }
    return ret;
  },

  /*
    The types of fields we request, minimal for search, small for list, full for page.
   */
  entitiyFields: {
    minimal: [],
    small: [],
    full: []
  },
  setEntityFields: function(type, fields) {
    if (fields == null) {
      fields = [];
    }
    this.entitiyFields[type] = fields;
    return this;
  },
  getEntityFields: function(type) {
    var fields;
    fields = this.entitiyFields.minimal;
    if (type === 'full') {
      return fields.concat(this.entitiyFields.small).concat(this.entitiyFields.full);
    } else if (type === 'small') {
      return fields.concat(this.entitiyFields.small);
    } else {
      return fields;
    }
  },
  modelDefaults: {
    id: 0,
    fullyloaded: false,
    thumbnail: '',
    thumbsUp: false
  },
  setModelDefaultFields: function(defaultFields) {
    var field, _i, _len, _ref, _results;
    if (defaultFields == null) {
      defaultFields = {};
    }
    defaultFields = _.extend(this.modelDefaults, defaultFields);
    _ref = this.getEntityFields('full');
    _results = [];
    for (_i = 0, _len = _ref.length; _i < _len; _i++) {
      field = _ref[_i];
      _results.push(this.defaults[field] = '');
    }
    return _results;
  },

  /*
    JsonRPC common paterns and helpers.
   */
  callMethodName: '',
  callArgs: [],
  callIgnoreArticle: true,
  setMethod: function(method) {
    this.callMethodName = method;
    return this;
  },
  setArgStatic: function(callback) {
    this.callArgs.push(callback);
    return this;
  },
  setArgHelper: function(helper, param1, param2) {
    var func;
    func = 'argHelper' + helper;
    this.callArgs.push(this[func](param1, param2));
    return this;
  },
  argCheckOption: function(option, fallback) {
    if ((this.options != null) && (this.options[option] != null)) {
      return this.options[option];
    } else {
      return fallback;
    }
  },
  argHelperfields: function(type) {
    var arg;
    if (type == null) {
      type = 'small';
    }
    arg = this.getEntityFields(type);
    return this.argCheckOption('fields', arg);
  },
  argHelpersort: function(method, order) {
    var arg;
    if (order == null) {
      order = 'ascending';
    }
    arg = {
      method: method,
      order: order,
      ignorearticle: this.callIgnoreArticle
    };
    return this.argCheckOption('sort', arg);
  },
  argHelperlimit: function(start, end) {
    var arg;
    if (start == null) {
      start = 0;
    }
    if (end == null) {
      end = 'all';
    }
    arg = {
      start: start
    };
    if (end !== 'all') {
      arg.end = end;
    }
    return this.argCheckOption('limit', arg);
  },
  argHelperfilter: function(name, value) {
    var arg;
    arg = {};
    if (name != null) {
      arg[name] = value;
    }
    return this.argCheckOption('filter', arg);
  },
  buildRpcRequest: function(type) {
    var arg, func, key, req, _i, _len, _ref;
    if (type == null) {
      type = 'read';
    }
    req = [this.callMethodName];
    _ref = this.callArgs;
    for (_i = 0, _len = _ref.length; _i < _len; _i++) {
      arg = _ref[_i];
      func = 'argHelper' + arg;
      if (typeof func === 'function') {
        key = 'arg' + req.length;
        req.push(key);
        this[key] = func;
      } else {
        req.push(arg);
      }
    }
    return req;
  }
};


/*
  Handle errors.
 */

helpers.debug = {
  verbose: false
};


/*
  Debug styles

  @param severity
  The severity level: info, success, warning, error
 */

helpers.debug.consoleStyle = function(severity) {
  var defaults, mods, prop, styles;
  if (severity == null) {
    severity = 'error';
  }
  defaults = {
    background: "#ccc",
    padding: "0 5px",
    color: "#444",
    "font-weight": "bold",
    "font-size": "110%"
  };
  styles = [];
  mods = {
    info: "#D8FEFE",
    success: "#CCFECD",
    warning: "#FFFDD9",
    error: "#FFCECD"
  };
  defaults.background = mods[severity];
  for (prop in defaults) {
    styles.push(prop + ": " + defaults[prop]);
  }
  return styles.join("; ");
};


/*
  Basic debug message
 */

helpers.debug.msg = function(msg, severity, data) {
  if (severity == null) {
    severity = 'info';
  }
  if (typeof console !== "undefined" && console !== null) {
    console.log("%c " + msg, helpers.debug.consoleStyle(severity));
    if (data != null) {
      return console.log(data);
    }
  }
};


/*
  Log a deubg error message.
 */

helpers.debug.log = function(msg, data, severity, caller) {
  if (data == null) {
    data = 'No data provided';
  }
  if (severity == null) {
    severity = 'error';
  }
  if (caller == null) {
    caller = arguments.callee.caller.toString();
  }
  if ((data[0] != null) && data[0].error === "Internal server error") {

  } else {
    if (typeof console !== "undefined" && console !== null) {
      console.log("%c Error in: " + msg, helpers.debug.consoleStyle(severity), data);
      if (helpers.debug.verbose && caller !== false) {
        return console.log(caller);
      }
    }
  }
};


/*
  Request Error.
 */

helpers.debug.rpcError = function(commands, data) {
  var detail, msg;
  detail = {
    called: commands
  };
  msg = '';
  if (data.error && data.error.message) {
    msg = '"' + data.error.message + '"';
    detail.error = data.error;
  } else {
    detail.error = data;
  }
  return helpers.debug.log("jsonRPC request - " + msg, detail, 'error');
};


/*
  Entity Helpers
 */

helpers.entities = {};

helpers.entities.createUid = function(model, type) {
  var file, hash, id, uid;
  type = type ? type : model.type;
  id = model.id;
  uid = 'none';
  if (typeof id === 'number') {
    uid = id;
  } else {
    file = model.file;
    if (file) {
      hash = helpers.global.hashEncode(file);
      uid = 'path-' + hash.substring(0, 26);
    }
  }
  return type + '-' + uid;
};

helpers.entities.getFields = function(set, type) {
  var fields;
  if (type == null) {
    type = 'small';
  }
  fields = set.minimal;
  if (type === 'full') {
    return fields.concat(set.small).concat(set.full);
  } else if (type === 'small') {
    return fields.concat(set.small);
  } else {
    return fields;
  }
};

helpers.entities.getSubtitle = function(model) {
  var subtitle;
  switch (model.type) {
    case 'song':
      subtitle = model.artist.join(',');
      break;
    default:
      subtitle = '';
  }
  return subtitle;
};

helpers.entities.playingLink = function(model) {
  return "<a href='#" + model.url + "'>" + model.label + "</a>";
};

helpers.entities.isWatched = function(model) {
  var watched;
  watched = false;
  if ((model != null) && model.get('playcount')) {
    watched = model.get('playcount') > 0 ? true : false;
  }
  return watched;
};


/*
  Our generic global helpers so we dont have add complexity to our app.
 */

helpers.global = {};

helpers.global.shuffle = function(array) {
  var i, j, temp;
  i = array.length - 1;
  while (i > 0) {
    j = Math.floor(Math.random() * (i + 1));
    temp = array[i];
    array[i] = array[j];
    array[j] = temp;
    i--;
  }
  return array;
};

helpers.global.getRandomInt = function(min, max) {
  return Math.floor(Math.random() * (max - min + 1)) + min;
};

helpers.global.time = function() {
  var timestamp;
  timestamp = new Date().getTime();
  return timestamp / 1000;
};

helpers.global.inArray = function(needle, haystack) {
  return _.indexOf(haystack, needle) > -1;
};

helpers.global.loading = (function(_this) {
  return function(state) {
    var op;
    if (state == null) {
      state = 'start';
    }
    op = state === 'start' ? 'add' : 'remove';
    if (_this.Kodi != null) {
      return _this.Kodi.execute("body:state", op, "loading");
    }
  };
})(this);

helpers.global.numPad = function(num, size) {
  var s;
  s = "000000000" + num;
  return s.substr(s.length - size);
};

helpers.global.secToTime = function(totalSec) {
  var hours, minutes, seconds;
  if (totalSec == null) {
    totalSec = 0;
  }
  totalSec = Math.round(totalSec);
  hours = parseInt(totalSec / 3600) % 24;
  minutes = parseInt(totalSec / 60) % 60;
  seconds = totalSec % 60;
  return {
    hours: hours,
    minutes: minutes,
    seconds: seconds
  };
};

helpers.global.timeToSec = function(time) {
  var hours, minutes;
  hours = parseInt(time.hours) * (60 * 60);
  minutes = parseInt(time.minutes) * 60;
  return parseInt(hours) + parseInt(minutes) + parseInt(time.seconds);
};

helpers.global.epgDateTimeToJS = function(datetime) {
  if (!datetime) {
    return new Date(0);
  } else {
    return new Date(datetime.replace(" ", "t"));
  }
};

helpers.global.formatTime = function(time) {
  var hrStr;
  if (time == null) {
    return 0;
  } else {
    hrStr = "";
    if (time.hours > 0) {
      if (time.hours < 10) {
        hrStr = "0";
      }
      hrStr += time.hours + ':';
    }
    return hrStr + (time.minutes<10 ? '0' : '') + time.minutes + ':' + (time.seconds<10 ? '0' : '') + time.seconds;
  }
};

helpers.global.paramObj = function(key, value) {
  var obj;
  obj = {};
  obj[key] = value;
  return obj;
};

helpers.global.regExpEscape = function(str) {
  return str.replace(/([.?*+^$[\]\\(){}|-])/g, "\\$1");
};

helpers.global.stringStartsWith = function(start, data) {
  return new RegExp('^' + helpers.global.regExpEscape(start)).test(data);
};

helpers.global.stringStripStartsWith = function(start, data) {
  return data.substring(start.length);
};

helpers.global.arrayToSentence = function(arr, pluralise) {
  var i, item, last, prefix, str;
  if (pluralise == null) {
    pluralise = true;
  }
  str = '';
  prefix = pluralise ? 's' : '';
  last = arr.pop();
  if (arr.length > 0) {
    for (i in arr) {
      item = arr[i];
      str += '<strong>' + item + prefix + '</strong>';
      str += parseInt(i) !== (arr.length - 1) ? ', ' : '';
    }
    str += ' ' + t.gettext('and') + ' ';
  }
  return str += '<strong>' + last + prefix + '</strong>';
};

helpers.global.hashEncode = function(value) {
  return Base64.encode(value);
};

helpers.global.hashDecode = function(value) {
  return Base64.decode(value);
};

helpers.global.rating = function(rating) {
  return Math.round(rating * 10) / 10;
};

helpers.global.appTitle = function(playingItem) {
  var titlePrefix;
  if (playingItem == null) {
    playingItem = false;
  }
  titlePrefix = '';
  if (_.isObject(playingItem) && (playingItem.label != null)) {
    titlePrefix = '▶ ' + playingItem.label + ' | ';
  }
  return document.title = titlePrefix + config.get('static', 'appTitle');
};

helpers.global.localVideoPopup = function(path, height) {
  if (height == null) {
    height = 590;
  }
  return window.open(path, "_blank", "toolbar=no, scrollbars=no, resizable=yes, width=925, height=" + height + ", top=100, left=100");
};

helpers.global.stripTags = function(string) {
  if (string != null) {
    return string.replace(/(<([^>]+)>)/ig, "");
  } else {
    return '';
  }
};

helpers.global.round = function(x, places) {
  if (places == null) {
    places = 0;
  }
  return parseFloat(x.toFixed(places));
};

helpers.global.getPercent = function(pos, total, places) {
  if (places == null) {
    places = 2;
  }
  return Math.floor((pos / total) * (100 * Math.pow(10, places))) / 100;
};


/*
  A collection of small jquery plugin helpers.
 */

$.fn.removeClassRegex = function(regex) {
  return $(this).removeClass(function(index, classes) {
    return classes.split(/\s+/).filter(function(c) {
      return regex.test(c);
    }).join(' ');
  });
};

$.fn.removeClassStartsWith = function(startsWith) {
  var regex;
  regex = new RegExp('^' + startsWith, 'g');
  return $(this).removeClassRegex(regex);
};

$.fn.scrollStopped = function(callback) {
  var $this, self;
  $this = $(this);
  self = this;
  return $this.scroll(function() {
    if ($this.data('scrollTimeout')) {
      clearTimeout($this.data('scrollTimeout'));
    }
    return $this.data('scrollTimeout', setTimeout(callback, 250, self));
  });
};

$.fn.resizeStopped = function(callback) {
  var $this, self;
  $this = $(this);
  self = this;
  return $this.resize(function() {
    if ($this.data('resizeTimeout')) {
      clearTimeout($this.data('resizeTimeout'));
    }
    return $this.data('resizeTimeout', setTimeout(callback, 250, self));
  });
};

$.fn.filterList = function(settings, callback) {
  var $this, defaults;
  $this = $(this);
  defaults = {
    hiddenClass: 'hidden',
    items: '.filter-options-list li',
    textSelector: '.option'
  };
  settings = $.extend(defaults, settings);
  return $this.on('keyup', (function(_this) {
    return function() {
      var $list, val;
      val = $this.val().toLocaleLowerCase();
      $list = $(settings.items).removeClass(settings.hiddenClass);
      if (val.length > 0) {
        $list.each(function(i, d) {
          var text;
          text = $(d).find(settings.textSelector).text().toLowerCase();
          if (text.indexOf(val) === -1) {
            return $(d).addClass(settings.hiddenClass);
          }
        });
      }
      if (typeof callback === "function") {
        return callback();
      }
    };
  })(this));
};

$(document).ready(function() {
  $('.dropdown li').on('click', function() {
    return $(this).closest('.dropdown').removeClass('open');
  });
  return $('.dropdown').on('click', function() {
    return $(this).removeClass('open').trigger('hide.bs.dropdown');
  });
});


/*
  Stream helpers
 */

helpers.stream = {};


/*
  Maps.
 */

helpers.stream.videoSizeMap = [
  {
    min: 0,
    max: 360,
    label: 'SD'
  }, {
    min: 361,
    max: 480,
    label: '480'
  }, {
    min: 481,
    max: 720,
    label: '720p'
  }, {
    min: 721,
    max: 1080,
    label: '1080p'
  }, {
    min: 1081,
    max: 100000,
    label: '4K'
  }
];

helpers.stream.audioChannelMap = [
  {
    channels: 6,
    label: '5.1'
  }, {
    channels: 8,
    label: '7.1'
  }, {
    channels: 2,
    label: '2.1'
  }, {
    channels: 1,
    label: 'mono'
  }
];

helpers.stream.langMap = {
  'eng': 'English',
  'und': 'Unknown',
  'bul': 'Bulgaria',
  'ara': 'Arabic',
  'zho': 'Chinese',
  'ces': 'Czech',
  'dan': 'Danish',
  'nld': 'Netherland',
  'fin': 'Finish',
  'fra': 'French',
  'deu': 'German',
  'nor': 'Norwegian',
  'pol': 'Polish',
  'por': 'Portuguese',
  'ron': 'Romanian',
  'spa': 'Spanish',
  'swe': 'Swedish',
  'tur': 'Turkish',
  'vie': 'Vietnamese'
};


/*
  Formatters.
 */

helpers.stream.videoFormat = function(videoStreams) {
  var i, match, stream;
  for (i in videoStreams) {
    stream = videoStreams[i];
    match = {
      label: 'SD'
    };
    if (stream.height && stream.height > 0) {
      match = _.find(helpers.stream.videoSizeMap, function(res) {
        var _ref;
        if ((res.min < (_ref = stream.height) && _ref <= res.max)) {
          return true;
        } else {
          return false;
        }
      });
    }
    videoStreams[i].label = stream.codec + ' ' + match.label + ' (' + stream.width + ' x ' + stream.height + ')';
    videoStreams[i].shortlabel = stream.codec + ' ' + match.label;
    videoStreams[i].res = match.label;
  }
  return videoStreams;
};

helpers.stream.formatLanguage = function(lang) {
  if (helpers.stream.langMap[lang]) {
    return helpers.stream.langMap[lang];
  } else {
    return lang;
  }
};

helpers.stream.audioFormat = function(audioStreams) {
  var ch, i, lang, stream;
  for (i in audioStreams) {
    stream = audioStreams[i];
    ch = _.findWhere(helpers.stream.audioChannelMap, {
      channels: stream.channels
    });
    ch = ch ? ch.label : stream.channels;
    lang = '';
    if (stream.language !== '') {
      lang = ' (' + helpers.stream.formatLanguage(stream.language) + ')';
    }
    audioStreams[i].label = stream.codec + ' ' + ch + lang;
    audioStreams[i].shortlabel = stream.codec + ' ' + ch;
    audioStreams[i].ch = ch;
  }
  return audioStreams;
};

helpers.stream.subtitleFormat = function(subtitleStreams) {
  var i, stream;
  for (i in subtitleStreams) {
    stream = subtitleStreams[i];
    subtitleStreams[i].label = helpers.stream.formatLanguage(stream.language);
    subtitleStreams[i].shortlabel = helpers.stream.formatLanguage(stream.language);
  }
  return subtitleStreams;
};

helpers.stream.streamFormat = function(streams) {
  var streamTypes, type, _i, _len;
  streamTypes = ['audio', 'video', 'subtitle'];
  for (_i = 0, _len = streamTypes.length; _i < _len; _i++) {
    type = streamTypes[_i];
    if (streams[type] && streams[type].length > 0) {
      streams[type] = helpers.stream[type + 'Format'](streams[type]);
    } else {
      streams[type] = [];
    }
  }
  return streams;
};


/*
  For everything translatable.
 */

helpers.translate = {};

helpers.translate.getLanguages = function() {
  return {
    en: "English",
    fr: "French",
    de: "German",
    nl: "Dutch"
  };
};

helpers.translate.init = function(callback) {
  var defaultLang, lang;
  defaultLang = config.get("static", "lang", "en");
  lang = config.preStartGet("lang", defaultLang);
  return $.getJSON("lang/_strings/" + lang + ".json", function(data) {
    window.t = new Jed(data);
    t.options["missing_key_callback"] = function(key) {
      return helpers.translate.missingKeyLog(key);
    };
    return callback();
  });
};

helpers.translate.missingKeyLog = function(key) {
  var item;
  item = '\n\n' + 'msgctxt ""\n' + 'msgid "' + key + '"\n' + 'msgstr "' + key + '"\n';
  return helpers.debug.msg(item, 'warning');
};


/*
  Translate aliases.
 */

tr = function(text) {
  return t.gettext(text);
};


/*
  UI helpers for the app.
 */

helpers.ui = {};

helpers.ui.getSwatch = function(imgSrc, callback) {
  var img, ret;
  ret = {};
  img = document.createElement('img');
  img.setAttribute('src', imgSrc);
  return img.addEventListener('load', function() {
    var sw, swatch, swatches, vibrant;
    vibrant = new Vibrant(img);
    swatches = vibrant.swatches();
    for (swatch in swatches) {
      if (swatches.hasOwnProperty(swatch) && swatches[swatch]) {
        sw = swatches[swatch];
        ret[swatch] = {
          hex: sw.getHex(),
          rgb: _.map(sw.getRgb(), function(c) {
            return Math.round(c);
          }),
          title: sw.getTitleTextColor(),
          body: sw.getBodyTextColor()
        };
      }
    }
    return callback(ret);
  });
};

helpers.ui.applyHeaderSwatch = function(swatches) {
  var $header, color, gradient, rgb;
  if ((swatches != null) && (swatches.DarkVibrant != null) && (swatches.DarkVibrant.hex != null)) {
    if (config.get('static', 'vibrantHeaders') === true) {
      color = swatches.DarkVibrant;
      $header = $('.details-header');
      $header.css('background-color', color.hex);
      rgb = color.rgb.join(',');
      gradient = 'linear-gradient(to right, ' + color.hex + ' 0%, rgba(' + rgb + ',0.9) 30%, rgba(' + rgb + ',0) 100%)';
      return $('.region-details-fanart .gradient', $header).css('background-image', gradient);
    }
  }
};


/*
  Handle urls.
 */

helpers.url = {};

helpers.url.map = {
  artist: 'music/artist/:id',
  album: 'music/album/:id',
  song: 'music/song/:id',
  movie: 'movie/:id',
  tvshow: 'tvshow/:id',
  season: 'tvshow/:id',
  episode: 'tvshow/:tvshowid/:season/:id',
  channeltv: 'tvshows/live/:id',
  channelradio: 'music/radio/:id',
  file: 'browser/file/:id',
  playlist: 'playlist/:id'
};

helpers.url.baseKodiUrl = function(query) {
  var path;
  if (query == null) {
    query = 'Kodi';
  }
  path = (config.getLocal('jsonRpcEndpoint')) + "?" + query;
  if (config.getLocal('reverseProxy')) {
    return path;
  } else {
    return "/" + path;
  }
};

helpers.url.get = function(type, id, replacements) {
  var path, token;
  if (id == null) {
    id = '';
  }
  if (replacements == null) {
    replacements = {};
  }
  path = '';
  if (helpers.url.map[type] != null) {
    path = helpers.url.map[type];
  }
  replacements[':id'] = id;
  for (token in replacements) {
    id = replacements[token];
    path = path.replace(token, id);
  }
  return path;
};

helpers.url.filterLinks = function(entities, key, items, limit) {
  var baseUrl, i, item, links;
  if (limit == null) {
    limit = 5;
  }
  baseUrl = '#' + entities + '?' + key + '=';
  links = [];
  for (i in items) {
    item = items[i];
    if (i < limit) {
      links.push('<a href="' + baseUrl + encodeURIComponent(item) + '">' + item + '</a>');
    }
  }
  return links.join(', ');
};

helpers.url.playlistUrl = function(item) {
  if (item.type === 'song') {
    if (item.albumid !== '') {
      item.url = helpers.url.get('album', item.albumid);
    } else {
      item.url('music/albums');
    }
  }
  return item.url;
};

helpers.url.arg = function(arg) {
  var args, hash;
  if (arg == null) {
    arg = 'none';
  }
  hash = location.hash;
  args = hash.substring(1).split('/');
  if (arg === 'none') {
    return args;
  } else if (args[arg] != null) {
    return args[arg];
  } else {
    return '';
  }
};

helpers.url.params = function(params) {
  var p, path, query, _ref;
  if (params == null) {
    params = 'auto';
  }
  if (params === 'auto') {
    p = document.location.href;
    if (p.indexOf('?') === -1) {
      return {};
    } else {
      _ref = p.split('?'), path = _ref[0], query = _ref[1];
    }
  }
  if (query == null) {
    query = params;
  }
  return _.object(_.compact(_.map(query.split('&'), function(item) {
    if (item) {
      return item.split('=');
    }
  })));
};

helpers.url.buildParams = function(params) {
  var key, q, val;
  q = [];
  for (key in params) {
    val = params[key];
    q.push(key + '=' + encodeURIComponent(val));
  }
  return '?' + q.join('&');
};

helpers.url.alterParams = function(add, remove) {
  var curParams, k, params, _i, _len;
  if (add == null) {
    add = {};
  }
  if (remove == null) {
    remove = [];
  }
  curParams = helpers.url.params();
  if (remove.length > 0) {
    for (_i = 0, _len = remove.length; _i < _len; _i++) {
      k = remove[_i];
      delete curParams[k];
    }
  }
  params = _.extend(curParams, add);
  return helpers.url.path() + helpers.url.buildParams(params);
};

helpers.url.path = function() {
  var p, path, query, _ref;
  p = document.location.hash;
  _ref = p.split('?'), path = _ref[0], query = _ref[1];
  return path.substring(1);
};

helpers.url.imdbUrl = function(imdbNumber, text) {
  var url;
  url = "http://www.imdb.com/title/" + imdbNumber + "/";
  return "<a href='" + url + "' class='imdblink' target='_blank'>" + (t.gettext(text)) + "</a>";
};

helpers.url.parseTrailerUrl = function(trailer) {
  var ret, urlParts;
  ret = {
    source: '',
    id: '',
    img: '',
    url: ''
  };
  urlParts = helpers.url.params(trailer);
  if (trailer.indexOf('youtube') > -1) {
    ret.source = 'youtube';
    ret.id = urlParts.videoid;
    ret.img = "http://img.youtube.com/vi/" + ret.id + "/0.jpg";
    ret.url = "https://www.youtube.com/watch?v=" + ret.id;
  }
  return ret;
};

helpers.url.isSecureProtocol = function() {
  var secure;
  secure = (typeof document !== "undefined" && document !== null) && document.location.protocol === "https:" ? true : false;
  return secure;
};

Cocktail.patch(Backbone);

(function(Backbone) {
  var methods, _sync;
  _sync = Backbone.sync;
  Backbone.sync = function(method, entity, options) {
    var sync;
    if (options == null) {
      options = {};
    }
    _.defaults(options, {
      beforeSend: _.bind(methods.beforeSend, entity),
      complete: _.bind(methods.complete, entity)
    });
    sync = _sync(method, entity, options);
    if (!entity._fetch && method === "read") {
      return entity._fetch = sync;
    }
  };
  return methods = {
    beforeSend: function() {
      return this.trigger("sync:start", this);
    },
    complete: function() {
      return this.trigger("sync:stop", this);
    }
  };
})(Backbone);

(function(Backbone) {
  return _.extend(Backbone.Marionette.Application.prototype, {
    navigate: function(route, options) {
      if (options == null) {
        options = {};
      }
      return Backbone.history.navigate(route, options);
    },
    getCurrentRoute: function() {
      var frag;
      frag = Backbone.history.fragment;
      if (_.isEmpty(frag)) {
        return null;
      } else {
        return frag;
      }
    },
    startHistory: function() {
      if (Backbone.history) {
        return Backbone.history.start();
      }
    },
    register: function(instance, id) {
      if (this._registry == null) {
        this._registry = {};
      }
      return this._registry[id] = instance;
    },
    unregister: function(instance, id) {
      return delete this._registry[id];
    },
    resetRegistry: function() {
      var controller, key, msg, oldCount, _ref;
      oldCount = this.getRegistrySize();
      _ref = this._registry;
      for (key in _ref) {
        controller = _ref[key];
        controller.region.close();
      }
      msg = "There were " + oldCount + " controllers in the registry, there are now " + (this.getRegistrySize());
      if (this.getRegistrySize() > 0) {
        return console.warn(msg, this._registry);
      } else {
        return console.log(msg);
      }
    },
    getRegistrySize: function() {
      return _.size(this._registry);
    }
  });
})(Backbone);

(function(Marionette) {
  return _.extend(Marionette.Renderer, {
    extension: [".jst"],
    render: function(template, data) {
      var path;
      path = this.getTemplate(template);
      if (!path) {
        throw "Template " + template + " not found!";
      }
      return path(data);
    },
    getTemplate: function(template) {
      var path;
      path = this.insertAt(template.split("/"), -1, "tpl").join("/");
      path = path + this.extension;
      if (JST[path]) {
        return JST[path];
      }
    },
    insertAt: function(array, index, item) {
      array.splice(index, 0, item);
      return array;
    }
  });
})(Marionette);

this.Kodi.module("Entities", function(Entities, App, Backbone, Marionette, $, _) {
  return Entities.Collection = (function(_super) {
    __extends(Collection, _super);

    function Collection() {
      return Collection.__super__.constructor.apply(this, arguments);
    }

    Collection.prototype.getRawCollection = function() {
      var model, objs, _i, _len, _ref;
      objs = [];
      if (this.models.length > 0) {
        _ref = this.models;
        for (_i = 0, _len = _ref.length; _i < _len; _i++) {
          model = _ref[_i];
          objs.push(model.attributes);
        }
      }
      return objs;
    };

    Collection.prototype.getCacheKey = function(options) {
      var key;
      key = this.constructor.name;
      return key;
    };

    Collection.prototype.autoSort = function(params) {
      var order;
      if (params.sort) {
        order = params.order ? params.order : 'asc';
        return this.sortCollection(params.sort, order);
      }
    };

    Collection.prototype.sortCollection = function(property, order) {
      if (order == null) {
        order = 'asc';
      }
      this.comparator = (function(_this) {
        return function(model) {
          return _this.ignoreArticleParse(model.get(property));
        };
      })(this);
      if (order === 'desc') {
        this.comparator = this.reverseSortBy(this.comparator);
      }
      this.sort();
    };

    Collection.prototype.reverseSortBy = function(sortByFunction) {
      return function(left, right) {
        var l, r;
        l = sortByFunction(left);
        r = sortByFunction(right);
        if (l === void 0) {
          return -1;
        }
        if (r === void 0) {
          return 1;
        }
        if (l < r) {
          return 1;
        } else if (l > r) {
          return -1;
        } else {
          return 0;
        }
      };
    };

    Collection.prototype.ignoreArticleParse = function(string) {
      var articles, parsed, s, _i, _len;
      articles = ["'", '"', 'the ', 'a '];
      if (typeof string === 'string' && config.get('static', 'ignoreArticle', true)) {
        string = string.toLowerCase();
        parsed = false;
        for (_i = 0, _len = articles.length; _i < _len; _i++) {
          s = articles[_i];
          if (parsed) {
            continue;
          }
          if (helpers.global.stringStartsWith(s, string)) {
            string = helpers.global.stringStripStartsWith(s, string);
            parsed = true;
          }
        }
      }
      return string;
    };

    return Collection;

  })(Backbone.Collection);
});

this.Kodi.module("Entities", function(Entities, App, Backbone, Marionette, $, _) {
  return Entities.Filtered = (function(_super) {
    __extends(Filtered, _super);

    function Filtered() {
      return Filtered.__super__.constructor.apply(this, arguments);
    }

    Filtered.prototype.filterByMultiple = function(key, values) {
      if (values == null) {
        values = [];
      }
      return this.filterBy(key, function(model) {
        return helpers.global.inArray(model.get(key), values);
      });
    };

    Filtered.prototype.filterByMultipleArray = function(key, values) {
      if (values == null) {
        values = [];
      }
      return this.filterBy(key, function(model) {
        var match, v, _i, _len, _ref;
        match = false;
        _ref = model.get(key);
        for (_i = 0, _len = _ref.length; _i < _len; _i++) {
          v = _ref[_i];
          if (helpers.global.inArray(v, values)) {
            match = true;
          }
        }
        return match;
      });
    };

    Filtered.prototype.filterByMultipleObject = function(key, property, values) {
      if (values == null) {
        values = [];
      }
      return this.filterBy(key, function(model) {
        var items, match, v, _i, _len;
        match = false;
        items = _.pluck(model.get(key), property);
        for (_i = 0, _len = items.length; _i < _len; _i++) {
          v = items[_i];
          if (helpers.global.inArray(v, values)) {
            match = true;
          }
        }
        return match;
      });
    };

    Filtered.prototype.filterByUnwatched = function() {
      return this.filterBy('unwatched', function(model) {
        var unwatched;
        unwatched = 1;
        if (model.get('type') === 'tvshow') {
          unwatched = model.get('episode') - model.get('watchedepisodes');
        } else if (model.get('type') === 'movie' || model.get('type') === 'episode') {
          unwatched = model.get('playcount') > 0 ? 0 : 1;
        }
        return unwatched > 0;
      });
    };

    Filtered.prototype.filterByString = function(key, query) {
      return this.filterBy('search', function(model) {
        var value;
        if (query.length < 3) {
          return false;
        } else {
          value = model.get(key).toLowerCase();
          return value.indexOf(query.toLowerCase()) > -1;
        }
      });
    };

    return Filtered;

  })(FilteredCollection);
});

this.Kodi.module("Entities", function(Entities, App, Backbone, Marionette, $, _) {
  return Entities.Model = (function(_super) {
    __extends(Model, _super);

    function Model() {
      this.saveError = __bind(this.saveError, this);
      this.saveSuccess = __bind(this.saveSuccess, this);
      return Model.__super__.constructor.apply(this, arguments);
    }

    Model.prototype.getCacheKey = function(options) {
      var key;
      key = this.constructor.name;
      return key;
    };

    Model.prototype.destroy = function(options) {
      if (options == null) {
        options = {};
      }
      _.defaults(options, {
        wait: true
      });
      this.set({
        _destroy: true
      });
      return Model.__super__.destroy.call(this, options);
    };

    Model.prototype.isDestroyed = function() {
      return this.get("_destroy");
    };

    Model.prototype.save = function(data, options) {
      var isNew;
      if (options == null) {
        options = {};
      }
      isNew = this.isNew();
      _.defaults(options, {
        wait: true,
        success: _.bind(this.saveSuccess, this, isNew, options.collection),
        error: _.bind(this.saveError, this)
      });
      this.unset("_errors");
      return Model.__super__.save.call(this, data, options);
    };

    Model.prototype.saveSuccess = function(isNew, collection) {
      if (isNew) {
        if (collection) {
          collection.add(this);
        }
        if (collection) {
          collection.trigger("model:created", this);
        }
        return this.trigger("created", this);
      } else {
        if (collection == null) {
          collection = this.collection;
        }
        if (collection) {
          collection.trigger("model:updated", this);
        }
        return this.trigger("updated", this);
      }
    };

    Model.prototype.saveError = function(model, xhr, options) {
      var _ref;
      if (!(xhr.status === 500 || xhr.status === 404)) {
        return this.set({
          _errors: (_ref = $.parseJSON(xhr.responseText)) != null ? _ref.errors : void 0
        });
      }
    };

    return Model;

  })(Backbone.Model);
});


/*
  App configuration settings, items stored in local storage and are
  specific to the browser/user instance. Not Kodi settings.
 */

this.Kodi.module("Entities", function(Entities, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    storageKey: 'config:app',
    getCollection: function() {
      var collection;
      collection = new Entities.ConfigAppCollection();
      collection.fetch();
      return collection;
    },
    getConfig: function(id, collection) {
      if (collection == null) {
        collection = API.getCollection();
      }
      return collection.find({
        id: id
      });
    }
  };
  Entities.ConfigApp = (function(_super) {
    __extends(ConfigApp, _super);

    function ConfigApp() {
      return ConfigApp.__super__.constructor.apply(this, arguments);
    }

    ConfigApp.prototype.defaults = {
      data: {}
    };

    return ConfigApp;

  })(Entities.Model);
  Entities.ConfigAppCollection = (function(_super) {
    __extends(ConfigAppCollection, _super);

    function ConfigAppCollection() {
      return ConfigAppCollection.__super__.constructor.apply(this, arguments);
    }

    ConfigAppCollection.prototype.model = Entities.ConfigApp;

    ConfigAppCollection.prototype.localStorage = new Backbone.LocalStorage(API.storageKey);

    return ConfigAppCollection;

  })(Entities.Collection);
  App.reqres.setHandler("config:app:get", function(configId, defaultData) {
    var model;
    model = API.getConfig(configId);
    if (model != null) {
      return model.get('data');
    } else {
      return defaultData;
    }
  });
  App.reqres.setHandler("config:app:set", function(configId, configData) {
    var collection, model;
    collection = API.getCollection();
    model = API.getConfig(configId, collection);
    if (model != null) {
      return model.save({
        data: configData
      });
    } else {
      collection.create({
        id: configId,
        data: configData
      });
      return configData;
    }
  });
  App.reqres.setHandler("config:static:get", function(configId, defaultData) {
    var data;
    data = config["static"][configId] != null ? config["static"][configId] : defaultData;
    return data;
  });
  return App.reqres.setHandler("config:static:set", function(configId, data) {
    config["static"][configId] = data;
    return data;
  });
});


/*
  Youtube collection
 */

this.Kodi.module("Entities", function(Entities, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    apiKey: 'AIzaSyBxvaR6mCnUWN8cv2TiPRmuEh0FykBTAH0',
    searchUrl: 'https://www.googleapis.com/youtube/v3/search?part=snippet&type=video&videoDefinition=any&videoEmbeddable=true&order=relevance&safeSearch=none',
    getSearchUrl: function() {
      return this.searchUrl + '&key=' + this.apiKey;
    },
    parseItems: function(response) {
      var i, item, items, resp, _ref;
      items = [];
      _ref = response.items;
      for (i in _ref) {
        item = _ref[i];
        resp = {
          id: item.id.videoId,
          title: item.snippet.title,
          desc: item.snippet.description,
          thumbnail: item.snippet.thumbnails["default"].url
        };
        items.push(resp);
      }
      return items;
    }
  };
  Entities.youtubeItem = (function(_super) {
    __extends(youtubeItem, _super);

    function youtubeItem() {
      return youtubeItem.__super__.constructor.apply(this, arguments);
    }

    youtubeItem.prototype.defaults = {
      id: '',
      title: '',
      desc: '',
      thumbnail: ''
    };

    return youtubeItem;

  })(Entities.Model);
  Entities.youtubeCollection = (function(_super) {
    __extends(youtubeCollection, _super);

    function youtubeCollection() {
      return youtubeCollection.__super__.constructor.apply(this, arguments);
    }

    youtubeCollection.prototype.model = Entities.youtubeItem;

    youtubeCollection.prototype.url = API.getSearchUrl();

    youtubeCollection.prototype.sync = function(method, collection, options) {
      options.dataType = "jsonp";
      options.timeout = 5000;
      return Backbone.sync(method, collection, options);
    };

    youtubeCollection.prototype.parse = function(resp) {
      return API.parseItems(resp);
    };

    return youtubeCollection;

  })(Entities.Collection);
  return App.commands.setHandler("youtube:search:entities", function(query, callback) {
    var yt;
    yt = new Entities.youtubeCollection();
    return yt.fetch({
      data: {
        q: query
      },
      success: function(collection) {
        return callback(collection);
      },
      error: function(collection) {
        return helpers.debug.log('Youtube search error', 'error', collection);
      }
    });
  });
});

this.Kodi.module("Entities", function(Entities, App, Backbone, Marionette, $, _) {
  Entities.Filter = (function(_super) {
    __extends(Filter, _super);

    function Filter() {
      return Filter.__super__.constructor.apply(this, arguments);
    }

    Filter.prototype.defaults = {
      alias: '',
      type: 'string',
      key: '',
      sortOrder: 'asc',
      title: '',
      active: false
    };

    return Filter;

  })(Entities.Model);
  Entities.FilterCollection = (function(_super) {
    __extends(FilterCollection, _super);

    function FilterCollection() {
      return FilterCollection.__super__.constructor.apply(this, arguments);
    }

    FilterCollection.prototype.model = Entities.Filter;

    return FilterCollection;

  })(Entities.Collection);
  Entities.FilterOption = (function(_super) {
    __extends(FilterOption, _super);

    function FilterOption() {
      return FilterOption.__super__.constructor.apply(this, arguments);
    }

    FilterOption.prototype.defaults = {
      key: '',
      value: '',
      title: ''
    };

    return FilterOption;

  })(Entities.Model);
  Entities.FilterOptionCollection = (function(_super) {
    __extends(FilterOptionCollection, _super);

    function FilterOptionCollection() {
      return FilterOptionCollection.__super__.constructor.apply(this, arguments);
    }

    FilterOptionCollection.prototype.model = Entities.Filter;

    return FilterOptionCollection;

  })(Entities.Collection);
  Entities.FilterSort = (function(_super) {
    __extends(FilterSort, _super);

    function FilterSort() {
      return FilterSort.__super__.constructor.apply(this, arguments);
    }

    FilterSort.prototype.defaults = {
      alias: '',
      type: 'string',
      defaultSort: false,
      defaultOrder: 'asc',
      key: '',
      active: false,
      order: 'asc',
      title: ''
    };

    return FilterSort;

  })(Entities.Model);
  Entities.FilterSortCollection = (function(_super) {
    __extends(FilterSortCollection, _super);

    function FilterSortCollection() {
      return FilterSortCollection.__super__.constructor.apply(this, arguments);
    }

    FilterSortCollection.prototype.model = Entities.FilterSort;

    return FilterSortCollection;

  })(Entities.Collection);
  Entities.FilterActive = (function(_super) {
    __extends(FilterActive, _super);

    function FilterActive() {
      return FilterActive.__super__.constructor.apply(this, arguments);
    }

    FilterActive.prototype.defaults = {
      key: '',
      values: [],
      title: ''
    };

    return FilterActive;

  })(Entities.Model);
  Entities.FilterActiveCollection = (function(_super) {
    __extends(FilterActiveCollection, _super);

    function FilterActiveCollection() {
      return FilterActiveCollection.__super__.constructor.apply(this, arguments);
    }

    FilterActiveCollection.prototype.model = Entities.FilterActive;

    return FilterActiveCollection;

  })(Entities.Collection);
  App.reqres.setHandler('filter:filters:entities', function(collection) {
    return new Entities.FilterCollection(collection);
  });
  App.reqres.setHandler('filter:filters:options:entities', function(collection) {
    return new Entities.FilterOptionCollection(collection);
  });
  App.reqres.setHandler('filter:sort:entities', function(collection) {
    return new Entities.FilterSortCollection(collection);
  });
  return App.reqres.setHandler('filter:active:entities', function(collection) {
    return new Entities.FilterActiveCollection(collection);
  });
});

this.Kodi.module("Entities", function(Entities, App, Backbone, Marionette, $, _) {
  var API;
  Entities.FormItem = (function(_super) {
    __extends(FormItem, _super);

    function FormItem() {
      return FormItem.__super__.constructor.apply(this, arguments);
    }

    FormItem.prototype.defaults = {
      id: 0,
      title: '',
      type: '',
      element: '',
      options: [],
      defaultValue: '',
      description: '',
      children: [],
      attributes: {},
      "class": ''
    };

    return FormItem;

  })(Entities.Model);
  Entities.Form = (function(_super) {
    __extends(Form, _super);

    function Form() {
      return Form.__super__.constructor.apply(this, arguments);
    }

    Form.prototype.model = Entities.FormItem;

    return Form;

  })(Entities.Collection);
  API = {
    applyState: function(item, formState) {
      if (formState[item.id] != null) {
        item.defaultValue = formState[item.id];
        item.defaultsApplied = true;
      }
      return item;
    },
    processItems: function(items, formState, isChild) {
      var collection, item, _i, _len;
      if (formState == null) {
        formState = {};
      }
      if (isChild == null) {
        isChild = false;
      }
      collection = [];
      for (_i = 0, _len = items.length; _i < _len; _i++) {
        item = items[_i];
        item = this.applyState(item, formState);
        if (item.children && item.children.length > 0) {
          item.children = API.processItems(item.children, formState, true);
        }
        collection.push(item);
      }
      return collection;
    },
    toCollection: function(items) {
      var childCollection, i, item;
      for (i in items) {
        item = items[i];
        if (item.children && item.children.length > 0) {
          childCollection = new Entities.Form(item.children);
          items[i].children = childCollection;
        }
      }
      return new Entities.Form(items);
    }
  };
  return App.reqres.setHandler("form:item:entities", function(form, formState) {
    if (form == null) {
      form = [];
    }
    if (formState == null) {
      formState = {};
    }
    return API.toCollection(API.processItems(form, formState));
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    cacheSynced: function(entities, callback) {
      return entities.on('cachesync', function() {
        callback();
        return helpers.global.loading("end");
      });
    },
    xhrsFetch: function(entities, callback) {
      var xhrs;
      xhrs = _.chain([entities]).flatten().pluck("_fetch").value();
      return $.when.apply($, xhrs).done(function() {
        callback();
        return helpers.global.loading("end");
      });
    }
  };
  return App.commands.setHandler("when:entity:fetched", function(entities, callback) {
    helpers.global.loading("start");
    if (!entities.params) {
      return API.cacheSynced(entities, callback);
    } else {
      return API.xhrsFetch(entities, callback);
    }
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {
  Backbone.fetchCache.localStorage = false;
  return KodiEntities.Collection = (function(_super) {
    __extends(Collection, _super);

    function Collection() {
      return Collection.__super__.constructor.apply(this, arguments);
    }

    Collection.prototype.url = function() {
      return helpers.url.baseKodiUrl("Collection");
    };

    Collection.prototype.rpc = new Backbone.Rpc({
      useNamedParameters: true,
      namespaceDelimiter: ''
    });

    Collection.prototype.sync = function(method, model, options) {
      if (method === 'read') {
        this.options = options;
      }
      return Backbone.sync(method, model, options);
    };

    Collection.prototype.getCacheKey = function(options) {
      var k, key, prop, val, _i, _len, _ref, _ref1;
      this.options = options;
      key = this.constructor.name;
      _ref = ['filter', 'sort', 'limit', 'file'];
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        k = _ref[_i];
        if (options[k]) {
          _ref1 = options[k];
          for (prop in _ref1) {
            val = _ref1[prop];
            key += ':' + prop + ':' + val;
          }
        }
      }
      return key;
    };

    Collection.prototype.getResult = function(response, key) {
      var result;
      result = response.jsonrpc && response.result ? response.result : response;
      return result[key];
    };

    Collection.prototype.argCheckOption = function(option, fallback) {
      if ((this.options != null) && (this.options[option] != null)) {
        return this.options[option];
      } else {
        return fallback;
      }
    };

    Collection.prototype.argSort = function(method, order) {
      var arg;
      if (order == null) {
        order = 'ascending';
      }
      arg = {
        method: method,
        order: order,
        ignorearticle: this.isIgnoreArticle()
      };
      return this.argCheckOption('sort', arg);
    };

    Collection.prototype.argLimit = function(start, end) {
      var arg;
      if (start == null) {
        start = 0;
      }
      if (end == null) {
        end = 'all';
      }
      arg = {
        start: start
      };
      if (end !== 'all') {
        arg.end = end;
      }
      return this.argCheckOption('limit', arg);
    };

    Collection.prototype.argFilter = function(name, value) {
      var arg;
      arg = {};
      if (name != null) {
        arg[name] = value;
      }
      return this.argCheckOption('filter', arg);
    };

    Collection.prototype.isIgnoreArticle = function() {
      return config.getLocal('ignoreArticle', true);
    };

    return Collection;

  })(App.Entities.Collection);
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {
  return KodiEntities.Model = (function(_super) {
    __extends(Model, _super);

    function Model() {
      return Model.__super__.constructor.apply(this, arguments);
    }

    Model.prototype.url = function() {
      return helpers.url.baseKodiUrl("Model");
    };

    Model.prototype.rpc = new Backbone.Rpc({
      useNamedParameters: true,
      namespaceDelimiter: ''
    });

    Model.prototype.modelDefaults = {
      fullyloaded: false,
      thumbnail: '',
      thumbsUp: false,
      parsed: false
    };

    Model.prototype.parseModel = function(type, model, id) {
      if (!model.parsed) {
        if (id !== 'mixed') {
          model.id = id;
        }
        if (model.rating) {
          model.rating = helpers.global.rating(model.rating);
        }
        if (model.streamdetails && _.isObject(model.streamdetails)) {
          model.streamdetails = helpers.stream.streamFormat(model.streamdetails);
        }
        if (model.resume) {
          model.progress = model.resume.position === 0 ? 0 : Math.round((model.resume.position / model.resume.total) * 100);
        }
        if (model.trailer) {
          model.trailer = helpers.url.parseTrailerUrl(model.trailer);
        }
        if (type === 'tvshow' || type === 'season') {
          model.progress = helpers.global.round((model.watchedepisodes / model.episode) * 100, 2);
        }
        if (type === 'episode') {
          model.url = helpers.url.get(type, id, {
            ':tvshowid': model.tvshowid,
            ':season': model.season
          });
        } else if (type === 'channel') {
          if (model.channeltype === 'tv') {
            type = "channeltv";
          } else {
            type = "channelradio";
          }
          model.url = helpers.url.get(type, id);
        } else {
          model.url = helpers.url.get(type, id);
        }
        model = App.request("images:path:entity", model);
        model.type = type;
        model.uid = helpers.entities.createUid(model, type);
        model.parsed = true;
      }
      return model;
    };

    Model.prototype.parseFieldsToDefaults = function(fields, defaults) {
      var field, _i, _len;
      if (defaults == null) {
        defaults = {};
      }
      for (_i = 0, _len = fields.length; _i < _len; _i++) {
        field = fields[_i];
        defaults[field] = '';
      }
      return defaults;
    };

    Model.prototype.checkResponse = function(response, checkKey) {
      var obj;
      obj = response[checkKey] != null ? response[checkKey] : response;
      if (response[checkKey] != null) {
        obj.fullyloaded = true;
      }
      return obj;
    };

    return Model;

  })(App.Entities.Model);
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    getAlbumFields: function(type) {
      var baseFields, extraFields, fields;
      if (type == null) {
        type = 'small';
      }
      baseFields = ['thumbnail', 'playcount', 'artistid', 'artist', 'genre', 'albumlabel', 'year'];
      extraFields = ['fanart', 'style', 'mood', 'description', 'genreid', 'rating'];
      if (type === 'full') {
        fields = baseFields.concat(extraFields);
        return fields;
      } else {
        return baseFields;
      }
    },
    getAlbum: function(id, options) {
      var album;
      album = new App.KodiEntities.Album();
      album.set({
        albumid: parseInt(id),
        properties: API.getAlbumFields('full')
      });
      album.fetch(options);
      return album;
    },
    getAlbums: function(options) {
      var albums, defaultOptions;
      defaultOptions = {
        cache: true,
        expires: config.get('static', 'collectionCacheExpiry')
      };
      options = _.extend(defaultOptions, options);
      albums = new KodiEntities.AlbumCollection();
      albums.fetch(options);
      return albums;
    },
    getRecentlyAddedAlbums: function(options) {
      var albums;
      albums = new KodiEntities.AlbumRecentlyAddedCollection();
      albums.fetch(options);
      return albums;
    },
    getRecentlyPlayedAlbums: function(options) {
      var albums;
      albums = new KodiEntities.AlbumRecentlyPlayedCollection();
      albums.fetch(options);
      return albums;
    }
  };
  KodiEntities.Album = (function(_super) {
    __extends(Album, _super);

    function Album() {
      return Album.__super__.constructor.apply(this, arguments);
    }

    Album.prototype.defaults = function() {
      var fields;
      fields = _.extend(this.modelDefaults, {
        albumid: 1,
        album: ''
      });
      return this.parseFieldsToDefaults(API.getAlbumFields('full'), fields);
    };

    Album.prototype.methods = {
      read: ['AudioLibrary.GetAlbumDetails', 'albumid', 'properties']
    };

    Album.prototype.arg2 = API.getAlbumFields('full');

    Album.prototype.parse = function(resp, xhr) {
      var obj;
      obj = resp.albumdetails != null ? resp.albumdetails : resp;
      if (resp.albumdetails != null) {
        obj.fullyloaded = true;
      }
      return this.parseModel('album', obj, obj.albumid);
    };

    return Album;

  })(App.KodiEntities.Model);
  KodiEntities.AlbumCollection = (function(_super) {
    __extends(AlbumCollection, _super);

    function AlbumCollection() {
      return AlbumCollection.__super__.constructor.apply(this, arguments);
    }

    AlbumCollection.prototype.model = KodiEntities.Album;

    AlbumCollection.prototype.methods = {
      read: ['AudioLibrary.GetAlbums', 'arg1', 'arg2', 'arg3', 'arg4']
    };

    AlbumCollection.prototype.arg1 = function() {
      return API.getAlbumFields('small');
    };

    AlbumCollection.prototype.arg2 = function() {
      return this.argLimit();
    };

    AlbumCollection.prototype.arg3 = function() {
      return this.argSort("title", "ascending");
    };

    AlbumCollection.prototype.arg3 = function() {
      return this.argFilter();
    };

    AlbumCollection.prototype.parse = function(resp, xhr) {
      return this.getResult(resp, 'albums');
    };

    return AlbumCollection;

  })(App.KodiEntities.Collection);
  KodiEntities.AlbumRecentlyAddedCollection = (function(_super) {
    __extends(AlbumRecentlyAddedCollection, _super);

    function AlbumRecentlyAddedCollection() {
      return AlbumRecentlyAddedCollection.__super__.constructor.apply(this, arguments);
    }

    AlbumRecentlyAddedCollection.prototype.model = KodiEntities.Album;

    AlbumRecentlyAddedCollection.prototype.methods = {
      read: ['AudioLibrary.GetRecentlyAddedAlbums', 'arg1', 'arg2']
    };

    AlbumRecentlyAddedCollection.prototype.arg1 = function() {
      return API.getAlbumFields('small');
    };

    AlbumRecentlyAddedCollection.prototype.arg2 = function() {
      return this.argLimit(0, 21);
    };

    AlbumRecentlyAddedCollection.prototype.parse = function(resp, xhr) {
      return this.getResult(resp, 'albums');
    };

    return AlbumRecentlyAddedCollection;

  })(App.KodiEntities.Collection);
  KodiEntities.AlbumRecentlyPlayedCollection = (function(_super) {
    __extends(AlbumRecentlyPlayedCollection, _super);

    function AlbumRecentlyPlayedCollection() {
      return AlbumRecentlyPlayedCollection.__super__.constructor.apply(this, arguments);
    }

    AlbumRecentlyPlayedCollection.prototype.model = KodiEntities.Album;

    AlbumRecentlyPlayedCollection.prototype.methods = {
      read: ['AudioLibrary.GetRecentlyPlayedAlbums', 'arg1', 'arg2']
    };

    AlbumRecentlyPlayedCollection.prototype.arg1 = function() {
      return API.getAlbumFields('small');
    };

    AlbumRecentlyPlayedCollection.prototype.arg2 = function() {
      return this.argLimit(0, 21);
    };

    AlbumRecentlyPlayedCollection.prototype.parse = function(resp, xhr) {
      return this.getResult(resp, 'albums');
    };

    return AlbumRecentlyPlayedCollection;

  })(App.KodiEntities.Collection);
  App.reqres.setHandler("album:entity", function(id, options) {
    if (options == null) {
      options = {};
    }
    return API.getAlbum(id, options);
  });
  App.reqres.setHandler("album:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getAlbums(options);
  });
  App.reqres.setHandler("album:recentlyadded:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getRecentlyAddedAlbums(options);
  });
  App.reqres.setHandler("album:recentlyplayed:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getRecentlyPlayedAlbums(options);
  });
  return App.commands.setHandler("album:search:entities", function(query, limit, callback) {
    var collection;
    collection = API.getAlbums({});
    return App.execute("when:entity:fetched", collection, (function(_this) {
      return function() {
        var filtered;
        filtered = new App.Entities.Filtered(collection);
        filtered.filterByString('label', query);
        if (callback) {
          return callback(filtered);
        }
      };
    })(this));
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    getArtistFields: function(type) {
      var baseFields, extraFields, fields;
      if (type == null) {
        type = 'small';
      }
      baseFields = ['thumbnail', 'mood', 'genre', 'style'];
      extraFields = ['fanart', 'born', 'formed', 'description'];
      if (type === 'full') {
        fields = baseFields.concat(extraFields);
        return fields;
      } else {
        return baseFields;
      }
    },
    getArtist: function(id, options) {
      var artist;
      artist = new App.KodiEntities.Artist();
      artist.set({
        artistid: parseInt(id),
        properties: API.getArtistFields('full')
      });
      artist.fetch(options);
      return artist;
    },
    getArtists: function(options) {
      var artists, defaultOptions;
      defaultOptions = {
        cache: true,
        expires: config.get('static', 'collectionCacheExpiry')
      };
      options = _.extend(defaultOptions, options);
      artists = helpers.cache.get("artist:entities");
      if (artists === false || options.reset === true) {
        artists = new KodiEntities.ArtistCollection();
        artists.fetch(options);
      }
      helpers.cache.set("artist:entities", artists);
      return artists;
    }
  };
  KodiEntities.Artist = (function(_super) {
    __extends(Artist, _super);

    function Artist() {
      return Artist.__super__.constructor.apply(this, arguments);
    }

    Artist.prototype.defaults = function() {
      var fields;
      fields = _.extend(this.modelDefaults, {
        artistid: 1,
        artist: ''
      });
      return this.parseFieldsToDefaults(API.getArtistFields('full'), fields);
    };

    Artist.prototype.methods = {
      read: ['AudioLibrary.GetArtistDetails', 'artistid', 'properties']
    };

    Artist.prototype.arg2 = API.getArtistFields('full');

    Artist.prototype.parse = function(resp, xhr) {
      var obj;
      obj = resp.artistdetails != null ? resp.artistdetails : resp;
      if (resp.artistdetails != null) {
        obj.fullyloaded = true;
      }
      return this.parseModel('artist', obj, obj.artistid);
    };

    return Artist;

  })(App.KodiEntities.Model);
  KodiEntities.ArtistCollection = (function(_super) {
    __extends(ArtistCollection, _super);

    function ArtistCollection() {
      return ArtistCollection.__super__.constructor.apply(this, arguments);
    }

    ArtistCollection.prototype.model = KodiEntities.Artist;

    ArtistCollection.prototype.methods = {
      read: ['AudioLibrary.GetArtists', 'arg1', 'arg2', 'arg3', 'arg4']
    };

    ArtistCollection.prototype.arg1 = function() {
      return config.getLocal('albumAtristsOnly', true);
    };

    ArtistCollection.prototype.arg2 = function() {
      return API.getArtistFields('small');
    };

    ArtistCollection.prototype.arg3 = function() {
      return this.argLimit();
    };

    ArtistCollection.prototype.arg4 = function() {
      return this.argSort("artist", "ascending");
    };

    ArtistCollection.prototype.parse = function(resp, xhr) {
      return this.getResult(resp, 'artists');
    };

    return ArtistCollection;

  })(App.KodiEntities.Collection);
  App.reqres.setHandler("artist:entity", function(id, options) {
    if (options == null) {
      options = {};
    }
    return API.getArtist(id, options);
  });
  App.reqres.setHandler("artist:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getArtists(options);
  });
  return App.commands.setHandler("artist:search:entities", function(query, limit, callback) {
    var collection;
    collection = API.getArtists({});
    return App.execute("when:entity:fetched", collection, (function(_this) {
      return function() {
        var filtered;
        filtered = new App.Entities.Filtered(collection);
        filtered.filterByString('label', query);
        if (callback) {
          return callback(filtered);
        }
      };
    })(this));
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {

  /*
    API Helpers
   */
  var API;
  API = {
    fields: {
      minimal: ['name'],
      small: ['order', 'role', 'thumbnail', 'origin', 'url'],
      full: []
    },
    getCollection: function(cast, origin) {
      var collection, i, item;
      for (i in cast) {
        item = cast[i];
        cast[i].origin = origin;
      }
      collection = new KodiEntities.CastCollection(cast);
      return collection;
    }
  };

  /*
   Models and collections.
   */
  KodiEntities.Cast = (function(_super) {
    __extends(Cast, _super);

    function Cast() {
      return Cast.__super__.constructor.apply(this, arguments);
    }

    Cast.prototype.idAttribute = "order";

    Cast.prototype.defaults = function() {
      return this.parseFieldsToDefaults(helpers.entities.getFields(API.fields, 'small'), {});
    };

    Cast.prototype.parse = function(obj, xhr) {
      obj.url = '?cast=' + obj.name;
      return obj;
    };

    return Cast;

  })(App.KodiEntities.Model);
  KodiEntities.CastCollection = (function(_super) {
    __extends(CastCollection, _super);

    function CastCollection() {
      return CastCollection.__super__.constructor.apply(this, arguments);
    }

    CastCollection.prototype.model = KodiEntities.Cast;

    return CastCollection;

  })(App.KodiEntities.Collection);

  /*
   Request Handlers.
   */
  return App.reqres.setHandler("cast:entities", function(cast, origin) {
    return API.getCollection(cast, origin);
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {

  /*
    API Helpers
   */
  var API;
  API = {
    fields: {
      minimal: [],
      small: ['title', 'runtime', 'starttime', 'endtime', 'genre'],
      full: ['plot', 'progress', 'progresspercentage', 'episodename', 'episodenum', 'episodepart', 'firstaired', 'hastimer', 'isactive', 'parentalrating', 'wasactive', 'thumbnail']
    },

    /*getEntity: (collection, channel) ->
      collection.findWhere({channel: channel})
     */
    getEntity: function(channelid, options) {
      var entity;
      entity = new App.KodiEntities.Broadcast();
      entity.set({
        channelid: parseInt(channelid),
        properties: helpers.entities.getFields(API.fields, 'full')
      });
      entity.fetch(options);
      return entity;
    },
    getCollection: function(options) {
      var collection;
      collection = new KodiEntities.BroadcastCollection();
      collection.fetch(options);
      return collection;
    }
  };

  /*
   Models and collections.
   */
  KodiEntities.Broadcast = (function(_super) {
    __extends(Broadcast, _super);

    function Broadcast() {
      return Broadcast.__super__.constructor.apply(this, arguments);
    }

    Broadcast.prototype.defaults = function() {
      var fields;
      fields = _.extend(this.modelDefaults, {
        channelid: 1,
        channel: ''
      });
      return this.parseFieldsToDefaults(helpers.entities.getFields(API.fields, 'full'), fields);
    };

    Broadcast.prototype.methods = {
      read: ['PVR.GetBroadcasts', 'channelid', 'properties']
    };

    Broadcast.prototype.parse = function(resp, xhr) {
      var obj;
      obj = resp.broadcasts != null ? resp.broadcasts : resp;
      if (resp.broadcasts != null) {
        obj.fullyloaded = true;
      }
      return this.parseModel('broadcast', obj, obj.channelid);
    };


    /*defaults: ->
      @parseFieldsToDefaults helpers.entities.getFields(API.fields, 'full'), {}
    parse: (obj, xhr) ->
      obj.fullyloaded = true
      @parseModel 'broadcast', obj, obj.channelid
     */

    return Broadcast;

  })(App.KodiEntities.Model);
  KodiEntities.BroadcastCollection = (function(_super) {
    __extends(BroadcastCollection, _super);

    function BroadcastCollection() {
      return BroadcastCollection.__super__.constructor.apply(this, arguments);
    }

    BroadcastCollection.prototype.model = KodiEntities.Broadcast;

    BroadcastCollection.prototype.methods = {
      read: ['PVR.GetBroadcasts', 'arg1', 'arg2', 'arg3']
    };

    BroadcastCollection.prototype.arg1 = function() {
      return parseInt(this.argCheckOption('channelid', 0));
    };

    BroadcastCollection.prototype.arg2 = function() {
      return helpers.entities.getFields(API.fields, 'full');
    };

    BroadcastCollection.prototype.arg3 = function() {
      return this.argLimit();
    };

    BroadcastCollection.prototype.parse = function(resp, xhr) {
      return this.getResult(resp, 'broadcasts');
    };

    return BroadcastCollection;

  })(App.KodiEntities.Collection);

  /*
   Request Handlers.
   */
  App.reqres.setHandler("broadcast:entity", function(collection, channelid) {
    return API.getEntity(collection, channelid);
  });
  return App.reqres.setHandler("broadcast:entities", function(channelid, options) {
    if (options == null) {
      options = {};
    }
    options.channelid = channelid;
    return API.getCollection(options);
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {

  /*
    API Helpers
   */
  var API;
  API = {
    fields: {
      minimal: ['title'],
      small: ['thumbnail', 'playcount', 'lastplayed', 'dateadded', 'episode', 'season', 'rating', 'file', 'cast', 'showtitle', 'tvshowid', 'uniqueid', 'resume'],
      full: ['fanart', 'plot', 'firstaired', 'director', 'writer', 'runtime', 'streamdetails']
    },
    getEntity: function(id, options) {
      var entity;
      entity = new App.KodiEntities.Episode();
      entity.set({
        episodeid: parseInt(id),
        properties: helpers.entities.getFields(API.fields, 'full')
      });
      entity.fetch(options);
      return entity;
    },
    getCollection: function(options) {
      var collection, defaultOptions;
      defaultOptions = {
        cache: false,
        expires: config.get('static', 'collectionCacheExpiry')
      };
      options = _.extend(defaultOptions, options);
      collection = new KodiEntities.EpisodeCollection();
      collection.fetch(options);
      return collection;
    },
    getRecentlyAddedCollection: function(options) {
      var collection;
      collection = new KodiEntities.EpisodeRecentlyAddedCollection();
      collection.fetch(options);
      return collection;
    }
  };

  /*
   Models and collections.
   */
  KodiEntities.Episode = (function(_super) {
    __extends(Episode, _super);

    function Episode() {
      return Episode.__super__.constructor.apply(this, arguments);
    }

    Episode.prototype.defaults = function() {
      var fields;
      fields = _.extend(this.modelDefaults, {
        episodeid: 1,
        episode: ''
      });
      return this.parseFieldsToDefaults(helpers.entities.getFields(API.fields, 'full'), fields);
    };

    Episode.prototype.methods = {
      read: ['VideoLibrary.GetEpisodeDetails', 'episodeid', 'properties']
    };

    Episode.prototype.parse = function(resp, xhr) {
      var obj;
      obj = resp.episodedetails != null ? resp.episodedetails : resp;
      if (resp.episodedetails != null) {
        obj.fullyloaded = true;
      }
      obj.unwatched = obj.playcount > 0 ? 0 : 1;
      return this.parseModel('episode', obj, obj.episodeid);
    };

    return Episode;

  })(App.KodiEntities.Model);
  KodiEntities.EpisodeCollection = (function(_super) {
    __extends(EpisodeCollection, _super);

    function EpisodeCollection() {
      return EpisodeCollection.__super__.constructor.apply(this, arguments);
    }

    EpisodeCollection.prototype.model = KodiEntities.Episode;

    EpisodeCollection.prototype.methods = {
      read: ['VideoLibrary.GetEpisodes', 'arg1', 'arg2', 'arg3']
    };

    EpisodeCollection.prototype.arg1 = function() {
      return this.argCheckOption('tvshowid', 0);
    };

    EpisodeCollection.prototype.arg2 = function() {
      return this.argCheckOption('season', 0);
    };

    EpisodeCollection.prototype.arg3 = function() {
      return helpers.entities.getFields(API.fields, 'small');
    };

    EpisodeCollection.prototype.arg4 = function() {
      return this.argLimit();
    };

    EpisodeCollection.prototype.arg5 = function() {
      return this.argSort("episode", "ascending");
    };

    EpisodeCollection.prototype.parse = function(resp, xhr) {
      return this.getResult(resp, 'episodes');
    };

    return EpisodeCollection;

  })(App.KodiEntities.Collection);
  KodiEntities.EpisodeRecentlyAddedCollection = (function(_super) {
    __extends(EpisodeRecentlyAddedCollection, _super);

    function EpisodeRecentlyAddedCollection() {
      return EpisodeRecentlyAddedCollection.__super__.constructor.apply(this, arguments);
    }

    EpisodeRecentlyAddedCollection.prototype.model = KodiEntities.Episode;

    EpisodeRecentlyAddedCollection.prototype.methods = {
      read: ['VideoLibrary.GetRecentlyAddedEpisodes', 'arg1', 'arg2']
    };

    EpisodeRecentlyAddedCollection.prototype.arg1 = function() {
      return helpers.entities.getFields(API.fields, 'small');
    };

    EpisodeRecentlyAddedCollection.prototype.arg2 = function() {
      return this.argLimit();
    };

    EpisodeRecentlyAddedCollection.prototype.parse = function(resp, xhr) {
      return this.getResult(resp, 'episodes');
    };

    return EpisodeRecentlyAddedCollection;

  })(App.KodiEntities.Collection);

  /*
   Request Handlers.
   */
  App.reqres.setHandler("episode:entity", function(id, options) {
    if (options == null) {
      options = {};
    }
    return API.getEntity(id, options);
  });
  App.reqres.setHandler("episode:entities", function(tvshowid, season, options) {
    if (options == null) {
      options = {};
    }
    options.tvshowid = tvshowid;
    options.season = season;
    return API.getCollection(options);
  });
  return App.reqres.setHandler("episode:recentlyadded:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getRecentlyAddedCollection(options);
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {

  /*
    API Helpers
   */
  var API;
  API = {
    fields: {
      minimal: ['title', 'file', 'mimetype'],
      small: ['thumbnail'],
      full: ['fanart', 'streamdetails']
    },
    addonFields: ['path', 'name'],
    sources: [
      {
        media: 'video',
        label: 'Video',
        type: 'source',
        provides: 'video'
      }, {
        media: 'music',
        label: 'Music',
        type: 'source',
        provides: 'audio'
      }, {
        media: 'music',
        label: 'Audio add-ons',
        type: 'addon',
        provides: 'audio',
        addonType: 'xbmc.addon.audio',
        content: 'unknown'
      }, {
        media: 'video',
        label: 'Video add-ons',
        type: 'addon',
        provides: 'files',
        addonType: 'xbmc.addon.video',
        content: 'unknown'
      }
    ],
    directorySeperator: '/',
    getEntity: function(id, options) {
      var entity;
      entity = new App.KodiEntities.File();
      entity.set({
        file: id,
        properties: helpers.entities.getFields(API.fields, 'full')
      });
      entity.fetch(options);
      return entity;
    },
    getCollection: function(type, options) {
      var collection, defaultOptions;
      defaultOptions = {
        cache: true
      };
      options = _.extend(defaultOptions, options);
      if (type === 'sources') {
        collection = new KodiEntities.SourceCollection();
      } else {
        collection = new KodiEntities.FileCollection();
      }
      collection.fetch(options);
      return collection;
    },
    parseToFilesAndFolders: function(collection) {
      var all, collections;
      all = collection.getRawCollection();
      collections = {};
      collections.file = new KodiEntities.FileCustomCollection(_.where(all, {
        filetype: 'file'
      }));
      collections.directory = new KodiEntities.FileCustomCollection(_.where(all, {
        filetype: 'directory'
      }));
      return collections;
    },
    getSources: function() {
      var collection, commander, commands, source, _i, _len, _ref;
      commander = App.request("command:kodi:controller", 'auto', 'Commander');
      commands = [];
      collection = new KodiEntities.SourceCollection();
      _ref = this.sources;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        source = _ref[_i];
        if (source.type === 'source') {
          commands.push({
            method: 'Files.GetSources',
            params: [source.media]
          });
        }
        if (source.type === 'addon') {
          commands.push({
            method: 'Addons.GetAddons',
            params: [source.addonType, source.content, true, this.addonFields]
          });
        }
      }
      commander.multipleCommands(commands, (function(_this) {
        return function(resp) {
          var i, item, model, repsonseKey, _j, _len1, _ref1;
          for (i in resp) {
            item = resp[i];
            source = _this.sources[i];
            repsonseKey = source.type + 's';
            if (item[repsonseKey]) {
              _ref1 = item[repsonseKey];
              for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
                model = _ref1[_j];
                model.media = source.media;
                model.sourcetype = source.type;
                if (source.type === 'addon') {
                  model.file = _this.createAddonFile(model);
                  model.label = model.name;
                }
                model.url = _this.createFileUrl(source.media, model.file);
                collection.add(model);
              }
            }
          }
          return collection.trigger('cachesync');
        };
      })(this));
      return collection;
    },
    parseSourceCollection: function(collection) {
      var all, items, source, _i, _len, _ref;
      all = collection.getRawCollection();
      collection = [];
      _ref = this.sources;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        source = _ref[_i];
        items = _.where(all, {
          media: source.media
        });
        if (items.length > 0 && source.type === 'source') {
          source.sources = new KodiEntities.SourceCollection(items);
          source.url = 'browser/' + source.media;
          collection.push(source);
        }
      }
      return new KodiEntities.SourceSetCollection(collection);
    },
    createFileUrl: function(media, file) {
      return 'browser/' + media + '/' + encodeURIComponent(file);
    },
    createAddonFile: function(addon) {
      return 'plugin://' + addon.addonid;
    },
    parseFiles: function(items, media) {
      var i, item;
      for (i in items) {
        item = items[i];
        if (!item.parsed) {
          item = App.request("images:path:entity", item);
          items[i] = this.correctFileType(item);
          items[i].media = media;
          items[i].player = this.getPlayer(media);
          items[i].url = this.createFileUrl(media, item.file);
          items[i].parsed = true;
        }
      }
      return items;
    },
    correctFileType: function(item) {
      var directoryMimeTypes;
      directoryMimeTypes = ['x-directory/normal'];
      if (item.mimetype && helpers.global.inArray(item.mimetype, directoryMimeTypes)) {
        item.filetype = 'directory';
      }
      return item;
    },
    createPathCollection: function(file, sourcesCollection) {
      var allSources, basePath, items, parentSource, part, pathParts, source, _i, _j, _len, _len1;
      items = [];
      parentSource = {};
      allSources = sourcesCollection.getRawCollection();
      for (_i = 0, _len = allSources.length; _i < _len; _i++) {
        source = allSources[_i];
        if (parentSource.file) {
          continue;
        }
        if (helpers.global.stringStartsWith(source.file, file)) {
          parentSource = source;
        }
      }
      if (parentSource.file) {
        items.push(parentSource);
        basePath = parentSource.file;
        pathParts = helpers.global.stringStripStartsWith(parentSource.file, file).split(this.directorySeperator);
        for (_j = 0, _len1 = pathParts.length; _j < _len1; _j++) {
          part = pathParts[_j];
          if (part !== '') {
            basePath += part + this.directorySeperator;
            items.push(this.createPathModel(parentSource.media, part, basePath));
          }
        }
      }
      return new KodiEntities.FileCustomCollection(items);
    },
    createPathModel: function(media, label, file) {
      var model;
      model = {
        label: label,
        file: file,
        media: media,
        url: this.createFileUrl(media, file)
      };
      return model;
    },
    getPlayer: function(media) {
      if (media === 'music') {
        'audio';
      }
      return media;
    }
  };

  /*
   Models and collections.
   */
  KodiEntities.EmptyFile = (function(_super) {
    __extends(EmptyFile, _super);

    function EmptyFile() {
      return EmptyFile.__super__.constructor.apply(this, arguments);
    }

    EmptyFile.prototype.idAttribute = "file";

    EmptyFile.prototype.defaults = function() {
      var fields;
      fields = _.extend(this.modelDefaults, {
        filetype: 'directory',
        media: '',
        label: '',
        url: ''
      });
      return this.parseFieldsToDefaults(helpers.entities.getFields(API.fields, 'full'), fields);
    };

    return EmptyFile;

  })(App.KodiEntities.Model);
  KodiEntities.File = (function(_super) {
    __extends(File, _super);

    function File() {
      return File.__super__.constructor.apply(this, arguments);
    }

    File.prototype.methods = {
      read: ['Files.GetFileDetails', 'file', 'properties']
    };

    File.prototype.parse = function(resp, xhr) {
      var obj;
      obj = resp.filedetails != null ? resp.filedetails : resp;
      if (resp.filedetails != null) {
        obj.fullyloaded = true;
      }
      return obj;
    };

    return File;

  })(KodiEntities.EmptyFile);
  KodiEntities.FileCollection = (function(_super) {
    __extends(FileCollection, _super);

    function FileCollection() {
      return FileCollection.__super__.constructor.apply(this, arguments);
    }

    FileCollection.prototype.model = KodiEntities.File;

    FileCollection.prototype.methods = {
      read: ['Files.GetDirectory', 'arg1', 'arg2', 'arg3', 'arg4']
    };

    FileCollection.prototype.arg1 = function() {
      return this.argCheckOption('file', '');
    };

    FileCollection.prototype.arg2 = function() {
      return this.argCheckOption('media', '');
    };

    FileCollection.prototype.arg3 = function() {
      return helpers.entities.getFields(API.fields, 'small');
    };

    FileCollection.prototype.arg4 = function() {
      return this.argSort("label", "ascending");
    };

    FileCollection.prototype.parse = function(resp, xhr) {
      var items;
      items = this.getResult(resp, 'files');
      return API.parseFiles(items, this.options.media);
    };

    return FileCollection;

  })(App.KodiEntities.Collection);
  KodiEntities.FileCustomCollection = (function(_super) {
    __extends(FileCustomCollection, _super);

    function FileCustomCollection() {
      return FileCustomCollection.__super__.constructor.apply(this, arguments);
    }

    FileCustomCollection.prototype.model = KodiEntities.File;

    return FileCustomCollection;

  })(App.KodiEntities.Collection);
  KodiEntities.Source = (function(_super) {
    __extends(Source, _super);

    function Source() {
      return Source.__super__.constructor.apply(this, arguments);
    }

    Source.prototype.idAttribute = "file";

    Source.prototype.defaults = {
      label: '',
      file: '',
      media: '',
      url: ''
    };

    return Source;

  })(App.KodiEntities.Model);
  KodiEntities.SourceCollection = (function(_super) {
    __extends(SourceCollection, _super);

    function SourceCollection() {
      return SourceCollection.__super__.constructor.apply(this, arguments);
    }

    SourceCollection.prototype.model = KodiEntities.Source;

    return SourceCollection;

  })(App.KodiEntities.Collection);
  KodiEntities.SourceSet = (function(_super) {
    __extends(SourceSet, _super);

    function SourceSet() {
      return SourceSet.__super__.constructor.apply(this, arguments);
    }

    SourceSet.prototype.idAttribute = "file";

    SourceSet.prototype.defaults = {
      label: '',
      sources: ''
    };

    return SourceSet;

  })(App.KodiEntities.Model);
  KodiEntities.SourceSetCollection = (function(_super) {
    __extends(SourceSetCollection, _super);

    function SourceSetCollection() {
      return SourceSetCollection.__super__.constructor.apply(this, arguments);
    }

    SourceSetCollection.prototype.model = KodiEntities.Source;

    return SourceSetCollection;

  })(App.KodiEntities.Collection);

  /*
   Request Handlers.
   */
  App.reqres.setHandler("file:entity", function(id, options) {
    if (options == null) {
      options = {};
    }
    return API.getEntity(id, options);
  });
  App.reqres.setHandler("file:url:entity", function(media, hash) {
    var file;
    file = decodeURIComponent(hash);
    return new KodiEntities.EmptyFile({
      media: media,
      file: file,
      url: API.createFileUrl(media, file)
    });
  });
  App.reqres.setHandler("file:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getCollection('files', options);
  });
  App.reqres.setHandler("file:path:entities", function(file, sourceCollection) {
    return API.createPathCollection(file, sourceCollection);
  });
  App.reqres.setHandler("file:parsed:entities", function(collection) {
    return API.parseToFilesAndFolders(collection);
  });
  App.reqres.setHandler("file:source:entities", function(media) {
    return API.getSources();
  });
  App.reqres.setHandler("file:source:media:entities", function(collection) {
    return API.parseSourceCollection(collection);
  });
  return App.reqres.setHandler("file:source:mediatypes", function() {
    return API.availableSources;
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {

  /*
    API Helpers
   */
  var API;
  API = {
    fields: {
      minimal: ['title'],
      small: ['thumbnail', 'playcount', 'lastplayed', 'dateadded', 'resume', 'rating', 'year', 'file', 'genre', 'writer', 'director', 'cast', 'set'],
      full: ['fanart', 'plotoutline', 'studio', 'mpaa', 'imdbnumber', 'runtime', 'streamdetails', 'plot', 'trailer']
    },
    getEntity: function(id, options) {
      var entity;
      entity = new App.KodiEntities.Movie();
      entity.set({
        movieid: parseInt(id),
        properties: helpers.entities.getFields(API.fields, 'full')
      });
      entity.fetch(options);
      return entity;
    },
    getCollection: function(options) {
      var collection, defaultOptions;
      defaultOptions = {
        cache: true,
        expires: config.get('static', 'collectionCacheExpiry')
      };
      options = _.extend(defaultOptions, options);
      collection = new KodiEntities.MovieCollection();
      collection.fetch(options);
      return collection;
    },
    getRecentlyAddedCollection: function(options) {
      var collection;
      collection = new KodiEntities.MovieRecentlyAddedCollection();
      collection.fetch(options);
      return collection;
    },
    getFilteredCollection: function(options) {
      var collection;
      collection = new KodiEntities.MovieFilteredCollection();
      collection.fetch(options);
      return collection;
    }
  };

  /*
   Models and collections.
   */
  KodiEntities.Movie = (function(_super) {
    __extends(Movie, _super);

    function Movie() {
      return Movie.__super__.constructor.apply(this, arguments);
    }

    Movie.prototype.defaults = function() {
      var fields;
      fields = _.extend(this.modelDefaults, {
        movieid: 1,
        movie: ''
      });
      return this.parseFieldsToDefaults(helpers.entities.getFields(API.fields, 'full'), fields);
    };

    Movie.prototype.methods = {
      read: ['VideoLibrary.GetMovieDetails', 'movieid', 'properties']
    };

    Movie.prototype.parse = function(resp, xhr) {
      var obj;
      obj = resp.moviedetails != null ? resp.moviedetails : resp;
      if (resp.moviedetails != null) {
        obj.fullyloaded = true;
      }
      obj.unwatched = obj.playcount > 0 ? 0 : 1;
      return this.parseModel('movie', obj, obj.movieid);
    };

    return Movie;

  })(App.KodiEntities.Model);
  KodiEntities.MovieCollection = (function(_super) {
    __extends(MovieCollection, _super);

    function MovieCollection() {
      return MovieCollection.__super__.constructor.apply(this, arguments);
    }

    MovieCollection.prototype.model = KodiEntities.Movie;

    MovieCollection.prototype.methods = {
      read: ['VideoLibrary.GetMovies', 'arg1', 'arg2', 'arg3']
    };

    MovieCollection.prototype.arg1 = function() {
      return helpers.entities.getFields(API.fields, 'small');
    };

    MovieCollection.prototype.arg2 = function() {
      return this.argLimit();
    };

    MovieCollection.prototype.arg3 = function() {
      return this.argSort("title", "ascending");
    };

    MovieCollection.prototype.parse = function(resp, xhr) {
      return this.getResult(resp, 'movies');
    };

    return MovieCollection;

  })(App.KodiEntities.Collection);
  KodiEntities.MovieRecentlyAddedCollection = (function(_super) {
    __extends(MovieRecentlyAddedCollection, _super);

    function MovieRecentlyAddedCollection() {
      return MovieRecentlyAddedCollection.__super__.constructor.apply(this, arguments);
    }

    MovieRecentlyAddedCollection.prototype.model = KodiEntities.Movie;

    MovieRecentlyAddedCollection.prototype.methods = {
      read: ['VideoLibrary.GetRecentlyAddedMovies', 'arg1', 'arg2']
    };

    MovieRecentlyAddedCollection.prototype.arg1 = function() {
      return helpers.entities.getFields(API.fields, 'small');
    };

    MovieRecentlyAddedCollection.prototype.arg2 = function() {
      return this.argLimit();
    };

    MovieRecentlyAddedCollection.prototype.parse = function(resp, xhr) {
      return this.getResult(resp, 'movies');
    };

    return MovieRecentlyAddedCollection;

  })(App.KodiEntities.Collection);
  KodiEntities.MovieFilteredCollection = (function(_super) {
    __extends(MovieFilteredCollection, _super);

    function MovieFilteredCollection() {
      return MovieFilteredCollection.__super__.constructor.apply(this, arguments);
    }

    MovieFilteredCollection.prototype.methods = {
      read: ['VideoLibrary.GetMovies', 'arg1', 'arg2', 'arg3', 'arg4']
    };

    MovieFilteredCollection.prototype.arg4 = function() {
      return this.argFilter();
    };

    return MovieFilteredCollection;

  })(KodiEntities.MovieCollection);

  /*
   Request Handlers.
   */
  App.reqres.setHandler("movie:entity", function(id, options) {
    if (options == null) {
      options = {};
    }
    return API.getEntity(id, options);
  });
  App.reqres.setHandler("movie:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getCollection(options);
  });
  App.reqres.setHandler("movie:filtered:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getFilteredCollection(options);
  });
  App.reqres.setHandler("movie:recentlyadded:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getRecentlyAddedCollection(options);
  });
  return App.commands.setHandler("movie:search:entities", function(query, limit, callback) {
    var collection;
    collection = API.getCollection({});
    return App.execute("when:entity:fetched", collection, (function(_this) {
      return function() {
        var filtered;
        filtered = new App.Entities.Filtered(collection);
        filtered.filterByString('label', query);
        if (callback) {
          return callback(filtered);
        }
      };
    })(this));
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {

  /*
    API Helpers
   */
  var API;
  API = {
    fields: {
      minimal: ['title', 'thumbnail', 'file'],
      small: ['artist', 'genre', 'year', 'rating', 'album', 'track', 'duration', 'playcount', 'dateadded', 'episode', 'artistid', 'albumid', 'tvshowid'],
      full: ['fanart']
    },
    getCollection: function(options) {
      var collection, defaultOptions;
      defaultOptions = {
        cache: false
      };
      options = _.extend(defaultOptions, options);
      collection = new KodiEntities.PlaylistCollection();
      collection.fetch(options);
      return collection;
    },
    getType: function(item, media) {
      var type;
      type = 'file';
      if (item.id !== void 0 && item.id !== '') {
        if (media === 'audio') {
          type = 'song';
        } else if (media === 'video') {
          if (item.episode !== '') {
            type = 'episode';
          } else {
            type = 'movie';
          }
        }
      }
      return type;
    },
    parseItems: function(items, options) {
      var i, item;
      for (i in items) {
        item = items[i];
        item.position = parseInt(i);
        items[i] = this.parseItem(item, options);
      }
      return items;
    },
    parseItem: function(item, options) {
      item.playlistid = options.playlistid;
      item.media = options.media;
      item.player = 'kodi';
      if (!item.type || item.type === 'unknown') {
        item.type = API.getType(item, options.media);
      }
      if (item.type === 'file') {
        item.id = item.file;
      }
      item.uid = helpers.entities.createUid(item);
      return item;
    }
  };

  /*
   Models and collections.
   */
  KodiEntities.PlaylistItem = (function(_super) {
    __extends(PlaylistItem, _super);

    function PlaylistItem() {
      return PlaylistItem.__super__.constructor.apply(this, arguments);
    }

    PlaylistItem.prototype.idAttribute = "position";

    PlaylistItem.prototype.defaults = function() {
      var fields;
      fields = _.extend(this.modelDefaults, {
        position: 0
      });
      return this.parseFieldsToDefaults(helpers.entities.getFields(API.fields, 'full'), fields);
    };

    PlaylistItem.prototype.parse = function(resp, xhr) {
      var model;
      resp.fullyloaded = true;
      model = this.parseModel(resp.type, resp, resp.id);
      model.url = helpers.url.playlistUrl(model);
      return model;
    };

    return PlaylistItem;

  })(App.KodiEntities.Model);
  KodiEntities.PlaylistCollection = (function(_super) {
    __extends(PlaylistCollection, _super);

    function PlaylistCollection() {
      return PlaylistCollection.__super__.constructor.apply(this, arguments);
    }

    PlaylistCollection.prototype.model = KodiEntities.PlaylistItem;

    PlaylistCollection.prototype.methods = {
      read: ['Playlist.GetItems', 'arg1', 'arg2', 'arg3']
    };

    PlaylistCollection.prototype.arg1 = function() {
      return this.argCheckOption('playlistid', 0);
    };

    PlaylistCollection.prototype.arg2 = function() {
      return helpers.entities.getFields(API.fields, 'small');
    };

    PlaylistCollection.prototype.arg3 = function() {
      return this.argLimit();
    };

    PlaylistCollection.prototype.arg4 = function() {
      return this.argSort("position", "ascending");
    };

    PlaylistCollection.prototype.parse = function(resp, xhr) {
      var items;
      items = this.getResult(resp, 'items');
      return API.parseItems(items, this.options);
    };

    return PlaylistCollection;

  })(App.KodiEntities.Collection);

  /*
   Request Handlers.
   */
  App.reqres.setHandler("playlist:kodi:entities", function(media) {
    var collection, options, playlist;
    if (media == null) {
      media = 'audio';
    }
    playlist = App.request("command:kodi:controller", media, 'PlayList');
    options = {};
    options.media = media;
    options.playlistid = playlist.getPlayer();
    collection = API.getCollection(options);
    collection.sortCollection('position', 'asc');
    return collection;
  });
  return App.reqres.setHandler("playlist:kodi:entity:api", function() {
    return API;
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {

  /*
    API Helpers
   */
  var API;
  API = {
    fields: {
      minimal: ['thumbnail'],
      small: ['channeltype', 'hidden', 'locked', 'channel', 'lastplayed', 'broadcastnow'],
      full: []
    },
    getEntity: function(collection, channel) {
      return collection.findWhere({
        channel: channel
      });
    },
    getCollection: function(options) {
      var collection;
      collection = new KodiEntities.ChannelCollection();
      collection.fetch(options);
      return collection;
    }
  };

  /*
   Models and collections.
   */
  KodiEntities.Channel = (function(_super) {
    __extends(Channel, _super);

    function Channel() {
      return Channel.__super__.constructor.apply(this, arguments);
    }

    Channel.prototype.defaults = function() {
      return this.parseFieldsToDefaults(helpers.entities.getFields(API.fields, 'full'), {});
    };

    Channel.prototype.parse = function(obj, xhr) {
      obj.fullyloaded = true;
      return this.parseModel('channel', obj, obj.channelid);
    };

    return Channel;

  })(App.KodiEntities.Model);
  KodiEntities.ChannelCollection = (function(_super) {
    __extends(ChannelCollection, _super);

    function ChannelCollection() {
      return ChannelCollection.__super__.constructor.apply(this, arguments);
    }

    ChannelCollection.prototype.model = KodiEntities.Channel;

    ChannelCollection.prototype.methods = {
      read: ['PVR.GetChannels', 'arg1', 'arg2', 'arg3']
    };

    ChannelCollection.prototype.arg1 = function() {
      return this.argCheckOption('group', 0);
    };

    ChannelCollection.prototype.arg2 = function() {
      return helpers.entities.getFields(API.fields, 'small');
    };

    ChannelCollection.prototype.arg3 = function() {
      return this.argLimit();
    };

    ChannelCollection.prototype.parse = function(resp, xhr) {
      return this.getResult(resp, 'channels');
    };

    return ChannelCollection;

  })(App.KodiEntities.Collection);

  /*
   Request Handlers.
   */
  App.reqres.setHandler("channel:entity", function(collection, channel) {
    return API.getEntity(collection, channel);
  });
  return App.reqres.setHandler("channel:entities", function(group, options) {
    if (group == null) {
      group = 'alltv';
    }
    if (options == null) {
      options = {};
    }
    options.group = group;
    return API.getCollection(options);
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {

  /*
    API Helpers
   */
  var API;
  API = {
    fields: {
      minimal: ['season'],
      small: ['showtitle', 'playcount', 'thumbnail', 'tvshowid', 'episode', 'watchedepisodes', 'fanart'],
      full: []
    },
    getEntity: function(collection, season) {
      return collection.findWhere({
        season: season
      });
    },
    getCollection: function(options) {
      var collection, defaultOptions;
      defaultOptions = {
        cache: false,
        expires: config.get('static', 'collectionCacheExpiry')
      };
      options = _.extend(defaultOptions, options);
      collection = new KodiEntities.SeasonCollection();
      collection.fetch(options);
      return collection;
    }
  };

  /*
   Models and collections.
   */
  KodiEntities.Season = (function(_super) {
    __extends(Season, _super);

    function Season() {
      return Season.__super__.constructor.apply(this, arguments);
    }

    Season.prototype.defaults = function() {
      var fields;
      fields = _.extend(this.modelDefaults, {
        seasonid: 1,
        season: ''
      });
      return this.parseFieldsToDefaults(helpers.entities.getFields(API.fields, 'full'), fields);
    };

    Season.prototype.parse = function(resp, xhr) {
      var obj;
      obj = resp.seasondetails != null ? resp.seasondetails : resp;
      if (resp.seasondetails != null) {
        obj.fullyloaded = true;
      }
      obj.unwatched = obj.episode - obj.watchedepisodes;
      return this.parseModel('season', obj, obj.tvshowid + '/' + obj.season);
    };

    return Season;

  })(App.KodiEntities.Model);
  KodiEntities.SeasonCollection = (function(_super) {
    __extends(SeasonCollection, _super);

    function SeasonCollection() {
      return SeasonCollection.__super__.constructor.apply(this, arguments);
    }

    SeasonCollection.prototype.model = KodiEntities.Season;

    SeasonCollection.prototype.methods = {
      read: ['VideoLibrary.GetSeasons', 'arg1', 'arg2', 'arg3', 'arg4']
    };

    SeasonCollection.prototype.arg1 = function() {
      return this.argCheckOption('tvshowid', 0);
    };

    SeasonCollection.prototype.arg2 = function() {
      return helpers.entities.getFields(API.fields, 'small');
    };

    SeasonCollection.prototype.arg3 = function() {
      return this.argLimit();
    };

    SeasonCollection.prototype.arg4 = function() {
      return this.argSort("season", "ascending");
    };

    SeasonCollection.prototype.parse = function(resp, xhr) {
      return this.getResult(resp, 'seasons');
    };

    return SeasonCollection;

  })(App.KodiEntities.Collection);

  /*
   Request Handlers.
   */
  App.reqres.setHandler("season:entity", function(collection, season) {
    return API.getEntity(collection, season);
  });
  return App.reqres.setHandler("season:entities", function(tvshowid, options) {
    if (options == null) {
      options = {};
    }
    options.tvshowid = tvshowid;
    return API.getCollection(options);
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {

  /*
    API Helpers
   */
  var API;
  API = {
    settingsType: {
      sections: "SettingSectionCollection",
      categories: "SettingCategoryCollection",
      settings: "SettingCollection"
    },
    ignoreKeys: ['weather'],
    fields: {
      minimal: ['settingstype'],
      small: ['title', 'control', 'options', 'parent', 'enabled', 'type', 'value', 'enabled', 'default', 'help', 'path', 'description', 'section', 'category'],
      full: []
    },
    getSettingsLevel: function() {
      return config.getLocal('kodiSettingsLevel', 'standard');
    },
    getEntity: function(id, collection) {
      var model;
      model = collection.where({
        method: id
      }).shift();
      return model;
    },
    getCollection: function(options) {
      var collection, collectionMethod;
      if (options == null) {
        options = {
          type: 'sections'
        };
      }
      collectionMethod = this.settingsType[options.type];
      collection = new KodiEntities[collectionMethod]();
      collection.fetch(options);
      if (options.section && options.type === 'settings') {
        collection.where({
          section: options.section
        });
      }
      return collection;
    },
    getSettings: function(section, categories, callback) {
      var commander, commands, items;
      if (categories == null) {
        categories = [];
      }
      commander = App.request("command:kodi:controller", 'auto', 'Commander');
      commands = [];
      items = [];
      $(categories).each((function(_this) {
        return function(i, category) {
          return commands.push({
            method: 'Settings.GetSettings',
            params: [
              _this.getSettingsLevel(), {
                "section": section,
                "category": category
              }
            ]
          });
        };
      })(this));
      return commander.multipleCommands(commands, (function(_this) {
        return function(resp) {
          var catId, i, item;
          for (i in resp) {
            item = resp[i];
            catId = categories[i];
            items[catId] = _this.parseCollection(item.settings, 'settings');
          }
          return callback(items);
        };
      })(this));
    },
    parseCollection: function(itemsRaw, type) {
      var item, items, method;
      if (itemsRaw == null) {
        itemsRaw = [];
      }
      if (type == null) {
        type = 'settings';
      }
      items = [];
      for (method in itemsRaw) {
        item = itemsRaw[method];
        if (_.lastIndexOf(this.ignoreKeys, item.id) === -1) {
          items.push(this.parseItem(item, type));
        }
      }
      return items;
    },
    parseItem: function(item, type) {
      if (type == null) {
        type = 'settings';
      }
      item.settingstype = type;
      item.title = item.label;
      item.description = item.help;
      item.path = 'settings/kodi/' + item.id;
      return item;
    },
    saveSettings: function(data, callback) {
      var commander, commands, key, val;
      commander = App.request("command:kodi:controller", 'auto', 'Commander');
      commands = [];
      for (key in data) {
        val = data[key];
        commands.push({
          method: 'Settings.SetSettingValue',
          params: [key, this.valuePreSave(val)]
        });
      }
      return commander.multipleCommands(commands, (function(_this) {
        return function(resp) {
          if (callback) {
            return callback(resp);
          }
        };
      })(this));
    },
    valuePreSave: function(val) {
      if (val === String(parseInt(val))) {
        val = parseInt(val);
      }
      return val;
    }
  };

  /*
   Models and collections.
   */
  KodiEntities.Setting = (function(_super) {
    __extends(Setting, _super);

    function Setting() {
      return Setting.__super__.constructor.apply(this, arguments);
    }

    Setting.prototype.defaults = function() {
      var fields;
      fields = _.extend(this.modelDefaults, {
        id: 0,
        params: {}
      });
      return this.parseFieldsToDefaults(helpers.entities.getFields(API.fields, 'small'), fields);
    };

    return Setting;

  })(App.KodiEntities.Model);
  KodiEntities.SettingSectionCollection = (function(_super) {
    __extends(SettingSectionCollection, _super);

    function SettingSectionCollection() {
      return SettingSectionCollection.__super__.constructor.apply(this, arguments);
    }

    SettingSectionCollection.prototype.model = KodiEntities.Setting;

    SettingSectionCollection.prototype.methods = {
      read: ['Settings.GetSections']
    };

    SettingSectionCollection.prototype.parse = function(resp, xhr) {
      var items;
      items = this.getResult(resp, this.options.type);
      return API.parseCollection(items, this.options.type);
    };

    return SettingSectionCollection;

  })(App.KodiEntities.Collection);
  KodiEntities.SettingCategoryCollection = (function(_super) {
    __extends(SettingCategoryCollection, _super);

    function SettingCategoryCollection() {
      return SettingCategoryCollection.__super__.constructor.apply(this, arguments);
    }

    SettingCategoryCollection.prototype.model = KodiEntities.Setting;

    SettingCategoryCollection.prototype.methods = {
      read: ['Settings.GetCategories', 'arg1', 'arg2']
    };

    SettingCategoryCollection.prototype.arg1 = function() {
      return API.getSettingsLevel();
    };

    SettingCategoryCollection.prototype.arg2 = function() {
      return this.argCheckOption('section', 0);
    };

    SettingCategoryCollection.prototype.parse = function(resp, xhr) {
      var items;
      items = this.getResult(resp, this.options.type);
      return API.parseCollection(items, this.options.type);
    };

    return SettingCategoryCollection;

  })(App.KodiEntities.Collection);
  KodiEntities.SettingCollection = (function(_super) {
    __extends(SettingCollection, _super);

    function SettingCollection() {
      return SettingCollection.__super__.constructor.apply(this, arguments);
    }

    SettingCollection.prototype.model = KodiEntities.Setting;

    SettingCollection.prototype.methods = {
      read: ['Settings.GetSettings', 'arg1']
    };

    SettingCollection.prototype.arg1 = function() {
      return API.getSettingsLevel();
    };

    SettingCollection.prototype.parse = function(resp, xhr) {
      var items;
      items = this.getResult(resp, this.options.type);
      return API.parseCollection(items, this.options.type);
    };

    return SettingCollection;

  })(App.KodiEntities.Collection);

  /*
   Request Handlers.
   */
  App.reqres.setHandler("settings:kodi:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getCollection(options);
  });
  App.reqres.setHandler("settings:kodi:filtered:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getSettings(options.section, options.categories, function(items) {
      return options.callback(items);
    });
  });
  return App.commands.setHandler("settings:kodi:save:entities", function(data, callback) {
    if (data == null) {
      data = {};
    }
    return API.saveSettings(data, callback);
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    songsByIdMax: 50,
    fields: {
      minimal: ['title', 'file'],
      small: ['thumbnail', 'artist', 'artistid', 'album', 'albumid', 'lastplayed', 'track', 'year', 'duration'],
      full: ['fanart', 'genre', 'style', 'mood', 'born', 'formed', 'description', 'lyrics']
    },
    getSong: function(id, options) {
      var artist;
      artist = new App.KodiEntities.Song();
      artist.set({
        songid: parseInt(id),
        properties: helpers.entities.getFields(API.fields, 'full')
      });
      artist.fetch(options);
      return artist;
    },
    getFilteredSongs: function(options) {
      var defaultOptions, songs;
      defaultOptions = {
        cache: true
      };
      options = _.extend(defaultOptions, options);
      if (options.indexOnly) {
        options.expires = config.getLocal('searchIndexCacheExpiry', 86400);
        songs = new KodiEntities.SongSearchIndexCollection();
      } else {
        songs = new KodiEntities.SongFilteredCollection();
      }
      songs.fetch(options);
      return songs;
    },
    parseSongsToAlbumSongs: function(songs) {
      var albumid, collections, parsedRaw, song, songSet, songsRaw, _i, _len;
      songsRaw = songs.getRawCollection();
      parsedRaw = {};
      collections = {};
      for (_i = 0, _len = songsRaw.length; _i < _len; _i++) {
        song = songsRaw[_i];
        if (!parsedRaw[song.albumid]) {
          parsedRaw[song.albumid] = [];
        }
        parsedRaw[song.albumid].push(song);
      }
      for (albumid in parsedRaw) {
        songSet = parsedRaw[albumid];
        collections[albumid] = new KodiEntities.SongCustomCollection(songSet);
      }
      return collections;
    },
    getSongsByIds: function(songIds, max, callback) {
      var cache, cacheKey, collection, commander, commands, id, items, model, _i, _len;
      if (songIds == null) {
        songIds = [];
      }
      if (max == null) {
        max = -1;
      }
      commander = App.request("command:kodi:controller", 'auto', 'Commander');
      songIds = this.getLimitIds(songIds, max);
      cacheKey = 'songs-' + songIds.join('-');
      items = [];
      cache = helpers.cache.get(cacheKey, false);
      if (cache) {
        collection = new KodiEntities.SongCustomCollection(cache);
        if (callback) {
          callback(collection);
        }
      } else {
        model = new KodiEntities.Song();
        commands = [];
        for (_i = 0, _len = songIds.length; _i < _len; _i++) {
          id = songIds[_i];
          commands.push({
            method: 'AudioLibrary.GetSongDetails',
            params: [id, helpers.entities.getFields(API.fields, 'small')]
          });
        }
        if (commands.length > 0) {
          commander.multipleCommands(commands, (function(_this) {
            return function(resp) {
              var item, _j, _len1, _ref;
              _ref = _.flatten([resp]);
              for (_j = 0, _len1 = _ref.length; _j < _len1; _j++) {
                item = _ref[_j];
                items.push(model.parseModel('song', item.songdetails, item.songdetails.songid));
              }
              helpers.cache.set(cacheKey, items);
              collection = new KodiEntities.SongCustomCollection(items);
              if (callback) {
                return callback(collection);
              }
            };
          })(this));
        }
      }
      return collection;
    },
    getLimitIds: function(ids, max) {
      var i, id, ret;
      max = max === -1 ? this.songsByIdMax : max;
      ret = [];
      for (i in ids) {
        id = ids[i];
        if (i < max) {
          ret.push(id);
        }
      }
      return ret;
    }
  };
  KodiEntities.Song = (function(_super) {
    __extends(Song, _super);

    function Song() {
      return Song.__super__.constructor.apply(this, arguments);
    }

    Song.prototype.defaults = function() {
      var fields;
      fields = _.extend(this.modelDefaults, {
        songid: 1,
        artist: ''
      });
      return this.parseFieldsToDefaults(helpers.entities.getFields(API.fields, 'full'), fields);
    };

    Song.prototype.methods = {
      read: ['AudioLibrary.GetSongDetails', 'songidid', 'properties']
    };

    Song.prototype.parse = function(resp, xhr) {
      var obj;
      obj = resp.songdetails != null ? resp.songdetails : resp;
      if (resp.songdetails != null) {
        obj.fullyloaded = true;
      }
      return this.parseModel('song', obj, obj.songid);
    };

    return Song;

  })(App.KodiEntities.Model);
  KodiEntities.SongFilteredCollection = (function(_super) {
    __extends(SongFilteredCollection, _super);

    function SongFilteredCollection() {
      return SongFilteredCollection.__super__.constructor.apply(this, arguments);
    }

    SongFilteredCollection.prototype.model = KodiEntities.Song;

    SongFilteredCollection.prototype.methods = {
      read: ['AudioLibrary.GetSongs', 'arg1', 'arg2', 'arg3', 'arg4']
    };

    SongFilteredCollection.prototype.arg1 = function() {
      return helpers.entities.getFields(API.fields, 'small');
    };

    SongFilteredCollection.prototype.arg2 = function() {
      return this.argLimit();
    };

    SongFilteredCollection.prototype.arg3 = function() {
      return this.argSort("track", "ascending");
    };

    SongFilteredCollection.prototype.arg4 = function() {
      return this.argFilter();
    };

    SongFilteredCollection.prototype.parse = function(resp, xhr) {
      return this.getResult(resp, 'songs');
    };

    return SongFilteredCollection;

  })(App.KodiEntities.Collection);
  KodiEntities.SongCustomCollection = (function(_super) {
    __extends(SongCustomCollection, _super);

    function SongCustomCollection() {
      return SongCustomCollection.__super__.constructor.apply(this, arguments);
    }

    SongCustomCollection.prototype.model = KodiEntities.Song;

    return SongCustomCollection;

  })(App.KodiEntities.Collection);
  KodiEntities.SongSearchIndexCollection = (function(_super) {
    __extends(SongSearchIndexCollection, _super);

    function SongSearchIndexCollection() {
      return SongSearchIndexCollection.__super__.constructor.apply(this, arguments);
    }

    SongSearchIndexCollection.prototype.methods = {
      read: ['AudioLibrary.GetSongs']
    };

    return SongSearchIndexCollection;

  })(KodiEntities.SongFilteredCollection);
  App.reqres.setHandler("song:entity", function(id, options) {
    if (options == null) {
      options = {};
    }
    return API.getSong(id, options);
  });
  App.reqres.setHandler("song:filtered:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getFilteredSongs(options);
  });
  App.reqres.setHandler("song:byid:entities", function(songIds, callback) {
    if (songIds == null) {
      songIds = [];
    }
    return API.getSongsByIds(songIds, -1, callback);
  });
  App.reqres.setHandler("song:albumparse:entities", function(songs) {
    return API.parseSongsToAlbumSongs(songs);
  });
  return App.commands.setHandler("song:search:entities", function(query, limit, callback) {
    var allLimit, collection, options;
    allLimit = 20;
    options = helpers.global.paramObj('indexOnly', true);
    collection = API.getFilteredSongs(options);
    App.execute("when:entity:fetched", collection, (function(_this) {
      return function() {
        var count, filtered, ids;
        filtered = new App.Entities.Filtered(collection);
        filtered.filterByString('label', query);
        ids = filtered.pluck('songid');
        count = limit === 'limit' ? allLimit : -1;
        return API.getSongsByIds(ids, count, function(loaded) {
          if (ids.length > allLimit && limit === 'limit') {
            loaded.more = true;
          }
          if (callback) {
            return callback(loaded);
          }
        });
      };
    })(this));
    return collection;
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {

  /*
    API Helpers
   */
  var API;
  API = {
    fields: {
      minimal: ['title'],
      small: ['thumbnail', 'playcount', 'lastplayed', 'dateadded', 'episode', 'rating', 'year', 'file', 'genre', 'watchedepisodes', 'cast'],
      full: ['fanart', 'studio', 'mpaa', 'imdbnumber', 'episodeguide', 'plot']
    },
    getEntity: function(id, options) {
      var entity;
      entity = new App.KodiEntities.TVShow();
      entity.set({
        tvshowid: parseInt(id),
        properties: helpers.entities.getFields(API.fields, 'full')
      });
      entity.fetch(options);
      return entity;
    },
    getCollection: function(options) {
      var collection, defaultOptions;
      defaultOptions = {
        cache: true,
        expires: config.get('static', 'collectionCacheExpiry')
      };
      options = _.extend(defaultOptions, options);
      collection = new KodiEntities.TVShowCollection();
      collection.fetch(options);
      return collection;
    }
  };

  /*
   Models and collections.
   */
  KodiEntities.TVShow = (function(_super) {
    __extends(TVShow, _super);

    function TVShow() {
      return TVShow.__super__.constructor.apply(this, arguments);
    }

    TVShow.prototype.defaults = function() {
      var fields;
      fields = _.extend(this.modelDefaults, {
        tvshowid: 1,
        tvshow: ''
      });
      return this.parseFieldsToDefaults(helpers.entities.getFields(API.fields, 'full'), fields);
    };

    TVShow.prototype.methods = {
      read: ['VideoLibrary.GetTVShowDetails', 'tvshowid', 'properties']
    };

    TVShow.prototype.parse = function(resp, xhr) {
      var obj;
      obj = resp.tvshowdetails != null ? resp.tvshowdetails : resp;
      if (resp.tvshowdetails != null) {
        obj.fullyloaded = true;
      }
      obj.unwatched = obj.episode - obj.watchedepisodes;
      return this.parseModel('tvshow', obj, obj.tvshowid);
    };

    return TVShow;

  })(App.KodiEntities.Model);
  KodiEntities.TVShowCollection = (function(_super) {
    __extends(TVShowCollection, _super);

    function TVShowCollection() {
      return TVShowCollection.__super__.constructor.apply(this, arguments);
    }

    TVShowCollection.prototype.model = KodiEntities.TVShow;

    TVShowCollection.prototype.methods = {
      read: ['VideoLibrary.GetTVShows', 'arg1', 'arg2', 'arg3']
    };

    TVShowCollection.prototype.arg1 = function() {
      return helpers.entities.getFields(API.fields, 'small');
    };

    TVShowCollection.prototype.arg2 = function() {
      return this.argLimit();
    };

    TVShowCollection.prototype.arg3 = function() {
      return this.argSort("title", "ascending");
    };

    TVShowCollection.prototype.parse = function(resp, xhr) {
      return this.getResult(resp, 'tvshows');
    };

    return TVShowCollection;

  })(App.KodiEntities.Collection);
  KodiEntities.TVShowFilteredCollection = (function(_super) {
    __extends(TVShowFilteredCollection, _super);

    function TVShowFilteredCollection() {
      return TVShowFilteredCollection.__super__.constructor.apply(this, arguments);
    }

    TVShowFilteredCollection.prototype.methods = {
      read: ['VideoLibrary.GetTVShowss', 'arg1', 'arg2', 'arg3', 'arg4']
    };

    TVShowFilteredCollection.prototype.arg4 = function() {
      return this.argFilter();
    };

    return TVShowFilteredCollection;

  })(KodiEntities.TVShowCollection);

  /*
   Request Handlers.
   */
  App.reqres.setHandler("tvshow:entity", function(id, options) {
    if (options == null) {
      options = {};
    }
    return API.getEntity(id, options);
  });
  App.reqres.setHandler("tvshow:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getCollection(options);
  });
  return App.commands.setHandler("tvshow:search:entities", function(query, limit, callback) {
    var collection;
    collection = API.getCollection({});
    return App.execute("when:entity:fetched", collection, (function(_this) {
      return function() {
        var filtered;
        filtered = new App.Entities.Filtered(collection);
        filtered.filterByString('label', query);
        if (callback) {
          return callback(filtered);
        }
      };
    })(this));
  });
});

this.Kodi.module("KodiEntities", function(KodiEntities, App, Backbone, Marionette, $, _) {

  /*
    API Helpers
   */
  var API;
  API = {
    fields: {
      minimal: [],
      small: ['method', 'description', 'thumbnail', 'params', 'permission', 'returns', 'type', 'namespace', 'methodname'],
      full: []
    },
    getEntity: function(id, collection) {
      var model;
      model = collection.where({
        method: id
      }).shift();
      return model;
    },
    getCollection: function(options) {
      var collection;
      if (options == null) {
        options = {};
      }
      collection = new KodiEntities.ApiMethodCollection();
      collection.fetch(options);
      return collection;
    },
    parseCollection: function(itemsRaw) {
      var item, items, method, methodParts;
      if (itemsRaw == null) {
        itemsRaw = [];
      }
      items = [];
      for (method in itemsRaw) {
        item = itemsRaw[method];
        item.method = method;
        item.id = method;
        methodParts = method.split('.');
        item.namespace = methodParts[0];
        item.methodname = methodParts[1];
        items.push(item);
      }
      return items;
    }
  };

  /*
   Models and collections.
   */
  KodiEntities.ApiMethod = (function(_super) {
    __extends(ApiMethod, _super);

    function ApiMethod() {
      return ApiMethod.__super__.constructor.apply(this, arguments);
    }

    ApiMethod.prototype.defaults = function() {
      var fields;
      fields = _.extend(this.modelDefaults, {
        id: 1,
        params: {}
      });
      return this.parseFieldsToDefaults(helpers.entities.getFields(API.fields, 'small'), fields);
    };

    return ApiMethod;

  })(App.KodiEntities.Model);
  KodiEntities.ApiMethodCollection = (function(_super) {
    __extends(ApiMethodCollection, _super);

    function ApiMethodCollection() {
      return ApiMethodCollection.__super__.constructor.apply(this, arguments);
    }

    ApiMethodCollection.prototype.model = KodiEntities.ApiMethod;

    ApiMethodCollection.prototype.methods = {
      read: ['JSONRPC.Introspect', 'arg1', 'arg2', 'arg3']
    };

    ApiMethodCollection.prototype.arg1 = function() {
      return true;
    };

    ApiMethodCollection.prototype.arg2 = function() {
      return true;
    };

    ApiMethodCollection.prototype.parse = function(resp, xhr) {
      var items;
      items = this.getResult(resp, 'methods');
      return API.parseCollection(items);
    };

    return ApiMethodCollection;

  })(App.KodiEntities.Collection);

  /*
   Request Handlers.
   */
  App.reqres.setHandler("introspect:entity", function(id, collection) {
    return API.getEntity(id, collection);
  });
  return App.reqres.setHandler("introspect:entities", function(options) {
    if (options == null) {
      options = {};
    }
    return API.getCollection(options);
  });
});


/*
  Custom saved playlists, saved in local storage
 */

this.Kodi.module("Entities", function(Entities, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    savedFields: ['id', 'position', 'file', 'type', 'label', 'thumbnail', 'artist', 'album', 'artistid', 'artistid', 'tvshowid', 'tvshow', 'year', 'rating', 'duration', 'track', 'url'],
    playlistKey: 'localplaylist:list',
    playlistItemNamespace: 'localplaylist:item:',
    thumbsUpNamespace: 'thumbs:',
    localPlayerNamespace: 'localplayer:',
    getPlaylistKey: function(key) {
      return this.playlistItemNamespace + key;
    },
    getThumbsKey: function(media) {
      return this.thumbsUpNamespace + media;
    },
    getlocalPlayerKey: function(media) {
      if (media == null) {
        media = 'audio';
      }
      return this.localPlayerNamespace + media;
    },
    getListCollection: function(type) {
      var collection;
      if (type == null) {
        type = 'list';
      }
      collection = new Entities.localPlaylistCollection();
      collection.fetch();
      collection.where({
        type: type
      });
      return collection;
    },
    addList: function(model) {
      var collection;
      collection = this.getListCollection();
      model.id = this.getNextId();
      collection.create(model);
      return model.id;
    },
    getNextId: function() {
      var collection, items, lastItem, nextId;
      collection = API.getListCollection();
      items = collection.getRawCollection();
      if (items.length === 0) {
        nextId = 1;
      } else {
        lastItem = _.max(items, function(item) {
          return item.id;
        });
        nextId = lastItem.id + 1;
      }
      return nextId;
    },
    getItemCollection: function(listId) {
      var collection;
      collection = new Entities.localPlaylistItemCollection([], {
        key: listId
      });
      collection.fetch();
      return collection;
    },
    addItemsToPlaylist: function(playlistId, collection) {
      var item, items, pos, _i, _len;
      if (_.isArray(collection)) {
        items = collection;
      } else {
        items = collection.getRawCollection();
      }
      collection = this.getItemCollection(playlistId);
      pos = collection.length;
      for (_i = 0, _len = items.length; _i < _len; _i++) {
        item = items[_i];
        collection.create(API.getSavedModelFromSource(item, pos));
        pos++;
      }
      return collection;
    },
    getSavedModelFromSource: function(item, position) {
      var fieldName, idfield, newItem, _i, _len, _ref;
      newItem = {};
      _ref = this.savedFields;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        fieldName = _ref[_i];
        if (item[fieldName]) {
          newItem[fieldName] = item[fieldName];
        }
      }
      newItem.position = parseInt(position);
      idfield = item.type + 'id';
      newItem[idfield] = item[idfield];
      return newItem;
    },
    clearPlaylist: function(playlistId) {
      var collection, model;
      collection = this.getItemCollection(playlistId);
      while (model = collection.first()) {
        model.destroy();
      }
    }
  };
  Entities.localPlaylist = (function(_super) {
    __extends(localPlaylist, _super);

    function localPlaylist() {
      return localPlaylist.__super__.constructor.apply(this, arguments);
    }

    localPlaylist.prototype.defaults = {
      id: 0,
      name: '',
      media: '',
      type: 'list'
    };

    return localPlaylist;

  })(Entities.Model);
  Entities.localPlaylistCollection = (function(_super) {
    __extends(localPlaylistCollection, _super);

    function localPlaylistCollection() {
      return localPlaylistCollection.__super__.constructor.apply(this, arguments);
    }

    localPlaylistCollection.prototype.model = Entities.localPlaylist;

    localPlaylistCollection.prototype.localStorage = new Backbone.LocalStorage(API.playlistKey);

    return localPlaylistCollection;

  })(Entities.Collection);
  Entities.localPlaylistItem = (function(_super) {
    __extends(localPlaylistItem, _super);

    function localPlaylistItem() {
      return localPlaylistItem.__super__.constructor.apply(this, arguments);
    }

    localPlaylistItem.prototype.idAttribute = "position";

    localPlaylistItem.prototype.defaults = function() {
      var f, fields, _i, _len, _ref;
      fields = {};
      _ref = API.savedFields;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        f = _ref[_i];
        fields[f] = '';
      }
      return fields;
    };

    return localPlaylistItem;

  })(Entities.Model);
  Entities.localPlaylistItemCollection = (function(_super) {
    __extends(localPlaylistItemCollection, _super);

    function localPlaylistItemCollection() {
      return localPlaylistItemCollection.__super__.constructor.apply(this, arguments);
    }

    localPlaylistItemCollection.prototype.model = Entities.localPlaylistItem;

    localPlaylistItemCollection.prototype.initialize = function(model, options) {
      return this.localStorage = new Backbone.LocalStorage(API.getPlaylistKey(options.key));
    };

    return localPlaylistItemCollection;

  })(Entities.Collection);

  /*
    Saved Playlists
   */
  App.reqres.setHandler("localplaylist:add:entity", function(name, media, type) {
    if (type == null) {
      type = 'list';
    }
    return API.addList({
      name: name,
      media: media,
      type: type
    });
  });
  App.commands.setHandler("localplaylist:remove:entity", function(id) {
    var collection, model;
    collection = API.getListCollection();
    model = collection.findWhere({
      id: parseInt(id)
    });
    return model.destroy();
  });
  App.reqres.setHandler("localplaylist:entities", function() {
    return API.getListCollection();
  });
  App.commands.setHandler("localplaylist:clear:entities", function(playlistId) {
    return API.clearPlaylist(playlistId);
  });
  App.reqres.setHandler("localplaylist:entity", function(id) {
    var collection;
    collection = API.getListCollection();
    return collection.findWhere({
      id: parseInt(id)
    });
  });
  App.reqres.setHandler("localplaylist:item:entities", function(key) {
    return API.getItemCollection(key);
  });
  App.reqres.setHandler("localplaylist:item:add:entities", function(playlistId, collection) {
    return API.addItemsToPlaylist(playlistId, collection);
  });

  /*
    Thumbs up lists
   */
  App.reqres.setHandler("thumbsup:toggle:entity", function(model) {
    var collection, existing, media, position;
    media = model.get('type');
    collection = API.getItemCollection(API.getThumbsKey(media));
    position = collection ? collection.length + 1 : 1;
    existing = collection.findWhere({
      id: model.get('id')
    });
    if (existing) {
      existing.destroy();
    } else {
      collection.create(API.getSavedModelFromSource(model.attributes, position));
    }
    return collection;
  });
  App.reqres.setHandler("thumbsup:get:entities", function(media) {
    return API.getItemCollection(API.getThumbsKey(media));
  });
  App.reqres.setHandler("thumbsup:check", function(model) {
    var collection, existing;
    if (model != null) {
      collection = API.getItemCollection(API.getThumbsKey(model.get('type')));
      existing = collection.findWhere({
        id: model.get('id')
      });
      return _.isObject(existing);
    } else {
      return false;
    }
  });

  /*
    Local player lists
   */
  App.reqres.setHandler("localplayer:get:entities", function(media) {
    if (media == null) {
      media = 'audio';
    }
    return API.getItemCollection(API.getlocalPlayerKey(media));
  });
  App.commands.setHandler("localplayer:clear:entities", function(media) {
    if (media == null) {
      media = 'audio';
    }
    return API.clearPlaylist(API.getlocalPlayerKey(media));
  });
  return App.reqres.setHandler("localplayer:item:add:entities", function(collection, media) {
    if (media == null) {
      media = 'audio';
    }
    return API.addItemsToPlaylist(API.getlocalPlayerKey(media), collection);
  });
});

this.Kodi.module("Entities", function(Entities, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    localKey: 'mainNav',
    getItems: function() {
      var items, navCollection;
      navCollection = this.getLocalCollection();
      items = navCollection.getRawCollection();
      if (items.length === 0) {
        items = this.getDefaultItems();
      }
      return items;
    },
    getDefaultItems: function() {
      var nav;
      nav = [];
      nav.push({
        id: 1,
        title: "Music",
        path: 'music',
        icon: 'mdi-av-my-library-music',
        classes: 'nav-music',
        parent: 0
      });
      nav.push({
        id: 2,
        title: "Recent",
        path: 'music',
        icon: '',
        classes: '',
        parent: 1
      });
      nav.push({
        id: 3,
        title: "Artists",
        path: 'music/artists',
        icon: '',
        classes: '',
        parent: 1
      });
      nav.push({
        id: 4,
        title: "Albums",
        path: 'music/albums',
        icon: '',
        classes: '',
        parent: 1
      });
      nav.push({
        id: 5,
        title: "Digital radio",
        path: 'music/radio',
        icon: '',
        classes: 'pvr-link',
        parent: 1,
        visibility: "addon:pvr:enabled"
      });
      nav.push({
        id: 11,
        title: "Movies",
        path: 'movies/recent',
        icon: 'mdi-av-movie',
        classes: 'nav-movies',
        parent: 0
      });
      nav.push({
        id: 12,
        title: "Recent movies",
        path: 'movies/recent',
        icon: '',
        classes: '',
        parent: 11
      });
      nav.push({
        id: 13,
        title: "All movies",
        path: 'movies',
        icon: '',
        classes: '',
        parent: 11
      });
      nav.push({
        id: 21,
        title: "TV shows",
        path: 'tvshows/recent',
        icon: 'mdi-hardware-tv',
        classes: 'nav-tv',
        parent: 0
      });
      nav.push({
        id: 22,
        title: "Recent episodes",
        path: 'tvshows/recent',
        icon: '',
        classes: '',
        parent: 21
      });
      nav.push({
        id: 23,
        title: "All TV shows",
        path: 'tvshows',
        icon: '',
        classes: '',
        parent: 21
      });
      nav.push({
        id: 24,
        title: "TV",
        path: 'tvshows/live',
        icon: '',
        classes: 'pvr-link',
        parent: 21,
        visibility: "addon:pvr:enabled"
      });
      nav.push({
        id: 31,
        title: "Browser",
        path: 'browser',
        icon: 'mdi-action-view-list',
        classes: 'nav-browser',
        parent: 0
      });
      nav.push({
        id: 41,
        title: "Thumbs up",
        path: 'thumbsup',
        icon: 'mdi-action-thumb-up',
        classes: 'nav-thumbs-up',
        parent: 0
      });
      nav.push({
        id: 42,
        title: "Playlists",
        path: 'playlists',
        icon: 'mdi-action-assignment',
        classes: 'playlists',
        parent: 0
      });
      nav.push({
        id: 51,
        title: "Settings",
        path: 'settings/web',
        icon: 'mdi-action-settings',
        classes: 'nav-settings',
        parent: 0
      });
      nav.push({
        id: 52,
        title: "Web interface",
        path: 'settings/web',
        icon: '',
        classes: '',
        parent: 51
      });
      nav.push({
        id: 53,
        title: "Add-ons",
        path: 'settings/addons',
        icon: '',
        classes: '',
        parent: 51
      });
      nav.push({
        id: 54,
        title: "Main Nav",
        path: 'settings/nav',
        icon: '',
        classes: '',
        parent: 51
      });
      nav.push({
        id: 61,
        title: "Help",
        path: 'help',
        icon: 'mdi-action-help',
        classes: 'nav-help',
        parent: 0
      });
      return this.checkVisibility(nav);
    },
    checkVisibility: function(items) {
      var item, newItems, _i, _len;
      newItems = [];
      for (_i = 0, _len = items.length; _i < _len; _i++) {
        item = items[_i];
        if (item.visibility != null) {
          if (App.request(item.visibility)) {
            newItems.push(item);
          }
        } else {
          newItems.push(item);
        }
      }
      return newItems;
    },
    getLocalCollection: function() {
      var collection;
      collection = new Entities.LocalNavMainCollection([], {
        key: this.localKey
      });
      collection.fetch();
      return collection;
    },
    getStructure: function() {
      var navCollection, navParsed;
      navParsed = this.sortStructure(this.getItems());
      navCollection = new Entities.NavMainCollection(navParsed);
      return navCollection;
    },
    getChildStructure: function(parentId) {
      var childItems, nav, parent;
      nav = this.getDefaultItems();
      parent = _.findWhere(nav, {
        id: parentId
      });
      childItems = _.where(nav, {
        parent: parentId
      });
      parent.items = new Entities.NavMainCollection(childItems);
      return new Entities.NavMain(parent);
    },
    sortStructure: function(structure) {
      var children, i, model, newParents, _i, _len, _name;
      children = {};
      for (_i = 0, _len = structure.length; _i < _len; _i++) {
        model = structure[_i];
        if (!((model.path != null) && model.parent !== 0)) {
          continue;
        }
        model.title = t.gettext(model.title);
        if (children[_name = model.parent] == null) {
          children[_name] = [];
        }
        children[model.parent].push(model);
      }
      newParents = [];
      for (i in structure) {
        model = structure[i];
        if (model.path != null) {
          if (model.parent === 0) {
            model.children = children[model.id];
            newParents.push(model);
          }
        }
      }
      return newParents;
    },
    getIdfromPath: function(path) {
      var model;
      model = _.findWhere(this.getDefaultItems(), {
        path: path
      });
      if (model != null) {
        return model.id;
      } else {
        return 1;
      }
    },
    saveLocal: function(items) {
      var collection, i, item;
      collection = this.clearLocal();
      for (i in items) {
        item = items[i];
        collection.create(item);
      }
      return collection;
    },
    clearLocal: function() {
      var collection, model;
      collection = this.getLocalCollection();
      while (model = collection.first()) {
        model.destroy();
      }
      return collection;
    }
  };
  Entities.NavMain = (function(_super) {
    __extends(NavMain, _super);

    function NavMain() {
      return NavMain.__super__.constructor.apply(this, arguments);
    }

    NavMain.prototype.defaults = {
      id: 0,
      title: 'Untitled',
      path: '',
      description: '',
      icon: '',
      classes: '',
      parent: 0,
      children: []
    };

    return NavMain;

  })(App.Entities.Model);
  Entities.NavMainCollection = (function(_super) {
    __extends(NavMainCollection, _super);

    function NavMainCollection() {
      return NavMainCollection.__super__.constructor.apply(this, arguments);
    }

    NavMainCollection.prototype.model = Entities.NavMain;

    return NavMainCollection;

  })(App.Entities.Collection);
  Entities.LocalNavMainCollection = (function(_super) {
    __extends(LocalNavMainCollection, _super);

    function LocalNavMainCollection() {
      return LocalNavMainCollection.__super__.constructor.apply(this, arguments);
    }

    LocalNavMainCollection.prototype.model = Entities.NavMain;

    LocalNavMainCollection.prototype.localStorage = new Backbone.LocalStorage(API.localKey);

    return LocalNavMainCollection;

  })(App.Entities.Collection);
  App.reqres.setHandler("navMain:entities", function(parent) {
    var parentId;
    if (parent == null) {
      parent = 'all';
    }
    if (parent === 'all') {
      return API.getStructure();
    } else {
      parentId = API.getIdfromPath(parent);
      return API.getChildStructure(parentId);
    }
  });
  App.reqres.setHandler("navMain:array:entities", function(items) {
    var i, item;
    for (i in items) {
      item = items[i];
      items[i].id = item.path;
    }
    return new Entities.NavMainCollection(items);
  });
  App.reqres.setHandler("navMain:update:entities", function(items) {
    return API.saveLocal(items);
  });
  return App.reqres.setHandler("navMain:update:defaults", function(items) {
    return API.clearLocal();
  });
});

this.Kodi.module("Controllers", function(Controllers, App, Backbone, Marionette, $, _) {
  return Controllers.Base = (function(_super) {
    __extends(Base, _super);

    Base.prototype.params = {};

    function Base(options) {
      if (options == null) {
        options = {};
      }
      this.region = options.region || App.request("default:region");
      this.params = helpers.url.params();
      Base.__super__.constructor.call(this, options);
      this._instance_id = _.uniqueId("controller");
      App.execute("register:instance", this, this._instance_id);
    }

    Base.prototype.close = function() {
      var args;
      args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
      delete this.region;
      delete this.options;
      Base.__super__.close.call(this, args);
      return App.execute("unregister:instance", this, this._instance_id);
    };

    Base.prototype.show = function(view) {
      this.listenTo(view, "close", this.close);
      return this.region.show(view);
    };

    return Base;

  })(Backbone.Marionette.Controller);
});

this.Kodi.module("Router", function(Router, App, Backbone, Marionette, $, _) {
  return Router.Base = (function(_super) {
    __extends(Base, _super);

    function Base() {
      return Base.__super__.constructor.apply(this, arguments);
    }

    Base.prototype.before = function(route, params) {
      return App.execute("loading:show:page");
    };

    Base.prototype.after = function(route, params) {
      return this.setBodyClasses();
    };

    Base.prototype.setBodyClasses = function() {
      var $body, section;
      $body = App.getRegion('root').$el;
      $body.removeClassRegex(/^section-/);
      $body.removeClassRegex(/^page-/);
      section = helpers.url.arg(0);
      if (section === '') {
        section = 'home';
      }
      $body.addClass('section-' + section);
      return $body.addClass('page-' + helpers.url.arg().join('-'));
    };

    return Base;

  })(Marionette.AppRouter);
});

this.Kodi.module("Views", function(Views, App, Backbone, Marionette, $, _) {
  return Views.CollectionView = (function(_super) {
    __extends(CollectionView, _super);

    function CollectionView() {
      return CollectionView.__super__.constructor.apply(this, arguments);
    }

    CollectionView.prototype.itemViewEventPrefix = "childview";

    CollectionView.prototype.onShow = function() {
      return $("img.lazy").lazyload({
        threshold: 200
      });
    };

    return CollectionView;

  })(Backbone.Marionette.CollectionView);
});

this.Kodi.module("Views", function(Views, App, Backbone, Marionette, $, _) {
  return Views.CompositeView = (function(_super) {
    __extends(CompositeView, _super);

    function CompositeView() {
      return CompositeView.__super__.constructor.apply(this, arguments);
    }

    CompositeView.prototype.itemViewEventPrefix = "childview";

    return CompositeView;

  })(Backbone.Marionette.CompositeView);
});

this.Kodi.module("Views", function(Views, App, Backbone, Marionette, $, _) {
  return Views.ItemView = (function(_super) {
    __extends(ItemView, _super);

    function ItemView() {
      return ItemView.__super__.constructor.apply(this, arguments);
    }

    return ItemView;

  })(Backbone.Marionette.ItemView);
});

this.Kodi.module("Views", function(Views, App, Backbone, Marionette, $, _) {
  return Views.LayoutView = (function(_super) {
    __extends(LayoutView, _super);

    function LayoutView() {
      return LayoutView.__super__.constructor.apply(this, arguments);
    }

    return LayoutView;

  })(Backbone.Marionette.LayoutView);
});

this.Kodi.module("Views", function(Views, App, Backbone, Marionette, $, _) {
  var _remove;
  _remove = Marionette.View.prototype.remove;
  return _.extend(Marionette.View.prototype, {
    themeLink: function(name, url, options) {
      var attrs;
      if (options == null) {
        options = {};
      }
      _.defaults(options, {
        external: false,
        className: ''
      });
      attrs = !options.external ? {
        href: "#" + url
      } : void 0;
      if (options.className !== '') {
        attrs["class"] = options.className;
      }
      return this.themeTag('a', attrs, name);
    },
    parseAttributes: function(attrs) {
      var a, attr, val;
      a = [];
      for (attr in attrs) {
        val = attrs[attr];
        a.push("" + attr + "='" + val + "'");
      }
      return a.join(' ');
    },
    themeTag: function(el, attrs, value) {
      var attrsString;
      attrsString = this.parseAttributes(attrs);
      return "<" + el + " " + attrsString + ">" + value + "</" + el + ">";
    },
    formatText: function(text, addInLineBreaks) {
      var res;
      if (addInLineBreaks == null) {
        addInLineBreaks = false;
      }
      res = XBBCODE.process({
        text: text,
        removeMisalignedTags: true,
        addInLineBreaks: addInLineBreaks
      });
      if (res.error === !false) {
        helpers.debug.msg('formatText error: ' + res.errorQueue.join(', '), 'warning', res);
      }
      return res.html;
    }
  });
});

this.Kodi.module("Views", function(Views, App, Backbone, Marionette, $, _) {
  return Views.VirtualListView = (function(_super) {
    __extends(VirtualListView, _super);

    function VirtualListView() {
      return VirtualListView.__super__.constructor.apply(this, arguments);
    }

    VirtualListView.prototype.originalCollection = {};

    VirtualListView.prototype.preload = 20;

    VirtualListView.prototype.originalChildView = {};

    VirtualListView.prototype.buffer = 30;

    VirtualListView.prototype.isTicking = false;

    VirtualListView.prototype.addChild = function(child, ChildView, index) {
      if (index > this.preload) {
        ChildView = App.Views.CardViewPlaceholder;
      }
      return Backbone.Marionette.CollectionView.prototype.addChild.apply(this, arguments);
    };

    VirtualListView.prototype.bindScroll = function() {
      $(window).scrollStopped((function(_this) {
        return function() {
          return _this.requestTick();
        };
      })(this));
      return $(window).resizeStopped((function(_this) {
        return function() {
          return _this.requestTick();
        };
      })(this));
    };

    VirtualListView.prototype.initialize = function() {
      this.originalChildView = this.getOption('childView');
      this.placeholderChildView = App.Views.CardViewPlaceholder;
      return this.bindScroll();
    };

    VirtualListView.prototype.onRender = function() {
      return this.requestTick();
    };

    VirtualListView.prototype.requestTick = function() {
      if (!this.isTicking) {
        requestAnimationFrame((function(_this) {
          return function() {
            return _this.renderItemsInViewport();
          };
        })(this));
      }
      return this.isTicking = true;
    };

    VirtualListView.prototype.renderItemsInViewport = function() {
      var $cards, max, min, visibleIndexes, visibleRange, _i, _results;
      this.isTicking = false;
      $cards = $(".card", this.$el);
      visibleIndexes = [];
      $cards.each((function(_this) {
        return function(i, d) {
          if ($(d).visible(true)) {
            return visibleIndexes.push(i);
          }
        };
      })(this));
      min = _.min(visibleIndexes);
      max = _.max(visibleIndexes);
      min = (min - this.buffer) < 0 ? 0 : min - this.buffer;
      max = (max + this.buffer) >= $cards.length ? $cards.length - 1 : max + this.buffer;
      visibleRange = (function() {
        _results = [];
        for (var _i = min; min <= max ? _i <= max : _i >= max; min <= max ? _i++ : _i--){ _results.push(_i); }
        return _results;
      }).apply(this);
      return $cards.each((function(_this) {
        return function(i, d) {
          if ($(d).hasClass('ph') && helpers.global.inArray(i, visibleRange)) {
            return $(d).replaceWith(_this.getRenderedChildView($(d).data('model'), _this.originalChildView, i));
          } else if (!$(d).hasClass('ph') && !helpers.global.inArray(i, visibleRange)) {
            return $(d).replaceWith(_this.getRenderedChildView($(d).data('model'), _this.placeholderChildView, i));
          }
        };
      })(this));
    };

    VirtualListView.prototype.getRenderedChildView = function(child, ChildView, index) {
      var childViewOptions, view;
      childViewOptions = this.getOption('childViewOptions');
      childViewOptions = Marionette._getValue(childViewOptions, this, [child, index]);
      view = this.buildChildView(child, ChildView, childViewOptions);
      this.proxyChildEvents(view);
      return view.render().$el;
    };

    VirtualListView.prototype.events = {
      "click a": "storeScroll"
    };

    VirtualListView.prototype.storeScroll = function() {
      return helpers.backscroll.setLast();
    };

    VirtualListView.prototype.onShow = function() {
      return helpers.backscroll.scrollToLast();
    };

    VirtualListView.prototype.onDestroy = function() {
      $(window).unbind('scroll');
      return $(window).unbind('resize');
    };

    return VirtualListView;

  })(Views.CollectionView);
});

this.Kodi.module("Views", function(Views, App, Backbone, Marionette, $, _) {
  Views.CardView = (function(_super) {
    __extends(CardView, _super);

    function CardView() {
      return CardView.__super__.constructor.apply(this, arguments);
    }

    CardView.prototype.template = "views/card/card";

    CardView.prototype.tagName = "li";

    CardView.prototype.events = {
      "click .dropdown > i": "populateMenu",
      "click .thumbs": "toggleThumbs"
    };

    CardView.prototype.populateMenu = function() {
      var key, menu, val, _ref;
      menu = '';
      if (this.model.get('menu')) {
        _ref = this.model.get('menu');
        for (key in _ref) {
          val = _ref[key];
          menu += this.themeTag('li', {
            "class": key
          }, val);
        }
        return this.$el.find('.dropdown-menu').html(menu);
      }
    };

    CardView.prototype.toggleThumbs = function() {
      App.request("thumbsup:toggle:entity", this.model);
      return this.$el.toggleClass('thumbs-up');
    };

    CardView.prototype.attributes = function() {
      var classes;
      classes = ['card', 'card-loaded'];
      if (App.request("thumbsup:check", this.model)) {
        classes.push('thumbs-up');
      }
      return {
        "class": classes.join(' ')
      };
    };

    CardView.prototype.onRender = function() {
      return this.$el.data('model', this.model);
    };

    CardView.prototype.onShow = function() {
      return $('.dropdown', this.$el).on('click', function() {
        return $(this).removeClass('open').trigger('hide.bs.dropdown');
      });
    };

    return CardView;

  })(App.Views.ItemView);
  return Views.CardViewPlaceholder = (function(_super) {
    __extends(CardViewPlaceholder, _super);

    function CardViewPlaceholder() {
      return CardViewPlaceholder.__super__.constructor.apply(this, arguments);
    }

    CardViewPlaceholder.prototype.template = "views/card/card_placeholder";

    CardViewPlaceholder.prototype.attributes = function() {
      return {
        "class": 'card ph'
      };
    };

    CardViewPlaceholder.prototype.onRender = function() {
      return this.$el.data('model', this.model);
    };

    return CardViewPlaceholder;

  })(App.Views.ItemView);
});

this.Kodi.module("Views", function(Views, App, Backbone, Marionette, $, _) {
  return Views.EmptyViewPage = (function(_super) {
    __extends(EmptyViewPage, _super);

    function EmptyViewPage() {
      return EmptyViewPage.__super__.constructor.apply(this, arguments);
    }

    EmptyViewPage.prototype.template = "views/empty/empty_page";

    EmptyViewPage.prototype.regions = {
      regionEmptyContent: ".empty--page"
    };

    Views.EmptyViewResults = (function(_super1) {
      __extends(EmptyViewResults, _super1);

      function EmptyViewResults() {
        return EmptyViewResults.__super__.constructor.apply(this, arguments);
      }

      EmptyViewResults.prototype.template = "views/empty/empty_results";

      EmptyViewResults.prototype.regions = {
        regionEmptyContent: ".empty-result"
      };

      return EmptyViewResults;

    })(App.Views.ItemView);

    return EmptyViewPage;

  })(App.Views.ItemView);
});

this.Kodi.module("Views", function(Views, App, Backbone, Marionette, $, _) {
  Views.LayoutWithSidebarFirstView = (function(_super) {
    __extends(LayoutWithSidebarFirstView, _super);

    function LayoutWithSidebarFirstView() {
      return LayoutWithSidebarFirstView.__super__.constructor.apply(this, arguments);
    }

    LayoutWithSidebarFirstView.prototype.template = "views/layouts/layout_with_sidebar_first";

    LayoutWithSidebarFirstView.prototype.regions = {
      regionSidebarFirst: ".region-first",
      regionContent: ".region-content"
    };

    LayoutWithSidebarFirstView.prototype.events = {
      "click .region-first-toggle": "toggleRegionFirst"
    };

    LayoutWithSidebarFirstView.prototype.toggleRegionFirst = function() {
      return this.$el.toggleClass('region-first-open');
    };

    return LayoutWithSidebarFirstView;

  })(App.Views.LayoutView);
  Views.LayoutWithHeaderView = (function(_super) {
    __extends(LayoutWithHeaderView, _super);

    function LayoutWithHeaderView() {
      return LayoutWithHeaderView.__super__.constructor.apply(this, arguments);
    }

    LayoutWithHeaderView.prototype.template = "views/layouts/layout_with_header";

    LayoutWithHeaderView.prototype.regions = {
      regionHeader: ".region-header",
      regionContentTop: ".region-content-top",
      regionContent: ".region-content"
    };

    return LayoutWithHeaderView;

  })(App.Views.LayoutView);
  return Views.LayoutDetailsHeaderView = (function(_super) {
    __extends(LayoutDetailsHeaderView, _super);

    function LayoutDetailsHeaderView() {
      return LayoutDetailsHeaderView.__super__.constructor.apply(this, arguments);
    }

    LayoutDetailsHeaderView.prototype.template = "views/layouts/layout_details_header";

    LayoutDetailsHeaderView.prototype.regions = {
      regionSide: ".region-details-side",
      regionTitle: ".region-details-title",
      regionMeta: ".region-details-meta",
      regionMetaSideFirst: ".region-details-meta-side-first",
      regionMetaSideSecond: ".region-details-meta-side-second",
      regionMetaBelow: ".region-details-meta-below",
      regionFanart: ".region-details-fanart"
    };

    LayoutDetailsHeaderView.prototype.onRender = function() {
      $('.region-details-fanart', this.$el).css('background-image', 'url("' + this.model.get('fanart') + '")');
      return helpers.ui.getSwatch(this.model.get('thumbnail'), function(swatches) {
        return helpers.ui.applyHeaderSwatch(swatches);
      });
    };

    return LayoutDetailsHeaderView;

  })(App.Views.LayoutView);
});

this.Kodi.module("Components.Form", function(Form, App, Backbone, Marionette, $, _) {
  Form.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      var config;
      if (options == null) {
        options = {};
      }
      config = options.config ? options.config : {};
      this.formLayout = this.getFormLayout(config);
      this.listenTo(this.formLayout, "show", (function(_this) {
        return function() {
          _this.formBuild(options.form, options.formState, config);
          $.material.init();
          if (config && typeof config.onShow === 'function') {
            return config.onShow(options);
          }
        };
      })(this));
      this.listenTo(this.formLayout, "form:submit", (function(_this) {
        return function() {
          return _this.formSubmit(options);
        };
      })(this));
      return this;
    };

    Controller.prototype.formSubmit = function(options) {
      var data;
      data = Backbone.Syphon.serialize(this.formLayout);
      return this.processFormSubmit(data, options);
    };

    Controller.prototype.processFormSubmit = function(data, options) {
      if (options.config && typeof options.config.callback === 'function') {
        return options.config.callback(data, this.formLayout);
      }
    };

    Controller.prototype.getFormLayout = function(options) {
      if (options == null) {
        options = {};
      }
      return new Form.FormWrapper({
        config: options
      });
    };

    Controller.prototype.formBuild = function(form, formState, options) {
      var buildView, collection;
      if (form == null) {
        form = [];
      }
      if (formState == null) {
        formState = {};
      }
      if (options == null) {
        options = {};
      }
      collection = App.request("form:item:entities", form, formState);
      buildView = new Form.Groups({
        collection: collection
      });
      return this.formLayout.formContentRegion.show(buildView);
    };

    return Controller;

  })(App.Controllers.Base);
  App.reqres.setHandler("form:render:items", function(form, formState, options) {
    var collection;
    if (options == null) {
      options = {};
    }
    collection = App.request("form:item:entities", form, formState);
    return new Form.Groups({
      collection: collection
    });
  });
  App.reqres.setHandler("form:wrapper", function(options) {
    var formController;
    if (options == null) {
      options = {};
    }
    formController = new Form.Controller(options);
    return formController.formLayout;
  });
  return App.reqres.setHandler("form:popup:wrapper", function(options) {
    var formContent, formController, originalCallback;
    if (options == null) {
      options = {};
    }
    originalCallback = options.config.callback;
    options.config.callback = function(data, layout) {
      App.execute("ui:modal:close");
      return originalCallback(data, layout);
    };
    formController = new Form.Controller(options);
    formContent = formController.formLayout.render().$el;
    formController.formLayout.trigger('show');
    return App.execute("ui:modal:form:show", options.title, formContent);
  });
});

this.Kodi.module("Components.Form", function(Form, App, Backbone, Marionette, $, _) {
  Form.FormWrapper = (function(_super) {
    __extends(FormWrapper, _super);

    function FormWrapper() {
      return FormWrapper.__super__.constructor.apply(this, arguments);
    }

    FormWrapper.prototype.template = "components/form/form";

    FormWrapper.prototype.tagName = "form";

    FormWrapper.prototype.regions = {
      formContentRegion: ".form-content-region",
      formResponse: ".response"
    };

    FormWrapper.prototype.triggers = {
      "click .form-save": "form:submit",
      "click [data-form-button='cancel']": "form:cancel"
    };

    FormWrapper.prototype.modelEvents = {
      "change:_errors": "changeErrors",
      "sync:start": "syncStart",
      "sync:stop": "syncStop"
    };

    FormWrapper.prototype.initialize = function() {
      this.config = this.options.config;
      return this.on("form:save", (function(_this) {
        return function(msg) {
          return _this.addSuccessMsg(msg);
        };
      })(this));
    };

    FormWrapper.prototype.attributes = function() {
      var attrs;
      attrs = {
        "class": 'component-form'
      };
      if (this.options.config && this.options.config.attributes) {
        attrs = _.extend(attrs, this.options.config.attributes);
      }
      return attrs;
    };

    FormWrapper.prototype.onShow = function() {
      return _.defer((function(_this) {
        return function() {
          if (_this.config.focusFirstInput) {
            _this.focusFirstInput();
          }
          $('.btn').ripples({
            color: 'rgba(255,255,255,0.1)'
          });
          return App.vent.trigger("form:onshow", _this.config);
        };
      })(this));
    };

    FormWrapper.prototype.focusFirstInput = function() {
      return this.$(":input:visible:enabled:first").focus();
    };

    FormWrapper.prototype.changeErrors = function(model, errors, options) {
      if (this.config.errors) {
        if (_.isEmpty(errors)) {
          return this.removeErrors();
        } else {
          return this.addErrors(errors);
        }
      }
    };

    FormWrapper.prototype.removeErrors = function() {
      return this.$(".error").removeClass("error").find("small").remove();
    };

    FormWrapper.prototype.addErrors = function(errors) {
      var array, name, _results;
      if (errors == null) {
        errors = {};
      }
      _results = [];
      for (name in errors) {
        array = errors[name];
        _results.push(this.addError(name, array[0]));
      }
      return _results;
    };

    FormWrapper.prototype.addError = function(name, error) {
      var el, sm;
      el = this.$("[name='" + name + "']");
      sm = $("<small>").text(error);
      return el.after(sm).closest(".row").addClass("error");
    };

    FormWrapper.prototype.addSuccessMsg = function(msg) {
      var $el;
      $el = $(".response", this.$el);
      $el.html(msg).show();
      return setTimeout((function() {
        return $el.fadeOut();
      }), 5000);
    };

    return FormWrapper;

  })(App.Views.LayoutView);
  Form.Item = (function(_super) {
    __extends(Item, _super);

    function Item() {
      return Item.__super__.constructor.apply(this, arguments);
    }

    Item.prototype.template = 'components/form/form_item';

    Item.prototype.tagName = 'div';

    Item.prototype.initialize = function() {
      var attrs, baseAttrs, el, key, materialBaseAttrs, name, options, val, value, _ref;
      name = this.model.get('name') ? this.model.get('name') : this.model.get('id');
      baseAttrs = _.extend({
        id: 'form-edit-' + this.model.get('id'),
        name: name
      }, this.model.get('attributes'));
      materialBaseAttrs = _.extend(baseAttrs, {
        "class": 'form-control'
      });
      switch (this.model.get('type')) {
        case 'checkbox':
          attrs = {
            type: 'checkbox',
            value: 1,
            "class": 'form-checkbox'
          };
          if (this.model.get('defaultValue') === true) {
            attrs.checked = 'checked';
          }
          el = this.themeTag('input', _.extend(baseAttrs, attrs), '');
          break;
        case 'textfield':
          attrs = {
            type: 'text',
            value: this.model.get('defaultValue')
          };
          el = this.themeTag('input', _.extend(materialBaseAttrs, attrs), '');
          break;
        case 'hidden':
          attrs = {
            type: 'hidden',
            value: this.model.get('defaultValue'),
            "class": 'form-hidden'
          };
          el = this.themeTag('input', _.extend(baseAttrs, attrs), '');
          break;
        case 'button':
          attrs = {
            "class": 'form-button btn btn-secondary'
          };
          el = this.themeTag('button', _.extend(baseAttrs, attrs), this.model.get('value'));
          break;
        case 'textarea':
          el = this.themeTag('textarea', materialBaseAttrs, this.model.get('defaultValue'));
          break;
        case 'markup':
          attrs = {
            "class": 'form-markup'
          };
          el = this.themeTag('div', attrs, this.model.get('markup'));
          break;
        case 'select':
          options = '';
          _ref = this.model.get('options');
          for (key in _ref) {
            val = _ref[key];
            attrs = {
              value: key
            };
            value = this.model.get('defaultValue');
            if (String(value) === String(key)) {
              attrs.selected = 'selected';
            }
            options += this.themeTag('option', attrs, val);
          }
          el = this.themeTag('select', _.extend(baseAttrs, {
            "class": 'form-control'
          }), options);
          break;
        default:
          el = null;
      }
      if (el) {
        return this.model.set({
          element: el
        });
      }
    };

    Item.prototype.attributes = function() {
      return {
        "class": 'form-item form-group form-type-' + this.model.get('type') + ' form-edit-' + this.model.get('id')
      };
    };

    return Item;

  })(App.Views.ItemView);
  Form.Group = (function(_super) {
    __extends(Group, _super);

    function Group() {
      return Group.__super__.constructor.apply(this, arguments);
    }

    Group.prototype.template = 'components/form/form_item_group';

    Group.prototype.tagName = 'div';

    Group.prototype.childView = Form.Item;

    Group.prototype.childViewContainer = '.form-items';

    Group.prototype.attributes = function() {
      return {
        "class": 'form-group group-parent ' + this.model.get('class')
      };
    };

    Group.prototype.initialize = function() {
      var children;
      children = this.model.get('children');
      if (children.length === 0) {
        return this.model.set('title', '');
      } else {
        return this.collection = children;
      }
    };

    return Group;

  })(App.Views.CompositeView);
  return Form.Groups = (function(_super) {
    __extends(Groups, _super);

    function Groups() {
      return Groups.__super__.constructor.apply(this, arguments);
    }

    Groups.prototype.childView = Form.Group;

    Groups.prototype.className = 'form-groups';

    return Groups;

  })(App.Views.CollectionView);
});

this.Kodi.module("AddonApp", function(AddonApp, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    addonController: function() {
      return App.request("command:kodi:controller", 'auto', 'AddOn');
    },
    getEnabledAddons: function(callback) {
      var addons;
      addons = [];
      if (config.getLocal("addOnsLoaded", false)) {
        addons = config.getLocal("addOnsEnabled", []);
        if (callback) {
          callback(addons);
        }
      } else {
        this.addonController().getEnabledAddons(true, function(addons) {
          config.setLocal("addOnsEnabled", addons);
          config.setLocal("addOnsLoaded", true);
          if (callback) {
            return callback(addons);
          }
        });
      }
      return addons;
    },
    isAddOnEnabled: function(filter, callback) {
      var addons;
      if (filter == null) {
        filter = {};
      }
      addons = this.getEnabledAddons(callback);
      return _.findWhere(addons, filter);
    }
  };
  App.on("before:start", function() {
    return API.getEnabledAddons(function(resp) {});
  });
  App.reqres.setHandler('addon:isEnabled', function(filter, callback) {
    return API.isAddOnEnabled(filter, function(enabled) {
      if (callback) {
        return callback(enabled);
      }
    });
  });
  return App.reqres.setHandler('addon:enabled:addons', function(callback) {
    return API.getEnabledAddons(function(addons) {
      if (callback) {
        return callback(addons);
      }
    });
  });
});

this.Kodi.module("AddonApp.Pvr", function(Pvr, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    isEnabled: function() {
      return App.request("addon:isEnabled", {
        type: 'xbmc.pvrclient'
      });
    }
  };
  return App.reqres.setHandler("addon:pvr:enabled", function() {
    return API.isEnabled();
  });
});

this.Kodi.module("AddonApp.SoundCloud", function(Soundcloud, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    addonId: 'plugin.audio.soundcloud',
    searchAddon: {
      id: this.addonId,
      url: 'plugin://plugin.audio.soundcloud/search/query/?q=[QUERY]',
      title: 'SoundCloud',
      media: 'music'
    },
    isEnabled: function() {
      return App.request("addon:isEnabled", {
        addonid: this.addonId
      });
    }
  };
  return App.reqres.setHandler("addon:soundcloud:enabled", function() {
    return API.isEnabled();
  });
});

this.Kodi.module("AlbumApp", function(AlbumApp, App, Backbone, Marionette, $, _) {
  var API;
  AlbumApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "music": "recent",
      "music/albums": "list",
      "music/album/:id": "view"
    };

    return Router;

  })(App.Router.Base);
  API = {
    recent: function() {
      return new AlbumApp.Landing.Controller();
    },
    list: function() {
      return new AlbumApp.List.Controller();
    },
    view: function(id) {
      return new AlbumApp.Show.Controller({
        id: id
      });
    },
    action: function(op, view) {
      var localPlaylist, model, playlist;
      model = view.model;
      playlist = App.request("command:kodi:controller", 'audio', 'PlayList');
      switch (op) {
        case 'play':
          return App.execute("command:audio:play", 'albumid', model.get('albumid'));
        case 'add':
          return playlist.add('albumid', model.get('albumid'));
        case 'localadd':
          return App.execute("localplaylist:addentity", 'albumid', model.get('albumid'));
        case 'localplay':
          localPlaylist = App.request("command:local:controller", 'audio', 'PlayList');
          return localPlaylist.play('albumid', model.get('albumid'));
      }
    }
  };
  App.on("before:start", function() {
    return new AlbumApp.Router({
      controller: API
    });
  });
  App.commands.setHandler('album:action', function(op, model) {
    return API.action(op, model);
  });
  return App.reqres.setHandler('album:action:items', function() {
    return {
      actions: {
        thumbs: 'Thumbs up'
      },
      menu: {
        add: 'Add to Kodi playlist',
        localadd: 'Add to local playlist',
        divider: '',
        localplay: 'Play in browser'
      }
    };
  });
});

this.Kodi.module("AlbumApp.Landing", function(Landing, App, Backbone, Marionette, $, _) {
  return Landing.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.subNavId = 'music';

    Controller.prototype.initialize = function() {
      this.layout = this.getLayoutView();
      this.listenTo(this.layout, "show", (function(_this) {
        return function() {
          _this.getPageView();
          return _this.getSubNav();
        };
      })(this));
      return App.regionContent.show(this.layout);
    };

    Controller.prototype.getLayoutView = function() {
      return new Landing.Layout();
    };

    Controller.prototype.getSubNav = function() {
      var subNav;
      subNav = App.request("navMain:children:show", this.subNavId, 'Sections');
      return this.layout.regionSidebarFirst.show(subNav);
    };

    Controller.prototype.getPageView = function() {
      this.page = new Landing.Page();
      this.listenTo(this.page, "show", (function(_this) {
        return function() {
          _this.renderRecentlyAdded();
          return _this.renderRecentlyPlayed();
        };
      })(this));
      return this.layout.regionContent.show(this.page);
    };

    Controller.prototype.renderRecentlyAdded = function() {
      var collection;
      collection = App.request("album:recentlyadded:entities");
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          var view;
          view = App.request("album:list:view", collection);
          return _this.page.regionRecentlyAdded.show(view);
        };
      })(this));
    };

    Controller.prototype.renderRecentlyPlayed = function() {
      var collection;
      collection = App.request("album:recentlyplayed:entities");
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          var view;
          view = App.request("album:list:view", collection);
          return _this.page.regionRecentlyPlayed.show(view);
        };
      })(this));
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("AlbumApp.Landing", function(Landing, App, Backbone, Marionette, $, _) {
  Landing.Layout = (function(_super) {
    __extends(Layout, _super);

    function Layout() {
      return Layout.__super__.constructor.apply(this, arguments);
    }

    Layout.prototype.className = "album-landing landing-page";

    return Layout;

  })(App.Views.LayoutWithSidebarFirstView);
  return Landing.Page = (function(_super) {
    __extends(Page, _super);

    function Page() {
      return Page.__super__.constructor.apply(this, arguments);
    }

    Page.prototype.template = 'apps/album/landing/landing';

    Page.prototype.className = "album-recent";

    Page.prototype.regions = {
      regionRecentlyAdded: '.region-recently-added',
      regionRecentlyPlayed: '.region-recently-played'
    };

    return Page;

  })(App.Views.LayoutView);
});

this.Kodi.module("AlbumApp.List", function(List, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    bindTriggers: function(view) {
      App.listenTo(view, 'childview:album:play', function(list, item) {
        return App.execute('album:action', 'play', item);
      });
      App.listenTo(view, 'childview:album:add', function(list, item) {
        return App.execute('album:action', 'add', item);
      });
      App.listenTo(view, 'childview:album:localadd', function(list, item) {
        return App.execute('album:action', 'localadd', item);
      });
      return App.listenTo(view, 'childview:album:localplay', function(list, item) {
        return App.execute('album:action', 'localplay', item);
      });
    },
    getAlbumsList: function(collection, set) {
      var view, viewName;
      if (set == null) {
        set = false;
      }
      viewName = set ? 'AlbumsSet' : 'Albums';
      view = new List[viewName]({
        collection: collection
      });
      API.bindTriggers(view);
      return view;
    }
  };
  List.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function() {
      var collection;
      collection = App.request("album:entities");
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          collection.availableFilters = _this.getAvailableFilters();
          collection.sectionId = 'music';
          App.request('filter:init', _this.getAvailableFilters());
          _this.layout = _this.getLayoutView(collection);
          _this.listenTo(_this.layout, "show", function() {
            _this.renderList(collection);
            return _this.getFiltersView(collection);
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getLayoutView = function(collection) {
      return new List.ListLayout({
        collection: collection
      });
    };

    Controller.prototype.getAvailableFilters = function() {
      return {
        sort: ['label', 'year', 'rating', 'artist'],
        filter: ['year', 'genre']
      };
    };

    Controller.prototype.getFiltersView = function(collection) {
      var filters;
      filters = App.request('filter:show', collection);
      this.layout.regionSidebarFirst.show(filters);
      return this.listenTo(filters, "filter:changed", (function(_this) {
        return function() {
          return _this.renderList(collection);
        };
      })(this));
    };

    Controller.prototype.renderList = function(collection) {
      var filteredCollection, view;
      App.execute("loading:show:view", this.layout.regionContent);
      filteredCollection = App.request('filter:apply:entities', collection);
      view = API.getAlbumsList(filteredCollection);
      return this.layout.regionContent.show(view);
    };

    return Controller;

  })(App.Controllers.Base);
  return App.reqres.setHandler("album:list:view", function(collection) {
    return API.getAlbumsList(collection, true);
  });
});

this.Kodi.module("AlbumApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.ListLayout = (function(_super) {
    __extends(ListLayout, _super);

    function ListLayout() {
      return ListLayout.__super__.constructor.apply(this, arguments);
    }

    ListLayout.prototype.className = "album-list with-filters";

    return ListLayout;

  })(App.Views.LayoutWithSidebarFirstView);
  List.AlbumTeaser = (function(_super) {
    __extends(AlbumTeaser, _super);

    function AlbumTeaser() {
      return AlbumTeaser.__super__.constructor.apply(this, arguments);
    }

    AlbumTeaser.prototype.triggers = {
      "click .play": "album:play",
      "click .dropdown .add": "album:add",
      "click .dropdown .localadd": "album:localadd",
      "click .dropdown .localplay": "album:localplay"
    };

    AlbumTeaser.prototype.initialize = function() {
      var artist, artistLink;
      AlbumTeaser.__super__.initialize.apply(this, arguments);
      if (this.model != null) {
        this.model.set(App.request('album:action:items'));
        artist = this.model.get('artist') !== '' ? this.model.get('artist') : '&nbsp;';
        artistLink = this.themeLink(artist, helpers.url.get('artist', this.model.get('artistid')));
        return this.model.set({
          subtitle: artistLink
        });
      }
    };

    return AlbumTeaser;

  })(App.Views.CardView);
  List.Empty = (function(_super) {
    __extends(Empty, _super);

    function Empty() {
      return Empty.__super__.constructor.apply(this, arguments);
    }

    Empty.prototype.tagName = "li";

    Empty.prototype.className = "album-empty-result";

    return Empty;

  })(App.Views.EmptyViewResults);
  List.Albums = (function(_super) {
    __extends(Albums, _super);

    function Albums() {
      return Albums.__super__.constructor.apply(this, arguments);
    }

    Albums.prototype.childView = List.AlbumTeaser;

    Albums.prototype.emptyView = List.Empty;

    Albums.prototype.tagName = "ul";

    Albums.prototype.sort = 'artist';

    Albums.prototype.className = "card-grid--square";

    return Albums;

  })(App.Views.VirtualListView);
  return List.AlbumsSet = (function(_super) {
    __extends(AlbumsSet, _super);

    function AlbumsSet() {
      return AlbumsSet.__super__.constructor.apply(this, arguments);
    }

    AlbumsSet.prototype.childView = List.AlbumTeaser;

    AlbumsSet.prototype.emptyView = List.Empty;

    AlbumsSet.prototype.tagName = "ul";

    AlbumsSet.prototype.sort = 'artist';

    AlbumsSet.prototype.className = "card-grid--square";

    return AlbumsSet;

  })(App.Views.CollectionView);
});

this.Kodi.module("AlbumApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    bindTriggers: function(view) {
      App.listenTo(view, 'album:play', function(item) {
        return App.execute('album:action', 'play', item);
      });
      App.listenTo(view, 'album:add', function(item) {
        return App.execute('album:action', 'add', item);
      });
      App.listenTo(view, 'album:localadd', function(item) {
        return App.execute('album:action', 'localadd', item);
      });
      return App.listenTo(view, 'album:localplay', function(item) {
        return App.execute('album:action', 'localplay', item);
      });
    },
    getAlbumsFromSongs: function(songs) {
      var album, albumid, albumsCollectionView, songCollection;
      albumsCollectionView = new Show.WithSongsCollection();
      albumsCollectionView.on("add:child", (function(_this) {
        return function(albumView) {
          return App.execute("when:entity:fetched", album, function() {
            var model, songView, teaser;
            model = albumView.model;
            teaser = new Show.AlbumTeaser({
              model: model
            });
            API.bindTriggers(teaser);
            albumView.regionMeta.show(teaser);
            songView = App.request("song:list:view", songs[model.get('albumid')]);
            return albumView.regionSongs.show(songView);
          });
        };
      })(this));
      for (albumid in songs) {
        songCollection = songs[albumid];
        album = App.request("album:entity", albumid, {
          success: function(album) {
            return albumsCollectionView.addChild(album, Show.WithSongsLayout);
          }
        });
      }
      return albumsCollectionView;
    }
  };
  Show.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      var album, id;
      id = parseInt(options.id);
      album = App.request("album:entity", id);
      return App.execute("when:entity:fetched", album, (function(_this) {
        return function() {
          _this.layout = _this.getLayoutView(album);
          _this.listenTo(_this.layout, "destroy", function() {
            return App.execute("images:fanart:set", 'none');
          });
          _this.listenTo(_this.layout, "show", function() {
            _this.getMusic(id);
            return _this.getDetailsLayoutView(album);
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getLayoutView = function(album) {
      return new Show.PageLayout({
        model: album
      });
    };

    Controller.prototype.getDetailsLayoutView = function(album) {
      var headerLayout;
      headerLayout = new Show.HeaderLayout({
        model: album
      });
      this.listenTo(headerLayout, "show", (function(_this) {
        return function() {
          var detail, teaser;
          teaser = new Show.AlbumDetailTeaser({
            model: album
          });
          API.bindTriggers(teaser);
          detail = new Show.Details({
            model: album
          });
          headerLayout.regionSide.show(teaser);
          return headerLayout.regionMeta.show(detail);
        };
      })(this));
      return this.layout.regionHeader.show(headerLayout);
    };

    Controller.prototype.getMusic = function(id) {
      var options, songs;
      options = {
        filter: {
          albumid: id
        }
      };
      songs = App.request("song:filtered:entities", options);
      return App.execute("when:entity:fetched", songs, (function(_this) {
        return function() {
          var albumView, songView;
          albumView = new Show.WithSongsLayout();
          songView = App.request("song:list:view", songs);
          _this.listenTo(albumView, "show", function() {
            return albumView.regionSongs.show(songView);
          });
          return _this.layout.regionContent.show(albumView);
        };
      })(this));
    };

    return Controller;

  })(App.Controllers.Base);
  return App.reqres.setHandler("albums:withsongs:view", function(songs) {
    return API.getAlbumsFromSongs(songs);
  });
});

this.Kodi.module("AlbumApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  Show.WithSongsLayout = (function(_super) {
    __extends(WithSongsLayout, _super);

    function WithSongsLayout() {
      return WithSongsLayout.__super__.constructor.apply(this, arguments);
    }

    WithSongsLayout.prototype.template = 'apps/album/show/album_with_songs';

    WithSongsLayout.prototype.className = 'album-wrapper';

    WithSongsLayout.prototype.regions = {
      regionMeta: '.region-album-meta',
      regionSongs: '.region-album-songs'
    };

    return WithSongsLayout;

  })(App.Views.LayoutView);
  Show.WithSongsCollection = (function(_super) {
    __extends(WithSongsCollection, _super);

    function WithSongsCollection() {
      return WithSongsCollection.__super__.constructor.apply(this, arguments);
    }

    WithSongsCollection.prototype.childView = Show.WithSongsLayout;

    WithSongsCollection.prototype.tagName = "div";

    WithSongsCollection.prototype.sort = 'year';

    WithSongsCollection.prototype.className = "albums-wrapper";

    return WithSongsCollection;

  })(App.Views.CollectionView);
  Show.PageLayout = (function(_super) {
    __extends(PageLayout, _super);

    function PageLayout() {
      return PageLayout.__super__.constructor.apply(this, arguments);
    }

    PageLayout.prototype.className = 'album-show detail-container';

    return PageLayout;

  })(App.Views.LayoutWithHeaderView);
  Show.HeaderLayout = (function(_super) {
    __extends(HeaderLayout, _super);

    function HeaderLayout() {
      return HeaderLayout.__super__.constructor.apply(this, arguments);
    }

    HeaderLayout.prototype.className = 'album-details';

    return HeaderLayout;

  })(App.Views.LayoutDetailsHeaderView);
  Show.Details = (function(_super) {
    __extends(Details, _super);

    function Details() {
      return Details.__super__.constructor.apply(this, arguments);
    }

    Details.prototype.template = 'apps/album/show/details_meta';

    return Details;

  })(App.Views.ItemView);
  Show.AlbumTeaser = (function(_super) {
    __extends(AlbumTeaser, _super);

    function AlbumTeaser() {
      return AlbumTeaser.__super__.constructor.apply(this, arguments);
    }

    AlbumTeaser.prototype.tagName = "div";

    AlbumTeaser.prototype.className = "card-minimal";

    AlbumTeaser.prototype.initialize = function() {
      this.model.set({
        subtitle: this.model.get('year')
      });
      return this.model.set(App.request('album:action:items'));
    };

    return AlbumTeaser;

  })(App.AlbumApp.List.AlbumTeaser);
  return Show.AlbumDetailTeaser = (function(_super) {
    __extends(AlbumDetailTeaser, _super);

    function AlbumDetailTeaser() {
      return AlbumDetailTeaser.__super__.constructor.apply(this, arguments);
    }

    AlbumDetailTeaser.prototype.className = "card-detail";

    return AlbumDetailTeaser;

  })(Show.AlbumTeaser);
});

this.Kodi.module("ArtistApp", function(ArtistApp, App, Backbone, Marionette, $, _) {
  var API;
  ArtistApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "music/artists": "list",
      "music/artist/:id": "view"
    };

    return Router;

  })(App.Router.Base);
  API = {
    list: function() {
      return new ArtistApp.List.Controller();
    },
    view: function(id) {
      return new ArtistApp.Show.Controller({
        id: id
      });
    },
    action: function(op, view) {
      var localPlaylist, model, playlist;
      model = view.model;
      playlist = App.request("command:kodi:controller", 'audio', 'PlayList');
      switch (op) {
        case 'play':
          return App.execute("command:audio:play", 'artistid', model.get('artistid'));
        case 'add':
          return playlist.add('artistid', model.get('artistid'));
        case 'localadd':
          return App.execute("localplaylist:addentity", 'artistid', model.get('artistid'));
        case 'localplay':
          localPlaylist = App.request("command:local:controller", 'audio', 'PlayList');
          return localPlaylist.play('artistid', model.get('artistid'));
      }
    }
  };
  App.on("before:start", function() {
    return new ArtistApp.Router({
      controller: API
    });
  });
  App.commands.setHandler('artist:action', function(op, model) {
    return API.action(op, model);
  });
  return App.reqres.setHandler('artist:action:items', function() {
    return {
      actions: {
        thumbs: 'Thumbs up'
      },
      menu: {
        add: 'Add to Kodi playlist',
        localadd: 'Add to local playlist',
        divider: '',
        localplay: 'Play in browser'
      }
    };
  });
});

this.Kodi.module("ArtistApp.List", function(List, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    bindTriggers: function(view) {
      App.listenTo(view, 'childview:artist:play', function(list, item) {
        return App.execute('artist:action', 'play', item);
      });
      App.listenTo(view, 'childview:artist:add', function(list, item) {
        return App.execute('artist:action', 'add', item);
      });
      App.listenTo(view, 'childview:artist:localadd', function(list, item) {
        return App.execute('artist:action', 'localadd', item);
      });
      return App.listenTo(view, 'childview:artist:localplay', function(list, item) {
        return App.execute('artist:action', 'localplay', item);
      });
    },
    getArtistList: function(collection, set) {
      var view, viewName;
      if (set == null) {
        set = false;
      }
      viewName = set ? 'ArtistsSet' : 'Artists';
      view = new List[viewName]({
        collection: collection
      });
      API.bindTriggers(view);
      return view;
    }
  };
  List.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function() {
      var collection;
      collection = App.request("artist:entities");
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          collection.availableFilters = _this.getAvailableFilters();
          collection.sectionId = 'music';
          App.request('filter:init', _this.getAvailableFilters());
          _this.layout = _this.getLayoutView(collection);
          _this.listenTo(_this.layout, "show", function() {
            _this.renderList(collection);
            return _this.getFiltersView(collection);
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getLayoutView = function(collection) {
      return new List.ListLayout({
        collection: collection
      });
    };

    Controller.prototype.getAvailableFilters = function() {
      return {
        sort: ['label'],
        filter: ['mood', 'genre', 'style']
      };
    };

    Controller.prototype.getFiltersView = function(collection) {
      var filters;
      filters = App.request('filter:show', collection);
      this.layout.regionSidebarFirst.show(filters);
      return this.listenTo(filters, "filter:changed", (function(_this) {
        return function() {
          return _this.renderList(collection);
        };
      })(this));
    };

    Controller.prototype.renderList = function(collection) {
      var filteredCollection, view;
      App.execute("loading:show:view", this.layout.regionContent);
      filteredCollection = App.request('filter:apply:entities', collection);
      view = API.getArtistList(filteredCollection);
      return this.layout.regionContent.show(view);
    };

    return Controller;

  })(App.Controllers.Base);
  return App.reqres.setHandler("artist:list:view", function(collection) {
    return API.getArtistList(collection, true);
  });
});

this.Kodi.module("ArtistApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.ListLayout = (function(_super) {
    __extends(ListLayout, _super);

    function ListLayout() {
      return ListLayout.__super__.constructor.apply(this, arguments);
    }

    ListLayout.prototype.className = "artist-list with-filters";

    return ListLayout;

  })(App.Views.LayoutWithSidebarFirstView);
  List.ArtistTeaser = (function(_super) {
    __extends(ArtistTeaser, _super);

    function ArtistTeaser() {
      return ArtistTeaser.__super__.constructor.apply(this, arguments);
    }

    ArtistTeaser.prototype.triggers = {
      "click .play": "artist:play",
      "click .dropdown .add": "artist:add",
      "click .dropdown .localadd": "artist:localadd",
      "click .dropdown .localplay": "artist:localplay"
    };

    ArtistTeaser.prototype.initialize = function() {
      ArtistTeaser.__super__.initialize.apply(this, arguments);
      if (this.model != null) {
        return this.model.set(App.request('album:action:items'));
      }
    };

    return ArtistTeaser;

  })(App.Views.CardView);
  List.Empty = (function(_super) {
    __extends(Empty, _super);

    function Empty() {
      return Empty.__super__.constructor.apply(this, arguments);
    }

    Empty.prototype.tagName = "li";

    Empty.prototype.className = "artist-empty-result";

    return Empty;

  })(App.Views.EmptyViewResults);
  List.Artists = (function(_super) {
    __extends(Artists, _super);

    function Artists() {
      return Artists.__super__.constructor.apply(this, arguments);
    }

    Artists.prototype.childView = List.ArtistTeaser;

    Artists.prototype.emptyView = List.Empty;

    Artists.prototype.tagName = "ul";

    Artists.prototype.className = "card-grid--wide";

    return Artists;

  })(App.Views.VirtualListView);
  return List.ArtistsSet = (function(_super) {
    __extends(ArtistsSet, _super);

    function ArtistsSet() {
      return ArtistsSet.__super__.constructor.apply(this, arguments);
    }

    ArtistsSet.prototype.childView = List.ArtistTeaser;

    ArtistsSet.prototype.emptyView = List.Empty;

    ArtistsSet.prototype.tagName = "ul";

    ArtistsSet.prototype.className = "card-grid--wide";

    return ArtistsSet;

  })(App.Views.CollectionView);
});

this.Kodi.module("ArtistApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  return Show.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      var artist, id;
      id = parseInt(options.id);
      artist = App.request("artist:entity", id);
      return App.execute("when:entity:fetched", artist, (function(_this) {
        return function() {
          _this.layout = _this.getLayoutView(artist);
          _this.listenTo(_this.layout, "destroy", function() {
            return App.execute("images:fanart:set", 'none');
          });
          _this.listenTo(_this.layout, "show", function() {
            _this.getMusic(id);
            return _this.getDetailsLayoutView(artist);
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getLayoutView = function(artist) {
      return new Show.PageLayout({
        model: artist
      });
    };

    Controller.prototype.getDetailsLayoutView = function(artist) {
      var headerLayout;
      headerLayout = new Show.HeaderLayout({
        model: artist
      });
      this.listenTo(headerLayout, "show", (function(_this) {
        return function() {
          var detail, teaser;
          teaser = new Show.ArtistTeaser({
            model: artist
          });
          detail = new Show.Details({
            model: artist
          });
          headerLayout.regionSide.show(teaser);
          return headerLayout.regionMeta.show(detail);
        };
      })(this));
      return this.layout.regionHeader.show(headerLayout);
    };

    Controller.prototype.getMusic = function(id) {
      var options, songs;
      options = {
        filter: {
          artistid: id
        }
      };
      songs = App.request("song:filtered:entities", options);
      return App.execute("when:entity:fetched", songs, (function(_this) {
        return function() {
          var albumsCollection, songsCollections;
          songsCollections = App.request("song:albumparse:entities", songs);
          albumsCollection = App.request("albums:withsongs:view", songsCollections);
          return _this.layout.regionContent.show(albumsCollection);
        };
      })(this));
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("ArtistApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  Show.PageLayout = (function(_super) {
    __extends(PageLayout, _super);

    function PageLayout() {
      return PageLayout.__super__.constructor.apply(this, arguments);
    }

    PageLayout.prototype.className = 'artist-show detail-container';

    return PageLayout;

  })(App.Views.LayoutWithHeaderView);
  Show.HeaderLayout = (function(_super) {
    __extends(HeaderLayout, _super);

    function HeaderLayout() {
      return HeaderLayout.__super__.constructor.apply(this, arguments);
    }

    HeaderLayout.prototype.className = 'artist-details';

    return HeaderLayout;

  })(App.Views.LayoutDetailsHeaderView);
  Show.Details = (function(_super) {
    __extends(Details, _super);

    function Details() {
      return Details.__super__.constructor.apply(this, arguments);
    }

    Details.prototype.template = 'apps/artist/show/details_meta';

    return Details;

  })(App.Views.ItemView);
  return Show.ArtistTeaser = (function(_super) {
    __extends(ArtistTeaser, _super);

    function ArtistTeaser() {
      return ArtistTeaser.__super__.constructor.apply(this, arguments);
    }

    ArtistTeaser.prototype.tagName = "div";

    ArtistTeaser.prototype.className = "card-detail";

    return ArtistTeaser;

  })(App.ArtistApp.List.ArtistTeaser);
});

soundManager.setup({
  url: 'lib/soundmanager/swf/',
  flashVersion: 9,
  preferFlash: true,
  useHTML5Audio: true,
  useFlashBlock: false,
  flashLoadTimeout: 3000,
  debugMode: false,
  noSWFCache: true,
  debugFlash: false,
  flashPollingInterval: 1000,
  html5PollingInterval: 1000,
  onready: function() {
    return $(window).trigger('audiostream:ready');
  },
  ontimeout: function() {
    $(window).trigger('audiostream:timout');
    soundManager.flashLoadTimeout = 0;
    soundManager.onerror = {};
    return soundManager.reboot();
  }
});

this.Kodi.module("BrowserApp", function(BrowserApp, App, Backbone, Marionette, $, _) {
  var API;
  BrowserApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "browser": "list",
      "browser/:media/:id": "view"
    };

    return Router;

  })(App.Router.Base);
  API = {
    list: function() {
      return new BrowserApp.List.Controller;
    },
    view: function(media, id) {
      return new BrowserApp.List.Controller({
        media: media,
        id: id
      });
    }
  };
  return App.on("before:start", function() {
    return new BrowserApp.Router({
      controller: API
    });
  });
});

this.Kodi.module("BrowserApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.sourceCollection = {};

    Controller.prototype.backButtonModel = {};

    Controller.prototype.initialize = function(options) {
      if (options == null) {
        options = {};
      }
      this.layout = this.getLayout();
      this.listenTo(this.layout, "show", (function(_this) {
        return function() {
          _this.getSources(options);
          return _this.getFolderLayout();
        };
      })(this));
      return App.regionContent.show(this.layout);
    };

    Controller.prototype.getLayout = function() {
      return new List.ListLayout();
    };

    Controller.prototype.getFolderLayout = function() {
      this.folderLayout = new List.FolderLayout();
      return this.layout.regionContent.show(this.folderLayout);
    };

    Controller.prototype.getSources = function(options) {
      var sources;
      sources = App.request("file:source:entities", 'video');
      return App.execute("when:entity:fetched", sources, (function(_this) {
        return function() {
          var setView, sets;
          _this.sourceCollection = sources;
          sets = App.request("file:source:media:entities", sources);
          setView = new List.SourcesSet({
            collection: sets
          });
          _this.layout.regionSidebarFirst.show(setView);
          _this.listenTo(setView, 'childview:childview:source:open', function(set, item) {
            return _this.getFolder(item.model);
          });
          return _this.loadFromUrl(options);
        };
      })(this));
    };

    Controller.prototype.loadFromUrl = function(options) {
      var model;
      if (options.media && options.id) {
        model = App.request("file:url:entity", options.media, options.id);
        return this.getFolder(model);
      }
    };

    Controller.prototype.getFolder = function(model) {
      var collection, pathCollection;
      App.navigate(model.get('url'));
      collection = App.request("file:entities", {
        file: model.get('file'),
        media: model.get('media')
      });
      pathCollection = App.request("file:path:entities", model.get('file'), this.sourceCollection);
      this.getPathList(pathCollection);
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          var collections;
          collections = App.request("file:parsed:entities", collection);
          _this.getFolderList(collections.directory);
          return _this.getFileList(collections.file);
        };
      })(this));
    };

    Controller.prototype.getFolderListView = function(collection) {
      var folderView;
      folderView = new List.FolderList({
        collection: collection
      });
      this.listenTo(folderView, 'childview:folder:open', (function(_this) {
        return function(set, item) {
          return _this.getFolder(item.model);
        };
      })(this));
      this.listenTo(folderView, 'childview:folder:play', (function(_this) {
        return function(set, item) {
          var playlist;
          playlist = App.request("command:kodi:controller", item.model.get('player'), 'PlayList');
          return playlist.play('directory', item.model.get('file'));
        };
      })(this));
      return folderView;
    };

    Controller.prototype.getFolderList = function(collection) {
      this.folderLayout.regionFolders.show(this.getFolderListView(collection));
      return this.getBackButton();
    };

    Controller.prototype.getFileListView = function(collection) {
      var fileView;
      fileView = new List.FileList({
        collection: collection
      });
      this.listenTo(fileView, 'childview:file:play', (function(_this) {
        return function(set, item) {
          var playlist;
          playlist = App.request("command:kodi:controller", item.model.get('player'), 'PlayList');
          return playlist.play('file', item.model.get('file'));
        };
      })(this));
      return fileView;
    };

    Controller.prototype.getFileList = function(collection) {
      return this.folderLayout.regionFiles.show(this.getFileListView(collection));
    };

    Controller.prototype.getPathList = function(collection) {
      var pathView;
      pathView = new List.PathList({
        collection: collection
      });
      this.folderLayout.regionPath.show(pathView);
      this.setBackModel(collection);
      return this.listenTo(pathView, 'childview:folder:open', (function(_this) {
        return function(set, item) {
          return _this.getFolder(item.model);
        };
      })(this));
    };

    Controller.prototype.setBackModel = function(pathCollection) {
      if (pathCollection.length >= 2) {
        return this.backButtonModel = pathCollection.models[pathCollection.length - 2];
      } else {
        return this.backButtonModel = {};
      }
    };

    Controller.prototype.getBackButton = function() {
      var backView;
      if (this.backButtonModel.attributes) {
        backView = new List.Back({
          model: this.backButtonModel
        });
        this.folderLayout.regionBack.show(backView);
        return this.listenTo(backView, 'folder:open', (function(_this) {
          return function(model) {
            return _this.getFolder(model.model);
          };
        })(this));
      } else {
        return this.folderLayout.regionBack.empty();
      }
    };

    Controller.prototype.getFileViewByPath = function(path, media, callback) {
      var collection;
      collection = App.request("file:entities", {
        file: path,
        media: media
      });
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          var view;
          view = _this.getFileListView(collection);
          if (callback) {
            return callback(view);
          }
        };
      })(this));
    };

    return Controller;

  })(App.Controllers.Base);
  return App.reqres.setHandler("browser:files:view", function(path, media, callback) {
    var browserController;
    browserController = new List.Controller();
    return browserController.getFileViewByPath(path, media, callback);
  });
});

this.Kodi.module("BrowserApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.ListLayout = (function(_super) {
    __extends(ListLayout, _super);

    function ListLayout() {
      return ListLayout.__super__.constructor.apply(this, arguments);
    }

    ListLayout.prototype.className = "browser-page";

    return ListLayout;

  })(App.Views.LayoutWithSidebarFirstView);

  /*
    Sources
   */
  List.Source = (function(_super) {
    __extends(Source, _super);

    function Source() {
      return Source.__super__.constructor.apply(this, arguments);
    }

    Source.prototype.template = 'apps/browser/list/source';

    Source.prototype.tagName = 'li';

    Source.prototype.triggers = {
      'click .source': 'source:open'
    };

    Source.prototype.attributes = function() {
      return {
        "class": 'type-' + this.model.get('sourcetype')
      };
    };

    return Source;

  })(App.Views.ItemView);
  List.Sources = (function(_super) {
    __extends(Sources, _super);

    function Sources() {
      return Sources.__super__.constructor.apply(this, arguments);
    }

    Sources.prototype.template = 'apps/browser/list/source_set';

    Sources.prototype.childView = List.Source;

    Sources.prototype.tagName = "div";

    Sources.prototype.childViewContainer = 'ul.sources';

    Sources.prototype.className = "source-set";

    Sources.prototype.initialize = function() {
      return this.collection = this.model.get('sources');
    };

    return Sources;

  })(App.Views.CompositeView);
  List.SourcesSet = (function(_super) {
    __extends(SourcesSet, _super);

    function SourcesSet() {
      return SourcesSet.__super__.constructor.apply(this, arguments);
    }

    SourcesSet.prototype.childView = List.Sources;

    SourcesSet.prototype.tagName = "div";

    SourcesSet.prototype.className = "sources-sets";

    return SourcesSet;

  })(App.Views.CollectionView);

  /*
    Folder
   */
  List.FolderLayout = (function(_super) {
    __extends(FolderLayout, _super);

    function FolderLayout() {
      return FolderLayout.__super__.constructor.apply(this, arguments);
    }

    FolderLayout.prototype.template = 'apps/browser/list/folder_layout';

    FolderLayout.prototype.className = "folder-page-wrapper";

    FolderLayout.prototype.regions = {
      regionPath: '.path',
      regionFolders: '.folders',
      regionFiles: '.files',
      regionBack: '.back'
    };

    return FolderLayout;

  })(App.Views.LayoutView);
  List.Item = (function(_super) {
    __extends(Item, _super);

    function Item() {
      return Item.__super__.constructor.apply(this, arguments);
    }

    Item.prototype.template = 'apps/browser/list/file';

    Item.prototype.tagName = 'li';

    Item.prototype.initialize = function() {
      return this.model.set({
        label: this.formatText(this.model.get('label'))
      });
    };

    return Item;

  })(App.Views.ItemView);
  List.Folder = (function(_super) {
    __extends(Folder, _super);

    function Folder() {
      return Folder.__super__.constructor.apply(this, arguments);
    }

    Folder.prototype.className = 'folder';

    Folder.prototype.triggers = {
      'click .title': 'folder:open',
      'click .play': 'folder:play'
    };

    return Folder;

  })(List.Item);
  List.EmptyFiles = (function(_super) {
    __extends(EmptyFiles, _super);

    function EmptyFiles() {
      return EmptyFiles.__super__.constructor.apply(this, arguments);
    }

    EmptyFiles.prototype.tagName = 'li';

    EmptyFiles.prototype.initialize = function() {
      return this.model.set({
        id: 'empty',
        content: t.gettext('no media in this folder')
      });
    };

    return EmptyFiles;

  })(App.Views.EmptyViewPage);
  List.File = (function(_super) {
    __extends(File, _super);

    function File() {
      return File.__super__.constructor.apply(this, arguments);
    }

    File.prototype.className = 'file';

    File.prototype.triggers = {
      'click .play': 'file:play'
    };

    return File;

  })(List.Item);
  List.FolderList = (function(_super) {
    __extends(FolderList, _super);

    function FolderList() {
      return FolderList.__super__.constructor.apply(this, arguments);
    }

    FolderList.prototype.tagName = 'ul';

    FolderList.prototype.childView = List.Folder;

    return FolderList;

  })(App.Views.CollectionView);
  List.FileList = (function(_super) {
    __extends(FileList, _super);

    function FileList() {
      return FileList.__super__.constructor.apply(this, arguments);
    }

    FileList.prototype.tagName = 'ul';

    FileList.prototype.childView = List.File;

    FileList.prototype.emptyView = List.EmptyFiles;

    return FileList;

  })(App.Views.CollectionView);

  /*
    Path
   */
  List.Path = (function(_super) {
    __extends(Path, _super);

    function Path() {
      return Path.__super__.constructor.apply(this, arguments);
    }

    Path.prototype.template = 'apps/browser/list/path';

    Path.prototype.tagName = 'li';

    Path.prototype.triggers = {
      'click .title': 'folder:open'
    };

    return Path;

  })(App.Views.ItemView);
  List.PathList = (function(_super) {
    __extends(PathList, _super);

    function PathList() {
      return PathList.__super__.constructor.apply(this, arguments);
    }

    PathList.prototype.tagName = 'ul';

    PathList.prototype.childView = List.Path;

    return PathList;

  })(App.Views.CollectionView);
  return List.Back = (function(_super) {
    __extends(Back, _super);

    function Back() {
      return Back.__super__.constructor.apply(this, arguments);
    }

    Back.prototype.template = 'apps/browser/list/back_button';

    Back.prototype.tagName = 'div';

    Back.prototype.className = 'back-button';

    Back.prototype.triggers = {
      'click .title': 'folder:open',
      'click i': 'folder:open'
    };

    return Back;

  })(App.Views.ItemView);
});

this.Kodi.module("CastApp", function(CastApp, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    getCastCollection: function(cast, origin) {
      return App.request("cast:entities", cast, origin);
    },
    getCastView: function(collection) {
      var view;
      view = new CastApp.List.CastList({
        collection: collection
      });
      App.listenTo(view, 'childview:cast:google', function(parent, child) {
        return window.open('https://www.google.com/webhp?#q=' + encodeURIComponent(child.model.get('name')));
      });
      App.listenTo(view, 'childview:cast:imdb', function(parent, child) {
        return window.open('http://www.imdb.com/find?s=nm&q=' + encodeURIComponent(child.model.get('name')));
      });
      return view;
    }
  };
  return App.reqres.setHandler('cast:list:view', function(cast, origin) {
    var collection;
    collection = API.getCastCollection(cast, origin);
    return API.getCastView(collection);
  });
});

this.Kodi.module("CastApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.CastTeaser = (function(_super) {
    __extends(CastTeaser, _super);

    function CastTeaser() {
      return CastTeaser.__super__.constructor.apply(this, arguments);
    }

    CastTeaser.prototype.template = 'apps/cast/list/cast';

    CastTeaser.prototype.tagName = "li";

    CastTeaser.prototype.triggers = {
      "click .imdb": "cast:imdb",
      "click .google": "cast:google"
    };

    return CastTeaser;

  })(App.Views.ItemView);
  return List.CastList = (function(_super) {
    __extends(CastList, _super);

    function CastList() {
      return CastList.__super__.constructor.apply(this, arguments);
    }

    CastList.prototype.childView = List.CastTeaser;

    CastList.prototype.tagName = "ul";

    CastList.prototype.className = "cast-full";

    return CastList;

  })(App.Views.CollectionView);
});

this.Kodi.module("CommandApp", function(CommandApp, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    currentAudioPlaylistController: function() {
      var stateObj;
      stateObj = App.request("state:current");
      return App.request("command:" + stateObj.getPlayer() + ":controller", 'audio', 'PlayList');
    },
    currentVideoPlayerController: function() {
      var method, stateObj;
      stateObj = App.request("state:current");
      method = stateObj.getPlayer() === 'local' ? 'VideoPlayer' : 'PlayList';
      return App.request("command:" + stateObj.getPlayer() + ":controller", 'video', method);
    }
  };

  /*
    Kodi.
   */
  App.reqres.setHandler("command:kodi:player", function(method, params, callback) {
    var commander;
    commander = new CommandApp.Kodi.Player('auto');
    return commander.sendCommand(method, params, callback);
  });
  App.reqres.setHandler("command:kodi:controller", function(media, controller) {
    if (media == null) {
      media = 'auto';
    }
    return new CommandApp.Kodi[controller](media);
  });

  /*
    Local.
   */
  App.reqres.setHandler("command:local:player", function(method, params, callback) {
    var commander;
    commander = new CommandApp.Local.Player('audio');
    return commander.sendCommand(method, params, callback);
  });
  App.reqres.setHandler("command:local:controller", function(media, controller) {
    if (media == null) {
      media = 'auto';
    }
    return new CommandApp.Local[controller](media);
  });

  /*
    Wrappers single command for playing in kodi and local.
   */
  App.commands.setHandler("command:audio:play", function(type, value) {
    return API.currentAudioPlaylistController().play(type, value);
  });
  App.commands.setHandler("command:audio:add", function(type, value) {
    return API.currentAudioPlaylistController().add(type, value);
  });
  App.commands.setHandler("command:video:play", function(model, type, resume, callback) {
    var value;
    if (resume == null) {
      resume = 0;
    }
    value = model.get(type);
    return API.currentVideoPlayerController().play(type, value, model, resume, function(resp) {
      var kodiVideo, stateObj;
      stateObj = App.request("state:current");
      if (stateObj.getPlayer() === 'kodi') {
        kodiVideo = App.request("command:kodi:controller", 'video', 'GUI');
        return kodiVideo.setFullScreen(true, callback);
      }
    });
  });
  return App.addInitializer(function() {});
});

this.Kodi.module("CommandApp.Kodi", function(Api, App, Backbone, Marionette, $, _) {
  return Api.Base = (function(_super) {
    __extends(Base, _super);

    function Base() {
      return Base.__super__.constructor.apply(this, arguments);
    }

    Base.prototype.ajaxOptions = {};

    Base.prototype.initialize = function(options) {
      if (options == null) {
        options = {};
      }
      $.jsonrpc.defaultUrl = helpers.url.baseKodiUrl("Base");
      return this.setOptions(options);
    };

    Base.prototype.setOptions = function(options) {
      return this.ajaxOptions = options;
    };

    Base.prototype.multipleCommands = function(commands, callback) {
      var obj;
      obj = $.jsonrpc(commands, this.ajaxOptions);
      obj.fail((function(_this) {
        return function(error) {
          return _this.onError(commands, error);
        };
      })(this));
      obj.done((function(_this) {
        return function(response) {
          response = _this.parseResponse(commands, response);
          _this.triggerMethod("response:ready", response);
          if (callback != null) {
            return _this.doCallback(callback, response);
          }
        };
      })(this));
      return obj;
    };

    Base.prototype.singleCommand = function(command, params, callback) {
      var obj;
      command = {
        method: command
      };
      if ((params != null) && (params.length > 0 || _.isObject(params))) {
        command.params = params;
      }
      obj = this.multipleCommands([command], callback);
      return obj;
    };

    Base.prototype.parseResponse = function(commands, response) {
      var i, result, results;
      results = [];
      for (i in response) {
        result = response[i];
        if (result.result || result.result === false) {
          results.push(result.result);
        } else {
          this.onError(commands[i], result);
        }
      }
      if (commands.length === 1 && results.length === 1) {
        results = results[0];
      }
      return results;
    };

    Base.prototype.paramObj = function(key, val) {
      return helpers.global.paramObj(key, val);
    };

    Base.prototype.doCallback = function(callback, response) {
      if (callback != null) {
        return callback(response);
      }
    };

    Base.prototype.onError = function(commands, error) {
      return helpers.debug.rpcError(commands, error);
    };

    return Base;

  })(Marionette.Object);
});

this.Kodi.module("CommandApp.Kodi", function(Api, App, Backbone, Marionette, $, _) {
  Api.Commander = (function(_super) {
    __extends(Commander, _super);

    function Commander() {
      return Commander.__super__.constructor.apply(this, arguments);
    }

    Commander.prototype.playerActive = 0;

    Commander.prototype.playerName = 'music';

    Commander.prototype.playerForced = false;

    Commander.prototype.playerIds = {
      audio: 0,
      video: 1
    };

    Commander.prototype.setPlayer = function(player) {
      if (player === 'audio' || player === 'video') {
        this.playerActive = this.playerIds[player];
        this.playerName = player;
        return this.playerForced = true;
      }
    };

    Commander.prototype.getPlayer = function() {
      return this.playerActive;
    };

    Commander.prototype.getPlayerName = function() {
      return this.playerName;
    };

    Commander.prototype.playerIdToName = function(playerId) {
      playerName;
      var id, name, playerName, _ref;
      _ref = this.playerIds;
      for (name in _ref) {
        id = _ref[name];
        if (id === playerId) {
          playerName = name;
        }
      }
      return playerName;
    };

    Commander.prototype.commandNameSpace = 'JSONRPC';

    Commander.prototype.getCommand = function(command, namespace) {
      if (namespace == null) {
        namespace = this.commandNameSpace;
      }
      return namespace + '.' + command;
    };

    Commander.prototype.sendCommand = function(command, params, callback) {
      return this.singleCommand(this.getCommand(command), params, (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    return Commander;

  })(Api.Base);
  return Api.Player = (function(_super) {
    __extends(Player, _super);

    function Player() {
      return Player.__super__.constructor.apply(this, arguments);
    }

    Player.prototype.commandNameSpace = 'Player';

    Player.prototype.playlistApi = {};

    Player.prototype.initialize = function(media) {
      if (media == null) {
        media = 'audio';
      }
      this.setPlayer(media);
      return this.playlistApi = App.request("playlist:kodi:entity:api");
    };

    Player.prototype.getParams = function(params, callback) {
      var defaultParams;
      if (params == null) {
        params = [];
      }
      if (this.playerForced) {
        defaultParams = [this.playerActive];
        return this.doCallback(callback, defaultParams.concat(params));
      } else {
        return this.getActivePlayers((function(_this) {
          return function(activeId) {
            defaultParams = [activeId];
            return _this.doCallback(callback, defaultParams.concat(params));
          };
        })(this));
      }
    };

    Player.prototype.getActivePlayers = function(callback) {
      return this.singleCommand(this.getCommand("GetActivePlayers"), {}, (function(_this) {
        return function(resp) {
          if (resp.length > 0) {
            _this.playerActive = resp[0].playerid;
            _this.playerName = _this.playerIdToName(_this.playerActive);
            _this.triggerMethod("player:ready", _this.playerActive);
            return _this.doCallback(callback, _this.playerActive);
          } else {
            return _this.doCallback(callback, _this.playerActive);
          }
        };
      })(this));
    };

    Player.prototype.sendCommand = function(command, params, callback) {
      if (params == null) {
        params = [];
      }
      return this.getParams(params, (function(_this) {
        return function(playerParams) {
          return _this.singleCommand(_this.getCommand(command), playerParams, function(resp) {
            return _this.doCallback(callback, resp);
          });
        };
      })(this));
    };

    Player.prototype.playEntity = function(type, value, options, callback) {
      var params;
      if (options == null) {
        options = {};
      }
      params = {
        'item': this.paramObj(type, value),
        'options': options
      };
      if (type === 'position') {
        params.item.playlistid = this.getPlayer();
      }
      return this.singleCommand(this.getCommand('Open', 'Player'), params, (function(_this) {
        return function(resp) {
          if (!App.request('sockets:active')) {
            App.request('state:kodi:update');
          }
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    Player.prototype.getPlaying = function(callback) {
      var obj;
      obj = {
        active: false,
        properties: false,
        item: false
      };
      return this.singleCommand(this.getCommand('GetActivePlayers'), {}, (function(_this) {
        return function(resp) {
          var commands, itemFields, playerFields;
          if (resp.length > 0) {
            obj.active = resp[0];
            commands = [];
            itemFields = helpers.entities.getFields(_this.playlistApi.fields, 'full');
            playerFields = ["playlistid", "speed", "position", "totaltime", "time", "percentage", "shuffled", "repeat", "canrepeat", "canshuffle", "canseek", "partymode"];
            commands.push({
              method: _this.getCommand('GetProperties'),
              params: [obj.active.playerid, playerFields]
            });
            commands.push({
              method: _this.getCommand('GetItem'),
              params: [obj.active.playerid, itemFields]
            });
            return _this.multipleCommands(commands, function(playing) {
              obj.properties = playing[0];
              obj.item = playing[1].item;
              return _this.doCallback(callback, obj);
            });
          } else {
            return _this.doCallback(callback, false);
          }
        };
      })(this));
    };

    return Player;

  })(Api.Commander);
});

this.Kodi.module("CommandApp.Kodi", function(Api, App, Backbone, Marionette, $, _) {
  return Api.AddOn = (function(_super) {
    __extends(AddOn, _super);

    function AddOn() {
      this.getAllAddons = __bind(this.getAllAddons, this);
      this.getEnabledAddons = __bind(this.getEnabledAddons, this);
      return AddOn.__super__.constructor.apply(this, arguments);
    }

    AddOn.prototype.commandNameSpace = 'Addons';

    AddOn.prototype.addonAllFields = ["name", "version", "summary", "description", "path", "author", "thumbnail", "disclaimer", "fanart", "dependencies", "broken", "extrainfo", "rating", "enabled"];

    AddOn.prototype.getAddons = function(type, enabled, fields, callback) {
      if (type == null) {
        type = "unknown";
      }
      if (enabled == null) {
        enabled = true;
      }
      if (fields == null) {
        fields = [];
      }
      return this.singleCommand(this.getCommand('GetAddons'), [type, "unknown", enabled, fields], (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp.addons);
        };
      })(this));
    };

    AddOn.prototype.getEnabledAddons = function(load, callback) {
      var fields;
      if (load == null) {
        load = true;
      }
      fields = load ? this.addonAllFields : ["name"];
      return this.getAddons("unknown", true, fields, (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    AddOn.prototype.getAllAddons = function(callback) {
      return this.getAddons("unknown", "all", this.addonAllFields, (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    return AddOn;

  })(Api.Commander);
});

this.Kodi.module("CommandApp.Kodi", function(Api, App, Backbone, Marionette, $, _) {
  return Api.Application = (function(_super) {
    __extends(Application, _super);

    function Application() {
      return Application.__super__.constructor.apply(this, arguments);
    }

    Application.prototype.commandNameSpace = 'Application';

    Application.prototype.getProperties = function(callback) {
      return this.singleCommand(this.getCommand('GetProperties'), [["volume", "muted", "version"]], (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    Application.prototype.setVolume = function(volume, callback) {
      return this.singleCommand(this.getCommand('SetVolume'), [volume], (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    Application.prototype.toggleMute = function(callback) {
      var stateObj;
      stateObj = App.request("state:kodi");
      return this.singleCommand(this.getCommand('SetMute'), [!stateObj.getState('muted')], (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    Application.prototype.quit = function(callback) {
      return this.singleCommand(this.getCommand('Quit'), [], (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    return Application;

  })(Api.Commander);
});

this.Kodi.module("CommandApp.Kodi", function(Api, App, Backbone, Marionette, $, _) {
  return Api.AudioLibrary = (function(_super) {
    __extends(AudioLibrary, _super);

    function AudioLibrary() {
      return AudioLibrary.__super__.constructor.apply(this, arguments);
    }

    AudioLibrary.prototype.commandNameSpace = 'AudioLibrary';

    AudioLibrary.prototype.setAlbumDetails = function(id, fields, callback) {
      var params;
      if (fields == null) {
        fields = {};
      }
      params = {
        albumid: id
      };
      params = _.extend(params, fields);
      return this.singleCommand(this.getCommand('SetAlbumDetails'), params, (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    AudioLibrary.prototype.setArtistDetails = function(id, fields, callback) {
      var params;
      if (fields == null) {
        fields = {};
      }
      params = {
        artistid: id
      };
      params = _.extend(params, fields);
      return this.singleCommand(this.getCommand('SetArtistDetails'), params, (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    AudioLibrary.prototype.setArtistDetails = function(id, fields, callback) {
      var params;
      if (fields == null) {
        fields = {};
      }
      params = {
        songid: id
      };
      params = _.extend(params, fields);
      return this.singleCommand(this.getCommand('SetSongDetails'), params, (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    AudioLibrary.prototype.scan = function(callback) {
      return this.singleCommand(this.getCommand('Scan'), (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    return AudioLibrary;

  })(Api.Commander);
});

this.Kodi.module("CommandApp.Kodi", function(Api, App, Backbone, Marionette, $, _) {
  return Api.Files = (function(_super) {
    __extends(Files, _super);

    function Files() {
      return Files.__super__.constructor.apply(this, arguments);
    }

    Files.prototype.commandNameSpace = 'Files';

    Files.prototype.prepareDownload = function(file, callback) {
      return this.singleCommand(this.getCommand('PrepareDownload'), [file], (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    Files.prototype.downloadPath = function(file, callback) {
      return this.prepareDownload(file, (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp.details.path);
        };
      })(this));
    };

    Files.prototype.downloadFile = function(file) {
      var dl;
      dl = window.open('about:blank', 'download');
      return this.downloadPath(file, function(path) {
        return dl.location = path;
      });
    };

    Files.prototype.videoStream = function(file, background, player) {
      var st;
      if (background == null) {
        background = '';
      }
      if (player == null) {
        player = 'html5';
      }
      st = helpers.global.localVideoPopup('about:blank');
      return this.downloadPath(file, function(path) {
        return st.location = "videoPlayer.html?player=" + player + '&src=' + encodeURIComponent(path) + '&bg=' + encodeURIComponent(background);
      });
    };

    return Files;

  })(Api.Commander);
});

this.Kodi.module("CommandApp.Kodi", function(Api, App, Backbone, Marionette, $, _) {
  return Api.GUI = (function(_super) {
    __extends(GUI, _super);

    function GUI() {
      return GUI.__super__.constructor.apply(this, arguments);
    }

    GUI.prototype.commandNameSpace = 'GUI';

    GUI.prototype.setFullScreen = function(fullscreen, callback) {
      if (fullscreen == null) {
        fullscreen = true;
      }
      return this.sendCommand("SetFullscreen", [fullscreen], (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    GUI.prototype.activateWindow = function(window, params, callback) {
      if (params == null) {
        params = [];
      }
      return this.sendCommand("ActivateWindow", [window, params], (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    return GUI;

  })(Api.Commander);
});

this.Kodi.module("CommandApp.Kodi", function(Api, App, Backbone, Marionette, $, _) {
  return Api.Input = (function(_super) {
    __extends(Input, _super);

    function Input() {
      return Input.__super__.constructor.apply(this, arguments);
    }

    Input.prototype.commandNameSpace = 'Input';

    Input.prototype.sendText = function(text, callback) {
      return this.singleCommand(this.getCommand('SendText'), [text], (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    Input.prototype.sendInput = function(type, params, callback) {
      if (params == null) {
        params = [];
      }
      return this.singleCommand(this.getCommand(type), params, (function(_this) {
        return function(resp) {
          _this.doCallback(callback, resp);
          if (!App.request('sockets:active')) {
            return App.request('state:kodi:update', callback);
          }
        };
      })(this));
    };

    return Input;

  })(Api.Commander);
});

this.Kodi.module("CommandApp.Kodi", function(Api, App, Backbone, Marionette, $, _) {
  return Api.PlayList = (function(_super) {
    __extends(PlayList, _super);

    function PlayList() {
      return PlayList.__super__.constructor.apply(this, arguments);
    }

    PlayList.prototype.commandNameSpace = 'Playlist';

    PlayList.prototype.play = function(type, value, model, resume, callback) {
      var stateObj;
      if (resume == null) {
        resume = 0;
      }
      stateObj = App.request("state:kodi");
      if (stateObj.isPlaying()) {
        return this.insertAndPlay(type, value, stateObj.getPlaying('position') + 1, resume, callback);
      } else {
        return this.clear((function(_this) {
          return function() {
            return _this.insertAndPlay(type, value, 0, resume, callback);
          };
        })(this));
      }
    };

    PlayList.prototype.playCollection = function(collection, position) {
      if (position == null) {
        position = 0;
      }
      return this.clear((function(_this) {
        return function() {
          var commands, i, model, models, params, player, pos, type;
          models = collection.getRawCollection();
          player = _this.getPlayer();
          commands = [];
          for (i in models) {
            model = models[i];
            pos = parseInt(position) + parseInt(i);
            type = model.type === 'file' ? 'file' : model.type + 'id';
            params = [player, pos, _this.paramObj(type, model[type])];
            commands.push({
              method: _this.getCommand('Insert'),
              params: params
            });
          }
          return _this.multipleCommands(commands, function(resp) {
            return _this.playEntity('position', parseInt(position), {}, function() {
              return _this.refreshPlaylistView();
            });
          });
        };
      })(this));
    };

    PlayList.prototype.add = function(type, value) {
      return this.playlistSize((function(_this) {
        return function(size) {
          return _this.insert(type, value, size);
        };
      })(this));
    };

    PlayList.prototype.remove = function(position, callback) {
      return this.singleCommand(this.getCommand('Remove'), [this.getPlayer(), parseInt(position)], (function(_this) {
        return function(resp) {
          _this.refreshPlaylistView();
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    PlayList.prototype.clear = function(callback) {
      return this.singleCommand(this.getCommand('Clear'), [this.getPlayer()], (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    PlayList.prototype.insert = function(type, value, position, callback) {
      if (position == null) {
        position = 0;
      }
      return this.singleCommand(this.getCommand('Insert'), [this.getPlayer(), parseInt(position), this.paramObj(type, value)], (function(_this) {
        return function(resp) {
          _this.refreshPlaylistView();
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    PlayList.prototype.getItems = function(callback) {
      return this.singleCommand(this.getCommand('GetItems'), [this.getPlayer(), ['title']], (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    PlayList.prototype.insertAndPlay = function(type, value, position, resume, callback) {
      if (position == null) {
        position = 0;
      }
      if (resume == null) {
        resume = 0;
      }
      return this.insert(type, value, position, (function(_this) {
        return function(resp) {
          return _this.playEntity('position', parseInt(position), {}, function() {
            if (resume > 0) {
              App.execute("player:kodi:progress:update", resume);
            }
            return _this.doCallback(callback, resp);
          });
        };
      })(this));
    };

    PlayList.prototype.playlistSize = function(callback) {
      return this.getItems((function(_this) {
        return function(resp) {
          var position;
          position = resp.items != null ? resp.items.length : 0;
          return _this.doCallback(callback, position);
        };
      })(this));
    };

    PlayList.prototype.refreshPlaylistView = function() {
      var wsActive;
      wsActive = App.request("sockets:active");
      if (!wsActive) {
        return App.execute("playlist:refresh", 'kodi', this.playerName);
      }
    };

    PlayList.prototype.moveItem = function(media, id, position1, position2, callback) {
      var idProp;
      idProp = media === 'file' ? 'file' : media + 'id';
      return this.singleCommand(this.getCommand('Remove'), [this.getPlayer(), parseInt(position1)], (function(_this) {
        return function(resp) {
          return _this.insert(idProp, id, position2, function() {
            return _this.doCallback(callback, position2);
          });
        };
      })(this));
    };

    return PlayList;

  })(Api.Player);
});

this.Kodi.module("CommandApp.Kodi", function(Api, App, Backbone, Marionette, $, _) {
  return Api.PVR = (function(_super) {
    __extends(PVR, _super);

    function PVR() {
      return PVR.__super__.constructor.apply(this, arguments);
    }

    PVR.prototype.commandNameSpace = 'PVR';

    PVR.prototype.setPVRRecord = function(id, fields, callback) {
      var params;
      if (fields == null) {
        fields = {};
      }
      params = {
        channel: id
      };
      params = _.extend(params, fields);
      return this.singleCommand(this.getCommand('Record'), params, (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    return PVR;

  })(Api.Commander);
});

this.Kodi.module("CommandApp.Kodi", function(Api, App, Backbone, Marionette, $, _) {
  return Api.VideoLibrary = (function(_super) {
    __extends(VideoLibrary, _super);

    function VideoLibrary() {
      return VideoLibrary.__super__.constructor.apply(this, arguments);
    }

    VideoLibrary.prototype.commandNameSpace = 'VideoLibrary';

    VideoLibrary.prototype.setEpisodeDetails = function(id, fields, callback) {
      var params;
      if (fields == null) {
        fields = {};
      }
      params = {
        episodeid: id
      };
      params = _.extend(params, fields);
      return this.singleCommand(this.getCommand('SetEpisodeDetails'), params, (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    VideoLibrary.prototype.setMovieDetails = function(id, fields, callback) {
      var params;
      if (fields == null) {
        fields = {};
      }
      params = {
        movieid: id
      };
      params = _.extend(params, fields);
      return this.singleCommand(this.getCommand('SetMovieDetails'), params, (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    VideoLibrary.prototype.scan = function(callback) {
      return this.singleCommand(this.getCommand('Scan'), (function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp);
        };
      })(this));
    };

    VideoLibrary.prototype.toggleWatched = function(model, callback) {
      var fields, setPlaycount;
      setPlaycount = model.get('playcount') > 0 ? 0 : 1;
      fields = helpers.global.paramObj('playcount', setPlaycount);
      if (model.get('type') === 'movie') {
        this.setMovieDetails(model.get('id'), fields, (function(_this) {
          return function() {
            helpers.cache.updateCollection('MovieCollection', 'movies', model.get('id'), 'playcount', setPlaycount);
            return _this.doCallback(callback, setPlaycount);
          };
        })(this));
      }
      if (model.get('type') === 'episode') {
        return this.setEpisodeDetails(model.get('id'), fields, (function(_this) {
          return function() {
            helpers.cache.updateCollection('TVShowCollection', 'tvshows', model.get('tvshowid'), 'playcount', setPlaycount);
            return _this.doCallback(callback, setPlaycount);
          };
        })(this));
      }
    };

    return VideoLibrary;

  })(Api.Commander);
});

this.Kodi.module("CommandApp.Local", function(Api, App, Backbone, Marionette, $, _) {
  return Api.Base = (function(_super) {
    __extends(Base, _super);

    function Base() {
      return Base.__super__.constructor.apply(this, arguments);
    }

    Base.prototype.localLoad = function(model, callback) {
      var files, stateObj;
      stateObj = App.request("state:local");
      if (model == null) {
        stateObj.setPlaying('playing', false);
        this.localStateUpdate();
        return;
      }
      stateObj.setState('currentPlaybackId', 'browser-' + model.get('id'));
      files = App.request("command:kodi:controller", 'video', 'Files');
      return files.downloadPath(model.get('file'), (function(_this) {
        return function(path) {
          var sm;
          sm = soundManager;
          _this.localStop();
          stateObj.setState('localPlay', sm.createSound({
            id: stateObj.getState('currentPlaybackId'),
            url: path,
            autoplay: false,
            autoLoad: true,
            stream: true,
            onerror: function() {
              return console.log('SM ERROR!');
            },
            onplay: function() {
              stateObj.setPlayer('local');
              stateObj.setPlaying('playing', true);
              stateObj.setPlaying('paused', false);
              stateObj.setPlaying('playState', 'playing');
              stateObj.setPlaying('position', model.get('position'));
              stateObj.setPlaying('itemChanged', true);
              stateObj.setPlaying('item', model.attributes);
              stateObj.setPlaying('totaltime', helpers.global.secToTime(model.get('duration')));
              return _this.localStateUpdate();
            },
            onstop: function() {
              stateObj.setPlaying('playing', false);
              return _this.localStateUpdate();
            },
            onpause: function() {
              stateObj.setPlaying('paused', true);
              stateObj.setPlaying('playState', 'paused');
              return _this.localStateUpdate();
            },
            onresume: function() {
              stateObj.setPlaying('paused', false);
              stateObj.setPlaying('playState', 'playing');
              return _this.localStateUpdate();
            },
            onfinish: function() {
              return _this.localFinished();
            },
            whileplaying: function() {
              var dur, percentage, pos;
              pos = parseInt(this.position) / 1000;
              dur = parseInt(model.get('duration'));
              percentage = Math.round((pos / dur) * 100);
              stateObj.setPlaying('time', helpers.global.secToTime(pos));
              stateObj.setPlaying('percentage', percentage);
              return App.execute('player:local:progress:update', percentage, helpers.global.secToTime(pos));
            }
          }));
          return _this.doCallback(callback);
        };
      })(this));
    };

    Base.prototype.localFinished = function() {
      return this.localGoTo('next');
    };

    Base.prototype.localPlay = function() {
      return this.localCommand('play');
    };

    Base.prototype.localStop = function() {
      return this.localCommand('stop');
    };

    Base.prototype.localPause = function() {
      return this.localCommand('pause');
    };

    Base.prototype.localPlayPause = function() {
      var stateObj;
      stateObj = App.request("state:local");
      if (stateObj.getPlaying('paused')) {
        return this.localCommand('play');
      } else {
        return this.localCommand('pause');
      }
    };

    Base.prototype.localSetVolume = function(volume) {
      return this.localCommand('setVolume', volume);
    };

    Base.prototype.localCommand = function(command, param) {
      var currentItem, stateObj;
      stateObj = App.request("state:local");
      currentItem = stateObj.getState('localPlay');
      if (currentItem !== false) {
        currentItem[command](param);
      }
      return this.localStateUpdate();
    };

    Base.prototype.localGoTo = function(param) {
      var collection, currentPos, model, posToPlay, stateObj;
      collection = App.request("localplayer:get:entities");
      stateObj = App.request("state:local");
      currentPos = stateObj.getPlaying('position');
      posToPlay = false;
      if (collection.length > 0) {
        if (stateObj.getState('repeat') === 'one') {
          posToPlay = currentPos;
        } else if (stateObj.getState('shuffled') === true) {
          posToPlay = helpers.global.getRandomInt(0, collection.length - 1);
        } else {
          if (param === 'next') {
            if (currentPos === collection.length - 1 && stateObj.getState('repeat') === 'all') {
              posToPlay = 0;
            } else if (currentPos < collection.length) {
              posToPlay = currentPos + 1;
            }
          }
          if (param === 'previous') {
            if (currentPos === 0 && stateObj.getState('repeat') === 'all') {
              posToPlay = collection.length - 1;
            } else if (currentPos > 0) {
              posToPlay = currentPos - 1;
            }
          }
        }
      }
      if (posToPlay !== false) {
        model = collection.findWhere({
          position: posToPlay
        });
        return this.localLoad(model, (function(_this) {
          return function() {
            _this.localPlay();
            return _this.localStateUpdate();
          };
        })(this));
      }
    };

    Base.prototype.localSeek = function(percent) {
      var localPlay, newPos, sound, stateObj;
      stateObj = App.request("state:local");
      localPlay = stateObj.getState('localPlay');
      if (localPlay !== false) {
        newPos = (percent / 100) * localPlay.duration;
        sound = soundManager.getSoundById(stateObj.getState('currentPlaybackId'));
        return sound.setPosition(newPos);
      }
    };

    Base.prototype.localRepeat = function(param) {
      var i, key, newState, state, stateObj, states;
      stateObj = App.request("state:local");
      if (param !== 'cycle') {
        return stateObj.setState('repeat', param);
      } else {
        newState = false;
        states = ['off', 'all', 'one'];
        for (i in states) {
          state = states[i];
          i = parseInt(i);
          if (newState !== false) {
            continue;
          }
          if (stateObj.getState('repeat') === state) {
            if (i !== (states.length - 1)) {
              key = i + 1;
              newState = states[key];
            } else {
              newState = 'off';
            }
          }
        }
        return stateObj.setState('repeat', newState);
      }
    };

    Base.prototype.localShuffle = function() {
      var currentShuffle, stateObj;
      stateObj = App.request("state:local");
      currentShuffle = stateObj.getState('shuffled');
      return stateObj.setState('shuffled', !currentShuffle);
    };

    Base.prototype.localStateUpdate = function() {
      return App.vent.trigger("state:local:changed");
    };

    Base.prototype.paramObj = function(key, val) {
      return helpers.global.paramObj(key, val);
    };

    Base.prototype.doCallback = function(callback, response) {
      if (typeof callback === 'function') {
        return callback(response);
      }
    };

    Base.prototype.onError = function(commands, error) {
      return helpers.debug.rpcError(commands, error);
    };

    return Base;

  })(Marionette.Object);
});

this.Kodi.module("CommandApp.Local", function(Api, App, Backbone, Marionette, $, _) {
  Api.Commander = (function(_super) {
    __extends(Commander, _super);

    function Commander() {
      return Commander.__super__.constructor.apply(this, arguments);
    }

    return Commander;

  })(Api.Base);
  return Api.Player = (function(_super) {
    __extends(Player, _super);

    function Player() {
      return Player.__super__.constructor.apply(this, arguments);
    }

    Player.prototype.playEntity = function(type, position, callback) {
      var collection, model;
      if (type == null) {
        type = 'position';
      }
      collection = App.request("localplayer:get:entities");
      model = collection.findWhere({
        position: position
      });
      return this.localLoad(model, (function(_this) {
        return function() {
          _this.localPlay();
          return _this.doCallback(callback, position);
        };
      })(this));
    };

    Player.prototype.sendCommand = function(command, param) {
      switch (command) {
        case 'GoTo':
          this.localGoTo(param);
          break;
        case 'PlayPause':
          this.localPlayPause();
          break;
        case 'Seek':
          this.localSeek(param);
          break;
        case 'SetRepeat':
          this.localRepeat(param);
          break;
        case 'SetShuffle':
          this.localShuffle();
          break;
        case 'Stop':
          this.localStop();
          break;
      }
      return this.localStateUpdate();
    };

    return Player;

  })(Api.Commander);
});

this.Kodi.module("CommandApp.Local", function(Api, App, Backbone, Marionette, $, _) {
  return Api.Application = (function(_super) {
    __extends(Application, _super);

    function Application() {
      return Application.__super__.constructor.apply(this, arguments);
    }

    Application.prototype.getProperties = function(callback) {
      var resp, stateObj;
      stateObj = App.request("state:local");
      resp = {
        volume: stateObj.getState('volume'),
        muted: stateObj.getState('muted')
      };
      return this.doCallback(callback, resp);
    };

    Application.prototype.setVolume = function(volume, callback) {
      var stateObj;
      stateObj = App.request("state:local");
      stateObj.setState('volume', volume);
      this.localSetVolume(volume);
      return this.doCallback(callback, volume);
    };

    Application.prototype.toggleMute = function(callback) {
      var stateObj, volume;
      stateObj = App.request("state:local");
      volume = 0;
      if (stateObj.getState('muted')) {
        volume = stateObj.getState('lastVolume');
        stateObj.setState('muted', false);
      } else {
        stateObj.setState('lastVolume', stateObj.getState('volume'));
        stateObj.setState('muted', true);
        volume = 0;
      }
      this.localSetVolume(volume);
      return this.doCallback(callback, volume);
    };

    return Application;

  })(Api.Commander);
});

this.Kodi.module("CommandApp.Local", function(Api, App, Backbone, Marionette, $, _) {
  return Api.PlayList = (function(_super) {
    __extends(PlayList, _super);

    function PlayList() {
      return PlayList.__super__.constructor.apply(this, arguments);
    }

    PlayList.prototype.play = function(type, value, resume) {
      if (resume == null) {
        resume = 0;
      }
      return this.getSongs(type, value, (function(_this) {
        return function(songs) {
          return _this.playCollection(songs);
        };
      })(this));
    };

    PlayList.prototype.add = function(type, value) {
      return this.getSongs(type, value, (function(_this) {
        return function(songs) {
          return _this.addCollection(songs);
        };
      })(this));
    };

    PlayList.prototype.playCollection = function(models) {
      if (!_.isArray(models)) {
        models = models.getRawCollection();
      }
      return this.clear((function(_this) {
        return function() {
          return _this.insertAndPlay(models, 0);
        };
      })(this));
    };

    PlayList.prototype.addCollection = function(models) {
      return this.playlistSize((function(_this) {
        return function(size) {
          return _this.insert(models, size);
        };
      })(this));
    };

    PlayList.prototype.remove = function(position, callback) {
      return this.getItems((function(_this) {
        return function(collection) {
          var item, pos, raw, ret;
          raw = collection.getRawCollection();
          ret = [];
          for (pos in raw) {
            item = raw[pos];
            if (parseInt(pos) !== parseInt(position)) {
              ret.push(item);
            }
          }
          return _this.clear(function() {
            collection = _this.addItems(ret);
            return _this.doCallback(callback, collection);
          });
        };
      })(this));
    };

    PlayList.prototype.clear = function(callback) {
      var collection;
      collection = App.execute("localplayer:clear:entities");
      this.refreshPlaylistView();
      return this.doCallback(callback, collection);
    };

    PlayList.prototype.insert = function(models, position, callback) {
      if (position == null) {
        position = 0;
      }
      return this.getItems((function(_this) {
        return function(collection) {
          var item, model, pos, raw, ret, _i, _j, _len, _len1, _ref, _ref1;
          raw = collection.getRawCollection();
          if (raw.length === 0) {
            ret = _.flatten([models]);
          } else if (parseInt(position) >= raw.length) {
            ret = raw;
            _ref = _.flatten([models]);
            for (_i = 0, _len = _ref.length; _i < _len; _i++) {
              model = _ref[_i];
              ret.push(model);
            }
          } else {
            ret = [];
            for (pos in raw) {
              item = raw[pos];
              if (parseInt(pos) === parseInt(position)) {
                _ref1 = _.flatten([models]);
                for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
                  model = _ref1[_j];
                  ret.push(model);
                }
              }
              ret.push(item);
            }
          }
          return _this.clear(function() {
            collection = _this.addItems(ret);
            return _this.doCallback(callback, collection);
          });
        };
      })(this));
    };

    PlayList.prototype.addItems = function(items) {
      App.request("localplayer:item:add:entities", items);
      return this.refreshPlaylistView();
    };

    PlayList.prototype.getSongs = function(type, value, callback) {
      var songs;
      if (type === 'songid') {
        return App.request("song:byid:entities", [value], (function(_this) {
          return function(songs) {
            return _this.doCallback(callback, songs.getRawCollection());
          };
        })(this));
      } else {
        songs = App.request("song:filtered:entities", {
          filter: helpers.global.paramObj(type, value)
        });
        return App.execute("when:entity:fetched", songs, (function(_this) {
          return function() {
            return _this.doCallback(callback, songs.getRawCollection());
          };
        })(this));
      }
    };

    PlayList.prototype.getItems = function(callback) {
      var collection;
      collection = App.request("localplayer:get:entities");
      return this.doCallback(callback, collection);
    };

    PlayList.prototype.insertAndPlay = function(models, position, callback) {
      if (position == null) {
        position = 0;
      }
      return this.insert(models, position, (function(_this) {
        return function(resp) {
          return _this.playEntity('position', parseInt(position), {}, function() {
            return _this.doCallback(callback, position);
          });
        };
      })(this));
    };

    PlayList.prototype.playlistSize = function(callback) {
      return this.getItems((function(_this) {
        return function(resp) {
          return _this.doCallback(callback, resp.length);
        };
      })(this));
    };

    PlayList.prototype.refreshPlaylistView = function() {
      return App.execute("playlist:refresh", 'local', 'audio');
    };

    PlayList.prototype.moveItem = function(media, id, position1, position2, callback) {
      return this.getItems((function(_this) {
        return function(collection) {
          var item, raw;
          raw = collection.getRawCollection();
          item = raw[position1];
          return _this.remove(position1, function() {
            return _this.insert(item, position2, function() {
              return _this.doCallback(callback, position2);
            });
          });
        };
      })(this));
    };

    return PlayList;

  })(Api.Player);
});

this.Kodi.module("CommandApp.Local", function(Api, App, Backbone, Marionette, $, _) {
  return Api.VideoPlayer = (function(_super) {
    __extends(VideoPlayer, _super);

    function VideoPlayer() {
      return VideoPlayer.__super__.constructor.apply(this, arguments);
    }

    VideoPlayer.prototype.getKodiFilesController = function() {
      return new App.CommandApp.Kodi.Files;
    };

    VideoPlayer.prototype.play = function(type, value, model) {
      return this.videoStream(model.get('file'), model.get('fanart'));
    };

    VideoPlayer.prototype.videoStream = function(file, background, player) {
      var st;
      if (background == null) {
        background = '';
      }
      if (player == null) {
        player = 'html5';
      }
      st = helpers.global.localVideoPopup('about:blank');
      return this.getKodiFilesController().downloadPath(file, function(path) {
        return st.location = "videoPlayer.html?player=" + player + '&src=' + encodeURIComponent(path) + '&bg=' + encodeURIComponent(background);
      });
    };

    return VideoPlayer;

  })(Api.Player);
});

this.Kodi.module("EPGApp", function(EPGApp, App, Backbone, Marionette, $, _) {
  var API;
  EPGApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "tvshows/live/:channelid": "tv",
      "music/radio/:channelid": "radio"
    };

    return Router;

  })(App.Router.Base);
  API = {
    tv: function(channelid) {
      return new EPGApp.List.Controller({
        channelid: channelid,
        type: "tv"
      });
    },
    radio: function(channelid) {
      return new EPGApp.List.Controller({
        channelid: channelid,
        type: "radio"
      });
    }
  };
  return App.on("before:start", function() {
    return new EPGApp.Router({
      controller: API
    });
  });
});

this.Kodi.module("EPGApp.List", function(List, App, Backbone, Marionette, $, _) {
  return List.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      var collection;
      collection = App.request("broadcast:entities", options.channelid);
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          _this.layout = _this.getLayoutView(collection);
          _this.listenTo(_this.layout, "show", function() {
            _this.getSubNav();
            return _this.renderProgrammes(collection);
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getLayoutView = function(collection) {
      return new List.Layout({
        collection: collection
      });
    };

    Controller.prototype.renderProgrammes = function(collection) {
      var view;
      view = new List.EPGList({
        collection: collection
      });

      /*@listenTo view, 'childview:channel:play', (parent, child) ->
        player = App.request "command:kodi:controller", 'auto', 'Player'
        player.playEntity 'channelid', child.model.get('id'), {},  =>
           *# update state?
       */
      return this.layout.regionContent.show(view);
    };

    Controller.prototype.getSubNav = function() {
      var subNav, subNavId;
      subNavId = this.getOption('type') === 'tv' ? 'tvshows/recent' : 'music';
      subNav = App.request("navMain:children:show", subNavId, 'Sections');
      return this.layout.regionSidebarFirst.show(subNav);
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("EPGApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.Layout = (function(_super) {
    __extends(Layout, _super);

    function Layout() {
      return Layout.__super__.constructor.apply(this, arguments);
    }

    Layout.prototype.className = "epg-page";

    return Layout;

  })(App.Views.LayoutWithSidebarFirstView);
  List.ProgrammeList = (function(_super) {
    __extends(ProgrammeList, _super);

    function ProgrammeList() {
      return ProgrammeList.__super__.constructor.apply(this, arguments);
    }

    ProgrammeList.prototype.template = 'apps/epg/list/programmes';

    ProgrammeList.prototype.tagName = "li";

    ProgrammeList.prototype.className = "programme";

    ProgrammeList.prototype.onRender = function() {
      if (this.model.attributes.wasactive) {
        return this.$el.addClass("aired");
      }
    };

    return ProgrammeList;

  })(App.Views.ItemView);
  return List.EPGList = (function(_super) {
    __extends(EPGList, _super);

    function EPGList() {
      return EPGList.__super__.constructor.apply(this, arguments);
    }

    EPGList.prototype.childView = List.ProgrammeList;

    EPGList.prototype.tagName = "ul";

    EPGList.prototype.className = "programmes";

    EPGList.prototype.onShow = function() {
      return $(window).scrollTop(this.$el.find('.airing').offset().top - 150);
    };

    return EPGList;

  })(App.Views.CollectionView);
});

this.Kodi.module("ExternalApp", function(ExternalApp, App, Backbone, Marionette, $, _) {});

this.Kodi.module("ExternalApp.Youtube", function(Youtube, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    getSearchView: function(query, callback) {
      return App.execute("youtube:search:entities", query, function(collection) {
        var view;
        view = new Youtube.List({
          collection: collection
        });
        App.listenTo(view, 'childview:youtube:kodiplay', function(parent, item) {
          var playlist;
          playlist = App.request("command:kodi:controller", 'video', 'PlayList');
          return playlist.play('file', 'plugin://plugin.video.youtube/play/?video_id=' + item.model.get('id'));
        });
        App.listenTo(view, 'childview:youtube:localplay', function(parent, item) {
          var localPlayer;
          localPlayer = "videoPlayer.html?yt=" + item.model.get('id');
          return helpers.global.localVideoPopup(localPlayer, 500);
        });
        return callback(view);
      });
    }
  };
  return App.commands.setHandler("youtube:search:view", function(query, callback) {
    return API.getSearchView(query, callback);
  });
});

this.Kodi.module("ExternalApp.Youtube", function(Youtube, App, Backbone, Marionette, $, _) {
  Youtube.Item = (function(_super) {
    __extends(Item, _super);

    function Item() {
      return Item.__super__.constructor.apply(this, arguments);
    }

    Item.prototype.template = 'apps/external/youtube/youtube';

    Item.prototype.tagName = 'li';

    Item.prototype.triggers = {
      'click .play-kodi': 'youtube:kodiplay',
      'click .play-local': 'youtube:localplay'
    };

    Item.prototype.events = {
      'click .action': 'closeModal'
    };

    Item.prototype.closeModal = function() {
      return App.execute("ui:modal:close");
    };

    return Item;

  })(App.Views.ItemView);
  return Youtube.List = (function(_super) {
    __extends(List, _super);

    function List() {
      return List.__super__.constructor.apply(this, arguments);
    }

    List.prototype.childView = Youtube.Item;

    List.prototype.tagName = 'ul';

    List.prototype.className = 'youtube-list';

    return List;

  })(App.Views.CollectionView);
});

this.Kodi.module("FilterApp", function(FilterApp, App, Backbone, Marionette, $, _) {
  var API;
  API = {

    /*
      Settings/fields
     */
    sortFields: [
      {
        alias: 'Title',
        type: 'string',
        defaultSort: true,
        defaultOrder: 'asc',
        key: 'title'
      }, {
        alias: 'Title',
        type: 'string',
        defaultSort: true,
        defaultOrder: 'asc',
        key: 'label'
      }, {
        alias: 'Year',
        type: 'number',
        key: 'year',
        defaultOrder: 'desc'
      }, {
        alias: 'Date added',
        type: 'string',
        key: 'dateadded',
        defaultOrder: 'desc'
      }, {
        alias: 'Rating',
        type: 'float',
        key: 'rating',
        defaultOrder: 'desc'
      }, {
        alias: 'Artist',
        type: 'string',
        key: 'artist',
        defaultOrder: 'asc'
      }
    ],
    filterFields: [
      {
        alias: 'Year',
        type: 'number',
        key: 'year',
        sortOrder: 'desc',
        filterCallback: 'multiple'
      }, {
        alias: 'Genre',
        type: 'array',
        key: 'genre',
        sortOrder: 'asc',
        filterCallback: 'multiple'
      }, {
        alias: 'Mood',
        type: 'array',
        key: 'mood',
        sortOrder: 'asc',
        filterCallback: 'multiple'
      }, {
        alias: 'Style',
        type: 'array',
        key: 'style',
        sortOrder: 'asc',
        filterCallback: 'multiple'
      }, {
        alias: 'Unwatched',
        type: "boolean",
        key: 'unwatched',
        sortOrder: 'asc',
        filterCallback: 'unwatched'
      }, {
        alias: 'Writer',
        type: 'array',
        key: 'writer',
        sortOrder: 'asc',
        filterCallback: 'multiple'
      }, {
        alias: 'Director',
        type: 'array',
        key: 'director',
        sortOrder: 'asc',
        filterCallback: 'multiple'
      }, {
        alias: 'Actor',
        type: 'object',
        property: 'name',
        key: 'cast',
        sortOrder: 'asc',
        filterCallback: 'multiple'
      }, {
        alias: 'Set',
        type: 'string',
        property: 'set',
        key: 'set',
        sortOrder: 'asc',
        filterCallback: 'multiple'
      }
    ],
    getFilterFields: function(type) {
      var available, availableFilters, field, fields, key, ret, _i, _len;
      key = type + 'Fields';
      fields = API[key];
      availableFilters = API.getAvailable();
      available = availableFilters[type];
      ret = [];
      for (_i = 0, _len = fields.length; _i < _len; _i++) {
        field = fields[_i];
        if (helpers.global.inArray(field.key, available)) {
          ret.push(field);
        }
      }
      return ret;
    },

    /*
      Storage
     */
    storeFiltersNamespace: 'filter:store:',
    getStoreNameSpace: function(type) {
      return API.storeFiltersNamespace + type;
    },
    setStoreFilters: function(filters, type) {
      var store;
      if (filters == null) {
        filters = {};
      }
      if (type == null) {
        type = 'filters';
      }
      store = {};
      store[helpers.url.path()] = filters;
      helpers.cache.set(API.getStoreNameSpace(type), store);
      return App.vent.trigger('filter:changed', filters);
    },
    getStoreFilters: function(type) {
      var filters, key, path, ret, store, val;
      if (type == null) {
        type = 'filters';
      }
      store = helpers.cache.get(API.getStoreNameSpace(type), {});
      path = helpers.url.path();
      filters = store[path] ? store[path] : {};
      ret = {};
      if (!_.isEmpty(filters)) {
        for (key in filters) {
          val = filters[key];
          if (val.length > 0) {
            ret[key] = val;
          }
        }
      }
      return ret;
    },
    updateStoreFiltersKey: function(key, values) {
      var filters;
      if (values == null) {
        values = [];
      }
      filters = API.getStoreFilters();
      filters[key] = values;
      API.setStoreFilters(filters);
      return filters;
    },
    getStoreFiltersKey: function(key) {
      var filter, filters;
      filters = API.getStoreFilters();
      filter = filters[key] ? filters[key] : [];
      return filter;
    },
    setStoreSort: function(method, order) {
      var sort;
      if (order == null) {
        order = 'asc';
      }
      sort = {
        method: method,
        order: order
      };
      return API.setStoreFilters(sort, 'sort');
    },
    getStoreSort: function() {
      var defaults, sort;
      sort = API.getStoreFilters('sort');
      if (!sort.method) {
        defaults = _.findWhere(API.getFilterFields('sort'), {
          defaultSort: true
        });
        sort = {
          method: defaults.key,
          order: defaults.defaultOrder
        };
      }
      return sort;
    },
    setAvailable: function(available) {
      return API.setStoreFilters(available, 'available');
    },
    getAvailable: function() {
      return API.getStoreFilters('available');
    },

    /*
      Parsing
     */
    toggleOrder: function(order) {
      order = order === 'asc' ? 'desc' : 'asc';
      return order;
    },
    parseSortable: function(items) {
      var i, item, params;
      params = API.getStoreSort(false, 'asc');
      for (i in items) {
        item = items[i];
        items[i].active = false;
        items[i].order = item.defaultOrder;
        if (params.method && item.key === params.method) {
          items[i].active = true;
          items[i].order = this.toggleOrder(params.order);
        } else if (item.defaultSort && params.method === false) {
          items[i].active = true;
        }
      }
      return items;
    },
    parseFilterable: function(items) {
      var active, activeItem, i, val;
      active = API.getFilterActive();
      for (i in items) {
        val = items[i];
        activeItem = _.findWhere(active, {
          key: val.key
        });
        items[i].active = activeItem !== void 0;
      }
      return items;
    },
    getFilterOptions: function(key, collection) {
      var collectionItems, data, i, item, items, limited, s, values, _i, _len;
      values = App.request('filter:store:key:get', key);
      s = API.getFilterSettings(key);
      items = [];
      collectionItems = collection.pluck(key);
      if (s.filterCallback === 'multiple' && s.type === 'object') {
        limited = [];
        for (_i = 0, _len = collectionItems.length; _i < _len; _i++) {
          item = collectionItems[_i];
          for (i in item) {
            data = item[i];
            if (i < 5) {
              limited.push(data[s.property]);
            }
          }
        }
        collectionItems = limited;
      }
      _.map(_.uniq(_.flatten(collectionItems)), function(val) {
        return items.push({
          key: key,
          value: val,
          active: helpers.global.inArray(val, values)
        });
      });
      return items;
    },

    /*
      Apply filters
     */
    applyFilters: function(collection) {
      var filteredCollection, key, sort, values, _ref;
      sort = API.getStoreSort();
      collection.sortCollection(sort.method, sort.order);
      filteredCollection = new App.Entities.Filtered(collection);
      _ref = API.getStoreFilters();
      for (key in _ref) {
        values = _ref[key];
        if (values.length > 0) {
          filteredCollection = API.applyFilter(filteredCollection, key, values);
        }
      }
      return filteredCollection;
    },
    applyFilter: function(collection, key, vals) {
      var s;
      s = API.getFilterSettings(key);
      switch (s.filterCallback) {
        case 'multiple':
          if (s.type === 'array') {
            collection.filterByMultipleArray(key, vals);
          } else if (s.type === 'object') {
            collection.filterByMultipleObject(key, s.property, vals);
          } else {
            collection.filterByMultiple(key, vals);
          }
          break;
        case 'unwatched':
          collection.filterByUnwatched();
          break;
        default:
          collection;
      }
      return collection;
    },
    getFilterSettings: function(key) {
      return _.findWhere(API.getFilterFields('filter'), {
        key: key
      });
    },
    getFilterActive: function() {
      var items, key, values, _ref;
      items = [];
      _ref = API.getStoreFilters();
      for (key in _ref) {
        values = _ref[key];
        if (values.length > 0) {
          items.push({
            key: key,
            values: values
          });
        }
      }
      return items;
    }
  };

  /*
    Handlers.
   */
  App.reqres.setHandler('filter:show', function(collection) {
    var filters, view;
    API.setAvailable(collection.availableFilters);
    filters = new FilterApp.Show.Controller({
      refCollection: collection
    });
    view = filters.getFilterView();
    return view;
  });
  App.reqres.setHandler('filter:options', function(key, collection) {
    var filterSettings, options, optionsCollection;
    options = API.getFilterOptions(key, collection);
    optionsCollection = App.request('filter:filters:options:entities', options);
    filterSettings = API.getFilterSettings(key);
    optionsCollection.sortCollection('value', filterSettings.sortOrder);
    return optionsCollection;
  });
  App.reqres.setHandler('filter:active', function() {
    return App.request('filter:active:entities', API.getFilterActive());
  });
  App.reqres.setHandler('filter:apply:entities', function(collection) {
    API.setAvailable(collection.availableFilters);
    return API.applyFilters(collection);
  });
  App.reqres.setHandler('filter:sortable:entities', function() {
    return App.request('filter:sort:entities', API.parseSortable(API.getFilterFields('sort')));
  });
  App.reqres.setHandler('filter:filterable:entities', function() {
    return App.request('filter:filters:entities', API.parseFilterable(API.getFilterFields('filter')));
  });
  App.reqres.setHandler('filter:init', function(availableFilters) {
    var key, params, values, _i, _len, _ref, _results;
    params = helpers.url.params();
    if (!_.isEmpty(params)) {
      API.setStoreFilters({});
      _ref = availableFilters.filter;
      _results = [];
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        key = _ref[_i];
        if (params[key]) {
          values = API.getStoreFiltersKey(key);
          if (!helpers.global.inArray(params[key], values)) {
            values.push(decodeURIComponent(params[key]));
            _results.push(API.updateStoreFiltersKey(key, values));
          } else {
            _results.push(void 0);
          }
        } else {
          _results.push(void 0);
        }
      }
      return _results;
    }
  });
  App.reqres.setHandler('filter:store:set', function(filters) {
    API.setStoreFilters(filters);
    return filters;
  });
  App.reqres.setHandler('filter:store:get', function() {
    return API.getStoreFilters();
  });
  App.reqres.setHandler('filter:store:key:get', function(key) {
    return API.getStoreFiltersKey(key);
  });
  App.reqres.setHandler('filter:store:key:update', function(key, values) {
    if (values == null) {
      values = [];
    }
    return API.updateStoreFiltersKey(key, values);
  });
  App.reqres.setHandler('filter:store:key:toggle', function(key, value) {
    var i, newValues, ret, values, _i, _len;
    values = API.getStoreFiltersKey(key);
    ret = [];
    if (_.indexOf(values, value) > -1) {
      newValues = [];
      for (_i = 0, _len = values.length; _i < _len; _i++) {
        i = values[_i];
        if (i !== value) {
          newValues.push(i);
        }
      }
      ret = newValues;
    } else {
      values.push(value);
      ret = values;
    }
    API.updateStoreFiltersKey(key, ret);
    return ret;
  });
  App.reqres.setHandler('filter:sort:store:set', function(method, order) {
    if (order == null) {
      order = 'asc';
    }
    return API.setStoreSort(method, order);
  });
  return App.reqres.setHandler('filter:sort:store:get', function() {
    return API.getStoreSort();
  });
});

this.Kodi.module("FilterApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  return Show.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.getFilterView = function() {
      var collection;
      collection = this.getOption('refCollection');
      this.layoutFilters = this.getLayoutView(collection);
      this.listenTo(this.layoutFilters, "show", (function(_this) {
        return function() {
          _this.getSort();
          _this.getFilters();
          _this.getActive();
          return _this.getSections();
        };
      })(this));
      this.listenTo(this.layoutFilters, 'filter:layout:close:filters', (function(_this) {
        return function() {
          return _this.stateChange('normal');
        };
      })(this));
      this.listenTo(this.layoutFilters, 'filter:layout:close:options', (function(_this) {
        return function() {
          return _this.stateChange('filters');
        };
      })(this));
      this.listenTo(this.layoutFilters, 'filter:layout:open:filters', (function(_this) {
        return function() {
          return _this.stateChange('filters');
        };
      })(this));
      this.listenTo(this.layoutFilters, 'filter:layout:open:options', (function(_this) {
        return function() {
          return _this.stateChange('options');
        };
      })(this));
      return this.layoutFilters;
    };

    Controller.prototype.getLayoutView = function(collection) {
      return new Show.FilterLayout({
        collection: collection
      });
    };

    Controller.prototype.getSort = function() {
      var sortCollection, sortView;
      sortCollection = App.request('filter:sortable:entities');
      sortView = new Show.SortList({
        collection: sortCollection
      });
      this.layoutFilters.regionSort.show(sortView);
      return App.listenTo(sortView, "childview:filter:sortable:select", (function(_this) {
        return function(parentview, childview) {
          App.request('filter:sort:store:set', childview.model.get('key'), childview.model.get('order'));
          _this.layoutFilters.trigger('filter:changed');
          return _this.getSort();
        };
      })(this));
    };

    Controller.prototype.getFilters = function(clearOptions) {
      var filterCollection, filtersView;
      if (clearOptions == null) {
        clearOptions = true;
      }
      filterCollection = App.request('filter:filterable:entities');
      filtersView = new Show.FilterList({
        collection: filterCollection
      });
      App.listenTo(filtersView, "childview:filter:filterable:select", (function(_this) {
        return function(parentview, childview) {
          var key;
          key = childview.model.get('key');
          if (childview.model.get('type') === 'boolean') {
            App.request('filter:store:key:toggle', key, childview.model.get('alias'));
            return _this.triggerChange();
          } else {
            _this.getFilterOptions(key);
            return _this.stateChange('options');
          }
        };
      })(this));
      this.layoutFilters.regionFiltersList.show(filtersView);
      if (clearOptions) {
        return this.layoutFilters.regionFiltersOptions.empty();
      }
    };

    Controller.prototype.getActive = function() {
      var activeCollection, optionsView;
      activeCollection = App.request('filter:active');
      optionsView = new Show.ActiveList({
        collection: activeCollection
      });
      this.layoutFilters.regionFiltersActive.show(optionsView);
      App.listenTo(optionsView, "childview:filter:option:remove", (function(_this) {
        return function(parentview, childview) {
          var key;
          key = childview.model.get('key');
          App.request('filter:store:key:update', key, []);
          return _this.triggerChange();
        };
      })(this));
      App.listenTo(optionsView, "childview:filter:add", (function(_this) {
        return function(parentview, childview) {
          return _this.stateChange('filters');
        };
      })(this));
      return this.getFilterBar();
    };

    Controller.prototype.getFilterOptions = function(key) {
      var optionsCollection, optionsView;
      optionsCollection = App.request('filter:options', key, this.getOption('refCollection'));
      optionsView = new Show.OptionList({
        collection: optionsCollection
      });
      this.layoutFilters.regionFiltersOptions.show(optionsView);
      App.listenTo(optionsView, "childview:filter:option:select", (function(_this) {
        return function(parentview, childview) {
          var value;
          value = childview.model.get('value');
          childview.view.$el.find('.option').toggleClass('active');
          App.request('filter:store:key:toggle', key, value);
          return _this.triggerChange(false);
        };
      })(this));
      return App.listenTo(optionsView, 'filter:option:deselectall', (function(_this) {
        return function(parentview) {
          parentview.view.$el.find('.option').removeClass('active');
          App.request('filter:store:key:update', key, []);
          return _this.triggerChange(false);
        };
      })(this));
    };

    Controller.prototype.triggerChange = function(clearOptions) {
      if (clearOptions == null) {
        clearOptions = true;
      }
      this.getFilters(clearOptions);
      this.getActive();
      App.navigate(helpers.url.path());
      return this.layoutFilters.trigger('filter:changed');
    };

    Controller.prototype.getFilterBar = function() {
      var $list, $wrapper, bar, currentFilters, list;
      currentFilters = App.request('filter:store:get');
      list = _.flatten(_.values(currentFilters));
      $wrapper = $('.layout-container');
      $list = $('.region-content-top', $wrapper);
      if (list.length > 0) {
        bar = new Show.FilterBar({
          filters: list
        });
        $list.html(bar.render().$el);
        $wrapper.addClass('filters-active');
        return App.listenTo(bar, 'filter:remove:all', (function(_this) {
          return function() {
            App.request('filter:store:set', {});
            _this.triggerChange();
            return _this.stateChange('normal');
          };
        })(this));
      } else {
        return $wrapper.removeClass('filters-active');
      }
    };

    Controller.prototype.stateChange = function(state) {
      var $wrapper;
      if (state == null) {
        state = 'normal';
      }
      $wrapper = this.layoutFilters.$el.find('.filters-container');
      switch (state) {
        case 'filters':
          return $wrapper.removeClass('show-options').addClass('show-filters');
        case 'options':
          return $wrapper.addClass('show-options').removeClass('show-filters');
        default:
          return $wrapper.removeClass('show-options').removeClass('show-filters');
      }
    };

    Controller.prototype.getSections = function() {
      var collection, nav;
      collection = this.getOption('refCollection');
      if (collection.sectionId) {
        nav = App.request("navMain:children:show", collection.sectionId, 'Sections');
        return this.layoutFilters.regionNavSection.show(nav);
      }
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("FilterApp.Show", function(Show, App, Backbone, Marionette, $, _) {

  /*
    Base.
   */
  Show.FilterLayout = (function(_super) {
    __extends(FilterLayout, _super);

    function FilterLayout() {
      return FilterLayout.__super__.constructor.apply(this, arguments);
    }

    FilterLayout.prototype.template = 'apps/filter/show/filters_ui';

    FilterLayout.prototype.className = "side-bar";

    FilterLayout.prototype.regions = {
      regionSort: '.sort-options',
      regionFiltersActive: '.filters-active',
      regionFiltersList: '.filters-list',
      regionFiltersOptions: '.filter-options-list',
      regionNavSection: '.nav-section'
    };

    FilterLayout.prototype.triggers = {
      'click .close-filters': 'filter:layout:close:filters',
      'click .close-options': 'filter:layout:close:options',
      'click .open-filters': 'filter:layout:open:filters'
    };

    return FilterLayout;

  })(App.Views.LayoutView);
  Show.ListItem = (function(_super) {
    __extends(ListItem, _super);

    function ListItem() {
      return ListItem.__super__.constructor.apply(this, arguments);
    }

    ListItem.prototype.template = 'apps/filter/show/list_item';

    ListItem.prototype.tagName = 'li';

    return ListItem;

  })(App.Views.ItemView);
  Show.List = (function(_super) {
    __extends(List, _super);

    function List() {
      return List.__super__.constructor.apply(this, arguments);
    }

    List.prototype.childView = Show.ListItem;

    List.prototype.tagName = "ul";

    List.prototype.className = "selection-list";

    return List;

  })(App.Views.CollectionView);

  /*
    Extends.
   */
  Show.SortListItem = (function(_super) {
    __extends(SortListItem, _super);

    function SortListItem() {
      return SortListItem.__super__.constructor.apply(this, arguments);
    }

    SortListItem.prototype.triggers = {
      "click .sortable": "filter:sortable:select"
    };

    SortListItem.prototype.initialize = function() {
      var classes, tag;
      classes = ['option', 'sortable'];
      if (this.model.get('active') === true) {
        classes.push('active');
      }
      classes.push('order-' + this.model.get('order'));
      tag = this.themeTag('span', {
        'class': classes.join(' ')
      }, this.model.get('alias'));
      return this.model.set({
        title: tag
      });
    };

    return SortListItem;

  })(Show.ListItem);
  Show.SortList = (function(_super) {
    __extends(SortList, _super);

    function SortList() {
      return SortList.__super__.constructor.apply(this, arguments);
    }

    SortList.prototype.childView = Show.SortListItem;

    return SortList;

  })(Show.List);
  Show.FilterListItem = (function(_super) {
    __extends(FilterListItem, _super);

    function FilterListItem() {
      return FilterListItem.__super__.constructor.apply(this, arguments);
    }

    FilterListItem.prototype.triggers = {
      "click .filterable": "filter:filterable:select"
    };

    FilterListItem.prototype.initialize = function() {
      var classes, tag;
      classes = ['option', 'option filterable'];
      if (this.model.get('active')) {
        classes.push('active');
      }
      tag = this.themeTag('span', {
        'class': classes.join(' ')
      }, this.model.get('alias'));
      return this.model.set({
        title: tag
      });
    };

    return FilterListItem;

  })(Show.ListItem);
  Show.FilterList = (function(_super) {
    __extends(FilterList, _super);

    function FilterList() {
      return FilterList.__super__.constructor.apply(this, arguments);
    }

    FilterList.prototype.childView = Show.FilterListItem;

    return FilterList;

  })(Show.List);
  Show.OptionListItem = (function(_super) {
    __extends(OptionListItem, _super);

    function OptionListItem() {
      return OptionListItem.__super__.constructor.apply(this, arguments);
    }

    OptionListItem.prototype.triggers = {
      "click .filterable-option": "filter:option:select"
    };

    OptionListItem.prototype.initialize = function() {
      var classes, tag;
      classes = ['option', 'option filterable-option'];
      if (this.model.get('active')) {
        classes.push('active');
      }
      tag = this.themeTag('span', {
        'class': classes.join(' ')
      }, this.model.get('value'));
      return this.model.set({
        title: tag
      });
    };

    return OptionListItem;

  })(Show.ListItem);
  Show.OptionList = (function(_super) {
    __extends(OptionList, _super);

    function OptionList() {
      return OptionList.__super__.constructor.apply(this, arguments);
    }

    OptionList.prototype.template = 'apps/filter/show/filter_options';

    OptionList.prototype.activeValues = [];

    OptionList.prototype.childView = Show.OptionListItem;

    OptionList.prototype.childViewContainer = 'ul.selection-list';

    OptionList.prototype.onRender = function() {
      if (this.collection.length <= 10) {
        $('.options-search-wrapper', this.$el).addClass('hidden');
      }
      return $('.options-search', this.$el).filterList();
    };

    OptionList.prototype.triggers = {
      'click .deselect-all': 'filter:option:deselectall'
    };

    return OptionList;

  })(App.Views.CompositeView);
  Show.ActiveListItem = (function(_super) {
    __extends(ActiveListItem, _super);

    function ActiveListItem() {
      return ActiveListItem.__super__.constructor.apply(this, arguments);
    }

    ActiveListItem.prototype.triggers = {
      "click .filterable-remove": "filter:option:remove"
    };

    ActiveListItem.prototype.initialize = function() {
      var tag, text, tooltip;
      tooltip = t.gettext('Remove') + ' ' + t.gettext(this.model.get('key')) + ' ' + t.gettext('filter');
      text = this.themeTag('span', {
        'class': 'text'
      }, this.model.get('values').join(', '));
      tag = this.themeTag('span', {
        'class': 'filter-btn filterable-remove',
        title: tooltip
      }, text);
      return this.model.set({
        title: tag
      });
    };

    return ActiveListItem;

  })(Show.ListItem);
  Show.ActiveNewListItem = (function(_super) {
    __extends(ActiveNewListItem, _super);

    function ActiveNewListItem() {
      return ActiveNewListItem.__super__.constructor.apply(this, arguments);
    }

    ActiveNewListItem.prototype.triggers = {
      "click .filterable-add": "filter:add"
    };

    ActiveNewListItem.prototype.initialize = function() {
      var tag;
      tag = this.themeTag('span', {
        'class': 'filter-btn filterable-add'
      }, t.gettext('Add filter'));
      return this.model.set({
        title: tag
      });
    };

    return ActiveNewListItem;

  })(Show.ListItem);
  Show.ActiveList = (function(_super) {
    __extends(ActiveList, _super);

    function ActiveList() {
      return ActiveList.__super__.constructor.apply(this, arguments);
    }

    ActiveList.prototype.childView = Show.ActiveListItem;

    ActiveList.prototype.emptyView = Show.ActiveNewListItem;

    ActiveList.prototype.className = "active-list";

    return ActiveList;

  })(Show.List);
  return Show.FilterBar = (function(_super) {
    __extends(FilterBar, _super);

    function FilterBar() {
      return FilterBar.__super__.constructor.apply(this, arguments);
    }

    FilterBar.prototype.template = 'apps/filter/show/filters_bar';

    FilterBar.prototype.className = "filters-active-bar";

    FilterBar.prototype.onRender = function() {
      if (this.options.filters) {
        return $('.filters-active-all', this.$el).html(this.options.filters.join(', '));
      }
    };

    FilterBar.prototype.triggers = {
      'click .remove': 'filter:remove:all'
    };

    return FilterBar;

  })(App.Views.ItemView);
});

this.Kodi.module("HelpApp", function(HelpApp, App, Backbone, Marionette, $, _) {
  var API;
  HelpApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "help": "helpOverview",
      "help/overview": "helpOverview",
      "help/:id": "helpPage"
    };

    return Router;

  })(App.Router.Base);
  API = {
    helpOverview: function() {
      return new App.HelpApp.Overview.Controller();
    },
    helpPage: function(id) {
      return new HelpApp.Show.Controller({
        id: id
      });
    },
    getPage: function(id, lang, callback) {
      var content;
      if (lang == null) {
        lang = 'en';
      }
      content = $.get("lang/" + lang + "/" + id + ".html");
      content.fail((function(_this) {
        return function(error) {
          if (lang === !'en') {
            return _this.getPage(id, 'en', callback);
          }
        };
      })(this));
      content.done(function(data) {
        return callback(data);
      });
      return content;
    },
    getSubNav: function() {
      var collection;
      collection = App.request("navMain:array:entities", this.getSideBarSructure());
      return App.request("navMain:collection:show", collection, t.gettext('Help topics'));
    },
    getSideBarSructure: function() {
      return [
        {
          title: t.gettext('About'),
          path: 'help'
        }, {
          title: t.gettext('Readme'),
          path: 'help/app-readme'
        }, {
          title: t.gettext('Changelog'),
          path: 'help/app-changelog'
        }, {
          title: t.gettext('Keyboard'),
          path: 'help/keybind-readme'
        }, {
          title: t.gettext('Translations'),
          path: 'help/lang-readme'
        }
      ];
    }
  };
  App.reqres.setHandler('help:subnav', function() {
    return API.getSubNav();
  });
  App.reqres.setHandler('help:page', function(id, callback) {
    var lang;
    lang = config.getLocal('lang', 'en');
    return API.getPage(id, lang, callback);
  });
  return App.on("before:start", function() {
    return new HelpApp.Router({
      controller: API
    });
  });
});

this.Kodi.module("HelpApp.Overview", function(Overview, App, Backbone, Marionette, $, _) {
  return Overview.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      return App.request("help:page", 'help-overview', (function(_this) {
        return function(data) {
          _this.layout = _this.getLayoutView(data);
          _this.listenTo(_this.layout, "show", function() {
            _this.getSideBar();
            return _this.getPage(data);
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getPage = function(data) {
      this.pageView = new Overview.Page({
        data: data
      });
      this.listenTo(this.pageView, "show", (function(_this) {
        return function() {
          return _this.getReport();
        };
      })(this));
      return this.layout.regionContent.show(this.pageView);
    };

    Controller.prototype.getSideBar = function() {
      var subNav;
      subNav = App.request("help:subnav");
      return this.layout.regionSidebarFirst.show(subNav);
    };

    Controller.prototype.getLayoutView = function() {
      return new Overview.Layout();
    };

    Controller.prototype.getReport = function() {
      this.$pageView = this.pageView.$el;
      this.getReportChorusVersion();
      this.getReportKodiVersion();
      this.getReportWebsocketsActive();
      this.getReportLocalAudio();
      App.vent.on("sockets:available", (function(_this) {
        return function() {
          return _this.getReportWebsocketsActive();
        };
      })(this));
      return App.vent.on("state:initialized", (function(_this) {
        return function() {
          return _this.getReportKodiVersion();
        };
      })(this));
    };

    Controller.prototype.getReportChorusVersion = function() {
      return $.get("addon.xml", (function(_this) {
        return function(data) {
          return $('.report-chorus-version > span', _this.$pageView).html($('addon', data).attr('version'));
        };
      })(this));
    };

    Controller.prototype.getReportKodiVersion = function() {
      var kodiVersion, state;
      state = App.request("state:kodi");
      kodiVersion = state.getState('version');
      return $('.report-kodi-version > span', this.$pageView).html(kodiVersion.major + '.' + kodiVersion.minor);
    };

    Controller.prototype.getReportWebsocketsActive = function() {
      var $ws, wsActive;
      wsActive = App.request("sockets:active");
      $ws = $('.report-websockets', this.$pageView);
      if (wsActive) {
        $('span', $ws).html(tr("Remote control is set up correctly"));
        return $ws.removeClass('warning');
      } else {
        $('span', $ws).html(tr("You need to 'Allow remote control' for Kodi. You can do that") + ' <a href="#settings/kodi/services">' + tr('here') + '</a>');
        return $ws.addClass('warning');
      }
    };

    Controller.prototype.getReportLocalAudio = function() {
      var localAudio;
      localAudio = soundManager.useHTML5Audio ? "HTML 5" : "Flash";
      return $('.report-local-audio > span', this.$pageView).html(localAudio);
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("HelpApp.Overview", function(Overview, App, Backbone, Marionette, $, _) {
  Overview.Page = (function(_super) {
    __extends(Page, _super);

    function Page() {
      return Page.__super__.constructor.apply(this, arguments);
    }

    Page.prototype.className = "help--overview";

    Page.prototype.template = 'apps/help/overview/overview';

    Page.prototype.tagName = "div";

    Page.prototype.onRender = function() {
      return $('.help--overview--header', this.$el).html(this.options.data);
    };

    return Page;

  })(App.Views.CompositeView);
  return Overview.Layout = (function(_super) {
    __extends(Layout, _super);

    function Layout() {
      return Layout.__super__.constructor.apply(this, arguments);
    }

    Layout.prototype.className = "help--page help--overview page-wrapper";

    return Layout;

  })(App.Views.LayoutWithSidebarFirstView);
});

this.Kodi.module("HelpApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  return Show.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      return App.request("help:page", options.id, (function(_this) {
        return function(data) {
          _this.layout = _this.getLayoutView(data);
          _this.listenTo(_this.layout, "show", function() {
            return _this.getSideBar();
          });
          App.regionContent.show(_this.layout);
          if (options.pageView) {
            return _this.layout.regionContent.show(options.pageView);
          }
        };
      })(this));
    };

    Controller.prototype.getSideBar = function() {
      var subNav;
      subNav = App.request("help:subnav");
      return this.layout.regionSidebarFirst.show(subNav);
    };

    Controller.prototype.getLayoutView = function(data) {
      return new Show.Layout({
        data: data,
        pageView: this.options.pageView
      });
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("HelpApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  return Show.Layout = (function(_super) {
    __extends(Layout, _super);

    function Layout() {
      return Layout.__super__.constructor.apply(this, arguments);
    }

    Layout.prototype.className = "help--page page-wrapper";

    Layout.prototype.onRender = function() {
      return $(this.regionContent.el, this.$el).html(this.options.data);
    };

    return Layout;

  })(App.Views.LayoutWithSidebarFirstView);
});

this.Kodi.module("Images", function(Images, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    imagesPath: 'images/',
    defaultFanartPath: 'fanart_default/',
    defaultFanartFiles: ['buds.jpg', 'cans.jpg', 'guitar.jpg', 'speaker.jpg', 'turntable.jpg'],
    getDefaultThumbnail: function() {
      return API.imagesPath + 'thumbnail_default.png';
    },
    getRandomFanart: function() {
      var file, path, rand;
      rand = helpers.global.getRandomInt(0, API.defaultFanartFiles.length - 1);
      file = API.defaultFanartFiles[rand];
      path = API.imagesPath + API.defaultFanartPath + file;
      return path;
    },
    parseRawPath: function(rawPath) {
      var path;
      path = config.getLocal('reverseProxy') ? 'image/' + encodeURIComponent(rawPath) : '/image/' + encodeURIComponent(rawPath);
      return path;
    },
    setFanartBackground: function(path, region) {
      var $body;
      $body = App.getRegion(region).$el;
      if (path !== 'none') {
        if (!path) {
          path = this.getRandomFanart();
        }
        return $body.css('background-image', 'url(' + path + ')');
      } else {
        return $body.removeAttr('style');
      }
    },
    getImageUrl: function(rawPath, type) {
      var path;
      if (type == null) {
        type = 'thumbnail';
      }
      path = '';
      if ((rawPath == null) || rawPath === '') {
        switch (type) {
          case 'fanart':
            path = API.getRandomFanart();
            break;
          default:
            path = API.getDefaultThumbnail();
        }
      } else {
        path = API.parseRawPath(rawPath);
      }
      return path;
    }
  };
  App.commands.setHandler("images:fanart:set", function(path, region) {
    if (region == null) {
      region = 'regionFanart';
    }
    return API.setFanartBackground(path, region);
  });
  App.reqres.setHandler("images:path:get", function(rawPath, type) {
    if (rawPath == null) {
      rawPath = '';
    }
    if (type == null) {
      type = 'thumbnail';
    }
    return API.getImageUrl(rawPath, type);
  });
  return App.reqres.setHandler("images:path:entity", function(model) {
    var i, person, _ref;
    if (model.thumbnail != null) {
      model.thumbnail = API.getImageUrl(model.thumbnail, 'thumbnail');
    }
    if (model.fanart != null) {
      model.fanart = API.getImageUrl(model.fanart, 'fanart');
    }
    if ((model.cast != null) && model.cast.length > 0) {
      _ref = model.cast;
      for (i in _ref) {
        person = _ref[i];
        model.cast[i].thumbnail = API.getImageUrl(person.thumbnail, 'thumbnail');
      }
    }
    return model;
  });
});

this.Kodi.module("InputApp", function(InputApp, App, Backbone, Marionette, $, _) {
  var API;
  InputApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "remote": "remotePage"
    };

    return Router;

  })(App.Router.Base);
  API = {
    initKeyBind: function() {
      return $(document).keydown((function(_this) {
        return function(e) {
          return _this.keyBind(e);
        };
      })(this));
    },
    inputController: function() {
      return App.request("command:kodi:controller", 'auto', 'Input');
    },
    doInput: function(type) {
      return this.inputController().sendInput(type, []);
    },
    doAction: function(action) {
      return this.inputController().sendInput('ExecuteAction', [action]);
    },
    doCommand: function(command, params, callback) {
      return App.request('command:kodi:player', command, params, (function(_this) {
        return function() {
          return _this.pollingUpdate(callback);
        };
      })(this));
    },
    appController: function() {
      return App.request("command:kodi:controller", 'auto', 'Application');
    },
    pollingUpdate: function(callback) {
      if (!App.request('sockets:active')) {
        return App.request('state:kodi:update', callback);
      }
    },
    toggleRemote: function(open) {
      var $body, rClass;
      if (open == null) {
        open = 'auto';
      }
      $body = $('body');
      rClass = 'section-remote';
      if (open === 'auto') {
        open = $body.hasClass(rClass);
      }
      if (open) {
        window.history.back();
        return helpers.backscroll.scrollToLast();
      } else {
        helpers.backscroll.setLast();
        return App.navigate("remote", {
          trigger: true
        });
      }
    },
    remotePage: function() {
      this.toggleRemote('auto');
      return App.regionContent.empty();
    },
    keyBind: function(e) {
      var kodiControl, remotePage, stateObj, vol;
      kodiControl = config.getLocal('keyboardControl') === 'kodi';
      remotePage = $('body').hasClass('page-remote');
      if ($(e.target).is("input, textarea, select")) {
        return;
      }
      if (!kodiControl && !remotePage) {
        return;
      }
      if (kodiControl || remotePage) {
        e.preventDefault();
      }
      stateObj = App.request("state:kodi");
      console.log(e.which);
      switch (e.which) {
        case 37:
          return this.doInput("Left");
        case 38:
          return this.doInput("Up");
        case 39:
          return this.doInput("Right");
        case 40:
          return this.doInput("Down");
        case 8:
          return this.doInput("Back");
        case 13:
          return this.doInput("Select");
        case 67:
          return this.doInput("ContextMenu");
        case 107 || 187:
          vol = stateObj.getState('volume') + 5;
          return this.appController().setVolume((vol > 100 ? 100 : Math.ceil(vol)));
        case 109 || 189:
          vol = stateObj.getState('volume') - 5;
          return this.appController().setVolume((vol < 0 ? 0 : Math.ceil(vol)));
        case 32:
          return this.doCommand("PlayPause", "toggle");
        case 88:
          return this.doCommand("Stop");
        case 84:
          return this.doAction("showsubtitles");
        case 9:
          return this.doAction("close");
        case 190:
          return this.doCommand("GoTo", "next");
        case 188:
          return this.doCommand("GoTo", "previous");
        case 220:
          return this.doAction("fullscreen");
      }
    }
  };
  App.commands.setHandler("input:textbox", function(msg) {
    return App.execute("ui:textinput:show", "Input required", msg, function(text) {
      API.inputController().sendText(text);
      return App.execute("notification:show", t.gettext('Sent text') + ' "' + text + '" ' + t.gettext('to Kodi'));
    });
  });
  App.commands.setHandler("input:textbox:close", function() {
    return App.execute("ui:modal:close");
  });
  App.commands.setHandler("input:send", function(action) {
    return API.doInput(action);
  });
  App.commands.setHandler("input:remote:toggle", function() {
    return API.toggleRemote();
  });
  App.commands.setHandler("input:action", function(action) {
    return API.doAction(action);
  });
  App.commands.setHandler("input:resume", function(model, idKey) {
    var controller;
    controller = new InputApp.Resume.Controller();
    return controller.resumePlay(model, idKey);
  });
  App.addInitializer(function() {
    var controller;
    controller = new InputApp.Remote.Controller();
    return API.initKeyBind();
  });
  return App.on("before:start", function() {
    return new InputApp.Router({
      controller: API
    });
  });
});

this.Kodi.module("InputApp.Remote", function(Remote, App, Backbone, Marionette, $, _) {
  return Remote.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function() {
      return App.vent.on("shell:ready", (function(_this) {
        return function(options) {
          return _this.getRemote();
        };
      })(this));
    };

    Controller.prototype.getRemote = function() {
      var view;
      view = new Remote.Control();
      this.listenTo(view, "remote:input", function(type) {
        return App.execute("input:send", type);
      });
      this.listenTo(view, "remote:player", function(type) {
        return App.request('command:kodi:player', type, []);
      });
      this.listenTo(view, "remote:info", function() {
        if (App.request("state:kodi").isPlaying()) {
          return App.execute('input:action', 'osd');
        } else {
          return App.execute("input:send", 'Info');
        }
      });
      this.listenTo(view, "remote:power", function() {
        var appController;
        appController = App.request("command:kodi:controller", 'auto', 'Application');
        return appController.quit();
      });
      App.regionRemote.show(view);
      return App.vent.on("state:changed", function(state) {
        var fanart, playingItem, stateObj;
        stateObj = App.request("state:current");
        if (stateObj.isPlayingItemChanged()) {
          playingItem = stateObj.getPlaying('item');
          fanart = App.request("images:path:get", playingItem.fanart, 'fanart');
          return $('#remote-background').css('background-image', 'url(' + playingItem.fanart + ')');
        }
      });
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("InputApp.Remote", function(Remote, App, Backbone, Marionette, $, _) {
  return Remote.Control = (function(_super) {
    __extends(Control, _super);

    function Control() {
      return Control.__super__.constructor.apply(this, arguments);
    }

    Control.prototype.template = 'apps/input/remote/remote_control';

    Control.prototype.events = {
      'click .input-button': 'inputClick',
      'click .player-button': 'playerClick',
      'click .close-remote': 'closeRemote'
    };

    Control.prototype.triggers = {
      'click .power-button': 'remote:power',
      'click .info-button': 'remote:info'
    };

    Control.prototype.inputClick = function(e) {
      var type;
      type = $(e.target).data('type');
      return this.trigger('remote:input', type);
    };

    Control.prototype.playerClick = function(e) {
      var type;
      type = $(e.target).data('type');
      return this.trigger('remote:player', type);
    };

    Control.prototype.closeRemote = function(e) {
      return App.execute("input:remote:toggle");
    };

    Remote.Landing = (function(_super1) {
      __extends(Landing, _super1);

      function Landing() {
        return Landing.__super__.constructor.apply(this, arguments);
      }

      return Landing;

    })(App.Views.ItemView);

    return Control;

  })(App.Views.ItemView);
});

this.Kodi.module("InputApp.Resume", function(Resume, App, Backbone, Marionette, $, _) {
  return Resume.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.resumePlay = function(model, idKey) {
      var $el, complete_string, item, items, options, percent, resume, resume_string, start_string, stateObj, time_string, title, _i, _len;
      stateObj = App.request("state:current");
      title = t.gettext('Resume playback');
      resume = model.get('resume');
      percent = 0;
      options = [];
      if (parseInt(resume.position) > 0 && stateObj.getPlayer() === 'kodi') {
        percent = helpers.global.getPercent(resume.position, resume.total);
        time_string = helpers.global.formatTime(helpers.global.secToTime(resume.position));
        complete_string = helpers.global.round(percent, 0) + '% ' + t.gettext('complete');
        resume_string = t.gettext('Resume from') + ' <strong>' + time_string + '</strong> <small>' + complete_string + '</small>';
        start_string = t.gettext('Start from the beginning');
        items = [
          {
            title: resume_string,
            percent: percent
          }, {
            title: start_string,
            percent: 0
          }
        ];
        for (_i = 0, _len = items.length; _i < _len; _i++) {
          item = items[_i];
          $el = $('<span>').attr('data-percent', item.percent).html(item.title).click(function(e) {
            return App.execute("command:video:play", model, idKey, $(this).data('percent'));
          });
          options.push($el);
        }
        return App.execute("ui:modal:options", title, options);
      } else {
        return App.execute("command:video:play", model, idKey, 0);
      }
    };

    Controller.prototype.initialize = function() {};

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("LabApp.apiBrowser", function(apiBrowser, App, Backbone, Marionette, $, _) {
  return apiBrowser.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function() {
      var collection;
      collection = App.request("introspect:entities");
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          _this.layout = _this.getLayoutView(collection);
          _this.listenTo(_this.layout, "show", function() {
            _this.renderList(collection);
            if (_this.options.method) {
              return _this.renderPage(_this.options.method, collection);
            } else {
              return _this.renderLanding();
            }
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getLayoutView = function(collection) {
      return new apiBrowser.Layout({
        collection: collection
      });
    };

    Controller.prototype.renderList = function(collection) {
      var view;
      view = new apiBrowser.apiMethods({
        collection: collection
      });
      this.listenTo(view, 'childview:lab:apibrowser:method:view', (function(_this) {
        return function(item) {
          return _this.renderPage(item.model.get('id'), collection);
        };
      })(this));
      return this.layout.regionSidebarFirst.show(view);
    };

    Controller.prototype.renderPage = function(id, collection) {
      var model, pageView;
      model = App.request("introspect:entity", id, collection);
      pageView = new apiBrowser.apiMethodPage({
        model: model
      });
      helpers.debug.msg("Params/Returns for " + (model.get('method')) + ":", 'info', [model.get('params'), model.get('returns')]);
      this.listenTo(pageView, 'lab:apibrowser:execute', (function(_this) {
        return function(item) {
          var api, input, method, params;
          input = $('.api-method--params').val();
          params = JSON.parse(input);
          method = item.model.get('method');
          helpers.debug.msg("Parameters for: " + method, 'info', params);
          api = App.request("command:kodi:controller", "auto", "Commander");
          return api.singleCommand(method, params, function(response) {
            var output;
            helpers.debug.msg("Response for: " + method, 'info', response);
            output = prettyPrint(response);
            return $('#api-result').html(output).prepend($('<h3>Response (check the console for more)</h3>'));
          });
        };
      })(this));
      App.navigate("lab/api-browser/" + (model.get('method')));
      return this.layout.regionContent.show(pageView);
    };

    Controller.prototype.renderLanding = function() {
      var view;
      view = new apiBrowser.apiBrowserLanding();
      return this.layout.regionContent.show(view);
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("LabApp.apiBrowser", function(apiBrowser, App, Backbone, Marionette, $, _) {
  apiBrowser.Layout = (function(_super) {
    __extends(Layout, _super);

    function Layout() {
      return Layout.__super__.constructor.apply(this, arguments);
    }

    Layout.prototype.className = "api-browser--page page-wrapper";

    return Layout;

  })(App.Views.LayoutWithSidebarFirstView);
  apiBrowser.apiMethodItem = (function(_super) {
    __extends(apiMethodItem, _super);

    function apiMethodItem() {
      return apiMethodItem.__super__.constructor.apply(this, arguments);
    }

    apiMethodItem.prototype.className = "api-browser--method";

    apiMethodItem.prototype.template = 'apps/lab/apiBrowser/api_method_item';

    apiMethodItem.prototype.tagName = "li";

    apiMethodItem.prototype.triggers = {
      'click .api-method--item': 'lab:apibrowser:method:view'
    };

    return apiMethodItem;

  })(App.Views.ItemView);
  apiBrowser.apiMethods = (function(_super) {
    __extends(apiMethods, _super);

    function apiMethods() {
      return apiMethods.__super__.constructor.apply(this, arguments);
    }

    apiMethods.prototype.template = 'apps/lab/apiBrowser/api_method_list';

    apiMethods.prototype.childView = apiBrowser.apiMethodItem;

    apiMethods.prototype.childViewContainer = 'ul.items';

    apiMethods.prototype.tagName = "div";

    apiMethods.prototype.className = "api-browser--methods";

    apiMethods.prototype.onRender = function() {
      return $('#api-search', this.$el).filterList({
        items: '.api-browser--methods .api-browser--method',
        textSelector: '.method'
      });
    };

    return apiMethods;

  })(App.Views.CompositeView);
  apiBrowser.apiMethodPage = (function(_super) {
    __extends(apiMethodPage, _super);

    function apiMethodPage() {
      return apiMethodPage.__super__.constructor.apply(this, arguments);
    }

    apiMethodPage.prototype.className = "api-browser--page";

    apiMethodPage.prototype.template = 'apps/lab/apiBrowser/api_method_page';

    apiMethodPage.prototype.tagName = "div";

    apiMethodPage.prototype.triggers = {
      'click #send-command': 'lab:apibrowser:execute'
    };

    apiMethodPage.prototype.regions = {
      'apiResult': '#api-result'
    };

    apiMethodPage.prototype.onShow = function() {
      $('.api-method--params', this.$el).html(prettyPrint(this.model.get('params')));
      return $('.api-method--return', this.$el).html(prettyPrint(this.model.get('returns')));
    };

    return apiMethodPage;

  })(App.Views.ItemView);
  return apiBrowser.apiBrowserLanding = (function(_super) {
    __extends(apiBrowserLanding, _super);

    function apiBrowserLanding() {
      return apiBrowserLanding.__super__.constructor.apply(this, arguments);
    }

    apiBrowserLanding.prototype.className = "api-browser--landing";

    apiBrowserLanding.prototype.template = 'apps/lab/apiBrowser/api_browser_landing';

    apiBrowserLanding.prototype.tagName = "div";

    return apiBrowserLanding;

  })(App.Views.ItemView);
});

this.Kodi.module("LabApp.lab", function(lab, App, Backbone, Marionette, $, _) {
  lab.labItem = (function(_super) {
    __extends(labItem, _super);

    function labItem() {
      return labItem.__super__.constructor.apply(this, arguments);
    }

    labItem.prototype.className = "lab--item";

    labItem.prototype.template = 'apps/lab/lab/lab_item';

    labItem.prototype.tagName = "div";

    return labItem;

  })(App.Views.ItemView);
  return lab.labItems = (function(_super) {
    __extends(labItems, _super);

    function labItems() {
      return labItems.__super__.constructor.apply(this, arguments);
    }

    labItems.prototype.tagName = "div";

    labItems.prototype.className = "lab--items page";

    labItems.prototype.childView = lab.labItem;

    labItems.prototype.onRender = function() {
      this.$el.prepend($('<h3>').html(t.gettext('Experimental code, use at own risk')));
      this.$el.prepend($('<h2>').html(t.gettext('The lab')));
      return this.$el.addClass('page-secondary');
    };

    return labItems;

  })(App.Views.CollectionView);
});

this.Kodi.module("LabApp", function(LabApp, App, Backbone, Marionette, $, _) {
  var API;
  LabApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "lab": "labLanding",
      "lab/api-browser": "apiBrowser",
      "lab/api-browser/:method": "apiBrowser",
      "lab/screenshot": "screenShot"
    };

    return Router;

  })(App.Router.Base);
  API = {
    labLanding: function() {
      var view;
      view = new LabApp.lab.labItems({
        collection: new App.Entities.NavMainCollection(this.labItems())
      });
      return App.regionContent.show(view);
    },
    labItems: function() {
      return [
        {
          title: 'API browser',
          description: 'Execute any API command.',
          path: 'lab/api-browser'
        }, {
          title: 'Screenshot',
          description: 'Take a screenshot of Kodi right now.',
          path: 'lab/screenshot'
        }
      ];
    },
    apiBrowser: function(method) {
      if (method == null) {
        method = false;
      }
      return new LabApp.apiBrowser.Controller({
        method: method
      });
    },
    screenShot: function() {
      App.execute("input:action", 'screenshot');
      App.execute("notification:show", t.gettext("Screenshot saved to your screenshots folder"));
      return App.navigate("#lab", {
        trigger: true
      });
    }
  };
  return App.on("before:start", function() {
    return new LabApp.Router({
      controller: API
    });
  });
});

this.Kodi.module("LoadingApp", function(LoadingApp, App, Backbone, Marionette, $, _) {
  App.commands.setHandler("loading:show:view", function(region, msgText) {
    var view;
    if (msgText == null) {
      msgText = 'Just a sec...';
    }
    view = new LoadingApp.Show.Page({
      text: msgText
    });
    return region.show(view);
  });
  return App.commands.setHandler("loading:show:page", function() {
    return App.execute("loading:show:view", App.regionContent);
  });
});

this.Kodi.module("LoadingApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  return Show.Page = (function(_super) {
    __extends(Page, _super);

    function Page() {
      return Page.__super__.constructor.apply(this, arguments);
    }

    Page.prototype.template = "apps/loading/show/loading_page";

    Page.prototype.onRender = function() {
      return this.$el.find('h2').html(this.options.text);
    };

    return Page;

  })(Backbone.Marionette.ItemView);
});

this.Kodi.module("localPlaylistApp.List", function(List, App, Backbone, Marionette, $, _) {
  return List.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      var id, playlists;
      id = options.id;
      playlists = App.request("localplaylist:entities");
      this.layout = this.getLayoutView(playlists);
      this.listenTo(this.layout, "show", (function(_this) {
        return function() {
          _this.getListsView(playlists);
          return _this.getItems(id);
        };
      })(this));
      return App.regionContent.show(this.layout);
    };

    Controller.prototype.getLayoutView = function(collection) {
      return new List.ListLayout({
        collection: collection
      });
    };

    Controller.prototype.getListsView = function(playlists) {
      var view;
      this.sideLayout = new List.SideLayout();
      view = new List.Lists({
        collection: playlists
      });
      App.listenTo(this.sideLayout, "show", (function(_this) {
        return function() {
          if (playlists.length > 0) {
            return _this.sideLayout.regionLists.show(view);
          }
        };
      })(this));
      App.listenTo(this.sideLayout, 'lists:new', function() {
        return App.execute("localplaylist:newlist");
      });
      return this.layout.regionSidebarFirst.show(this.sideLayout);
    };

    Controller.prototype.getItems = function(id) {
      var collection, playlist;
      playlist = App.request("localplaylist:entity", id);
      collection = App.request("localplaylist:item:entities", id);
      this.itemLayout = new List.Layout({
        list: playlist
      });
      App.listenTo(this.itemLayout, "show", (function(_this) {
        return function() {
          var media, view;
          if (collection.length > 0) {
            media = playlist.get('media');
            view = App.request("" + media + ":list:view", collection, true);
            return _this.itemLayout.regionListItems.show(view);
          }
        };
      })(this));
      this.bindLayout(id);
      return this.layout.regionContent.show(this.itemLayout);
    };

    Controller.prototype.bindLayout = function(id) {
      var collection;
      collection = App.request("localplaylist:item:entities", id);
      App.listenTo(this.itemLayout, 'list:clear', function() {
        App.execute("localplaylist:clear:entities", id);
        return App.execute("localplaylist:reload", id);
      });
      App.listenTo(this.itemLayout, 'list:delete', function() {
        App.execute("localplaylist:clear:entities", id);
        App.execute("localplaylist:remove:entity", id);
        return App.navigate("playlists", {
          trigger: true
        });
      });
      App.listenTo(this.itemLayout, 'list:play', function() {
        var kodiPlaylist;
        kodiPlaylist = App.request("command:kodi:controller", 'audio', 'PlayList');
        return kodiPlaylist.playCollection(collection);
      });
      return App.listenTo(this.itemLayout, 'list:localplay', function() {
        var localPlaylist;
        localPlaylist = App.request("command:local:controller", 'audio', 'PlayList');
        return localPlaylist.playCollection(collection);
      });
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("localPlaylistApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.ListLayout = (function(_super) {
    __extends(ListLayout, _super);

    function ListLayout() {
      return ListLayout.__super__.constructor.apply(this, arguments);
    }

    ListLayout.prototype.className = "local-playlist-list";

    return ListLayout;

  })(App.Views.LayoutWithSidebarFirstView);
  List.SideLayout = (function(_super) {
    __extends(SideLayout, _super);

    function SideLayout() {
      return SideLayout.__super__.constructor.apply(this, arguments);
    }

    SideLayout.prototype.template = 'apps/localPlaylist/list/playlist_sidebar_layout';

    SideLayout.prototype.tagName = 'div';

    SideLayout.prototype.className = 'side-inner';

    SideLayout.prototype.regions = {
      regionLists: '.current-lists'
    };

    SideLayout.prototype.triggers = {
      'click .new-list': 'lists:new'
    };

    return SideLayout;

  })(App.Views.LayoutView);
  List.List = (function(_super) {
    __extends(List, _super);

    function List() {
      return List.__super__.constructor.apply(this, arguments);
    }

    List.prototype.template = 'apps/localPlaylist/list/playlist';

    List.prototype.tagName = "li";

    List.prototype.initialize = function() {
      var classes, path, tag;
      path = helpers.url.get('playlist', this.model.get('id'));
      classes = [];
      if (path === helpers.url.path()) {
        classes.push('active');
      }
      tag = this.themeLink(this.model.get('name'), path, {
        'className': classes.join(' ')
      });
      return this.model.set({
        title: tag
      });
    };

    return List;

  })(App.Views.ItemView);
  List.Lists = (function(_super) {
    __extends(Lists, _super);

    function Lists() {
      return Lists.__super__.constructor.apply(this, arguments);
    }

    Lists.prototype.template = 'apps/localPlaylist/list/playlist_list';

    Lists.prototype.childView = List.List;

    Lists.prototype.tagName = "div";

    Lists.prototype.childViewContainer = 'ul.lists';

    Lists.prototype.onRender = function() {
      return $('h3', this.$el).html(t.gettext('Playlists'));
    };

    return Lists;

  })(App.Views.CompositeView);
  List.Selection = (function(_super) {
    __extends(Selection, _super);

    function Selection() {
      return Selection.__super__.constructor.apply(this, arguments);
    }

    Selection.prototype.template = 'apps/localPlaylist/list/playlist';

    Selection.prototype.tagName = "li";

    Selection.prototype.initialize = function() {
      return this.model.set({
        title: this.model.get('name')
      });
    };

    Selection.prototype.triggers = {
      'click .item': 'item:selected'
    };

    return Selection;

  })(App.Views.ItemView);
  List.SelectionList = (function(_super) {
    __extends(SelectionList, _super);

    function SelectionList() {
      return SelectionList.__super__.constructor.apply(this, arguments);
    }

    SelectionList.prototype.template = 'apps/localPlaylist/list/playlist_list';

    SelectionList.prototype.childView = List.Selection;

    SelectionList.prototype.tagName = "div";

    SelectionList.prototype.className = 'playlist-selection-list';

    SelectionList.prototype.childViewContainer = 'ul.lists';

    SelectionList.prototype.onRender = function() {
      return $('h3', this.$el).html(t.gettext('Existing playlists'));
    };

    return SelectionList;

  })(App.Views.CompositeView);
  return List.Layout = (function(_super) {
    __extends(Layout, _super);

    function Layout() {
      return Layout.__super__.constructor.apply(this, arguments);
    }

    Layout.prototype.template = 'apps/localPlaylist/list/playlist_layout';

    Layout.prototype.tagName = 'div';

    Layout.prototype.className = 'local-playlist';

    Layout.prototype.regions = {
      regionListItems: '.item-container'
    };

    Layout.prototype.triggers = {
      'click .local-playlist-header .clear': 'list:clear',
      'click .local-playlist-header .delete': 'list:delete',
      'click .local-playlist-header .play': 'list:play',
      'click .local-playlist-header .localplay': 'list:localplay'
    };

    Layout.prototype.onRender = function() {
      if (this.options && this.options.list) {
        return $('h2', this.$el).html(this.options.list.get('name'));
      }
    };

    return Layout;

  })(App.Views.LayoutView);
});

this.Kodi.module("localPlaylistApp", function(localPlaylistApp, App, Backbone, Marionette, $, _) {
  var API;
  localPlaylistApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "playlists": "list",
      "playlist/:id": "list"
    };

    return Router;

  })(App.Router.Base);

  /*
    Main functionality.
   */
  API = {
    list: function(id) {
      var item, items, lists;
      if (id === null) {
        lists = App.request("localplaylist:entities");
        items = lists.getRawCollection();
        if (_.isEmpty(lists)) {
          id = 0;
        } else {
          item = _.min(items, function(list) {
            return list.id;
          });
          id = item.id;
          App.navigate(helpers.url.get('playlist', id));
        }
      }
      return new localPlaylistApp.List.Controller({
        id: id
      });
    },
    addToList: function(entityType, id) {
      var $content, $new, playlists, view;
      playlists = App.request("localplaylist:entities");
      if (!playlists || playlists.length === 0) {
        return this.createNewList(entityType, id);
      } else {
        view = new localPlaylistApp.List.SelectionList({
          collection: playlists
        });
        $content = view.render().$el;
        $new = $('<button>').html(t.gettext('Create a new list')).addClass('btn btn-primary');
        $new.on('click', (function(_this) {
          return function() {
            return _.defer(function() {
              return API.createNewList(entityType, id);
            });
          };
        })(this));
        App.execute("ui:modal:show", t.gettext('Add to playlist'), $content, $new);
        return App.listenTo(view, 'childview:item:selected', (function(_this) {
          return function(list, item) {
            return _this.addToExistingList(item.model.get('id'), entityType, id);
          };
        })(this));
      }
    },
    addToExistingList: function(playlistId, entityType, id) {
      var collection;
      if (helpers.global.inArray(entityType, ['albumid', 'artistid'])) {
        collection = App.request("song:filtered:entities", {
          filter: helpers.global.paramObj(entityType, id)
        });
        return App.execute("when:entity:fetched", collection, (function(_this) {
          return function() {
            return _this.addCollectionToList(collection, playlistId);
          };
        })(this));
      } else if (entityType === 'songid') {
        return App.request("song:byid:entities", [id], (function(_this) {
          return function(collection) {
            return _this.addCollectionToList(collection, playlistId);
          };
        })(this));
      } else if (entityType === 'playlist') {
        collection = App.request("playlist:kodi:entities", 'audio');
        return App.execute("when:entity:fetched", collection, (function(_this) {
          return function() {
            return _this.addCollectionToList(collection, playlistId);
          };
        })(this));
      } else {

      }
    },
    addCollectionToList: function(collection, playlistId) {
      App.request("localplaylist:item:add:entities", playlistId, collection);
      App.execute("ui:modal:close");
      return App.execute("notification:show", t.gettext("Added to your playlist"));
    },
    createNewList: function(entityType, id) {
      return App.execute("ui:textinput:show", t.gettext('Add a new playlist'), t.gettext('Give your playlist a name'), (function(_this) {
        return function(text) {
          var playlistId;
          if (text !== '') {
            playlistId = App.request("localplaylist:add:entity", text, 'song');
            return _this.addToExistingList(playlistId, entityType, id);
          }
        };
      })(this), false);
    },
    createEmptyList: function() {
      return App.execute("ui:textinput:show", t.gettext('Add a new playlist'), t.gettext('Give your playlist a name'), (function(_this) {
        return function(text) {
          var playlistId;
          if (text !== '') {
            playlistId = App.request("localplaylist:add:entity", text, 'song');
            return App.navigate("playlist/" + playlistId, {
              trigger: true
            });
          }
        };
      })(this));
    }
  };

  /*
    Listeners.
   */
  App.commands.setHandler("localplaylist:addentity", function(entityType, id) {
    return API.addToList(entityType, id);
  });
  App.commands.setHandler("localplaylist:newlist", function() {
    return API.createEmptyList();
  });
  App.commands.setHandler("localplaylist:reload", function(id) {
    return API.list(id);
  });

  /*
    Init the router
   */
  return App.on("before:start", function() {
    return new localPlaylistApp.Router({
      controller: API
    });
  });
});

this.Kodi.module("MovieApp.Edit", function(Edit, App, Backbone, Marionette, $, _) {
  return Edit.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function() {
      var form, options;
      this.model = this.getOption('model');
      options = {
        title: this.model.get('title'),
        form: this.getSructure(),
        formState: this.model.attributes,
        config: {
          attributes: {
            "class": 'edit-form'
          },
          callback: (function(_this) {
            return function(data, formView) {
              return _this.saveCallback(data, formView);
            };
          })(this)
        }
      };
      return form = App.request("form:popup:wrapper", options);
    };

    Controller.prototype.getSructure = function() {
      return [
        {
          title: 'Information',
          id: 'general',
          children: [
            {
              id: 'title',
              title: 'Title',
              type: 'textfield',
              defaultValue: ''
            }, {
              id: 'plotoutline',
              title: 'Plot outline',
              type: 'textarea',
              defaultValue: ''
            }, {
              id: 'plot',
              title: 'Plot',
              type: 'textarea',
              defaultValue: ''
            }, {
              id: 'rating',
              title: 'Rating',
              type: 'textfield',
              defaultValue: ''
            }, {
              id: 'imdbnumber',
              title: 'IMDb',
              type: 'textfield',
              defaultValue: ''
            }
          ]
        }
      ];
    };

    Controller.prototype.saveCallback = function(data, formView) {
      var videoLib;
      data.rating = parseFloat(data.rating);
      videoLib = App.request("command:kodi:controller", 'video', 'VideoLibrary');
      return videoLib.setMovieDetails(this.model.get('id'), data, function() {
        return Kodi.execute("notification:show", t.gettext("Updated movie details"));
      });
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("MovieApp.Landing", function(Landing, App, Backbone, Marionette, $, _) {
  return Landing.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.subNavId = 'movies/recent';

    Controller.prototype.initialize = function() {
      this.layout = this.getLayoutView();
      this.listenTo(this.layout, "show", (function(_this) {
        return function() {
          _this.getPageView();
          return _this.getSubNav();
        };
      })(this));
      return App.regionContent.show(this.layout);
    };

    Controller.prototype.getLayoutView = function() {
      return new Landing.Layout();
    };

    Controller.prototype.getSubNav = function() {
      var subNav;
      subNav = App.request("navMain:children:show", this.subNavId, 'Sections');
      return this.layout.regionSidebarFirst.show(subNav);
    };

    Controller.prototype.getPageView = function() {
      this.page = new Landing.Page();
      this.listenTo(this.page, "show", (function(_this) {
        return function() {
          return _this.renderRecentlyAdded();
        };
      })(this));
      return this.layout.regionContent.show(this.page);
    };

    Controller.prototype.renderRecentlyAdded = function() {
      var collection;
      collection = App.request("movie:recentlyadded:entities");
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          var view;
          view = App.request("movie:list:view", collection);
          return _this.page.regionRecentlyAdded.show(view);
        };
      })(this));
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("MovieApp.Landing", function(Landing, App, Backbone, Marionette, $, _) {
  Landing.Layout = (function(_super) {
    __extends(Layout, _super);

    function Layout() {
      return Layout.__super__.constructor.apply(this, arguments);
    }

    Layout.prototype.className = "movie-landing landing-page";

    return Layout;

  })(App.Views.LayoutWithSidebarFirstView);
  return Landing.Page = (function(_super) {
    __extends(Page, _super);

    function Page() {
      return Page.__super__.constructor.apply(this, arguments);
    }

    Page.prototype.template = 'apps/movie/landing/landing';

    Page.prototype.className = "movie-recent";

    Page.prototype.regions = {
      regionRecentlyAdded: '.region-recently-added'
    };

    return Page;

  })(App.Views.LayoutView);
});

this.Kodi.module("MovieApp.List", function(List, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    getMoviesView: function(collection, set) {
      var view, viewName;
      if (set == null) {
        set = false;
      }
      viewName = set ? 'MoviesSet' : 'Movies';
      view = new List[viewName]({
        collection: collection
      });
      API.bindTriggers(view);
      return view;
    },
    bindTriggers: function(view) {
      App.listenTo(view, 'childview:movie:play', function(parent, viewItem) {
        return App.execute('movie:action', 'play', viewItem);
      });
      App.listenTo(view, 'childview:movie:add', function(parent, viewItem) {
        return App.execute('movie:action', 'add', viewItem);
      });
      App.listenTo(view, 'childview:movie:localplay', function(parent, viewItem) {
        return App.execute('movie:action', 'localplay', viewItem);
      });
      App.listenTo(view, 'childview:movie:download', function(parent, viewItem) {
        return App.execute('movie:action', 'download', viewItem);
      });
      App.listenTo(view, 'childview:movie:watched', function(parent, viewItem) {
        parent.$el.toggleClass('is-watched');
        return App.execute('movie:action', 'toggleWatched', viewItem);
      });
      return App.listenTo(view, 'childview:movie:edit', function(parent, viewItem) {
        return App.execute('movie:action', 'edit', viewItem);
      });
    }
  };
  List.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function() {
      var collection;
      collection = App.request("movie:entities");
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          collection.availableFilters = _this.getAvailableFilters();
          collection.sectionId = 'movies/recent';
          App.request('filter:init', _this.getAvailableFilters());
          _this.layout = _this.getLayoutView(collection);
          _this.listenTo(_this.layout, "show", function() {
            _this.renderList(collection);
            return _this.getFiltersView(collection);
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getLayoutView = function(collection) {
      return new List.ListLayout({
        collection: collection
      });
    };

    Controller.prototype.getAvailableFilters = function() {
      return {
        sort: ['title', 'year', 'dateadded', 'rating'],
        filter: ['year', 'genre', 'writer', 'director', 'cast', 'set', 'unwatched']
      };
    };

    Controller.prototype.getFiltersView = function(collection) {
      var filters;
      filters = App.request('filter:show', collection);
      this.layout.regionSidebarFirst.show(filters);
      return this.listenTo(filters, "filter:changed", (function(_this) {
        return function() {
          return _this.renderList(collection);
        };
      })(this));
    };

    Controller.prototype.renderList = function(collection) {
      var filteredCollection, view;
      App.execute("loading:show:view", this.layout.regionContent);
      filteredCollection = App.request('filter:apply:entities', collection);
      view = API.getMoviesView(filteredCollection);
      return this.layout.regionContent.show(view);
    };

    return Controller;

  })(App.Controllers.Base);
  return App.reqres.setHandler("movie:list:view", function(collection) {
    return API.getMoviesView(collection, true);
  });
});

this.Kodi.module("MovieApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.ListLayout = (function(_super) {
    __extends(ListLayout, _super);

    function ListLayout() {
      return ListLayout.__super__.constructor.apply(this, arguments);
    }

    ListLayout.prototype.className = "movie-list with-filters";

    return ListLayout;

  })(App.Views.LayoutWithSidebarFirstView);
  List.MovieTeaser = (function(_super) {
    __extends(MovieTeaser, _super);

    function MovieTeaser() {
      return MovieTeaser.__super__.constructor.apply(this, arguments);
    }

    MovieTeaser.prototype.triggers = {
      "click .play": "movie:play",
      "click .watched": "movie:watched",
      "click .add": "movie:add",
      "click .localplay": "movie:localplay",
      "click .download": "movie:download",
      "click .edit": "movie:edit"
    };

    MovieTeaser.prototype.initialize = function() {
      MovieTeaser.__super__.initialize.apply(this, arguments);
      if (this.model != null) {
        this.model.set({
          subtitle: this.model.get('year')
        });
        return this.model.set(App.request('movie:action:items'));
      }
    };

    MovieTeaser.prototype.attributes = function() {
      var classes;
      classes = ['card'];
      if (helpers.entities.isWatched(this.model)) {
        classes.push('is-watched');
      }
      return {
        "class": classes.join(' ')
      };
    };

    return MovieTeaser;

  })(App.Views.CardView);
  List.Empty = (function(_super) {
    __extends(Empty, _super);

    function Empty() {
      return Empty.__super__.constructor.apply(this, arguments);
    }

    Empty.prototype.tagName = "li";

    Empty.prototype.className = "movie-empty-result";

    return Empty;

  })(App.Views.EmptyViewResults);
  List.Movies = (function(_super) {
    __extends(Movies, _super);

    function Movies() {
      return Movies.__super__.constructor.apply(this, arguments);
    }

    Movies.prototype.childView = List.MovieTeaser;

    Movies.prototype.emptyView = List.Empty;

    Movies.prototype.tagName = "ul";

    Movies.prototype.className = "card-grid--tall";

    return Movies;

  })(App.Views.VirtualListView);
  return List.MoviesSet = (function(_super) {
    __extends(MoviesSet, _super);

    function MoviesSet() {
      return MoviesSet.__super__.constructor.apply(this, arguments);
    }

    MoviesSet.prototype.childView = List.MovieTeaser;

    MoviesSet.prototype.emptyView = List.Empty;

    MoviesSet.prototype.tagName = "ul";

    MoviesSet.prototype.className = "card-grid--tall";

    return MoviesSet;

  })(App.Views.CollectionView);
});

this.Kodi.module("MovieApp", function(MovieApp, App, Backbone, Marionette, $, _) {
  var API;
  MovieApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "movies/recent": "landing",
      "movies": "list",
      "movie/:id": "view"
    };

    return Router;

  })(App.Router.Base);
  API = {
    landing: function() {
      return new MovieApp.Landing.Controller();
    },
    list: function() {
      return new MovieApp.List.Controller();
    },
    view: function(id) {
      return new MovieApp.Show.Controller({
        id: id
      });
    },
    action: function(op, view) {
      var files, model, playlist, videoLib;
      model = view.model;
      playlist = App.request("command:kodi:controller", 'video', 'PlayList');
      files = App.request("command:kodi:controller", 'video', 'Files');
      videoLib = App.request("command:kodi:controller", 'video', 'VideoLibrary');
      switch (op) {
        case 'play':
          return App.execute("input:resume", model, 'movieid');
        case 'add':
          return playlist.add('movieid', model.get('movieid'));
        case 'localplay':
          return files.videoStream(model.get('file'), model.get('fanart'));
        case 'download':
          return files.downloadFile(model.get('file'));
        case 'toggleWatched':
          return videoLib.toggleWatched(model);
        case 'edit':
          return App.execute('movie:edit', model);
      }
    }
  };
  App.reqres.setHandler('movie:action:items', function() {
    return {
      actions: {
        watched: 'Watched',
        thumbs: 'Thumbs up'
      },
      menu: {
        add: 'Add to Kodi playlist',
        edit: 'Edit',
        divider: '',
        download: 'Download',
        localplay: 'Play in browser'
      }
    };
  });
  App.commands.setHandler('movie:action', function(op, view) {
    return API.action(op, view);
  });
  App.commands.setHandler('movie:edit', function(model) {
    var loadedModel;
    loadedModel = App.request("movie:entity", model.get('id'));
    return App.execute("when:entity:fetched", loadedModel, (function(_this) {
      return function() {
        return new MovieApp.Edit.Controller({
          model: loadedModel
        });
      };
    })(this));
  });
  return App.on("before:start", function() {
    return new MovieApp.Router({
      controller: API
    });
  });
});

this.Kodi.module("MovieApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    bindTriggers: function(view) {
      App.listenTo(view, 'movie:play', function(viewItem) {
        return App.execute('movie:action', 'play', viewItem);
      });
      App.listenTo(view, 'movie:add', function(viewItem) {
        return App.execute('movie:action', 'add', viewItem);
      });
      App.listenTo(view, 'movie:localplay', function(viewItem) {
        return App.execute('movie:action', 'localplay', viewItem);
      });
      return App.listenTo(view, 'movie:download', function(viewItem) {
        return App.execute('movie:action', 'download', viewItem);
      });
    }
  };
  return Show.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      var id, movie;
      id = parseInt(options.id);
      movie = App.request("movie:entity", id);
      return App.execute("when:entity:fetched", movie, (function(_this) {
        return function() {
          _this.layout = _this.getLayoutView(movie);
          _this.listenTo(_this.layout, "destroy", function() {
            return App.execute("images:fanart:set", 'none');
          });
          _this.listenTo(_this.layout, "show", function() {
            _this.getDetailsLayoutView(movie);
            return _this.getContentView(movie);
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getLayoutView = function(movie) {
      return new Show.PageLayout({
        model: movie
      });
    };

    Controller.prototype.getContentView = function(movie) {
      this.contentLayout = new Show.Content({
        model: movie
      });
      this.listenTo(this.contentLayout, "movie:youtube", function(view) {
        var trailer;
        trailer = movie.get('trailer');
        return App.execute("ui:modal:youtube", movie.get('title') + ' Trailer', trailer.id);
      });
      this.listenTo(this.contentLayout, 'show', (function(_this) {
        return function() {
          if (movie.get('cast').length > 0) {
            _this.contentLayout.regionCast.show(_this.getCast(movie));
          }
          return _this.getSetView(movie);
        };
      })(this));
      return this.layout.regionContent.show(this.contentLayout);
    };

    Controller.prototype.getCast = function(movie) {
      return App.request('cast:list:view', movie.get('cast'), 'movies');
    };

    Controller.prototype.getDetailsLayoutView = function(movie) {
      var headerLayout;
      headerLayout = new Show.HeaderLayout({
        model: movie
      });
      this.listenTo(headerLayout, "show", (function(_this) {
        return function() {
          var detail, teaser;
          teaser = new Show.MovieTeaser({
            model: movie
          });
          API.bindTriggers(teaser);
          detail = new Show.Details({
            model: movie
          });
          API.bindTriggers(detail);
          headerLayout.regionSide.show(teaser);
          return headerLayout.regionMeta.show(detail);
        };
      })(this));
      return this.layout.regionHeader.show(headerLayout);
    };

    Controller.prototype.getSetView = function(movie) {
      var collection;
      if (movie.get('set') !== '') {
        collection = App.request("movie:entities");
        return App.execute("when:entity:fetched", collection, (function(_this) {
          return function() {
            var filteredCollection, view;
            filteredCollection = new App.Entities.Filtered(collection);
            filteredCollection.filterBy('set', {
              set: movie.get('set')
            });
            view = new Show.Set({
              set: movie.get('set')
            });
            App.listenTo(view, "show", function() {
              var listview;
              listview = App.request("movie:list:view", filteredCollection);
              return view.regionCollection.show(listview);
            });
            return _this.contentLayout.regionSets.show(view);
          };
        })(this));
      }
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("MovieApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  Show.PageLayout = (function(_super) {
    __extends(PageLayout, _super);

    function PageLayout() {
      return PageLayout.__super__.constructor.apply(this, arguments);
    }

    PageLayout.prototype.className = 'movie-show detail-container';

    return PageLayout;

  })(App.Views.LayoutWithHeaderView);
  Show.HeaderLayout = (function(_super) {
    __extends(HeaderLayout, _super);

    function HeaderLayout() {
      return HeaderLayout.__super__.constructor.apply(this, arguments);
    }

    HeaderLayout.prototype.className = 'movie-details';

    return HeaderLayout;

  })(App.Views.LayoutDetailsHeaderView);
  Show.Details = (function(_super) {
    __extends(Details, _super);

    function Details() {
      return Details.__super__.constructor.apply(this, arguments);
    }

    Details.prototype.template = 'apps/movie/show/details_meta';

    Details.prototype.triggers = {
      'click .play': 'movie:play',
      'click .add': 'movie:add',
      'click .stream': 'movie:localplay',
      'click .download': 'movie:download'
    };

    return Details;

  })(App.Views.ItemView);
  Show.MovieTeaser = (function(_super) {
    __extends(MovieTeaser, _super);

    function MovieTeaser() {
      return MovieTeaser.__super__.constructor.apply(this, arguments);
    }

    MovieTeaser.prototype.tagName = "div";

    MovieTeaser.prototype.className = "card-detail";

    MovieTeaser.prototype.triggers = {
      'click .play': 'movie:play'
    };

    return MovieTeaser;

  })(App.Views.CardView);
  Show.Content = (function(_super) {
    __extends(Content, _super);

    function Content() {
      return Content.__super__.constructor.apply(this, arguments);
    }

    Content.prototype.template = 'apps/movie/show/content';

    Content.prototype.className = "movie-content content-sections";

    Content.prototype.onRender = $('[data-toggle="tooltip"]', Content.$el).tooltip();

    Content.prototype.triggers = {
      'click .youtube': 'movie:youtube'
    };

    Content.prototype.regions = {
      regionCast: '.region-cast',
      regionSets: '.region-sets'
    };

    return Content;

  })(App.Views.LayoutView);
  return Show.Set = (function(_super) {
    __extends(Set, _super);

    function Set() {
      return Set.__super__.constructor.apply(this, arguments);
    }

    Set.prototype.template = 'apps/movie/show/set';

    Set.prototype.className = 'movie-set';

    Set.prototype.onRender = function() {
      if (this.options) {
        if (this.options.set) {
          return $('h2.set-name', this.$el).html(this.options.set);
        }
      }
    };

    Set.prototype.regions = function() {
      return {
        regionCollection: '.collection-items'
      };
    };

    return Set;

  })(App.Views.LayoutView);
});

this.Kodi.module("NavMain", function(NavMain, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    getNav: function() {
      var navStructure;
      navStructure = App.request('navMain:entities');
      return new NavMain.List({
        collection: navStructure
      });
    },
    getNavChildren: function(path, title) {
      var navStructure;
      if (title == null) {
        title = 'default';
      }
      navStructure = App.request('navMain:entities', path);
      if (title !== 'default') {
        navStructure.set({
          title: title
        });
      }
      return new NavMain.ItemList({
        model: navStructure
      });
    },
    getNavCollection: function(collection, title) {
      var navStructure;
      navStructure = new App.Entities.NavMain({
        title: title,
        items: collection
      });
      return new NavMain.ItemList({
        model: navStructure
      });
    }
  };
  this.onStart = function(options) {
    return App.vent.on("shell:ready", (function(_this) {
      return function(options) {
        var nav;
        nav = API.getNav();
        return App.regionNav.show(nav);
      };
    })(this));
  };
  App.reqres.setHandler("navMain:children:show", function(path, title) {
    if (title == null) {
      title = 'default';
    }
    return API.getNavChildren(path, title);
  });
  App.reqres.setHandler("navMain:collection:show", function(collection, title) {
    if (title == null) {
      title = '';
    }
    return API.getNavCollection(collection, title);
  });
  return App.vent.on("navMain:refresh", function() {
    var nav;
    nav = API.getNav();
    return App.regionNav.show(nav);
  });
});

this.Kodi.module("NavMain", function(NavMain, App, Backbone, Marionette, $, _) {
  NavMain.List = (function(_super) {
    __extends(List, _super);

    function List() {
      return List.__super__.constructor.apply(this, arguments);
    }

    List.prototype.template = "apps/navMain/show/navMain";

    return List;

  })(Backbone.Marionette.ItemView);
  NavMain.Item = (function(_super) {
    __extends(Item, _super);

    function Item() {
      return Item.__super__.constructor.apply(this, arguments);
    }

    Item.prototype.template = "apps/navMain/show/nav_item";

    Item.prototype.tagName = "li";

    Item.prototype.initialize = function() {
      var classes, tag;
      classes = [];
      if (this.model.get('path') === helpers.url.path()) {
        classes.push('active');
      }
      tag = this.themeLink(this.model.get('title'), this.model.get('path'), {
        'className': classes.join(' ')
      });
      return this.model.set({
        link: tag
      });
    };

    return Item;

  })(Backbone.Marionette.ItemView);
  return NavMain.ItemList = (function(_super) {
    __extends(ItemList, _super);

    function ItemList() {
      return ItemList.__super__.constructor.apply(this, arguments);
    }

    ItemList.prototype.template = 'apps/navMain/show/nav_sub';

    ItemList.prototype.childView = NavMain.Item;

    ItemList.prototype.tagName = "div";

    ItemList.prototype.childViewContainer = 'ul.items';

    ItemList.prototype.className = "nav-sub";

    ItemList.prototype.initialize = function() {
      return this.collection = this.model.get('items');
    };

    return ItemList;

  })(App.Views.CompositeView);
});

this.Kodi.module("NotificationsApp", function(NotificationApp, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    notificationMinTimeOut: 5000
  };
  return App.commands.setHandler("notification:show", function(msg, severity) {
    var timeout;
    if (severity == null) {
      severity = 'normal';
    }
    timeout = msg.length < 50 ? API.notificationMinTimeOut : msg.length * 100;
    return $.snackbar({
      content: msg,
      style: 'type-' + severity,
      timeout: timeout
    });
  });
});

this.Kodi.module("PlayerApp", function(PlayerApp, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    getPlayer: function(player) {
      return new PlayerApp.Show.Player({
        player: player
      });
    },
    doCommand: function(player, command, params, callback) {
      return App.request("command:" + player + ":player", command, params, (function(_this) {
        return function() {
          return _this.pollingUpdate(callback);
        };
      })(this));
    },
    getAppController: function(player) {
      return App.request("command:" + player + ":controller", 'auto', 'Application');
    },
    pollingUpdate: function(callback) {
      var stateObj;
      stateObj = App.request("state:current");
      if (stateObj.getPlayer() === 'kodi') {
        if (!App.request('sockets:active')) {
          return App.request('state:kodi:update', callback);
        }
      } else {

      }
    },
    initPlayer: function(player, playerView) {
      var $playerCtx, $progress, $volume, appController;
      this.initProgress(player);
      this.initVolume(player);
      App.vent.trigger("state:player:updated", player);
      appController = this.getAppController(player);
      App.vent.on("state:initialized", (function(_this) {
        return function() {
          var stateObj;
          stateObj = App.request("state:kodi");
          if (stateObj.isPlaying()) {
            _this.timerStop();
            return _this.timerStart();
          }
        };
      })(this));
      App.listenTo(playerView, "control:play", (function(_this) {
        return function() {
          return _this.doCommand(player, 'PlayPause', 'toggle');
        };
      })(this));
      App.listenTo(playerView, "control:prev", (function(_this) {
        return function() {
          return _this.doCommand(player, 'GoTo', 'previous');
        };
      })(this));
      App.listenTo(playerView, "control:next", (function(_this) {
        return function() {
          return _this.doCommand(player, 'GoTo', 'next');
        };
      })(this));
      App.listenTo(playerView, "control:repeat", (function(_this) {
        return function() {
          return _this.doCommand(player, 'SetRepeat', 'cycle');
        };
      })(this));
      App.listenTo(playerView, "control:shuffle", (function(_this) {
        return function() {
          return _this.doCommand(player, 'SetShuffle', 'toggle');
        };
      })(this));
      App.listenTo(playerView, "control:mute", (function(_this) {
        return function() {
          return appController.toggleMute(function() {
            return _this.pollingUpdate();
          });
        };
      })(this));
      App.listenTo(playerView, 'control:menu', function() {
        return App.execute("ui:playermenu", 'toggle');
      });
      if (player === 'kodi') {
        App.listenTo(playerView, "remote:toggle", (function(_this) {
          return function() {
            return App.execute("input:remote:toggle");
          };
        })(this));
      }
      $playerCtx = $('#player-' + player);
      $progress = $('.playing-progress', $playerCtx);
      if (player === 'kodi') {
        $progress.on('change', function() {
          API.timerStop();
          return API.doCommand(player, 'Seek', Math.round(this.vGet()), function() {
            return API.timerStart();
          });
        });
        $progress.on('slide', function() {
          return API.timerStop();
        });
      } else {
        $progress.on('change', function() {
          return API.doCommand(player, 'Seek', Math.round(this.vGet()));
        });
      }
      $volume = $('.volume', $playerCtx);
      return $volume.on('change', function() {
        return appController.setVolume(Math.round(this.vGet()), function() {
          return API.pollingUpdate();
        });
      });
    },
    timerStart: function() {
      return App.playingTimerInterval = setTimeout(((function(_this) {
        return function() {
          return _this.timerUpdate();
        };
      })(this)), 1000);
    },
    timerStop: function() {
      return clearTimeout(App.playingTimerInterval);
    },
    timerUpdate: function() {
      var cur, curTimeObj, dur, percent, stateObj;
      stateObj = App.request("state:kodi");
      this.timerStop();
      if (stateObj.isPlaying() && (stateObj.getPlaying('time') != null)) {
        cur = helpers.global.timeToSec(stateObj.getPlaying('time')) + 1;
        dur = helpers.global.timeToSec(stateObj.getPlaying('totaltime'));
        percent = Math.ceil(cur / dur * 100);
        curTimeObj = helpers.global.secToTime(cur);
        stateObj.setPlaying('time', curTimeObj);
        this.setProgress('kodi', percent, curTimeObj);
        return this.timerStart();
      }
    },
    setProgress: function(player, percent, currentTime) {
      var $cur, $playerCtx;
      if (percent == null) {
        percent = 0;
      }
      if (currentTime == null) {
        currentTime = 0;
      }
      $playerCtx = $('#player-' + player);
      $('.playing-progress', $playerCtx).val(percent);
      $cur = $('.playing-time-current', $playerCtx);
      return $cur.html(helpers.global.formatTime(currentTime));
    },
    initProgress: function(player, percent) {
      var $playerCtx;
      if (percent == null) {
        percent = 0;
      }
      $playerCtx = $('#player-' + player);
      return $('.playing-progress', $playerCtx).noUiSlider({
        start: percent,
        connect: 'upper',
        step: 1,
        range: {
          min: 0,
          max: 100
        }
      });
    },
    initVolume: function(player, percent) {
      var $playerCtx;
      if (percent == null) {
        percent = 50;
      }
      $playerCtx = $('#player-' + player);
      return $('.volume', $playerCtx).noUiSlider({
        start: percent,
        connect: 'upper',
        step: 1,
        range: {
          min: 0,
          max: 100
        }
      });
    }
  };
  return this.onStart = function(options) {
    App.vent.on("shell:ready", (function(_this) {
      return function(options) {
        App.kodiPlayer = API.getPlayer('kodi');
        App.listenTo(App.kodiPlayer, "show", function() {
          API.initPlayer('kodi', App.kodiPlayer);
          return App.execute("player:kodi:timer", 'start');
        });
        App.regionPlayerKodi.show(App.kodiPlayer);
        App.localPlayer = API.getPlayer('local');
        App.listenTo(App.localPlayer, "show", function() {
          return API.initPlayer('local', App.localPlayer);
        });
        return App.regionPlayerLocal.show(App.localPlayer);
      };
    })(this));
    App.commands.setHandler('player:kodi:timer', function(state) {
      if (state == null) {
        state = 'start';
      }
      if (state === 'start') {
        return API.timerStart();
      } else if (state === 'stop') {
        return API.timerStop();
      } else if (state === 'update') {
        return API.timerUpdate();
      }
    });
    App.commands.setHandler('player:local:progress:update', function(percent, currentTime) {
      return API.setProgress('local', percent, currentTime);
    });
    return App.commands.setHandler('player:kodi:progress:update', function(percent, callback) {
      return API.doCommand('kodi', 'Seek', percent, callback);
    });
  };
});

this.Kodi.module("PlayerApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  return Show.Player = (function(_super) {
    __extends(Player, _super);

    function Player() {
      return Player.__super__.constructor.apply(this, arguments);
    }

    Player.prototype.template = "apps/player/show/player";

    Player.prototype.regions = {
      regionProgress: '.playing-progress',
      regionVolume: '.volume',
      regionThumbnail: '.playing-thumb',
      regionTitle: '.playing-title',
      regionSubtitle: '.playing-subtitle',
      regionTimeCur: '.playing-time-current',
      regionTimeDur: '.playing-time-duration'
    };

    Player.prototype.triggers = {
      'click .remote-toggle': 'remote:toggle',
      'click .control-prev': 'control:prev',
      'click .control-play': 'control:play',
      'click .control-next': 'control:next',
      'click .control-stop': 'control:stop',
      'click .control-mute': 'control:mute',
      'click .control-shuffle': 'control:shuffle',
      'click .control-repeat': 'control:repeat',
      'click .control-menu': 'control:menu'
    };

    return Player;

  })(App.Views.ItemView);
});

this.Kodi.module("PlaylistApp.List", function(List, App, Backbone, Marionette, $, _) {
  return List.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function() {
      return App.vent.on("shell:ready", (function(_this) {
        return function(options) {
          return _this.getPlaylistBar();
        };
      })(this));
    };

    Controller.prototype.playlistController = function(player, media) {
      return App.request("command:" + player + ":controller", media, 'PlayList');
    };

    Controller.prototype.playerCommand = function(player, command, params) {
      if (params == null) {
        params = [];
      }
      return App.request("command:" + player + ":player", command, params, function() {
        return App.request("state:kodi:update");
      });
    };

    Controller.prototype.stateObj = function() {
      return App.request("state:current");
    };

    Controller.prototype.getPlaylistBar = function() {
      this.layout = this.getLayout();
      this.listenTo(this.layout, "show", (function(_this) {
        return function() {
          _this.renderList('kodi', 'audio');
          _this.renderList('local', 'audio');
          return App.vent.on("state:initialized", function() {
            return _this.changePlaylist(_this.stateObj().getState('player'), _this.stateObj().getState('media'));
          });
        };
      })(this));
      this.listenTo(this.layout, 'playlist:kodi:audio', (function(_this) {
        return function() {
          return _this.changePlaylist('kodi', 'audio');
        };
      })(this));
      this.listenTo(this.layout, 'playlist:kodi:video', (function(_this) {
        return function() {
          return _this.changePlaylist('kodi', 'video');
        };
      })(this));
      this.listenTo(this.layout, 'playlist:kodi', (function(_this) {
        return function() {
          _this.stateObj().setPlayer('kodi');
          return _this.renderList('kodi', 'audio');
        };
      })(this));
      this.listenTo(this.layout, 'playlist:local', (function(_this) {
        return function() {
          _this.stateObj().setPlayer('local');
          return _this.renderList('local', 'audio');
        };
      })(this));
      this.listenTo(this.layout, 'playlist:clear', (function(_this) {
        return function() {
          return _this.playlistController(_this.stateObj().getPlayer(), _this.stateObj().getState('media')).clear();
        };
      })(this));
      this.listenTo(this.layout, 'playlist:refresh', (function(_this) {
        return function() {
          _this.renderList(_this.stateObj().getPlayer(), _this.stateObj().getState('media'));
          return App.execute("notification:show", t.gettext('Playlist refreshed'));
        };
      })(this));
      this.listenTo(this.layout, 'playlist:party', (function(_this) {
        return function() {
          return _this.playerCommand('kodi', 'SetPartymode', ['toggle']);
        };
      })(this));
      this.listenTo(this.layout, 'playlist:save', (function(_this) {
        return function() {
          return App.execute("localplaylist:addentity", 'playlist');
        };
      })(this));
      return App.regionPlaylist.show(this.layout);
    };

    Controller.prototype.getLayout = function() {
      return new List.Layout();
    };

    Controller.prototype.getList = function(collection) {
      return new List.Items({
        collection: collection
      });
    };

    Controller.prototype.renderList = function(type, media) {
      var collection, listView;
      this.layout.$el.removeClassStartsWith('media-').addClass('media-' + media);
      if (type === 'kodi') {
        collection = App.request("playlist:list", type, media);
        return App.execute("when:entity:fetched", collection, (function(_this) {
          return function() {
            var listView;
            listView = _this.getList(collection);
            App.listenTo(listView, "show", function() {
              _this.bindActions(listView, type, media);
              return App.vent.trigger("state:content:updated", type, media);
            });
            return _this.layout.kodiPlayList.show(listView);
          };
        })(this));
      } else {
        collection = App.request("localplayer:get:entities");
        listView = this.getList(collection);
        App.listenTo(listView, "show", (function(_this) {
          return function() {
            _this.bindActions(listView, type, media);
            return App.vent.trigger("state:content:updated", type, media);
          };
        })(this));
        return this.layout.localPlayList.show(listView);
      }
    };

    Controller.prototype.bindActions = function(listView, type, media) {
      var playlist;
      playlist = this.playlistController(type, media);
      this.listenTo(listView, "childview:playlist:item:remove", function(playlistView, item) {
        return playlist.remove(item.model.get('position'));
      });
      this.listenTo(listView, "childview:playlist:item:play", function(playlistView, item) {
        return playlist.playEntity('position', parseInt(item.model.get('position')));
      });
      return this.initSortable(type, media);
    };

    Controller.prototype.changePlaylist = function(player, media) {
      return this.renderList(player, media);
    };

    Controller.prototype.initSortable = function(type, media) {
      var $ctx, playlist;
      $ctx = $('.' + type + '-playlist');
      playlist = this.playlistController(type, media);
      return $('ul.playlist-items', $ctx).sortable({
        onEnd: function(e) {
          return playlist.moveItem($(e.item).data('type'), $(e.item).data('id'), e.oldIndex, e.newIndex);
        }
      });
    };

    Controller.prototype.focusPlaying = function(type, media) {
      var $playing;
      if (config.getLocal('playlistFocusPlaying', true)) {
        $playing = $('.' + type + '-playlist .row-playing');
        if ($playing.length > 0) {
          return $playing.get(0).scrollIntoView();
        }
      }
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("PlaylistApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.Layout = (function(_super) {
    __extends(Layout, _super);

    function Layout() {
      return Layout.__super__.constructor.apply(this, arguments);
    }

    Layout.prototype.template = "apps/playlist/list/playlist_bar";

    Layout.prototype.tagName = "div";

    Layout.prototype.className = "playlist-bar";

    Layout.prototype.regions = {
      kodiPlayList: '.kodi-playlist',
      localPlayList: '.local-playlist'
    };

    Layout.prototype.triggers = {
      'click .kodi-playlists .media-toggle .video': 'playlist:kodi:video',
      'click .kodi-playlists .media-toggle .audio': 'playlist:kodi:audio',
      'click .player-toggle .kodi': 'playlist:kodi',
      'click .player-toggle .local': 'playlist:local',
      'click .clear-playlist': 'playlist:clear',
      'click .refresh-playlist': 'playlist:refresh',
      'click .party-mode': 'playlist:party',
      'click .save-playlist': 'playlist:save'
    };

    Layout.prototype.events = {
      'click .playlist-menu a': 'menuClick'
    };

    Layout.prototype.menuClick = function(e) {
      return e.preventDefault();
    };

    return Layout;

  })(App.Views.LayoutView);
  List.Item = (function(_super) {
    __extends(Item, _super);

    function Item() {
      return Item.__super__.constructor.apply(this, arguments);
    }

    Item.prototype.template = "apps/playlist/list/playlist_item";

    Item.prototype.tagName = "li";

    Item.prototype.initialize = function() {
      var subtitle;
      subtitle = '';
      switch (this.model.get('type')) {
        case 'song':
          subtitle = this.model.get('artist') ? this.model.get('artist').join(', ') : '';
          break;
        default:
          subtitle = '';
      }
      return this.model.set({
        subtitle: subtitle
      });
    };

    Item.prototype.triggers = {
      "click .remove": "playlist:item:remove",
      "click .play": "playlist:item:play"
    };

    Item.prototype.attributes = function() {
      return {
        "class": 'item pos-' + this.model.get('position'),
        'data-type': this.model.get('type'),
        'data-id': this.model.get('id')
      };
    };

    return Item;

  })(App.Views.ItemView);
  return List.Items = (function(_super) {
    __extends(Items, _super);

    function Items() {
      return Items.__super__.constructor.apply(this, arguments);
    }

    Items.prototype.childView = List.Item;

    Items.prototype.tagName = "ul";

    Items.prototype.className = "playlist-items";

    return Items;

  })(App.Views.CollectionView);
});

this.Kodi.module("PlaylistApp", function(PlaylistApp, App, Backbone, Marionette, $, _) {
  var API;
  PlaylistApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "playlist": "list"
    };

    return Router;

  })(App.Router.Base);
  API = {
    list: function() {
      return new PlaylistApp.Show.Controller();
    },
    type: 'kodi',
    media: 'audio',
    setContext: function(type, media) {
      if (type == null) {
        type = 'kodi';
      }
      if (media == null) {
        media = 'audio';
      }
      this.type = type;
      return this.media = media;
    },
    getController: function() {
      return App.request("command:" + this.type + ":controller", this.media, 'PlayList');
    },
    getPlaylistItems: function() {
      return App.request("playlist:" + this.type + ":entities", this.media);
    }
  };
  App.reqres.setHandler("playlist:list", function(type, media) {
    API.setContext(type, media);
    return API.getPlaylistItems();
  });
  App.on("before:start", function() {
    return new PlaylistApp.Router({
      controller: API
    });
  });
  return App.addInitializer(function() {
    var controller;
    controller = new PlaylistApp.List.Controller();
    App.commands.setHandler("playlist:refresh", function(type, media) {
      return controller.renderList(type, media);
    });
    return App.vent.on("state:kodi:playing:updated", function(stateObj) {
      return controller.focusPlaying(stateObj.getState('player'), stateObj.getPlaying());
    });
  });
});

this.Kodi.module("PlaylistApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  return Show.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      this.landing = this.getLanding();
      return App.regionContent.show(this.landing);
    };

    Controller.prototype.getLanding = function() {
      return new Show.Landing();
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("PlaylistApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  return Show.Landing = (function(_super) {
    __extends(Landing, _super);

    function Landing() {
      return Landing.__super__.constructor.apply(this, arguments);
    }

    Landing.prototype.template = 'apps/playlist/show/landing';

    return Landing;

  })(App.Views.ItemView);
});

this.Kodi.module("ChannelApp.List", function(List, App, Backbone, Marionette, $, _) {
  return List.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      var collection;
      collection = App.request("channel:entities", options.group);
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          _this.layout = _this.getLayoutView(collection);
          _this.listenTo(_this.layout, "show", function() {
            _this.renderChannels(collection);
            return _this.getSubNav();
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getLayoutView = function(collection) {
      return new List.Layout({
        collection: collection
      });
    };

    Controller.prototype.renderChannels = function(collection) {
      var view;
      view = new List.ChannelList({
        collection: collection
      });
      this.listenTo(view, 'childview:channel:play', function(parent, child) {
        var player;
        player = App.request("command:kodi:controller", 'auto', 'Player');
        return player.playEntity('channelid', child.model.get('id'), {}, (function(_this) {
          return function() {};
        })(this));
      });
      this.listenTo(view, 'childview:channel:record', function(parent, child) {
        var record;
        record = App.request("command:kodi:controller", 'auto', 'PVR');
        return record.setPVRRecord(child.model.get('id'), {
          "record": "toggle"
        }, (function(_this) {
          return function() {
            return App.execute("notification:show", t.gettext("Channel recording toggled"));
          };
        })(this));
      });
      return this.layout.regionContent.show(view);
    };

    Controller.prototype.getSubNav = function() {
      var subNav, subNavId;
      subNavId = this.getOption('group') === 'alltv' ? 'tvshows/recent' : 'music';
      subNav = App.request("navMain:children:show", subNavId, 'Sections');
      return this.layout.regionSidebarFirst.show(subNav);
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("ChannelApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.Layout = (function(_super) {
    __extends(Layout, _super);

    function Layout() {
      return Layout.__super__.constructor.apply(this, arguments);
    }

    Layout.prototype.className = "pvr-page";

    return Layout;

  })(App.Views.LayoutWithSidebarFirstView);
  List.ChannelTeaser = (function(_super) {
    __extends(ChannelTeaser, _super);

    function ChannelTeaser() {
      return ChannelTeaser.__super__.constructor.apply(this, arguments);
    }

    ChannelTeaser.prototype.tagName = "li";

    ChannelTeaser.prototype.triggers = {
      "click .play": "channel:play",
      "click .record": "channel:record"
    };

    ChannelTeaser.prototype.initialize = function() {
      ChannelTeaser.__super__.initialize.apply(this, arguments);
      if (this.model != null) {
        return this.model.set({
          subtitle: this.model.get('broadcastnow').title
        });
      }
    };

    return ChannelTeaser;

  })(App.Views.CardView);
  return List.ChannelList = (function(_super) {
    __extends(ChannelList, _super);

    function ChannelList() {
      return ChannelList.__super__.constructor.apply(this, arguments);
    }

    ChannelList.prototype.childView = List.ChannelTeaser;

    ChannelList.prototype.tagName = "ul";

    ChannelList.prototype.className = "card-grid--square";

    return ChannelList;

  })(App.Views.CollectionView);
});

this.Kodi.module("ChannelApp", function(ChannelApp, App, Backbone, Marionette, $, _) {
  var API;
  ChannelApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "tvshows/live": "tv",
      "music/radio": "radio"
    };

    return Router;

  })(App.Router.Base);
  API = {
    tv: function() {
      return new ChannelApp.List.Controller({
        group: 'alltv'
      });
    },
    radio: function() {
      return new ChannelApp.List.Controller({
        group: 'allradio'
      });
    }
  };
  return App.on("before:start", function() {
    return new ChannelApp.Router({
      controller: API
    });
  });
});

this.Kodi.module("SearchApp.List", function(List, App, Backbone, Marionette, $, _) {
  return List.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      this.updateProgress = __bind(this.updateProgress, this);
      this.getLoader = __bind(this.getLoader, this);
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function() {
      var media;
      this.layout = this.getLayout();
      this.processed = [];
      media = this.getOption('media');
      if (media === 'all') {
        this.entities = ['song', 'artist', 'album', 'tvshow', 'movie'];
      } else {
        this.entities = [media];
      }
      this.listenTo(this.layout, "show", (function(_this) {
        return function() {
          var entity, _i, _len, _ref, _results;
          _this.getLoader();
          _ref = _this.entities;
          _results = [];
          for (_i = 0, _len = _ref.length; _i < _len; _i++) {
            entity = _ref[_i];
            _results.push(_this.getResult(entity));
          }
          return _results;
        };
      })(this));
      return App.regionContent.show(this.layout);
    };

    Controller.prototype.getLayout = function() {
      return new List.ListLayout();
    };

    Controller.prototype.getLoader = function() {
      var query, searchNames, text;
      searchNames = helpers.global.arrayToSentence(_.difference(this.entities, this.processed));
      query = helpers.global.arrayToSentence([this.getOption('query')], false);
      text = t.gettext('Searching for') + ' ' + query + ' ' + t.gettext('in') + ' ' + searchNames;
      return App.execute("loading:show:view", this.layout.loadingSet, text);
    };

    Controller.prototype.getResult = function(entity) {
      var limit, query;
      query = this.getOption('query');
      limit = this.getOption('media') === 'all' ? 'limit' : 'all';
      return App.execute("" + entity + ":search:entities", query, limit, (function(_this) {
        return function(loaded) {
          var setView, view;
          if (loaded.length > 0) {
            view = App.request("" + entity + ":list:view", loaded, true);
            setView = new List.ListSet({
              entity: entity,
              more: (loaded.more ? true : false),
              query: query
            });
            App.listenTo(setView, "show", function() {
              return setView.regionResult.show(view);
            });
            _this.layout["" + entity + "Set"].show(setView);
          }
          return _this.updateProgress(entity);
        };
      })(this));
    };

    Controller.prototype.updateProgress = function(done) {
      if (done != null) {
        this.processed.push(done);
      }
      this.getLoader();
      if (this.processed.length === this.entities.length) {
        return this.layout.loadingSet.$el.empty();
      }
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("SearchApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.ListLayout = (function(_super) {
    __extends(ListLayout, _super);

    function ListLayout() {
      return ListLayout.__super__.constructor.apply(this, arguments);
    }

    ListLayout.prototype.template = 'apps/search/list/search_layout';

    ListLayout.prototype.className = "search-page set-page";

    ListLayout.prototype.regions = {
      artistSet: '.entity-set-artist',
      albumSet: '.entity-set-album',
      songSet: '.entity-set-song',
      movieSet: '.entity-set-movie',
      tvshowSet: '.entity-set-tvshow',
      loadingSet: '.entity-set-loading'
    };

    return ListLayout;

  })(App.Views.LayoutView);
  return List.ListSet = (function(_super) {
    __extends(ListSet, _super);

    function ListSet() {
      return ListSet.__super__.constructor.apply(this, arguments);
    }

    ListSet.prototype.template = 'apps/search/list/search_set';

    ListSet.prototype.className = "search-set";

    ListSet.prototype.onRender = function() {
      var moreLink;
      if (this.options) {
        if (this.options.entity) {
          $('h2.set-header', this.$el).html(t.gettext(this.options.entity + 's'));
          if (this.options.more && this.options.query) {
            moreLink = this.themeLink(t.gettext('Show more'), 'search/' + this.options.entity + '/' + this.options.query);
            return $('.more', this.$el).html(moreLink);
          }
        }
      }
    };

    ListSet.prototype.regions = {
      regionResult: '.set-results'
    };

    return ListSet;

  })(App.Views.LayoutView);
});

this.Kodi.module("SearchApp", function(SearchApp, App, Backbone, Marionette, $, _) {
  var API;
  SearchApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "search": "view",
      "search/:media/:query": "list"
    };

    return Router;

  })(App.Router.Base);
  API = {
    keyUpTimeout: 2000,
    list: function(media, query) {
      var $search;
      App.navigate("search/" + media + "/" + query);
      $search = $('#search');
      if ($search.val() === '') {
        $search.val(query);
      }
      return new SearchApp.List.Controller({
        query: query,
        media: media
      });
    },
    view: function() {
      return new SearchApp.Show.Controller();
    },
    searchBind: function() {
      return $('#search').on('keyup', function(e) {
        var val;
        $('#search-region').removeClass('pre-search');
        val = $('#search').val();
        clearTimeout(App.searchAllTimeout);
        if (e.which === 13) {
          return API.list('all', val);
        } else {
          $('#search-region').addClass('pre-search');
          return App.searchAllTimeout = setTimeout((function() {
            $('#search-region').removeClass('pre-search');
            return API.list('all', val);
          }), API.keyUpTimeout);
        }
      });
    }
  };
  App.on("before:start", function() {
    return new SearchApp.Router({
      controller: API
    });
  });
  return App.addInitializer(function() {
    return App.vent.on("shell:ready", function() {
      return API.searchBind();
    });
  });
});

this.Kodi.module("SearchApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  return Show.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      this.landing = this.getLanding();
      this.listenTo(this.landing, "show", (function(_this) {
        return function() {
          return $('#search').focus();
        };
      })(this));
      return App.regionContent.show(this.landing);
    };

    Controller.prototype.getLanding = function() {
      return new Show.Landing();
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("SearchApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  return Show.Landing = (function(_super) {
    __extends(Landing, _super);

    function Landing() {
      return Landing.__super__.constructor.apply(this, arguments);
    }

    Landing.prototype.template = 'apps/search/show/landing';

    return Landing;

  })(App.Views.ItemView);
});

this.Kodi.module("SettingsApp", function(SettingsApp, App, Backbone, Marionette, $, _) {
  var API;
  SettingsApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "settings/web": "local",
      "settings/kodi": "kodi",
      "settings/kodi/:section": "kodi",
      "settings/addons": "addons",
      "settings/nav": "navMain"
    };

    return Router;

  })(App.Router.Base);
  API = {
    subNavId: 'settings/web',
    local: function() {
      return new SettingsApp.Show.Local.Controller();
    },
    addons: function() {
      return new SettingsApp.Show.Addons.Controller();
    },
    navMain: function() {
      return new SettingsApp.Show.navMain.Controller();
    },
    kodi: function(section, category) {
      return new SettingsApp.Show.Kodi.Controller({
        section: section,
        category: category
      });
    },
    getSubNav: function() {
      var collection, sidebarView;
      collection = App.request("settings:kodi:entities", {
        type: 'sections'
      });
      sidebarView = new SettingsApp.Show.Sidebar();
      App.listenTo(sidebarView, "show", (function(_this) {
        return function() {
          var settingsNavView;
          App.execute("when:entity:fetched", collection, function() {
            var kodiSettingsView;
            kodiSettingsView = App.request("navMain:collection:show", collection, t.gettext('Kodi settings'));
            return sidebarView.regionKodiNav.show(kodiSettingsView);
          });
          settingsNavView = App.request("navMain:children:show", API.subNavId, 'General');
          return sidebarView.regionLocalNav.show(settingsNavView);
        };
      })(this));
      return sidebarView;
    }
  };
  App.on("before:start", function() {
    return new SettingsApp.Router({
      controller: API
    });
  });
  return App.reqres.setHandler('settings:subnav', function() {
    return API.getSubNav();
  });
});

this.Kodi.module("SettingsApp.Show.Base", function(SettingsBase, App, Backbone, Marionette, $, _) {
  return SettingsBase.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function() {
      this.layout = this.getLayoutView();
      this.listenTo(this.layout, "show", (function(_this) {
        return function() {
          _this.getSubNav();
          return _this.getForm();
        };
      })(this));
      return App.regionContent.show(this.layout);
    };

    Controller.prototype.getLayoutView = function() {
      return new App.SettingsApp.Show.Layout();
    };

    Controller.prototype.getSubNav = function() {
      var subNav;
      subNav = App.request('settings:subnav');
      return this.layout.regionSidebarFirst.show(subNav);
    };

    Controller.prototype.getForm = function() {
      return this.getCollection((function(_this) {
        return function(collection) {
          var form, options;
          options = {
            form: _this.getStructure(collection),
            formState: [],
            config: {
              attributes: {
                "class": 'settings-form'
              },
              callback: function(formState, formView) {
                return _this.saveCallback(formState, formView);
              },
              onShow: function() {
                return _this.onReady();
              }
            }
          };
          form = App.request("form:wrapper", options);
          return _this.layout.regionContent.show(form);
        };
      })(this));
    };

    Controller.prototype.getCollection = function(callback) {
      var res;
      res = {};
      return callback(res);
    };

    Controller.prototype.getStructure = function(collection) {
      return [];
    };

    Controller.prototype.saveCallback = function(formState, formView) {};

    Controller.prototype.onReady = function() {
      return this.layout;
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("SettingsApp.Show.Addons", function(Addons, App, Backbone, Marionette, $, _) {
  return Addons.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function() {
      this.layout = this.getLayoutView();
      this.listenTo(this.layout, "show", (function(_this) {
        return function() {
          _this.getSubNav();
          return _this.getForm();
        };
      })(this));
      return App.regionContent.show(this.layout);
    };

    Controller.prototype.getLayoutView = function() {
      return new App.SettingsApp.Show.Layout();
    };

    Controller.prototype.getSubNav = function() {
      var subNav;
      subNav = App.request('settings:subnav');
      return this.layout.regionSidebarFirst.show(subNav);
    };

    Controller.prototype.addonController = function() {
      return App.request("command:kodi:controller", 'auto', 'AddOn');
    };

    Controller.prototype.getAllAddons = function(callback) {
      return this.addonController().getAllAddons(callback);
    };

    Controller.prototype.getForm = function() {
      return this.getAllAddons((function(_this) {
        return function(addons) {
          var form, options;
          options = {
            form: _this.getStructure(addons),
            formState: [],
            config: {
              attributes: {
                "class": 'settings-form'
              },
              callback: function(data, formView) {
                return _this.saveCallback(data, formView);
              }
            }
          };
          form = App.request("form:wrapper", options);
          return _this.layout.regionContent.show(form);
        };
      })(this));
    };

    Controller.prototype.getStructure = function(addons) {
      var addon, el, elements, enabled, form, i, type, types;
      form = [];
      types = [];
      for (i in addons) {
        addon = addons[i];
        types[addon.type] = true;
      }
      for (type in types) {
        enabled = types[type];
        elements = _.where(addons, {
          type: type
        });
        for (i in elements) {
          el = elements[i];
          elements[i] = $.extend(el, {
            id: el.addonid,
            type: 'checkbox',
            defaultValue: el.enabled,
            title: el.name
          });
        }
        form.push({
          title: type,
          id: type,
          children: elements
        });
      }
      return form;
    };

    Controller.prototype.saveCallback = function(data, formView) {
      var updating;
      updating = [];
      return this.getAllAddons((function(_this) {
        return function(addons) {
          var addon, addonid, commander, commands, key, val;
          for (key in addons) {
            addon = addons[key];
            addonid = addon.addonid;
            if (addon.enabled === !data[addonid]) {
              updating[addonid] = data[addonid];
            }
          }
          commander = App.request("command:kodi:controller", 'auto', 'Commander');
          commands = [];
          for (key in updating) {
            val = updating[key];
            commands.push({
              method: 'Addons.SetAddonEnabled',
              params: [key, val]
            });
          }
          return commander.multipleCommands(commands, function(resp) {
            return Kodi.execute("notification:show", 'Toggled ' + commands.length + ' addons');
          });
        };
      })(this));
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("SettingsApp.Show.Kodi", function(Kodi, App, Backbone, Marionette, $, _) {
  return Kodi.Controller = (function(_super) {
    var API;

    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    API = {
      optionLookups: {
        'lookandfeel.skin': 'xbmc.gui.skin',
        'locale.language': 'kodi.resource.language',
        'screensaver.mode': 'xbmc.ui.screensaver',
        'musiclibrary.albumsscraper': 'xbmc.metadata.scraper.albums',
        'musiclibrary.artistsscraper': 'xbmc.metadata.scraper.artists',
        'musicplayer.visualisation': 'xbmc.player.musicviz',
        'services.webskin': 'xbmc.webinterface',
        'subtitles.tv': 'xbmc.subtitle.module',
        'subtitles.movie': 'xbmc.subtitle.module',
        'audiocds.encoder': 'xbmc.audioencoder'
      },
      parseOptions: function(options) {
        var out;
        out = {};
        $(options).each(function(i, option) {
          return out[option.value] = option.label;
        });
        return out;
      }
    };

    Controller.prototype.initialize = function(options) {
      this.layout = this.getLayoutView();
      this.listenTo(this.layout, "show", (function(_this) {
        return function() {
          _this.getSubNav();
          if (options.section) {
            return _this.getSettingsForm(options.section);
          }
        };
      })(this));
      return App.regionContent.show(this.layout);
    };

    Controller.prototype.getLayoutView = function() {
      return new App.SettingsApp.Show.Layout();
    };

    Controller.prototype.getSubNav = function() {
      var subNav;
      subNav = App.request('settings:subnav');
      return this.layout.regionSidebarFirst.show(subNav);
    };

    Controller.prototype.getSettingsForm = function(section) {
      var categoryCollection, formStructure;
      formStructure = [];
      categoryCollection = App.request("settings:kodi:entities", {
        type: 'categories',
        section: section
      });
      return App.execute("when:entity:fetched", categoryCollection, (function(_this) {
        return function() {
          var categories, categoryNames;
          categoryNames = categoryCollection.pluck("id");
          categories = categoryCollection.toJSON();
          return App.request("settings:kodi:filtered:entities", {
            type: 'settings',
            section: section,
            categories: categoryNames,
            callback: function(categorySettings) {
              $(categories).each(function(i, category) {
                var items;
                items = _this.mapSettingsToElements(categorySettings[category.id]);
                if (items.length > 0) {
                  return formStructure.push({
                    title: category.title,
                    id: category.id,
                    children: items
                  });
                }
              });
              return _this.getForm(section, formStructure);
            }
          });
        };
      })(this));
    };

    Controller.prototype.getForm = function(section, formStructure) {
      var form, options;
      options = {
        form: formStructure,
        config: {
          attributes: {
            "class": 'settings-form'
          },
          callback: (function(_this) {
            return function(data, formView) {
              return _this.saveCallback(data, formView);
            };
          })(this)
        }
      };
      form = App.request("form:wrapper", options);
      return this.layout.regionContent.show(form);
    };

    Controller.prototype.getAddonOptions = function(elId, value) {
      var addon, addons, filteredAddons, i, lookup, mappedType, options;
      mappedType = API.optionLookups[elId];
      options = [];
      lookup = {};
      if (mappedType) {
        addons = App.request('addon:enabled:addons');
        filteredAddons = _.where(addons, {
          type: mappedType
        });
        for (i in filteredAddons) {
          addon = filteredAddons[i];
          options.push({
            value: addon.addonid,
            label: addon.name
          });
          lookup[addon.addonid] = true;
        }
        if (!lookup[value]) {
          options.push({
            value: value,
            label: value
          });
        }
        return options;
      }
      return false;
    };

    Controller.prototype.mapSettingsToElements = function(items) {
      var elements;
      elements = [];
      $(items).each((function(_this) {
        return function(i, item) {
          var options, type;
          type = null;
          switch (item.type) {
            case 'boolean':
              type = 'checkbox';
              break;
            case 'path':
              type = 'textfield';
              break;
            case 'addon':
              options = _this.getAddonOptions(item.id, item.value);
              if (options) {
                item.options = options;
              } else {
                type = 'textfield';
              }
              break;
            case 'integer':
              type = 'textfield';
              break;
            case 'string':
              type = 'textfield';
              break;
            default:
              type = 'hide';
          }
          if (item.options) {
            type = 'select';
            item.options = API.parseOptions(item.options);
          }
          if (type === 'hide') {
            return console.log('no setting to field mapping for: ' + item.type + ' -> ' + item.id);
          } else {
            item.type = type;
            item.defaultValue = item.value;
            return elements.push(item);
          }
        };
      })(this));
      return elements;
    };

    Controller.prototype.saveCallback = function(data, formView) {
      return App.execute("settings:kodi:save:entities", data, (function(_this) {
        return function(resp) {
          return App.execute("notification:show", t.gettext("Saved Kodi settings"));
        };
      })(this));
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("SettingsApp.Show.Kodi", function(Kodi, App, Backbone, Marionette, $, _) {});

this.Kodi.module("SettingsApp.Show.Local", function(Local, App, Backbone, Marionette, $, _) {
  return Local.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function() {
      this.layout = this.getLayoutView();
      this.listenTo(this.layout, "show", (function(_this) {
        return function() {
          _this.getSubNav();
          return _this.getForm();
        };
      })(this));
      return App.regionContent.show(this.layout);
    };

    Controller.prototype.getLayoutView = function() {
      return new App.SettingsApp.Show.Layout();
    };

    Controller.prototype.getSubNav = function() {
      var subNav;
      subNav = App.request('settings:subnav');
      return this.layout.regionSidebarFirst.show(subNav);
    };

    Controller.prototype.getForm = function() {
      var form, options;
      options = {
        form: this.getStructure(),
        formState: this.getState(),
        config: {
          attributes: {
            "class": 'settings-form'
          },
          callback: (function(_this) {
            return function(data, formView) {
              return _this.saveCallback(data, formView);
            };
          })(this)
        }
      };
      form = App.request("form:wrapper", options);
      return this.layout.regionContent.show(form);
    };

    Controller.prototype.getStructure = function() {
      return [
        {
          title: 'General options',
          id: 'general',
          children: [
            {
              id: 'lang',
              title: t.gettext("Language"),
              type: 'select',
              options: helpers.translate.getLanguages(),
              defaultValue: 'en',
              description: t.gettext('Preferred language, need to refresh browser to take effect')
            }, {
              id: 'defaultPlayer',
              title: t.gettext("Default player"),
              type: 'select',
              options: {
                auto: 'Auto',
                kodi: 'Kodi',
                local: 'Local'
              },
              defaultValue: 'auto',
              description: t.gettext('Which player to start with')
            }, {
              id: 'keyboardControl',
              title: t.gettext("Keyboard controls"),
              type: 'select',
              options: {
                kodi: 'Kodi',
                local: 'Browser',
                both: 'Both'
              },
              defaultValue: 'kodi',
              description: t.gettext('In Chorus, will you keyboard control Kodi, the browser or both') + '. <a href="#help/keybind-readme">' + t.gettext('Learn more') + '</a>'
            }
          ]
        }, {
          title: 'List options',
          id: 'list',
          children: [
            {
              id: 'ignoreArticle',
              title: t.gettext("Ignore article"),
              type: 'checkbox',
              defaultValue: true,
              description: t.gettext("Ignore articles (terms such as 'The' and 'A') when sorting lists")
            }, {
              id: 'albumAtristsOnly',
              title: t.gettext("Album artists only"),
              type: 'checkbox',
              defaultValue: true,
              description: t.gettext('When listing artists should we only see artists with albums or all artists found. Warning: turning this off can impact performance with large libraries')
            }, {
              id: 'playlistFocusPlaying',
              title: t.gettext("Focus playlist on playing"),
              type: 'checkbox',
              defaultValue: true,
              description: t.gettext('Automatically scroll the playlist to the current playing item. This happens whenever the playing item is changed')
            }
          ]
        }, {
          title: 'Appearance',
          id: 'appearance',
          children: [
            {
              id: 'vibrantHeaders',
              title: t.gettext("Vibrant headers"),
              type: 'checkbox',
              defaultValue: true,
              description: t.gettext("Use colourful headers for media pages")
            }
          ]
        }, {
          title: 'Advanced options',
          id: 'advanced',
          children: [
            {
              id: 'socketsPort',
              title: t.gettext("Websockets port"),
              type: 'textfield',
              defaultValue: '9090',
              description: "9090 " + t.gettext("is the default")
            }, {
              id: 'socketsHost',
              title: t.gettext("Websockets host"),
              type: 'textfield',
              defaultValue: 'auto',
              description: t.gettext("The hostname used for websockets connection. Set to 'auto' to use the current hostname.")
            }, {
              id: 'pollInterval',
              title: t.gettext("Poll interval"),
              type: 'select',
              defaultValue: '10000',
              options: {
                '5000': "5 " + t.gettext('sec'),
                '10000': "10 " + t.gettext('sec'),
                '30000': "30 " + t.gettext('sec'),
                '60000': "60 " + t.gettext('sec')
              },
              description: t.gettext("How often do I poll for updates from Kodi (Only applies when websockets inactive)")
            }, {
              id: 'kodiSettingsLevel',
              title: t.gettext("Kodi settings level"),
              type: 'select',
              defaultValue: 'standard',
              options: {
                'standard': 'Standard',
                'advanced': 'Advanced'
              },
              description: t.gettext('Advanced setting level is recommended for those who know what they are doing.')
            }, {
              id: 'reverseProxy',
              title: t.gettext("Reverse proxy support"),
              type: 'checkbox',
              defaultValue: false,
              description: t.gettext('Enable support for reverse proxying.')
            }
          ]
        }
      ];
    };

    Controller.prototype.getState = function() {
      return config.get('app', 'config:local', config["static"]);
    };

    Controller.prototype.saveCallback = function(data, formView) {
      config.set('app', 'config:local', data);
      config["static"] = _.extend(config["static"], config.get('app', 'config:local', config["static"]));
      return Kodi.execute("notification:show", t.gettext("Web Settings saved."));
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("SettingsApp.Show.navMain", function(NavMain, App, Backbone, Marionette, $, _) {
  return NavMain.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      this.onReady = __bind(this.onReady, this);
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.getCollection = function(callback) {
      var collection;
      collection = App.request('navMain:entities');
      return $.getJSON('lib/icons/mdi.json', function(iconList) {
        return callback({
          collection: collection,
          icons: iconList
        });
      });
    };

    Controller.prototype.getStructure = function(data) {
      var defaults, form, i, item, row, _ref;
      this.data = data;
      defaults = ' <a class="nav-restore-defaults">' + t.gettext('Click here restore defaults') + '</a>';
      form = [
        {
          title: t.gettext('Main Menu Structure'),
          id: 'intro',
          children: [
            {
              id: 'intro-text',
              type: 'markup',
              markup: t.gettext('Here you can change the title, url and icons for menu items. You can also remove, re-order and add new items.') + defaults
            }
          ]
        }
      ];
      _ref = data.collection.getRawCollection();
      for (i in _ref) {
        item = _ref[i];
        item.weight = i;
        console.log(item);
        row = this.getRow(item, data.icons);
        form.push(row);
      }
      form.push({
        id: 'add-another',
        "class": 'add-another-wrapper',
        children: [
          {
            type: 'button',
            value: 'Add another',
            id: 'add-another'
          }
        ]
      });
      return form;
    };

    Controller.prototype.saveCallback = function(formState, formView) {
      var i, item, items, _ref;
      items = [];
      _ref = formState.title;
      for (i in _ref) {
        item = _ref[i];
        items.push({
          title: formState.title[i],
          path: formState.path[i],
          icon: formState.icon[i],
          weight: formState.weight[i],
          id: formState.id[i],
          parent: 0
        });
      }
      App.request("navMain:update:entities", items);
      App.vent.trigger("navMain:refresh");
      return Kodi.execute("notification:show", t.gettext('Menu updated'));
    };

    Controller.prototype.onReady = function() {
      var $ctx, self;
      self = this;
      $ctx = this.layout.regionContent.$el;
      $('.settings-form').addClass('settings-form-draggable');
      this.binds();
      $('#form-edit-add-another', $ctx).click(function(e) {
        var blank, formView, row;
        e.preventDefault();
        blank = {
          weight: $(".nav-item-row").length + 1,
          title: '',
          path: '',
          icon: 'mdi-action-extension'
        };
        row = self.getRow(blank);
        formView = App.request("form:render:items", [row]);
        $(this).closest('.add-another-wrapper').before(formView.render().$el);
        return self.binds();
      });
      $('.form-groups', $ctx).sortable({
        draggable: ".draggable-row",
        onEnd: function(e) {
          return $('input[id^="form-edit-weight-"]', e.target).each(function(i, d) {
            return $(d).attr('value', i);
          });
        }
      });
      return $('.nav-restore-defaults', $ctx).on("click", (function(_this) {
        return function(e) {
          e.preventDefault();
          App.request("navMain:update:defaults");
          _this.initialize();
          return App.vent.trigger("navMain:refresh");
        };
      })(this));
    };

    Controller.prototype.binds = function() {
      var $ctx;
      $ctx = $('.settings-form');
      $('select[id^="form-edit-icon"]', $ctx).once('icon-changer').on("change", function(e) {
        return $(this).closest('.group-parent', $ctx).find('i').first().attr('class', $(this).val());
      });
      $('.remove-item', $ctx).on("click", function(e) {
        return $(this).closest('.group-parent', $ctx).remove();
      });
      return $.material.init();
    };

    Controller.prototype.getRow = function(item) {
      var i, icon, icons;
      icons = this.data.icons;
      i = item.weight;
      icon = '<i class="' + item.icon + '"></i>';
      return {
        id: 'item-' + item.weight,
        "class": 'nav-item-row draggable-row',
        children: [
          {
            id: 'title-' + i,
            name: 'title[]',
            type: 'textfield',
            title: 'Title',
            defaultValue: item.title
          }, {
            id: 'path-' + i,
            name: 'path[]',
            type: 'textfield',
            title: 'Url',
            defaultValue: item.path
          }, {
            id: 'icon-' + i,
            name: 'icon[]',
            type: 'select',
            title: 'Icon' + icon,
            defaultValue: item.icon,
            options: icons
          }, {
            id: 'weight-' + i,
            name: 'weight[]',
            type: 'hidden',
            title: '',
            defaultValue: i
          }, {
            id: 'id-' + i,
            name: 'id[]',
            type: 'hidden',
            title: '',
            defaultValue: 1000 + i
          }, {
            id: 'remove-' + i,
            type: 'markup',
            markup: '<span class="remove-item">&times;</span>'
          }
        ]
      };
    };

    return Controller;

  })(App.SettingsApp.Show.Base.Controller);
});

this.Kodi.module("SettingsApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  Show.Layout = (function(_super) {
    __extends(Layout, _super);

    function Layout() {
      return Layout.__super__.constructor.apply(this, arguments);
    }

    Layout.prototype.className = "settings-page";

    return Layout;

  })(App.Views.LayoutWithSidebarFirstView);
  return Show.Sidebar = (function(_super) {
    __extends(Sidebar, _super);

    function Sidebar() {
      return Sidebar.__super__.constructor.apply(this, arguments);
    }

    Sidebar.prototype.className = "settings-sidebar";

    Sidebar.prototype.template = "apps/settings/show/settings_sidebar";

    Sidebar.prototype.tagName = "div";

    Sidebar.prototype.regions = {
      regionKodiNav: '.kodi-nav',
      regionLocalNav: '.local-nav'
    };

    return Sidebar;

  })(App.Views.LayoutView);
});

this.Kodi.module("Shell", function(Shell, App, Backbone, Marionette, $, _) {
  var API;
  Shell.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "": "homePage",
      "home": "homePage"
    };

    return Router;

  })(App.Router.Base);
  API = {
    homePage: function() {
      var home;
      home = new Shell.HomepageLayout();
      App.regionContent.show(home);
      this.setFanart();
      App.vent.on("state:changed", (function(_this) {
        return function(state) {
          var stateObj;
          stateObj = App.request("state:current");
          if (stateObj.isPlayingItemChanged() && helpers.url.arg(0) === '') {
            return _this.setFanart();
          }
        };
      })(this));
      return App.listenTo(home, "destroy", (function(_this) {
        return function() {
          return App.execute("images:fanart:set", 'none');
        };
      })(this));
    },
    setFanart: function() {
      var playingItem, stateObj;
      stateObj = App.request("state:current");
      if (stateObj != null) {
        playingItem = stateObj.getPlaying('item');
        return App.execute("images:fanart:set", playingItem.fanart);
      } else {
        return App.execute("images:fanart:set");
      }
    },
    renderLayout: function() {
      var playlistState, shellLayout;
      shellLayout = new Shell.Layout();
      App.root.show(shellLayout);
      App.addRegions(shellLayout.regions);
      App.execute("loading:show:page");
      playlistState = config.get('app', 'shell:playlist:state', 'open');
      if (playlistState === 'closed') {
        this.alterRegionClasses('add', "shell-playlist-closed");
      }
      App.listenTo(shellLayout, "shell:playlist:toggle", (function(_this) {
        return function(child, args) {
          var state;
          playlistState = config.get('app', 'shell:playlist:state', 'open');
          state = playlistState === 'open' ? 'closed' : 'open';
          config.set('app', 'shell:playlist:state', state);
          return _this.alterRegionClasses('toggle', "shell-playlist-closed");
        };
      })(this));
      App.listenTo(shellLayout, "shell:audio:scan", (function(_this) {
        return function() {
          return App.request("command:kodi:controller", 'auto', 'AudioLibrary').scan();
        };
      })(this));
      App.listenTo(shellLayout, "shell:video:scan", (function(_this) {
        return function() {
          return App.request("command:kodi:controller", 'auto', 'VideoLibrary').scan();
        };
      })(this));
      App.listenTo(shellLayout, "shell:goto:lab", (function(_this) {
        return function() {
          return App.navigate("#lab", {
            trigger: true
          });
        };
      })(this));
      App.listenTo(shellLayout, "shell:send:input", (function(_this) {
        return function() {
          return App.execute("input:textbox", '');
        };
      })(this));
      return App.listenTo(shellLayout, "shell:about", (function(_this) {
        return function() {
          return App.navigate("#help", {
            trigger: true
          });
        };
      })(this));
    },
    alterRegionClasses: function(op, classes, region) {
      var $body, action;
      if (region == null) {
        region = 'root';
      }
      $body = App.getRegion(region).$el;
      action = "" + op + "Class";
      return $body[action](classes);
    }
  };
  return App.addInitializer(function() {
    return App.commands.setHandler("shell:view:ready", function() {
      API.renderLayout();
      new Shell.Router({
        controller: API
      });
      App.vent.trigger("shell:ready");
      return App.commands.setHandler("body:state", function(op, state) {
        return API.alterRegionClasses(op, state);
      });
    });
  });
});

this.Kodi.module("Shell", function(Shell, App, Backbone, Marionette, $, _) {
  Shell.Layout = (function(_super) {
    __extends(Layout, _super);

    function Layout() {
      return Layout.__super__.constructor.apply(this, arguments);
    }

    Layout.prototype.template = "apps/shell/show/shell";

    Layout.prototype.regions = {
      regionNav: '#nav-bar',
      regionContent: '#content',
      regionSidebarFirst: '#sidebar-first',
      regionPlaylist: '#playlist-bar',
      regionTitle: '#page-title .title',
      regionTitleContext: '#page-title .context',
      regionFanart: '#fanart',
      regionPlayerKodi: '#player-kodi',
      regionPlayerLocal: '#player-local',
      regionModal: '#modal-window',
      regionModalTitle: '.modal-title',
      regionModalBody: '.modal-body',
      regionModalFooter: '.modal-footer',
      regionRemote: '#remote',
      regionSearch: '#search-region'
    };

    Layout.prototype.triggers = {
      "click .playlist-toggle-open": "shell:playlist:toggle",
      "click .audio-scan": "shell:audio:scan",
      "click .video-scan": "shell:video:scan",
      "click .goto-lab": "shell:goto:lab",
      "click .send-input": "shell:send:input",
      "click .about": "shell:about"
    };

    Layout.prototype.events = {
      "click .player-menu > li": "closePlayerMenu"
    };

    Layout.prototype.closePlayerMenu = function() {
      return App.execute("ui:playermenu", 'close');
    };

    return Layout;

  })(Backbone.Marionette.LayoutView);
  Shell.HomepageLayout = (function(_super) {
    __extends(HomepageLayout, _super);

    function HomepageLayout() {
      return HomepageLayout.__super__.constructor.apply(this, arguments);
    }

    HomepageLayout.prototype.template = "apps/shell/show/homepage";

    return HomepageLayout;

  })(Backbone.Marionette.LayoutView);
  return App.execute("shell:view:ready");
});

this.Kodi.module("SongApp.List", function(List, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    getSongsView: function(songs, verbose) {
      if (verbose == null) {
        verbose = false;
      }
      this.songsView = new List.Songs({
        collection: songs,
        verbose: verbose
      });
      App.listenTo(this.songsView, 'childview:song:play', (function(_this) {
        return function(list, item) {
          return _this.playSong(item.model.get('songid'));
        };
      })(this));
      App.listenTo(this.songsView, 'childview:song:add', (function(_this) {
        return function(list, item) {
          return _this.addSong(item.model.get('songid'));
        };
      })(this));
      App.listenTo(this.songsView, 'childview:song:localadd', (function(_this) {
        return function(list, item) {
          return _this.localAddSong(item.model.get('songid'));
        };
      })(this));
      App.listenTo(this.songsView, 'childview:song:localplay', (function(_this) {
        return function(list, item) {
          return _this.localPlaySong(item.model);
        };
      })(this));
      App.listenTo(this.songsView, 'childview:song:download', (function(_this) {
        return function(list, item) {
          return _this.downloadSong(item.model);
        };
      })(this));
      App.listenTo(this.songsView, 'childview:song:musicvideo', (function(_this) {
        return function(list, item) {
          return _this.musicVideo(item.model);
        };
      })(this));
      App.listenTo(this.songsView, "show", function() {
        return App.vent.trigger("state:content:updated");
      });
      return this.songsView;
    },
    playSong: function(songId) {
      return App.execute("command:audio:play", 'songid', songId);
    },
    addSong: function(songId) {
      return App.execute("command:audio:add", 'songid', songId);
    },
    localAddSong: function(songId) {
      return App.execute("localplaylist:addentity", 'songid', songId);
    },
    localPlaySong: function(model) {
      var localPlaylist;
      localPlaylist = App.request("command:local:controller", 'audio', 'PlayList');
      return localPlaylist.play(model.attributes);
    },
    downloadSong: function(model) {
      var files;
      files = App.request("command:kodi:controller", 'video', 'Files');
      return files.downloadFile(model.get('file'));
    },
    musicVideo: function(model) {
      var query;
      query = model.get('label') + ' ' + model.get('artist');
      return App.execute("youtube:search:view", query, function(view) {
        var $footer;
        $footer = $('<a>', {
          "class": 'btn btn-primary',
          href: 'https://www.youtube.com/results?search_query=' + query,
          target: '_blank'
        });
        $footer.html('More videos');
        return App.execute("ui:modal:show", query, view.render().$el, $footer);
      });
    }
  };
  return App.reqres.setHandler("song:list:view", function(songs, verbose) {
    if (verbose == null) {
      verbose = false;
    }
    return API.getSongsView(songs, verbose);
  });
});

this.Kodi.module("SongApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.Song = (function(_super) {
    __extends(Song, _super);

    function Song() {
      return Song.__super__.constructor.apply(this, arguments);
    }

    Song.prototype.template = 'apps/song/list/song';

    Song.prototype.tagName = "tr";

    Song.prototype.initialize = function() {
      var duration, menu;
      duration = helpers.global.secToTime(this.model.get('duration'));
      menu = {
        'song-localadd': 'Add to playlist',
        'song-download': 'Download song',
        'song-localplay': 'Play in browser',
        'song-musicvideo': 'Music video'
      };
      return this.model.set({
        displayDuration: helpers.global.formatTime(duration),
        menu: menu
      });
    };

    Song.prototype.triggers = {
      "click .play": "song:play",
      "dblclick .song-title": "song:play",
      "click .add": "song:add",
      "click .song-localadd": "song:localadd",
      "click .song-download": "song:download",
      "click .song-localplay": "song:localplay",
      "click .song-musicvideo": "song:musicvideo"
    };

    Song.prototype.events = {
      "click .dropdown > i": "populateMenu",
      "click .thumbs": "toggleThumbs"
    };

    Song.prototype.populateMenu = function() {
      var key, menu, val, _ref;
      menu = '';
      if (this.model.get('menu')) {
        _ref = this.model.get('menu');
        for (key in _ref) {
          val = _ref[key];
          menu += this.themeTag('li', {
            "class": key
          }, val);
        }
        return this.$el.find('.dropdown-menu').html(menu);
      }
    };

    Song.prototype.toggleThumbs = function() {
      App.request("thumbsup:toggle:entity", this.model);
      return this.$el.toggleClass('thumbs-up');
    };

    Song.prototype.attributes = function() {
      var classes;
      classes = ['song', 'table-row', 'can-play', 'item-' + this.model.get('uid')];
      if (App.request("thumbsup:check", this.model)) {
        classes.push('thumbs-up');
      }
      return {
        "class": classes.join(' ')
      };
    };

    Song.prototype.onShow = function() {
      $('.dropdown', this.$el).on('show.bs.dropdown', (function(_this) {
        return function() {
          return _this.$el.addClass('menu-open');
        };
      })(this));
      $('.dropdown', this.$el).on('hide.bs.dropdown', (function(_this) {
        return function() {
          return _this.$el.removeClass('menu-open');
        };
      })(this));
      return $('.dropdown', this.$el).on('click', function() {
        return $(this).removeClass('open').trigger('hide.bs.dropdown');
      });
    };

    return Song;

  })(App.Views.ItemView);
  return List.Songs = (function(_super) {
    __extends(Songs, _super);

    function Songs() {
      return Songs.__super__.constructor.apply(this, arguments);
    }

    Songs.prototype.childView = List.Song;

    Songs.prototype.tagName = "table";

    Songs.prototype.attributes = function() {
      var verbose;
      verbose = this.options.verbose ? 'verbose' : 'basic';
      return {
        "class": 'songs-table table table-hover ' + verbose
      };
    };

    return Songs;

  })(App.Views.CollectionView);
});

this.Kodi.module("StateApp", function(StateApp, App, Backbone, Marionette, $, _) {
  return StateApp.Base = (function(_super) {
    __extends(Base, _super);

    function Base() {
      return Base.__super__.constructor.apply(this, arguments);
    }

    Base.prototype.instanceSettings = {};

    Base.prototype.state = {
      player: 'kodi',
      media: 'audio',
      volume: 50,
      lastVolume: 50,
      muted: false,
      shuffled: false,
      repeat: 'off',
      version: {
        major: 15,
        minor: 0
      }
    };

    Base.prototype.playing = {
      playing: false,
      paused: false,
      playState: '',
      item: {},
      media: 'audio',
      itemChanged: false,
      latPlaying: '',
      canrepeat: true,
      canseek: true,
      canshuffle: true,
      partymode: false,
      percentage: 0,
      playlistid: 0,
      position: 0,
      speed: 0,
      time: {
        hours: 0,
        milliseconds: 0,
        minutes: 0,
        seconds: 0
      },
      totaltime: {
        hours: 0,
        milliseconds: 0,
        minutes: 0,
        seconds: 0
      }
    };

    Base.prototype.defaultPlayingItem = {
      thumbnail: '',
      fanart: '',
      id: 0,
      songid: 0,
      episodeid: 0,
      album: '',
      albumid: '',
      duration: 0,
      type: 'song'
    };

    Base.prototype.getState = function(key) {
      if (key == null) {
        key = 'all';
      }
      if (key === 'all') {
        return this.state;
      } else {
        return this.state[key];
      }
    };

    Base.prototype.setState = function(key, value) {
      return this.state[key] = value;
    };

    Base.prototype.getPlaying = function(key) {
      var ret;
      if (key == null) {
        key = 'all';
      }
      ret = this.playing;
      if (ret.item.length === 0) {
        ret.item = this.defaultPlayingItem;
      }
      if (key === 'all') {
        return this.playing;
      } else {
        return this.playing[key];
      }
    };

    Base.prototype.setPlaying = function(key, value) {
      return this.playing[key] = value;
    };

    Base.prototype.isPlaying = function() {
      return this.getPlaying('playing');
    };

    Base.prototype.isPlayingItemChanged = function() {
      return this.getPlaying('itemChanged');
    };

    Base.prototype.doCallback = function(callback, resp) {
      if (typeof callback === 'function') {
        return callback(resp);
      }
    };

    Base.prototype.getCurrentState = function(callback) {};

    Base.prototype.getCachedState = function() {
      return {
        state: this.state,
        playing: this.playing
      };
    };

    Base.prototype.setPlayer = function(player) {
      var $body;
      if (player == null) {
        player = 'kodi';
      }
      $body = App.getRegion('root').$el;
      $body.removeClassStartsWith('active-player-').addClass('active-player-' + player);
      return config.set('state', 'lastplayer', player);
    };

    Base.prototype.getPlayer = function() {
      var $body, player;
      player = 'kodi';
      $body = App.getRegion('root').$el;
      if ($body.hasClass('active-player-local')) {
        player = 'local';
      }
      return player;
    };

    return Base;

  })(Marionette.Object);
});

this.Kodi.module("StateApp.Kodi", function(StateApp, App, Backbone, Marionette, $, _) {
  return StateApp.State = (function(_super) {
    __extends(State, _super);

    function State() {
      return State.__super__.constructor.apply(this, arguments);
    }

    State.prototype.playerController = {};

    State.prototype.applicationController = {};

    State.prototype.playlistApi = {};

    State.prototype.initialize = function() {
      this.state = _.extend({}, this.state);
      this.playing = _.extend({}, this.playing);
      this.setState('player', 'kodi');
      this.playerController = App.request("command:kodi:controller", 'auto', 'Player');
      this.applicationController = App.request("command:kodi:controller", 'auto', 'Application');
      this.playlistApi = App.request("playlist:kodi:entity:api");
      App.reqres.setHandler("state:kodi:update", (function(_this) {
        return function(callback) {
          return _this.getCurrentState(callback);
        };
      })(this));
      return App.reqres.setHandler("state:kodi:get", (function(_this) {
        return function() {
          return _this.getCachedState();
        };
      })(this));
    };

    State.prototype.getCurrentState = function(callback) {
      return this.applicationController.getProperties((function(_this) {
        return function(properties) {
          _this.setState('volume', properties.volume);
          _this.setState('muted', properties.muted);
          _this.setState('version', properties.version);
          App.reqres.setHandler('player:kodi:timer', 'stop');
          return _this.playerController.getPlaying(function(playing) {
            var autoMap, key, media, _i, _len;
            if (playing) {
              _this.setPlaying('playing', true);
              _this.setPlaying('paused', playing.properties.speed === 0);
              _this.setPlaying('playState', (playing.properties.speed === 0 ? 'paused' : 'playing'));
              autoMap = ['canrepeat', 'canseek', 'canshuffle', 'partymode', 'percentage', 'playlistid', 'position', 'speed', 'time', 'totaltime'];
              for (_i = 0, _len = autoMap.length; _i < _len; _i++) {
                key = autoMap[_i];
                if (playing.properties[key] != null) {
                  _this.setPlaying(key, playing.properties[key]);
                }
              }
              _this.setState('shuffled', playing.properties.shuffled);
              _this.setState('repeat', playing.properties.repeat);
              media = _this.playerController.playerIdToName(playing.properties.playlistid);
              if (media) {
                _this.setState('media', media);
              }
              if (playing.item.file !== _this.getPlaying('lastPlaying')) {
                _this.setPlaying('itemChanged', true);
                App.vent.trigger("state:kodi:itemchanged", _this.getCachedState());
              } else {
                _this.setPlaying('itemChanged', false);
              }
              _this.setPlaying('lastPlaying', playing.item.file);
              _this.setPlaying('item', _this.parseItem(playing.item, {
                media: media,
                playlistid: playing.properties.playlistid
              }));
              App.reqres.setHandler('player:kodi:timer', 'start');
            } else {
              _this.setPlaying('playing', false);
              _this.setPlaying('paused', false);
              _this.setPlaying('item', _this.defaultPlayingItem);
              _this.setPlaying('lstPlaying', '');
            }
            App.vent.trigger("state:kodi:changed", _this.getCachedState());
            App.vent.trigger("state:changed");
            return _this.doCallback(callback, _this.getCachedState());
          });
        };
      })(this));
    };

    State.prototype.parseItem = function(model, options) {
      model = this.playlistApi.parseItem(model, options);
      model = App.request("images:path:entity", model);
      model.url = helpers.url.get(model.type, model.id);
      model.url = helpers.url.playlistUrl(model);
      return model;
    };

    return State;

  })(App.StateApp.Base);
});

this.Kodi.module("StateApp.Kodi", function(StateApp, App, Backbone, Marionette, $, _) {
  return StateApp.Notifications = (function(_super) {
    __extends(Notifications, _super);

    function Notifications() {
      return Notifications.__super__.constructor.apply(this, arguments);
    }

    Notifications.prototype.wsActive = false;

    Notifications.prototype.wsObj = {};

    Notifications.prototype.getConnection = function() {
      var host, protocol, socketHost, socketPath, socketPort;
      host = config.getLocal('socketsHost');
      socketPath = config.getLocal('jsonRpcEndpoint');
      socketPort = config.getLocal('socketsPort');
      socketHost = host === 'auto' ? location.hostname : host;
      protocol = helpers.url.isSecureProtocol() ? "wss" : "ws";
      return "" + protocol + "://" + socketHost + ":" + socketPort + "/" + socketPath + "?kodi";
    };

    Notifications.prototype.initialize = function() {
      var msg, ws;
      if (window.WebSocket) {
        ws = new WebSocket(this.getConnection());
        ws.onopen = (function(_this) {
          return function(e) {
            helpers.debug.msg("Websockets Active");
            _this.wsActive = true;
            return App.vent.trigger("sockets:available");
          };
        })(this);
        ws.onerror = (function(_this) {
          return function(resp) {
            helpers.debug.msg(_this.socketConnectionErrorMsg(), "warning", resp);
            _this.wsActive = false;
            return App.vent.trigger("sockets:unavailable");
          };
        })(this);
        ws.onmessage = (function(_this) {
          return function(resp) {
            return _this.messageRecieved(resp);
          };
        })(this);
        ws.onclose = (function(_this) {
          return function(resp) {
            helpers.debug.msg("Websockets Closed", "warning", resp);
            return _this.wsActive = false;
          };
        })(this);
      } else {
        msg = "Your browser doesn't support websockets! Get with the times and update your browser.";
        helpers.debug.msg(t.gettext(msg), "warning", resp);
        App.vent.trigger("sockets:unavailable");
      }
      return App.reqres.setHandler("sockets:active", (function(_this) {
        return function() {
          return _this.wsActive;
        };
      })(this));
    };

    Notifications.prototype.parseResponse = function(resp) {
      return jQuery.parseJSON(resp.data);
    };

    Notifications.prototype.messageRecieved = function(resp) {
      var data;
      data = this.parseResponse(resp);
      return this.onMessage(data);
    };

    Notifications.prototype.socketConnectionErrorMsg = function() {
      var msg;
      msg = "Failed to connect to websockets";
      return t.gettext(msg);
    };

    Notifications.prototype.refreshStateNow = function(callback) {
      App.vent.trigger("state:kodi:changed", this.getCachedState());
      return setTimeout(((function(_this) {
        return function() {
          return App.request("state:kodi:update", function(state) {
            if (callback) {
              return callback(state);
            }
          });
        };
      })(this)), 1000);
    };

    Notifications.prototype.onMessage = function(data) {
      var playerController, wait;
      switch (data.method) {
        case 'Player.OnPlay':
          this.setPlaying('paused', false);
          this.setPlaying('playState', 'playing');
          App.execute("player:kodi:timer", 'start');
          this.refreshStateNow();
          break;
        case 'Player.OnStop':
          this.setPlaying('playing', false);
          App.execute("player:kodi:timer", 'stop');
          this.refreshStateNow();
          break;
        case 'Player.OnPropertyChanged':
          this.refreshStateNow();
          break;
        case 'Player.OnPause':
          this.setPlaying('paused', true);
          this.setPlaying('playState', 'paused');
          App.execute("player:kodi:timer", 'stop');
          this.refreshStateNow();
          break;
        case 'Player.OnSeek':
          App.execute("player:kodi:timer", 'stop');
          this.refreshStateNow(function() {
            return App.execute("player:kodi:timer", 'start');
          });
          break;
        case 'Playlist.OnClear':
        case 'Playlist.OnAdd':
        case 'Playlist.OnRemove':
          playerController = App.request("command:kodi:controller", 'auto', 'Player');
          App.execute("playlist:refresh", 'kodi', playerController.playerIdToName(data.params.data.playlistid));
          this.refreshStateNow();
          break;
        case 'Application.OnVolumeChanged':
          this.setState('volume', data.params.data.volume);
          this.setState('muted', data.params.data.muted);
          this.refreshStateNow();
          break;
        case 'VideoLibrary.OnScanStarted':
          App.execute("notification:show", t.gettext("Video library scan started"));
          break;
        case 'VideoLibrary.OnScanFinished':
          App.execute("notification:show", t.gettext("Video library scan complete"));
          Backbone.fetchCache.clearItem('MovieCollection');
          Backbone.fetchCache.clearItem('TVShowCollection');
          break;
        case 'AudioLibrary.OnScanStarted':
          App.execute("notification:show", t.gettext("Audio library scan started"));
          break;
        case 'AudioLibrary.OnScanFinished':
          App.execute("notification:show", t.gettext("Audio library scan complete"));
          Backbone.fetchCache.clearItem('AlbumCollection');
          Backbone.fetchCache.clearItem('ArtistCollection');
          break;
        case 'Input.OnInputRequested':
          App.execute("input:textbox", '');
          wait = 60;
          App.inputTimeout = setTimeout((function() {
            var msg;
            msg = wait + t.gettext(' seconds ago, an input dialog opened on xbmc and it is still open! To prevent ' + 'a mainframe implosion, you should probably give me some text. I don\'t really care what it is at this point, ' + 'why not be creative? Do you have a ') + '<a href="http://goo.gl/PGE7wg" target="_blank">' + t.gettext('word of the day') + '</a>? ' + t.gettext('I won\'t tell...');
            App.execute("input:textbox", msg);
          }), 1000 * wait);
          break;
        case 'Input.OnInputFinished':
          clearTimeout(App.inputTimeout);
          App.execute("input:textbox:close");
          break;
        case 'System.OnQuit':
          App.execute("notification:show", t.gettext("Kodi has quit"));
          break;
      }
    };

    return Notifications;

  })(App.StateApp.Base);
});

this.Kodi.module("StateApp.Kodi", function(StateApp, App, Backbone, Marionette, $, _) {
  return StateApp.Polling = (function(_super) {
    __extends(Polling, _super);

    function Polling() {
      return Polling.__super__.constructor.apply(this, arguments);
    }

    Polling.prototype.commander = {};

    Polling.prototype.checkInterval = 10000;

    Polling.prototype.currentInterval = '';

    Polling.prototype.timeoutObj = {};

    Polling.prototype.failures = 0;

    Polling.prototype.maxFailures = 100;

    Polling.prototype.initialize = function() {
      var interval;
      interval = config.getLocal('pollInterval');
      this.checkInterval = parseInt(interval);
      return this.currentInterval = this.checkInterval;
    };

    Polling.prototype.startPolling = function() {
      return this.update();
    };

    Polling.prototype.updateState = function() {
      var stateObj;
      stateObj = App.request("state:kodi");
      return stateObj.getCurrentState();
    };

    Polling.prototype.update = function() {
      if (App.kodiPolling.failures < App.kodiPolling.maxFailures) {
        App.kodiPolling.updateState();
        return App.kodiPolling.timeout = setTimeout(App.kodiPolling.ping, App.kodiPolling.currentInterval);
      } else {
        return App.execute("notification:show", t.gettext("Unable to communicate with Kodi in a long time. I think it's dead Jim!"));
      }
    };

    Polling.prototype.ping = function() {
      var commander;
      commander = App.request("command:kodi:controller", 'auto', 'Commander');
      commander.setOptions({
        timeout: 5000,
        error: function() {
          return App.kodiPolling.failure();
        }
      });
      commander.onError = function() {};
      return commander.sendCommand('Ping', [], function() {
        return App.kodiPolling.alive();
      });
    };

    Polling.prototype.alive = function() {
      App.kodiPolling.failures = 0;
      App.kodiPolling.currentInterval = App.kodiPolling.checkInterval;
      return App.kodiPolling.update();
    };

    Polling.prototype.failure = function() {
      App.kodiPolling.failures++;
      if (App.kodiPolling.failures > 10) {
        App.kodiPolling.currentInterval = App.kodiPolling.checkInterval * 5;
      }
      if (App.kodiPolling.failures > 20) {
        App.kodiPolling.currentInterval = App.kodiPolling.checkInterval * 10;
      }
      if (App.kodiPolling.failures > 30) {
        App.kodiPolling.currentInterval = App.kodiPolling.checkInterval * 30;
      }
      return App.kodiPolling.update();
    };

    return Polling;

  })(App.StateApp.Base);
});

this.Kodi.module("StateApp.Local", function(StateApp, App, Backbone, Marionette, $, _) {
  return StateApp.State = (function(_super) {
    __extends(State, _super);

    function State() {
      return State.__super__.constructor.apply(this, arguments);
    }

    State.prototype.initialize = function() {
      this.state = _.extend({}, this.state);
      this.playing = _.extend({}, this.playing);
      this.setState('player', 'local');
      this.setState('currentPlaybackId', 'browser-none');
      this.setState('localPlay', false);
      App.reqres.setHandler("state:local:update", (function(_this) {
        return function(callback) {
          return _this.getCurrentState(callback);
        };
      })(this));
      return App.reqres.setHandler("state:local:get", (function(_this) {
        return function() {
          return _this.getCachedState();
        };
      })(this));
    };

    State.prototype.getCurrentState = function(callback) {
      return this.doCallback(callback, this.getCachedState());
    };

    return State;

  })(App.StateApp.Base);
});

this.Kodi.module("StateApp", function(StateApp, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    setState: function(player) {
      if (player == null) {
        player = 'kodi';
      }
      this.setBodyClasses(player);
      this.setPlayingContent(player);
      this.setPlayerPlaying(player);
      this.setAppProperties(player);
      return this.setTitle(player);
    },
    playerClass: function(className, player) {
      return player + '-' + className;
    },
    setBodyClasses: function(player) {
      var $body, c, newClasses, stateObj, _i, _len, _results;
      stateObj = App.request("state:" + player);
      $body = App.getRegion('root').$el;
      $body.removeClassStartsWith(player + '-');
      newClasses = [];
      newClasses.push('shuffled-' + (stateObj.getState('shuffled') ? 'on' : 'off'));
      newClasses.push('partymode-' + (stateObj.getPlaying('partymode') ? 'on' : 'off'));
      newClasses.push('mute-' + (stateObj.getState('muted') ? 'on' : 'off'));
      newClasses.push('repeat-' + stateObj.getState('repeat'));
      newClasses.push('media-' + stateObj.getState('media'));
      if (stateObj.isPlaying()) {
        newClasses.push(stateObj.getPlaying('playState'));
      } else {
        newClasses.push('not-playing');
      }
      _results = [];
      for (_i = 0, _len = newClasses.length; _i < _len; _i++) {
        c = newClasses[_i];
        _results.push($body.addClass(this.playerClass(c, player)));
      }
      return _results;
    },
    setPlayingContent: function(player) {
      var $playlistCtx, className, item, playState, stateObj;
      stateObj = App.request("state:" + player);
      $playlistCtx = $('.media-' + stateObj.getState('media') + ' .' + player + '-playlist');
      $('.can-play').removeClassStartsWith(player + '-row-');
      $('.item', $playlistCtx).removeClassStartsWith('row-');
      if (stateObj.isPlaying()) {
        item = stateObj.getPlaying('item');
        playState = stateObj.getPlaying('playState');
        className = '.item-' + item.uid;
        $(className).addClass(this.playerClass('row-' + playState, player));
        $('.pos-' + stateObj.getPlaying('position'), $playlistCtx).addClass('row-' + playState);
        return App.vent.trigger("state:" + player + ":playing:updated", stateObj);
      }
    },
    setPlayerPlaying: function(player) {
      var $dur, $img, $playerCtx, $subtitle, $title, item, stateObj;
      stateObj = App.request("state:" + player);
      $playerCtx = $('#player-' + player);
      $title = $('.playing-title', $playerCtx);
      $subtitle = $('.playing-subtitle', $playerCtx);
      $dur = $('.playing-time-duration', $playerCtx);
      $img = $('.playing-thumb', $playerCtx);
      if (stateObj.isPlaying()) {
        item = stateObj.getPlaying('item');
        $title.html(helpers.entities.playingLink(item));
        $subtitle.html(helpers.entities.getSubtitle(item));
        $dur.html(helpers.global.formatTime(stateObj.getPlaying('totaltime')));
        return $img.css("background-image", "url('" + item.thumbnail + "')");
      } else {
        $title.html(t.gettext('Nothing playing'));
        $subtitle.html('');
        $dur.html('0');
        return $img.attr('src', App.request("images:path:get"));
      }
    },
    setAppProperties: function(player) {
      var $playerCtx, stateObj;
      stateObj = App.request("state:" + player);
      $playerCtx = $('#player-' + player);
      return $('.volume', $playerCtx).val(stateObj.getState('volume'));
    },
    setTitle: function(player) {
      var stateObj;
      if (player === 'kodi') {
        stateObj = App.request("state:" + player);
        if (stateObj.isPlaying() && stateObj.getPlaying('playState') === 'playing') {
          return helpers.global.appTitle(stateObj.getPlaying('item'));
        } else {
          return helpers.global.appTitle();
        }
      }
    },
    initKodiState: function() {
      App.kodiState = new StateApp.Kodi.State();
      App.localState = new StateApp.Local.State();
      App.kodiState.setPlayer(config.get('state', 'lastplayer', 'kodi'));
      App.kodiState.getCurrentState(function(state) {
        API.setState('kodi');
        App.kodiSockets = new StateApp.Kodi.Notifications();
        App.kodiPolling = new StateApp.Kodi.Polling();
        App.vent.on("sockets:unavailable", function() {
          return App.kodiPolling.startPolling();
        });
        App.vent.on("playlist:rendered", function() {
          return App.request("playlist:refresh", App.kodiState.getState('player'), App.kodiState.getState('media'));
        });
        App.vent.on("state:content:updated", function() {
          API.setPlayingContent('kodi');
          return API.setPlayingContent('local');
        });
        App.vent.on("state:kodi:changed", function(state) {
          return API.setState('kodi');
        });
        App.vent.on("state:local:changed", function(state) {
          return API.setState('local');
        });
        App.vent.on("state:player:updated", function(player) {
          return API.setPlayerPlaying(player);
        });
        return App.vent.trigger("state:initialized");
      });
      App.reqres.setHandler("state:kodi", function() {
        return App.kodiState;
      });
      App.reqres.setHandler("state:local", function() {
        return App.localState;
      });
      App.reqres.setHandler("state:current", function() {
        var stateObj;
        stateObj = App.kodiState.getPlayer() === 'kodi' ? App.kodiState : App.localState;
        return stateObj;
      });
      return App.vent.trigger("state:changed");
    }
  };
  return App.addInitializer(function() {
    return API.initKodiState();
  });
});

this.Kodi.module("ThumbsApp.List", function(List, App, Backbone, Marionette, $, _) {
  return List.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function() {
      var entities;
      this.layout = this.getLayout();
      entities = ['song', 'artist', 'album', 'tvshow', 'movie'];
      this.listenTo(this.layout, "show", (function(_this) {
        return function() {
          var entity, _i, _len, _results;
          _results = [];
          for (_i = 0, _len = entities.length; _i < _len; _i++) {
            entity = entities[_i];
            _results.push(_this.getResult(entity));
          }
          return _results;
        };
      })(this));
      return App.regionContent.show(this.layout);
    };

    Controller.prototype.getLayout = function() {
      return new List.ListLayout();
    };

    Controller.prototype.getResult = function(entity) {
      var limit, loaded, query, setView, view;
      query = this.getOption('query');
      limit = this.getOption('media') === 'all' ? 'limit' : 'all';
      loaded = App.request("thumbsup:get:entities", entity);
      if (loaded.length > 0) {
        view = App.request("" + entity + ":list:view", loaded, true);
        setView = new List.ListSet({
          entity: entity
        });
        App.listenTo(setView, "show", (function(_this) {
          return function() {
            return setView.regionResult.show(view);
          };
        })(this));
        return this.layout["" + entity + "Set"].show(setView);
      }
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("ThumbsApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.ListLayout = (function(_super) {
    __extends(ListLayout, _super);

    function ListLayout() {
      return ListLayout.__super__.constructor.apply(this, arguments);
    }

    ListLayout.prototype.template = 'apps/thumbs/list/thumbs_layout';

    ListLayout.prototype.className = "thumbs-page set-page";

    ListLayout.prototype.regions = {
      artistSet: '.entity-set-artist',
      albumSet: '.entity-set-album',
      songSet: '.entity-set-song',
      movieSet: '.entity-set-movie',
      tvshowSet: '.entity-set-tvshow'
    };

    return ListLayout;

  })(App.Views.LayoutView);
  return List.ListSet = (function(_super) {
    __extends(ListSet, _super);

    function ListSet() {
      return ListSet.__super__.constructor.apply(this, arguments);
    }

    ListSet.prototype.template = 'apps/thumbs/list/thumbs_set';

    ListSet.prototype.className = "thumbs-set";

    ListSet.prototype.onRender = function() {
      if (this.options) {
        if (this.options.entity) {
          return $('h2.set-header', this.$el).html(t.gettext(this.options.entity + 's'));
        }
      }
    };

    ListSet.prototype.regions = {
      regionResult: '.set-results'
    };

    return ListSet;

  })(App.Views.LayoutView);
});

this.Kodi.module("ThumbsApp", function(ThumbsApp, App, Backbone, Marionette, $, _) {
  var API;
  ThumbsApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "thumbsup": "list"
    };

    return Router;

  })(App.Router.Base);
  API = {
    list: function() {
      return new ThumbsApp.List.Controller();
    }
  };
  return App.on("before:start", function() {
    return new ThumbsApp.Router({
      controller: API
    });
  });
});

this.Kodi.module("TVShowApp.Episode", function(Episode, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    getEpisodeList: function(collection) {
      var view;
      view = new Episode.Episodes({
        collection: collection
      });
      App.listenTo(view, 'childview:episode:play', function(parent, viewItem) {
        return App.execute('episode:action', 'play', viewItem);
      });
      App.listenTo(view, 'childview:episode:add', function(parent, viewItem) {
        return App.execute('episode:action', 'add', viewItem);
      });
      App.listenTo(view, 'childview:episode:localplay', function(parent, viewItem) {
        return App.execute('episode:action', 'localplay', viewItem);
      });
      App.listenTo(view, 'childview:episode:download', function(parent, viewItem) {
        return App.execute('episode:action', 'download', viewItem);
      });
      App.listenTo(view, 'childview:episode:watched', function(parent, viewItem) {
        parent.$el.toggleClass('is-watched');
        return App.execute('episode:action', 'toggleWatched', viewItem);
      });
      return view;
    },
    bindTriggers: function(view) {
      App.listenTo(view, 'episode:play', function(viewItem) {
        return App.execute('episode:action', 'play', viewItem);
      });
      App.listenTo(view, 'episode:add', function(viewItem) {
        return App.execute('episode:action', 'add', viewItem);
      });
      App.listenTo(view, 'episode:localplay', function(viewItem) {
        return App.execute('episode:action', 'localplay', viewItem);
      });
      App.listenTo(view, 'episode:download', function(viewItem) {
        return App.execute('episode:action', 'download', viewItem);
      });
      return App.listenTo(view, 'episode:watched', function(viewItem) {
        parent.$el.toggleClass('is-watched');
        return App.execute('episode:action', 'toggleWatched', viewItem);
      });
    }
  };
  Episode.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      var episode, episodeId, id, seasonId;
      id = parseInt(options.id);
      seasonId = parseInt(options.season);
      episodeId = parseInt(options.episodeid);
      episode = App.request("episode:entity", episodeId);
      return App.execute("when:entity:fetched", episode, (function(_this) {
        return function() {
          console.log(episode);
          _this.layout = _this.getLayoutView(episode);
          _this.listenTo(_this.layout, "show", function() {
            _this.getDetailsLayoutView(episode);
            return _this.getContentView(episode);
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getLayoutView = function(tvshow) {
      return new Episode.PageLayout({
        tvshow: tvshow
      });
    };

    Controller.prototype.getDetailsLayoutView = function(episode) {
      var headerLayout;
      headerLayout = new Episode.HeaderLayout({
        model: episode
      });
      this.listenTo(headerLayout, "show", (function(_this) {
        return function() {
          var detail, teaser;
          teaser = new Episode.EpisodeDetailTeaser({
            model: episode
          });
          detail = new Episode.Details({
            model: episode
          });
          API.bindTriggers(detail);
          headerLayout.regionSide.show(teaser);
          return headerLayout.regionMeta.show(detail);
        };
      })(this));
      return this.layout.regionHeader.show(headerLayout);
    };

    Controller.prototype.getContentView = function(episode) {
      var contentLayout;
      contentLayout = new Episode.Content({
        model: episode
      });
      App.listenTo(contentLayout, 'show', (function(_this) {
        return function() {
          if (episode.get('cast').length > 0) {
            return contentLayout.regionCast.show(_this.getCast(episode));
          }
        };
      })(this));
      return this.layout.regionContent.show(contentLayout);
    };

    Controller.prototype.getCast = function(episode) {
      return App.request('cast:list:view', episode.get('cast'), 'tvshows');
    };

    return Controller;

  })(App.Controllers.Base);
  return App.reqres.setHandler("episode:list:view", function(collection) {
    return API.getEpisodeList(collection);
  });
});

this.Kodi.module("TVShowApp.Episode", function(Episode, App, Backbone, Marionette, $, _) {
  Episode.EpisodeTeaser = (function(_super) {
    __extends(EpisodeTeaser, _super);

    function EpisodeTeaser() {
      return EpisodeTeaser.__super__.constructor.apply(this, arguments);
    }

    EpisodeTeaser.prototype.triggers = {
      "click .play": "episode:play",
      "click .watched": "episode:watched",
      "click .add": "episode:add",
      "click .localplay": "episode:localplay",
      "click .download": "episode:download"
    };

    EpisodeTeaser.prototype.initialize = function() {
      EpisodeTeaser.__super__.initialize.apply(this, arguments);
      if (this.model != null) {
        this.model.set(this.getMeta());
        this.model.set({
          actions: {
            watched: 'Watched'
          }
        });
        return this.model.set({
          menu: {
            add: 'Add to Kodi playlist',
            divider: '',
            download: 'Download',
            localplay: 'Play in browser'
          }
        });
      }
    };

    EpisodeTeaser.prototype.attributes = function() {
      var classes;
      classes = ['card'];
      if (helpers.entities.isWatched(this.model)) {
        classes.push('is-watched');
      }
      return {
        "class": classes.join(' ')
      };
    };

    EpisodeTeaser.prototype.getMeta = function() {
      var epNum, epNumFull, showLink;
      epNum = this.themeTag('span', {
        "class": 'ep-num'
      }, this.model.get('season') + 'x' + this.model.get('episode') + ' ');
      epNumFull = this.themeTag('span', {
        "class": 'ep-num-full'
      }, t.gettext('Episode') + ' ' + this.model.get('episode'));
      showLink = this.themeLink(this.model.get('showtitle') + ' ', 'tvshow/' + this.model.get('tvshowid'), {
        className: 'show-name'
      });
      return {
        label: epNum + this.model.get('title'),
        subtitle: showLink + epNumFull
      };
    };

    return EpisodeTeaser;

  })(App.Views.CardView);
  Episode.Empty = (function(_super) {
    __extends(Empty, _super);

    function Empty() {
      return Empty.__super__.constructor.apply(this, arguments);
    }

    Empty.prototype.tagName = "li";

    Empty.prototype.className = "episode-empty-result";

    return Empty;

  })(App.Views.EmptyViewResults);
  Episode.Episodes = (function(_super) {
    __extends(Episodes, _super);

    function Episodes() {
      return Episodes.__super__.constructor.apply(this, arguments);
    }

    Episodes.prototype.childView = Episode.EpisodeTeaser;

    Episodes.prototype.emptyView = Episode.Empty;

    Episodes.prototype.tagName = "ul";

    Episodes.prototype.className = "card-grid--episode";

    return Episodes;

  })(App.Views.CollectionView);
  Episode.PageLayout = (function(_super) {
    __extends(PageLayout, _super);

    function PageLayout() {
      return PageLayout.__super__.constructor.apply(this, arguments);
    }

    PageLayout.prototype.className = 'episode-show detail-container';

    return PageLayout;

  })(App.Views.LayoutWithHeaderView);
  Episode.HeaderLayout = (function(_super) {
    __extends(HeaderLayout, _super);

    function HeaderLayout() {
      return HeaderLayout.__super__.constructor.apply(this, arguments);
    }

    HeaderLayout.prototype.className = 'episode-details';

    return HeaderLayout;

  })(App.Views.LayoutDetailsHeaderView);
  Episode.Details = (function(_super) {
    __extends(Details, _super);

    function Details() {
      return Details.__super__.constructor.apply(this, arguments);
    }

    Details.prototype.template = 'apps/tvshow/episode/details_meta';

    Details.prototype.triggers = {
      'click .play': 'episode:play',
      'click .add': 'episode:add',
      'click .stream': 'episode:localplay',
      'click .download': 'episode:download'
    };

    return Details;

  })(App.Views.ItemView);
  Episode.EpisodeDetailTeaser = (function(_super) {
    __extends(EpisodeDetailTeaser, _super);

    function EpisodeDetailTeaser() {
      return EpisodeDetailTeaser.__super__.constructor.apply(this, arguments);
    }

    EpisodeDetailTeaser.prototype.tagName = "div";

    EpisodeDetailTeaser.prototype.className = "card-detail";

    EpisodeDetailTeaser.prototype.triggers = {
      "click .menu": "episode-menu:clicked"
    };

    return EpisodeDetailTeaser;

  })(App.Views.CardView);
  return Episode.Content = (function(_super) {
    __extends(Content, _super);

    function Content() {
      return Content.__super__.constructor.apply(this, arguments);
    }

    Content.prototype.template = 'apps/tvshow/episode/content';

    Content.prototype.className = "episode-content content-sections";

    Content.prototype.regions = {
      regionCast: '.region-cast'
    };

    return Content;

  })(App.Views.LayoutView);
});

this.Kodi.module("TVShowApp.Landing", function(Landing, App, Backbone, Marionette, $, _) {
  return Landing.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.subNavId = 'tvshows/recent';

    Controller.prototype.initialize = function() {
      this.layout = this.getLayoutView();
      this.listenTo(this.layout, "show", (function(_this) {
        return function() {
          _this.getPageView();
          return _this.getSubNav();
        };
      })(this));
      return App.regionContent.show(this.layout);
    };

    Controller.prototype.getLayoutView = function() {
      return new Landing.Layout();
    };

    Controller.prototype.getSubNav = function() {
      var subNav;
      subNav = App.request("navMain:children:show", this.subNavId, 'Sections');
      return this.layout.regionSidebarFirst.show(subNav);
    };

    Controller.prototype.getPageView = function() {
      this.page = new Landing.Page();
      this.listenTo(this.page, "show", (function(_this) {
        return function() {
          return _this.renderRecentlyAdded();
        };
      })(this));
      return this.layout.regionContent.show(this.page);
    };

    Controller.prototype.renderRecentlyAdded = function() {
      var collection;
      collection = App.request("episode:recentlyadded:entities");
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          var view;
          view = App.request("episode:list:view", collection);
          return _this.page.regionRecentlyAdded.show(view);
        };
      })(this));
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("TVShowApp.Landing", function(Landing, App, Backbone, Marionette, $, _) {
  Landing.Layout = (function(_super) {
    __extends(Layout, _super);

    function Layout() {
      return Layout.__super__.constructor.apply(this, arguments);
    }

    Layout.prototype.className = "movie-landing landing-page";

    return Layout;

  })(App.Views.LayoutWithSidebarFirstView);
  return Landing.Page = (function(_super) {
    __extends(Page, _super);

    function Page() {
      return Page.__super__.constructor.apply(this, arguments);
    }

    Page.prototype.template = 'apps/movie/landing/landing';

    Page.prototype.className = "movie-recent";

    Page.prototype.regions = {
      regionRecentlyAdded: '.region-recently-added'
    };

    return Page;

  })(App.Views.LayoutView);
});

this.Kodi.module("TVShowApp.List", function(List, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    getTVShowsList: function(tvshows, set) {
      var view, viewName;
      if (set == null) {
        set = false;
      }
      viewName = set ? 'TVShowsSet' : 'TVShows';
      view = new List[viewName]({
        collection: tvshows
      });
      App.listenTo(view, 'childview:tvshow:play', function(list, item) {
        var playlist;
        playlist = App.request("command:kodi:controller", 'video', 'PlayList');
        return playlist.play('tvshowid', item.model.get('tvshowid'));
      });
      return view;
    }
  };
  List.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function() {
      var collection;
      collection = App.request("tvshow:entities");
      collection.availableFilters = this.getAvailableFilters();
      collection.sectionId = 'tvshows/recent';
      App.request('filter:init', this.getAvailableFilters());
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          _this.layout = _this.getLayoutView(collection);
          _this.listenTo(_this.layout, "show", function() {
            _this.getFiltersView(collection);
            return _this.renderList(collection);
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getLayoutView = function(tvshows) {
      return new List.ListLayout({
        collection: tvshows
      });
    };

    Controller.prototype.getAvailableFilters = function() {
      return {
        sort: ['title', 'year', 'dateadded', 'rating'],
        filter: ['year', 'genre', 'unwatched', 'cast']
      };
    };

    Controller.prototype.getFiltersView = function(collection) {
      var filters;
      filters = App.request('filter:show', collection);
      this.layout.regionSidebarFirst.show(filters);
      return this.listenTo(filters, "filter:changed", (function(_this) {
        return function() {
          return _this.renderList(collection);
        };
      })(this));
    };

    Controller.prototype.renderList = function(collection) {
      var filteredCollection, view;
      App.execute("loading:show:view", this.layout.regionContent);
      filteredCollection = App.request('filter:apply:entities', collection);
      view = API.getTVShowsList(filteredCollection);
      return this.layout.regionContent.show(view);
    };

    return Controller;

  })(App.Controllers.Base);
  return App.reqres.setHandler("tvshow:list:view", function(collection) {
    return API.getTVShowsList(collection, true);
  });
});

this.Kodi.module("TVShowApp.List", function(List, App, Backbone, Marionette, $, _) {
  List.ListLayout = (function(_super) {
    __extends(ListLayout, _super);

    function ListLayout() {
      return ListLayout.__super__.constructor.apply(this, arguments);
    }

    ListLayout.prototype.className = "tvshow-list with-filters";

    return ListLayout;

  })(App.Views.LayoutWithSidebarFirstView);
  List.TVShowTeaser = (function(_super) {
    __extends(TVShowTeaser, _super);

    function TVShowTeaser() {
      return TVShowTeaser.__super__.constructor.apply(this, arguments);
    }

    TVShowTeaser.prototype.triggers = {
      "click .play": "tvshow:play",
      "click .menu": "tvshow-menu:clicked"
    };

    TVShowTeaser.prototype.initialize = function() {
      var subtitle;
      TVShowTeaser.__super__.initialize.apply(this, arguments);
      subtitle = '';
      subtitle += ' ' + this.model.get('rating');
      return this.model.set({
        subtitle: subtitle
      });
    };

    return TVShowTeaser;

  })(App.Views.CardView);
  List.Empty = (function(_super) {
    __extends(Empty, _super);

    function Empty() {
      return Empty.__super__.constructor.apply(this, arguments);
    }

    Empty.prototype.tagName = "li";

    Empty.prototype.className = "tvshow-empty-result";

    return Empty;

  })(App.Views.EmptyViewResults);
  List.TVShows = (function(_super) {
    __extends(TVShows, _super);

    function TVShows() {
      return TVShows.__super__.constructor.apply(this, arguments);
    }

    TVShows.prototype.childView = List.TVShowTeaser;

    TVShows.prototype.emptyView = List.Empty;

    TVShows.prototype.tagName = "ul";

    TVShows.prototype.className = "card-grid--tall";

    return TVShows;

  })(App.Views.VirtualListView);
  return List.TVShowsSet = (function(_super) {
    __extends(TVShowsSet, _super);

    function TVShowsSet() {
      return TVShowsSet.__super__.constructor.apply(this, arguments);
    }

    TVShowsSet.prototype.childView = List.TVShowTeaser;

    TVShowsSet.prototype.emptyView = List.Empty;

    TVShowsSet.prototype.tagName = "ul";

    TVShowsSet.prototype.className = "card-grid--tall";

    return TVShowsSet;

  })(App.Views.CollectionView);
});

this.Kodi.module("TVShowApp.Season", function(Season, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    getSeasonList: function(collection) {
      var view;
      view = new Season.Seasons({
        collection: collection
      });
      App.listenTo(view, 'childview:season:play', function(list, item) {
        var playlist;
        return playlist = App.request("command:kodi:controller", 'video', 'PlayList');
      });
      return view;
    }
  };
  Season.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      var id, seasonId, tvshow;
      id = parseInt(options.id);
      seasonId = parseInt(options.season);
      tvshow = App.request("tvshow:entity", id);
      return App.execute("when:entity:fetched", tvshow, (function(_this) {
        return function() {
          _this.layout = _this.getLayoutView(tvshow);
          _this.listenTo(_this.layout, "show", function() {
            _this.getDetailsLayoutView(tvshow, seasonId);
            return _this.getEpisodes(tvshow, seasonId);
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getLayoutView = function(tvshow) {
      return new Season.PageLayout({
        tvshow: tvshow
      });
    };

    Controller.prototype.getDetailsLayoutView = function(tvshow, seasonId) {
      var seasons;
      seasons = App.request("season:entities", tvshow.get('id'));
      return App.execute("when:entity:fetched", seasons, (function(_this) {
        return function() {
          var headerLayout, season;
          season = seasons.findWhere({
            season: seasonId
          });
          tvshow.set({
            season: seasonId,
            thumbnail: season.get('thumbnail'),
            seasons: seasons
          });
          headerLayout = new Season.HeaderLayout({
            model: tvshow
          });
          _this.listenTo(headerLayout, "show", function() {
            var detail, teaser;
            teaser = new Season.SeasonDetailTeaser({
              model: tvshow
            });
            detail = new Season.Details({
              model: tvshow
            });
            headerLayout.regionSide.show(teaser);
            return headerLayout.regionMeta.show(detail);
          });
          return _this.layout.regionHeader.show(headerLayout);
        };
      })(this));
    };

    Controller.prototype.getEpisodes = function(tvshow, seasonId) {
      var collection;
      collection = App.request("episode:entities", tvshow.get('tvshowid'), seasonId);
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          var view;
          collection.sortCollection('episode', 'asc');
          view = App.request("episode:list:view", collection);
          return _this.layout.regionContent.show(view);
        };
      })(this));
    };

    return Controller;

  })(App.Controllers.Base);
  return App.reqres.setHandler("season:list:view", function(collection) {
    return API.getSeasonList(collection);
  });
});

this.Kodi.module("TVShowApp.Season", function(Season, App, Backbone, Marionette, $, _) {
  Season.SeasonTeaser = (function(_super) {
    __extends(SeasonTeaser, _super);

    function SeasonTeaser() {
      return SeasonTeaser.__super__.constructor.apply(this, arguments);
    }

    SeasonTeaser.prototype.triggers = {
      "click .play": "season:play"
    };

    SeasonTeaser.prototype.initialize = function() {
      SeasonTeaser.__super__.initialize.apply(this, arguments);
      return this.model.set({
        label: 'Season ' + this.model.get('season')
      });
    };

    return SeasonTeaser;

  })(App.Views.CardView);
  Season.Empty = (function(_super) {
    __extends(Empty, _super);

    function Empty() {
      return Empty.__super__.constructor.apply(this, arguments);
    }

    Empty.prototype.tagName = "li";

    Empty.prototype.className = "season-empty-result";

    return Empty;

  })(App.Views.EmptyViewResults);
  Season.Seasons = (function(_super) {
    __extends(Seasons, _super);

    function Seasons() {
      return Seasons.__super__.constructor.apply(this, arguments);
    }

    Seasons.prototype.childView = Season.SeasonTeaser;

    Seasons.prototype.emptyView = Season.Empty;

    Seasons.prototype.tagName = "ul";

    Seasons.prototype.className = "card-grid--tall";

    return Seasons;

  })(App.Views.CollectionView);
  Season.PageLayout = (function(_super) {
    __extends(PageLayout, _super);

    function PageLayout() {
      return PageLayout.__super__.constructor.apply(this, arguments);
    }

    PageLayout.prototype.className = 'season-show detail-container';

    return PageLayout;

  })(App.Views.LayoutWithHeaderView);
  Season.HeaderLayout = (function(_super) {
    __extends(HeaderLayout, _super);

    function HeaderLayout() {
      return HeaderLayout.__super__.constructor.apply(this, arguments);
    }

    HeaderLayout.prototype.className = 'season-details';

    return HeaderLayout;

  })(App.Views.LayoutDetailsHeaderView);
  Season.Details = (function(_super) {
    __extends(Details, _super);

    function Details() {
      return Details.__super__.constructor.apply(this, arguments);
    }

    Details.prototype.template = 'apps/tvshow/season/details_meta';

    return Details;

  })(App.Views.ItemView);
  return Season.SeasonDetailTeaser = (function(_super) {
    __extends(SeasonDetailTeaser, _super);

    function SeasonDetailTeaser() {
      return SeasonDetailTeaser.__super__.constructor.apply(this, arguments);
    }

    SeasonDetailTeaser.prototype.tagName = "div";

    SeasonDetailTeaser.prototype.className = "card-detail";

    SeasonDetailTeaser.prototype.triggers = {
      "click .menu": "season-menu:clicked"
    };

    return SeasonDetailTeaser;

  })(App.Views.CardView);
});

this.Kodi.module("TVShowApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  return Show.Controller = (function(_super) {
    __extends(Controller, _super);

    function Controller() {
      return Controller.__super__.constructor.apply(this, arguments);
    }

    Controller.prototype.initialize = function(options) {
      var id, tvshow;
      id = parseInt(options.id);
      tvshow = App.request("tvshow:entity", id);
      return App.execute("when:entity:fetched", tvshow, (function(_this) {
        return function() {
          _this.layout = _this.getLayoutView(tvshow);
          _this.listenTo(_this.layout, "destroy", function() {
            return App.execute("images:fanart:set", 'none');
          });
          _this.listenTo(_this.layout, "show", function() {
            _this.getDetailsLayoutView(tvshow);
            return _this.getSeasons(tvshow);
          });
          return App.regionContent.show(_this.layout);
        };
      })(this));
    };

    Controller.prototype.getLayoutView = function(tvshow) {
      return new Show.PageLayout({
        model: tvshow
      });
    };

    Controller.prototype.getDetailsLayoutView = function(tvshow) {
      var headerLayout;
      headerLayout = new Show.HeaderLayout({
        model: tvshow
      });
      this.listenTo(headerLayout, "show", (function(_this) {
        return function() {
          var detail, teaser;
          teaser = new Show.TVShowTeaser({
            model: tvshow
          });
          detail = new Show.Details({
            model: tvshow
          });
          headerLayout.regionSide.show(teaser);
          return headerLayout.regionMeta.show(detail);
        };
      })(this));
      return this.layout.regionHeader.show(headerLayout);
    };

    Controller.prototype.getSeasons = function(tvshow) {
      var collection;
      collection = App.request("season:entities", tvshow.get('tvshowid'));
      return App.execute("when:entity:fetched", collection, (function(_this) {
        return function() {
          var view;
          view = App.request("season:list:view", collection);
          return _this.layout.regionContent.show(view);
        };
      })(this));
    };

    return Controller;

  })(App.Controllers.Base);
});

this.Kodi.module("TVShowApp.Show", function(Show, App, Backbone, Marionette, $, _) {
  Show.PageLayout = (function(_super) {
    __extends(PageLayout, _super);

    function PageLayout() {
      return PageLayout.__super__.constructor.apply(this, arguments);
    }

    PageLayout.prototype.className = 'tvshow-show detail-container';

    return PageLayout;

  })(App.Views.LayoutWithHeaderView);
  Show.HeaderLayout = (function(_super) {
    __extends(HeaderLayout, _super);

    function HeaderLayout() {
      return HeaderLayout.__super__.constructor.apply(this, arguments);
    }

    HeaderLayout.prototype.className = 'tvshow-details';

    return HeaderLayout;

  })(App.Views.LayoutDetailsHeaderView);
  Show.Details = (function(_super) {
    __extends(Details, _super);

    function Details() {
      return Details.__super__.constructor.apply(this, arguments);
    }

    Details.prototype.template = 'apps/tvshow/show/details_meta';

    return Details;

  })(App.Views.ItemView);
  return Show.TVShowTeaser = (function(_super) {
    __extends(TVShowTeaser, _super);

    function TVShowTeaser() {
      return TVShowTeaser.__super__.constructor.apply(this, arguments);
    }

    TVShowTeaser.prototype.tagName = "div";

    TVShowTeaser.prototype.className = "card-detail";

    TVShowTeaser.prototype.triggers = {
      "click .menu": "tvshow-menu:clicked"
    };

    return TVShowTeaser;

  })(App.Views.CardView);
});

this.Kodi.module("TVShowApp", function(TVShowApp, App, Backbone, Marionette, $, _) {
  var API;
  TVShowApp.Router = (function(_super) {
    __extends(Router, _super);

    function Router() {
      return Router.__super__.constructor.apply(this, arguments);
    }

    Router.prototype.appRoutes = {
      "tvshows/recent": "landing",
      "tvshows": "list",
      "tvshow/:tvshowid": "view",
      "tvshow/:tvshowid/:season": "season",
      "tvshow/:tvshowid/:season/:episodeid": "episode"
    };

    return Router;

  })(App.Router.Base);
  API = {
    landing: function() {
      return new TVShowApp.Landing.Controller();
    },
    list: function() {
      return new TVShowApp.List.Controller();
    },
    view: function(tvshowid) {
      return new TVShowApp.Show.Controller({
        id: tvshowid
      });
    },
    season: function(tvshowid, season) {
      return new TVShowApp.Season.Controller({
        id: tvshowid,
        season: season
      });
    },
    episode: function(tvshowid, season, episodeid) {
      return new TVShowApp.Episode.Controller({
        id: tvshowid,
        season: season,
        episodeid: episodeid
      });
    },
    episodeAction: function(op, view) {
      var files, model, playlist, videoLib;
      model = view.model;
      playlist = App.request("command:kodi:controller", 'video', 'PlayList');
      files = App.request("command:kodi:controller", 'video', 'Files');
      videoLib = App.request("command:kodi:controller", 'video', 'VideoLibrary');
      switch (op) {
        case 'play':
          return App.execute("input:resume", model, 'episodeid');
        case 'add':
          return playlist.add('episodeid', model.get('episodeid'));
        case 'localplay':
          return files.videoStream(model.get('file'), model.get('fanart'));
        case 'download':
          return files.downloadFile(model.get('file'));
        case 'toggleWatched':
          return videoLib.toggleWatched(model);
      }
    }
  };
  App.commands.setHandler('episode:action', function(op, view) {
    return API.episodeAction(op, view);
  });
  return App.on("before:start", function() {
    return new TVShowApp.Router({
      controller: API
    });
  });
});

this.Kodi.module("UiApp", function(UiApp, App, Backbone, Marionette, $, _) {
  var API;
  API = {
    openModal: function(title, msg, open, style) {
      var $body, $modal, $title;
      if (open == null) {
        open = true;
      }
      if (style == null) {
        style = '';
      }
      $title = App.getRegion('regionModalTitle').$el;
      $body = App.getRegion('regionModalBody').$el;
      $modal = App.getRegion('regionModal').$el;
      $modal.removeClassStartsWith('style-');
      $modal.addClass('style-' + style);
      $title.html(title);
      $body.html(msg);
      if (open) {
        $modal.modal();
      }
      $modal.on('hidden.bs.modal', function(e) {
        return $body.html('');
      });
      return $modal;
    },
    closeModal: function() {
      App.getRegion('regionModal').$el.modal('hide');
      return $('.modal-body').html('');
    },
    closeModalButton: function() {
      return API.getButton('Close', 'default').on('click', function() {
        return API.closeModal();
      });
    },
    getModalButtonContainer: function() {
      return App.getRegion('regionModalFooter').$el.empty();
    },
    getButton: function(text, type) {
      if (type == null) {
        type = 'primary';
      }
      return $('<button>').addClass('btn btn-' + type).html(text);
    },
    defaultButtons: function(callback) {
      var $ok;
      $ok = API.getButton('Ok', 'primary').on('click', function() {
        if (callback) {
          callback();
        }
        return API.closeModal();
      });
      return API.getModalButtonContainer().append(API.closeModalButton()).append($ok);
    },
    playerMenu: function(op) {
      var $el, openClass;
      if (op == null) {
        op = 'toggle';
      }
      $el = $('.player-menu-wrapper');
      openClass = 'opened';
      switch (op) {
        case 'open':
          return $el.addClass(openClass);
        case 'close':
          return $el.removeClass(openClass);
        default:
          return $el.toggleClass(openClass);
      }
    },
    buildOptions: function(options) {
      var $newOption, $option, $wrap, option, _i, _len;
      if (options.length === 0) {
        return;
      }
      $wrap = $('<ul>').addClass('modal-options options-list');
      $option = $('<li>');
      for (_i = 0, _len = options.length; _i < _len; _i++) {
        option = options[_i];
        $newOption = $option.clone();
        $newOption.html(option);
        $newOption.click(function(e) {
          API.closeModal();
          return $(this).closest('ul').find('li, span').unbind('click');
        });
        $wrap.append($newOption);
      }
      return $wrap;
    }
  };
  App.commands.setHandler("ui:textinput:show", function(title, msg, callback, open) {
    var $input, $msg, el;
    if (msg == null) {
      msg = '';
    }
    if (open == null) {
      open = true;
    }
    $input = $('<input>', {
      id: 'text-input',
      "class": 'form-control',
      type: 'text'
    }).on('keyup', function(e) {
      if (e.keyCode === 13 && callback) {
        callback($('#text-input').val());
        return API.closeModal();
      }
    });
    $msg = $('<p>').html(msg);
    API.defaultButtons(function() {
      return callback($('#text-input').val());
    });
    API.openModal(title, $msg, callback, open);
    el = App.getRegion('regionModalBody').$el.append($input.wrap('<div class="form-control-wrapper"></div>'));
    setTimeout(function() {
      return el.find('input').first().focus();
    }, 200);
    return $.material.init();
  });
  App.commands.setHandler("ui:modal:close", function() {
    return API.closeModal();
  });
  App.commands.setHandler("ui:modal:show", function(title, msg, footer) {
    if (msg == null) {
      msg = '';
    }
    if (footer == null) {
      footer = '';
    }
    API.getModalButtonContainer().html(footer);
    return API.openModal(title, msg, open);
  });
  App.commands.setHandler("ui:modal:form:show", function(title, msg) {
    if (msg == null) {
      msg = '';
    }
    return API.openModal(title, msg, true, 'form');
  });
  App.commands.setHandler("ui:modal:close", function() {
    return API.closeModal();
  });
  App.commands.setHandler("ui:modal:youtube", function(title, videoid) {
    var msg;
    API.getModalButtonContainer().html('');
    msg = '<iframe width="560" height="315" src="https://www.youtube.com/embed/' + videoid + '?rel=0&amp;showinfo=0&amp;autoplay=1" frameborder="0" allowfullscreen></iframe>';
    return API.openModal(title, msg, true, 'video');
  });
  App.commands.setHandler("ui:modal:options", function(title, items) {
    var $options;
    $options = API.buildOptions(items);
    return API.openModal(title, $options, true, 'options');
  });
  App.commands.setHandler("ui:playermenu", function(op) {
    return API.playerMenu(op);
  });
  return App.vent.on("shell:ready", (function(_this) {
    return function(options) {
      return $('html').on('click', function() {
        return API.playerMenu('close');
      });
    };
  })(this));
});
