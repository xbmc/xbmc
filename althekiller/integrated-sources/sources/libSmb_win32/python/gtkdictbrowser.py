#!/usr/bin/python
#
# Browse a Python dictionary in a two pane graphical interface written
# in GTK.
#
# The GtkDictBrowser class is supposed to be generic enough to allow
# applications to override enough methods and produce a
# domain-specific browser provided the information is presented as a
# Python dictionary.
#
# Possible applications:
#
#   - Windows registry browser
#   - SPOOLSS printerdata browser
#   - tdb file browser
#

from gtk import *
import string, re

class GtkDictBrowser:

    def __init__(self, dict):
        self.dict = dict
        
        # This variable stores a list of (regexp, function) used to
        # convert the raw value data to a displayable string.

        self.get_value_text_fns = []
        self.get_key_text = lambda x: x

        # We can filter the list of keys displayed using a regex

        self.filter_regex = ""

    # Create and configure user interface widgets.  A string argument is
    # used to set the window title.

    def build_ui(self, title):
        win = GtkWindow()
        win.set_title(title)

        win.connect("destroy", mainquit)

        hpaned = GtkHPaned()
        win.add(hpaned)
        hpaned.set_border_width(5)
        hpaned.show()

        vbox = GtkVBox()
        hpaned.add1(vbox)
        vbox.show()

        scrolled_win = GtkScrolledWindow()
        scrolled_win.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC)
        vbox.pack_start(scrolled_win)
        scrolled_win.show()

        hbox = GtkHBox()
        vbox.pack_end(hbox, expand = 0, padding = 5)
        hbox.show()

        label = GtkLabel("Filter:")
        hbox.pack_start(label, expand = 0, padding = 5)
        label.show()

        self.entry = GtkEntry()
        hbox.pack_end(self.entry, padding = 5)
        self.entry.show()

        self.entry.connect("activate", self.filter_activated)
        
        self.list = GtkList()
        self.list.set_selection_mode(SELECTION_MULTIPLE)
        self.list.set_selection_mode(SELECTION_BROWSE)
        scrolled_win.add_with_viewport(self.list)
        self.list.show()

        self.list.connect("select_child", self.key_selected)

        scrolled_win = GtkScrolledWindow()
        scrolled_win.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC)
        hpaned.add2(scrolled_win)
        scrolled_win.set_usize(500,400)
        scrolled_win.show()
        
        self.text = GtkText()
        self.text.set_editable(FALSE)
        scrolled_win.add_with_viewport(self.text)
        self.text.show()

        self.text.connect("event", self.event_handler)

        self.menu = GtkMenu()
        self.menu.show()

        self.font = load_font("fixed")

        self.update_keylist()

        win.show()

    # Add a key to the left hand side of the user interface

    def add_key(self, key):
        display_key = self.get_key_text(key)
        list_item = GtkListItem(display_key)
        list_item.set_data("raw_key", key) # Store raw key in item data
        self.list.add(list_item)
        list_item.show()

    # Event handler registered by build_ui()

    def event_handler(self, event, menu):
        return FALSE

    # Set the text to appear in the right hand side of the user interface 

    def set_value_text(self, item):

        # Clear old old value in text window

        self.text.delete_text(0, self.text.get_length())
        
        if type(item) == str:

            # The text widget has trouble inserting text containing NULL
            # characters.
            
            item = string.replace(item, "\x00", ".")
            
            self.text.insert(self.font, None, None, item)

        else:

            # A non-text item
            
            self.text.insert(self.font, None, None, repr(item))
            
    # This function is called when a key is selected in the left hand side
    # of the user interface.

    def key_selected(self, list, list_item):
        key = list_item.children()[0].get()

        # Look for a match in the value display function list

        text = self.dict[list_item.get_data("raw_key")]

        for entry in self.get_value_text_fns:
            if re.match(entry[0], key):
                text = entry[1](text)
                break

        self.set_value_text(text)

    # Refresh the key list by removing all items and re-inserting them.
    # Items are only inserted if they pass through the filter regexp.

    def update_keylist(self):
        self.list.remove_items(self.list.children())
        self.set_value_text("")
        for k in self.dict.keys():
            if re.match(self.filter_regex, k):
                self.add_key(k)

    # Invoked when the user hits return in the filter text entry widget.

    def filter_activated(self, entry):
        self.filter_regex = entry.get_text()
        self.update_keylist()

    # Register a key display function

    def register_get_key_text_fn(self, fn):
        self.get_key_text = fn

    # Register a value display function

    def register_get_value_text_fn(self, regexp, fn):
        self.get_value_text_fns.append((regexp, fn))

#
# A utility function to convert a string to the standard hex + ascii format.
# To display all values in hex do:
#   register_get_value_text_fn("", gtkdictbrowser.hex_string)
#

def hex_string(data):
    """Return a hex dump of a string as a string.

    The output produced is in the standard 16 characters per line hex +
    ascii format:

    00000000: 40 00 00 00 00 00 00 00  40 00 00 00 01 00 04 80  @....... @.......
    00000010: 01 01 00 00 00 00 00 01  00 00 00 00              ........ ....
    """
    
    pos = 0                             # Position in data
    line = 0                            # Line of data
    
    hex = ""                            # Hex display
    ascii = ""                          # ASCII display

    result = ""
    
    while pos < len(data):
        
	# Start with header
        
	if pos % 16 == 0:
            hex = "%08x: " % (line * 16)
            ascii = ""
            
        # Add character
            
	hex = hex + "%02x " % (ord(data[pos]))
        
        if ord(data[pos]) < 32 or ord(data[pos]) > 176:
            ascii = ascii + '.'
        else:
            ascii = ascii + data[pos]
                
        pos = pos + 1
            
        # Add separator if half way
            
	if pos % 16 == 8:
            hex = hex + " "
            ascii = ascii + " "

        # End of line

	if pos % 16 == 0:
            result = result + "%s %s\n" % (hex, ascii)
            line = line + 1
            
    # Leftover bits

    if pos % 16 != 0:

        # Pad hex string

        for i in range(0, (16 - (pos % 16))):
            hex = hex + "   "

        # Half way separator

        if (pos % 16) < 8:
            hex = hex + " "

        result = result + "%s %s\n" % (hex, ascii)

    return result

# For testing purposes, create a fixed dictionary to browse with

if __name__ == "__main__":

    dict = {"chicken": "ham", "spam": "fun", "subdict": {"a": "b", "c": "d"}}

    db = GtkDictBrowser(dict)

    db.build_ui("GtkDictBrowser")

    # Override Python's handling of ctrl-c so we can break out of the
    # gui from the command line.

    import signal
    signal.signal(signal.SIGINT, signal.SIG_DFL)

    mainloop()
