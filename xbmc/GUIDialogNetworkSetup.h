#pragma once
#include "GUIDialog.h"

class CGUIDialogNetworkSetup :
      public CGUIDialog
{
public:
  enum NET_PROTOCOL { NET_PROTOCOL_SMB = 0,
                      NET_PROTOCOL_XBMSP,
                      NET_PROTOCOL_FTP,
                      NET_PROTOCOL_DAAP,
                      NET_PROTOCOL_UPNP,
                      NET_PROTOCOL_TUXBOX};
  CGUIDialogNetworkSetup(void);
  virtual ~CGUIDialogNetworkSetup(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void OnInitWindow();

  static bool ShowAndGetNetworkAddress(CStdString &path);

  CStdString ConstructPath() const;
  void SetPath(const CStdString &path);
  bool IsConfirmed() const { return m_confirmed; };

protected:
  void OnProtocolChange();
  void OnServerBrowse();
  void OnServerAddress();
  void OnPath();
  void OnPort();
  void OnUserName();
  void OnPassword();
  void OnOK();
  void OnCancel();
  void UpdateButtons();

  NET_PROTOCOL m_protocol;
  CStdString m_server;
  CStdString m_path;
  CStdString m_username;
  CStdString m_password;
  CStdString m_port;

  bool m_confirmed;
};
