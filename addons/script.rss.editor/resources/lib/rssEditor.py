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
        self.feedsList = self.parser.feedsList[self.setNum]['feedslist'] #shortname
        if not self.feedsList:
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
        self.control_changeSet_button_id    = 11
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
        self.changeSet_button   = self.getControl(self.control_changeSet_button_id)
        self.ok_button          = self.getControl(self.control_ok_button_id)
        self.cancel_button      = self.getControl(self.control_cancel_button_id)

    def showDialog(self):
        self.heading_label.setLabel(getLS(0))
        self.list_label.setLabel(getLS(12))
        self.changeSet_button.setLabel(getLS(1))
        self.updateFeedsList()

    def closeDialog(self):
        self.close()

    def onClick(self, controlId):
        #edit existing feed
        if controlId == self.control_list_id:
            position = self.list.getSelectedPosition()
            oldUrl = self.feedsList[position]['url']
            oldUpdateInterval = self.feedsList[position]['updateinterval']
            newUrl, newUpdateInterval = self.getNewFeed(oldUrl, oldUpdateInterval)
            if newUrl:
                self.feedsList[position] = {'url':newUrl, 'updateinterval':newUpdateInterval}
            self.updateFeedsList()
        #add new feed
        elif controlId == self.control_add_button_id:
            newUrl, newUpdateInterval = self.getNewFeed()
            if newUrl:
                self.feedsList.append({'url':newUrl, 'updateinterval':newUpdateInterval})
            self.updateFeedsList()
        #remove existing feed
        elif controlId == self.control_remove_button_id:
            self.removeFeed()
            self.updateFeedsList()
        #change/modify set
        elif controlId == self.control_changeSet_button_id:
            import setEditor
            setEditorUI = setEditor.GUI("script-RSS_Editor-setEditor.xml", __cwd__, "default", setNum = self.setNum)
            self.close()
            del setEditorUI
        #save xml
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

    def removeFeed(self):
        position = self.list.getSelectedPosition()
        self.feedsList.remove(self.feedsList[position])
        #add empty feed if last one is deleted
        if len(self.feedsList) < 1:
            self.feedsList = [{'url':'http://', 'updateinterval':'30'}]

    def getNewFeed(self, url = 'http://', newUpdateInterval = '30'):
        kb = xbmc.Keyboard(url, getLS(12), False)
        kb.doModal()
        if kb.isConfirmed():
            newUrl = kb.getText()
            newUpdateInterval = xbmcgui.Dialog().numeric(0, getLS(13), newUpdateInterval)
        else:
            newUrl = None
        return newUrl, newUpdateInterval

    def updateFeedsList(self):
        self.list.reset()
        for feed in self.feedsList:
            self.list.addItem(feed['url'])
        if self.setNum == 'set1':
            self.list_label.setLabel(getLS(14) % (''))
        else:
            self.list_label.setLabel(getLS(14) % ('('+self.setNum+')'))
