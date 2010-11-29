import xbmc
import xbmcgui
import os, sys
from xmlParser import XMLParser

#enable localization
getLS   = sys.modules[ "__main__" ].__language__
__cwd__ = sys.modules[ "__main__" ].__cwd__


class GUI(xbmcgui.WindowXMLDialog):

    def __init__(self, *args, **kwargs):
        xbmcgui.WindowXMLDialog.__init__(self, *args, **kwargs)
        self.setNum = kwargs['setNum']
        self.parser = XMLParser()
        if self.parser.feedsTree:
            self.doModal()


    def onInit(self):
        self.defineControls()
        if not self.parser.feedsList:
            xbmcgui.Dialog().ok(getLS(40)+'RssFeeds.xml', 'RssFeeds.xml '+getLS(41), getLS(42), getLS(43))
            self.closeDialog()
        self.showDialog()

    def defineControls(self):
        #actions
        self.action_cancel_dialog = (9, 10)
        #control ids
        self.control_heading_label_id       = 2
        self.control_list_label_id          = 3
        self.control_list_id                = 10
        self.control_modifySet_button_id    = 11
        self.control_add_button_id          = 13
        self.control_remove_button_id       = 14
        self.control_ok_button_id           = 18
        self.control_cancel_button_id       = 19
        #controls
        self.heading_label      = self.getControl(self.control_heading_label_id)
        self.list_label         = self.getControl(self.control_list_label_id)
        self.list               = self.getControl(self.control_list_id)
        self.add_button         = self.getControl(self.control_add_button_id)
        self.remove_button      = self.getControl(self.control_remove_button_id)
        self.modifySet_button   = self.getControl(self.control_modifySet_button_id)
        self.ok_button          = self.getControl(self.control_ok_button_id)
        self.cancel_button      = self.getControl(self.control_cancel_button_id)
        #defaults
        self.dFeedsList = [{'url':'http://feeds.feedburner.com/xbmc', 'updateinterval':'30'}]

    def showDialog(self):
        self.heading_label.setLabel(getLS(30))
        self.list_label.setLabel(getLS(24))
        self.modifySet_button.setLabel(getLS(6))
        self.updateSetsList()

    def closeDialog(self):
        """Close the Set Editor Dialog and open RSS Editor Dialog"""
        import rssEditor
        rssEditorUI = rssEditor.GUI("script-RSS_Editor-rssEditor.xml", __cwd__, "default", setNum = self.setNum)
        self.close()
        del rssEditorUI

    def onClick(self, controlId):
        #select existing set
        if controlId == self.control_list_id:
            setItem = self.list.getSelectedItem()
            self.setNum = setItem.getLabel()
            self.parser.writeXmlToFile()
            self.closeDialog()
        #add new set
        elif controlId == self.control_add_button_id:
            self.getNewSet()
            self.updateSetsList()
        #remove existing set
        elif controlId == self.control_remove_button_id:
            self.removeSet()
            self.updateSetsList()
        #modify existing set
        elif controlId == self.control_modifySet_button_id:
            self.editSet()
            self.updateSetsList()
        #write sets to file/dialog to modify feeds within set.
        elif controlId == self.control_ok_button_id:
            self.parser.writeXmlToFile()
            self.closeDialog()
        #cancel dialog
        elif controlId == self.control_cancel_button_id:
            self.closeDialog()

    def onAction(self, action):
        if action in self.action_cancel_dialog:
            self.closeDialog()

    def onFocus(self, controlId):
        pass

    def editSet(self):
        """Edit the attributes of an existing set"""
        setItem = self.list.getSelectedItem()
        oldSetLabel = setItem.getLabel()
        #ask user for set number
        newSetNum = self.getSetNum(oldSetLabel[3:])
        if newSetNum:
            newSetLabel = 'set'+newSetNum
            #ask user if set contains right to left text
            rtl = self.containsRTLText()
            #copy settings from old label
            self.parser.feedsList[newSetLabel] = self.parser.feedsList[oldSetLabel]
            #apply new attributes
            self.parser.feedsList[newSetLabel]['attrs'] = {'rtl':rtl, 'id':newSetNum}
            #if the set# changes, remove the old one.
            if newSetLabel != oldSetLabel:
                self.removeSet(oldSetLabel)

    def getNewSet(self):
        """Add a new set with some default values"""
        #default setNumber = find highest numbered set, then add 1
        defaultSetNum = max([int(setNum[3:]) for setNum in self.parser.feedsList.keys()])+1
        #ask user for set number
        newSetNum = self.getSetNum(defaultSetNum)
        #check if set number already exists
        if newSetNum:
            newSetLabel = 'set'+newSetNum
            #ask user if set contains right to left text
            rtl = self.containsRTLText()
            #add default information
            self.parser.feedsList[newSetLabel] = {'feedslist':self.dFeedsList, 'attrs':{'rtl':rtl, 'id':newSetNum}}

    def getSetNum(self, defaultSetNum, title = getLS(25)):
        newSetNum = str(xbmcgui.Dialog().numeric(0, title, str(defaultSetNum)))
        if self.setNumExists(newSetNum) and newSetNum != defaultSetNum:
            self.getSetNum(defaultSetNum, getLS(50) % newSetNum)
        else:
            return newSetNum

    def setNumExists(self, setNum):
        if 'set'+setNum in self.parser.feedsList.keys():
            return True

    def containsRTLText(self):
        """Returns xml style lowercase 'true' or 'false'"""
        return str(bool(xbmcgui.Dialog().yesno(getLS(27), getLS(27)))).lower()

    def removeSet(self, setNum = None):
        """Removes a set or if set is required resets it to default"""
        if setNum is None:
            setNum = self.list.getSelectedItem().getLabel()
        if setNum == 'set1':
            #Ask if user wants to set everything to default.
            if xbmcgui.Dialog().yesno(getLS(45), getLS(46), getLS(47)):
                self.parser.feedsList[setNum] = {'feedslist':self.dFeedsList, 'attrs':{'rtl':'false','id':'1'}}
        else:
            del self.parser.feedsList[setNum]

    def updateSetsList(self):
        self.list.reset()
        for setNum in sorted(self.parser.feedsList.keys()):
            self.list.addItem(setNum)
            self.list_label.setLabel(getLS(24))
