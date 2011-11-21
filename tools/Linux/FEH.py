import os
import sys
import re

AvailableOutputs = []
Output = None

try:
    from qt import *
    AvailableOutputs.append("--error-output=Qt")
except:
    pass
try:
    import pygtk
    pygtk.require('2.0')
    import gtk
    AvailableOutputs.append("--error-output=GTK")
except:
    pass
try:
    import pygame
    import datetime
    AvailableOutputs.append("--error-output=SDL")
except:
    pass

def error(errorLine):
    if Output == "--error-output=Qt":
        createQt(errorLine)
    elif Output == "--error-output=GTK":
        createGTK(errorLine)
    elif Output == "--error-output=SDL":
        createSDL(errorLine)
    else:
        try:
            print(errorLine)
        except:
            print(errorLine)

    exit(1)

def createQt(errorLine):
    app = QApplication(sys.argv)
    QObject.connect(app, SIGNAL('lastWindowClosed()')
                       , app
                       , SLOT('quit()')
                       )
    
    dialog = QDialog(None, "Error", 0, 0)
    dialog.setCaption(dialog.tr("Error"))
    layout=QVBoxLayout(dialog)
    layout.setSpacing(6)
    layout.setMargin(5)

    label=QLabel(errorLine, dialog)

    layout.addWidget(label)

    bnExit=QPushButton("Quit", dialog, "add")
    dialog.connect(bnExit, SIGNAL("clicked()"), qApp, SLOT("quit()"))

    layout.addWidget(bnExit)

    app.setMainWidget(dialog)
    dialog.show()
    app.exec_loop()

def createGTK(errorLine):
    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    window.connect("destroy", lambda w: gtk.main_quit())

    window.set_title("Error")
    vbox = gtk.VBox(False, 5)
    window.add(vbox)
    window.set_border_width(5)

    frame = gtk.Frame()
    frame.set_shadow_type(gtk.SHADOW_NONE)
    label = gtk.Label(errorLine)
    frame.add(label)
    vbox.pack_start(frame, False, False, 0)

    button = gtk.Button("Quit")
    button.connect_object("clicked", gtk.Widget.destroy, window)

    vbox.pack_start(button, False, False, 0)

    window.show_all ()

    gtk.main()

def createSDL(errorLine):
    pygame.init()
    pygame.font.init()
    pygame.display.set_caption("Error")

    size = width, height = 800, 600
    speed = [2, 2]
    black = 0, 0, 0

    screen = pygame.display.set_mode(size)
    font = pygame.font.Font(None, 32)

    autoQuit = 10
    start = datetime.datetime.now()
    finish = datetime.datetime.now()
    delta = finish - start
    while delta.seconds < autoQuit:
        for event in pygame.event.get():
            if event.type == pygame.QUIT or event.type == pygame.KEYDOWN:
                sys.exit()

        screen.fill(black)

        place = [200, 200]
        for line in errorLine.split('\n'):
            text = font.render(line, 1, (255,255,255) )
            place[1] += font.size(line)[1]
            screen.blit(text, text.get_rect().move(place))
            

        quitline = "Press any button to continue ("
        quitline += str(autoQuit - delta.seconds)
        quitline += ")"
        text = font.render(quitline, 1, (255,255,255) )
        screen.blit(text, text.get_rect().move(200,400))

        pygame.display.flip()

        finish = datetime.datetime.now()
        delta = finish - start

def badDirectRendering():
    out = os.popen("glxinfo | grep \"direct rendering\"", 'r')
    line = out.read()
    direct = "Yes" not in line
    out.close()

    return direct

def badColorDepth():
    out = os.popen('xdpyinfo | grep "depth of root"', 'r')
    
    p = re.compile("([0-9]*) planes")
    for line in out.readlines():
        match = p.search(line)
        if (match is not None):
            if int(match.group(1)) > 16:
                bitDepth = False
            else:
                bitDepth = True
    out.close()

    return bitDepth

def possibleOutput(text):
    return text in sys.argv and text in AvailableOutputs

if __name__=="__main__":
    if len(AvailableOutputs) > 0:
        Output = AvailableOutputs[0]
    else:
        Output = None

    for text in sys.argv:
        if possibleOutput(text):
            Output = text

    if "--no-test" in sys.argv:
        exit(0)

    if (badDirectRendering()):
        error("XBMC needs hardware accelerated OpenGL rendering.\nInstall an appropriate graphics driver.\n\nPlease consult XBMC Wiki for supported hardware\nhttp://wiki.xbmc.org/?title=Supported_hardware")

    if (badColorDepth()):
        error("XBMC cannot run unless the\nscreen color depth is atleast 24 bit.\n\nPlease reconfigure your screen.")
