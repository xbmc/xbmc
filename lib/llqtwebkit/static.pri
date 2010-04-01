# Detect if Qt is static or shared
CONFIG(debug, debug|release) {
    win32:PRL = $$[QT_INSTALL_LIBS] QtGui.prl
} else {
    win32:PRL = $$[QT_INSTALL_LIBS] QtGuid.prl
}

unix:!mac: PRL = $$[QT_INSTALL_LIBS] libQtGui.prl
mac: PRL = $$[QT_INSTALL_LIBS] QtGui.framework/QtGui.prl
include($$join(PRL, "/"))

contains(QMAKE_PRL_CONFIG, static) {
    DEFINES += STATIC_QT
    QTPLUGIN += qgif
} else {
    DEFINES += SHARED_QT
}

