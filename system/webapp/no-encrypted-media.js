// The user did not allow this app DRM-protected video: as WebKit's YouTube quirk does, offer no EME.
delete Navigator.prototype.requestMediaKeySystemAccess;
