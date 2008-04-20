# Japanese messages for international release of SWAT.
# Copyright (C) 2003 TAKAHASHI Motonobu <monyo@samba.org>
# Copyright (C) 2000 Ryo Kawahara <rkawa@lbe.co.jp>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
msgid ""
msgstr ""
"Project-Id-Version: i18n-swat\n"
"POT-Creation-Date: 2003-10-06 05:30+0900\n"
"PO-Revision-Date: 2003-09-23 04:38+900\n"
"Last-Translator: TAKAHASHI Motonobu <monyo@samba.org>\n"
"Language-Team: Samba Users Group Japan <sugj-tech@samba.gr.jp>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"

#: ../web/swat.c:117
#, c-format
msgid "ERROR: Can't open %s"
msgstr "%s をオープンできません"

#: ../web/swat.c:200
msgid "Help"
msgstr "ヘルプ"

#: ../web/swat.c:206 ../web/swat.c:220 ../web/swat.c:235 ../web/swat.c:243 ../web/swat.c:252 ../web/swat.c:261 ../web/swat.c:267 ../web/swat.c:273 ../web/swat.c:286
msgid "Set Default"
msgstr "デフォルト値"

#: ../web/swat.c:408
#, c-format
msgid "failed to open %s for writing"
msgstr "%s を書き込み用にオープンできません"

#: ../web/swat.c:431
#, c-format
msgid "Can't reload %s"
msgstr "%s を再読み込みできません\n"

# msgid "Logged in as <b>%s</b><p>\n"
#: ../web/swat.c:501
#, c-format
msgid "Logged in as <b>%s</b>"
msgstr "<b>%s</b>としてログイン"

#: ../web/swat.c:505
msgid "Home"
msgstr "ホーム"

#: ../web/swat.c:507
msgid "Globals"
msgstr "グローバル"

#: ../web/swat.c:508
msgid "Shares"
msgstr "ファイル共有"

#: ../web/swat.c:509
msgid "Printers"
msgstr "印刷共有"

#: ../web/swat.c:510
msgid "Wizard"
msgstr "ウィザード"

#: ../web/swat.c:513
msgid "Status"
msgstr "サーバの状態"

#: ../web/swat.c:514
msgid "View Config"
msgstr "現在の設定"

#: ../web/swat.c:516
msgid "Password Management"
msgstr "パスワード管理"

#: ../web/swat.c:526
msgid "Current View Is"
msgstr "現在の表示モード"

#: ../web/swat.c:527 ../web/swat.c:530
msgid "Basic"
msgstr "標準表示"

#: ../web/swat.c:528 ../web/swat.c:531
msgid "Advanced"
msgstr "詳細表示"

#: ../web/swat.c:529
msgid "Change View To"
msgstr "表示モードの変更"

#: ../web/swat.c:554
msgid "Current Config"
msgstr "現在の設定"

#: ../web/swat.c:558
msgid "Normal View"
msgstr "標準表示"

#: ../web/swat.c:560
msgid "Full View"
msgstr "完全表示"

#. Here we first set and commit all the parameters that were selected
#. in the previous screen.
#: ../web/swat.c:579
msgid "Wizard Parameter Edit Page"
msgstr "ウィザードによるパラメータ編集ページ"

#: ../web/swat.c:608
msgid "Note: smb.conf file has been read and rewritten"
msgstr "smb.conf ファイルが書き換えられました。"

#. Here we go ...
#: ../web/swat.c:716
msgid "Samba Configuration Wizard"
msgstr "Samba 設定ウィザード"

#: ../web/swat.c:720
msgid "The \"Rewrite smb.conf file\" button will clear the smb.conf file of all default values and of comments."
msgstr "「smb.conf の書換」ボタンを押すと smb.conf ファイル中のすべてのデフォルト値やコメントは削除されます。"

#: ../web/swat.c:721
msgid "The same will happen if you press the commit button."
msgstr "「commit」ボタンを押した場合にも同様の変更が行なわれます。"

#: ../web/swat.c:724
msgid "Rewrite smb.conf file"
msgstr "smb.conf の書換"

#: ../web/swat.c:725
msgid "Commit"
msgstr "設定を反映"

#: ../web/swat.c:726
msgid "Edit Parameter Values"
msgstr "各パラメータの編集"

#: ../web/swat.c:732
msgid "Server Type"
msgstr "サーバタイプ"

#: ../web/swat.c:733
msgid "Stand Alone"
msgstr "スタンドアロン"

#: ../web/swat.c:734
msgid "Domain Member"
msgstr "ドメインメンバ"

#: ../web/swat.c:735
msgid "Domain Controller"
msgstr "ドメインコントローラ"

#: ../web/swat.c:738
msgid "Unusual Type in smb.conf - Please Select New Mode"
msgstr "通常の形式ではない - 新しいモードを選択のこと"

#: ../web/swat.c:740
msgid "Configure WINS As"
msgstr "WINS"

#: ../web/swat.c:741
msgid "Not Used"
msgstr "使わない"

#: ../web/swat.c:742
msgid "Server for client use"
msgstr "サーバとして構成"

#: ../web/swat.c:743
msgid "Client of another WINS server"
msgstr "別の WINS サーバのクライアントとして構成"

#: ../web/swat.c:745
msgid "Remote WINS Server"
msgstr "別の WINS サーバ"

#: ../web/swat.c:756
msgid "Error: WINS Server Mode and WINS Support both set in smb.conf"
msgstr "エラー: wins server と wins support の両パラメータが smb.conf で指定されています"

#: ../web/swat.c:757
msgid "Please Select desired WINS mode above."
msgstr "いずれかの WINS モードを選択してください。"

#: ../web/swat.c:759
msgid "Expose Home Directories"
msgstr "ホームディレクトリの公開"

#: ../web/swat.c:774
msgid "The above configuration options will set multiple parameters and will generally assist with rapid Samba deployment."
msgstr "上記の設定オプションにより、複数のパラメータが同期して設定されるため、 Samba の運用を迅速に開始する上での助けとなるでしょう。"

#: ../web/swat.c:787
msgid "Global Parameters"
msgstr "Global パラメータ"

#: ../web/swat.c:815 ../web/swat.c:916 ../web/swat.c:1265
msgid "Commit Changes"
msgstr "変更を反映"

#: ../web/swat.c:819 ../web/swat.c:919 ../web/swat.c:1267
msgid "Reset Values"
msgstr "変更を取消"

#: ../web/swat.c:844
msgid "Share Parameters"
msgstr "ファイル共有 パラメータ"

#: ../web/swat.c:887
msgid "Choose Share"
msgstr "ファイル共有の選択"

#: ../web/swat.c:901
msgid "Delete Share"
msgstr "ファイル共有の削除"

#: ../web/swat.c:908
msgid "Create Share"
msgstr "ファイル共有の作成"

#: ../web/swat.c:944
msgid "password change in demo mode rejected"
msgstr "デモ・モードでのパスワード変更はできません"

#: ../web/swat.c:957
msgid "Can't setup password database vectors."
msgstr "パスワード・データベースが見つけられません"

#: ../web/swat.c:983
msgid " Must specify \"User Name\" "
msgstr "「ユーザ名」欄に入力してください"

#: ../web/swat.c:999
msgid " Must specify \"Old Password\" "
msgstr "「旧パスワード」欄に入力してください"

#: ../web/swat.c:1005
msgid " Must specify \"Remote Machine\" "
msgstr "「リモートマシン」欄に入力してください"

#: ../web/swat.c:1012
msgid " Must specify \"New, and Re-typed Passwords\" "
msgstr "「新パスワード」欄と「新パスワードの再入力」欄に入力してください"

#: ../web/swat.c:1018
msgid " Re-typed password didn't match new password "
msgstr "「新パスワードの再入力」欄の入力内容が「新パスワード」欄の入力と一致していません。"

#: ../web/swat.c:1048
#, c-format
msgid " The passwd for '%s' has been changed."
msgstr " %s のパスワードは変更されました。"

#: ../web/swat.c:1051
#, c-format
msgid " The passwd for '%s' has NOT been changed."
msgstr " '%s' のパスワードは変更されませんでした。"

#: ../web/swat.c:1076
msgid "Server Password Management"
msgstr "ローカルマシンのパスワード管理"

#.
#. * Create all the dialog boxes for data collection
#.
#: ../web/swat.c:1085 ../web/swat.c:1132
msgid "User Name"
msgstr "ユーザ名"

#: ../web/swat.c:1088 ../web/swat.c:1134
msgid "Old Password"
msgstr "旧パスワード"

#: ../web/swat.c:1091 ../web/swat.c:1136
msgid "New Password"
msgstr "新パスワード"

#: ../web/swat.c:1093 ../web/swat.c:1138
msgid "Re-type New Password"
msgstr "新パスワードの再入力"

#: ../web/swat.c:1101 ../web/swat.c:1149
msgid "Change Password"
msgstr "パスワード変更"

#: ../web/swat.c:1104
msgid "Add New User"
msgstr "新規ユーザ追加"

#: ../web/swat.c:1106
msgid "Delete User"
msgstr "ユーザの削除"

#: ../web/swat.c:1108
msgid "Disable User"
msgstr "ユーザの無効化"

#: ../web/swat.c:1110
msgid "Enable User"
msgstr "ユーザの有効化"

#: ../web/swat.c:1123
msgid "Client/Server Password Management"
msgstr "リモートマシンのパスワード管理"

#: ../web/swat.c:1140
msgid "Remote Machine"
msgstr "リモートマシン"

#: ../web/swat.c:1179
msgid "Printer Parameters"
msgstr "印刷共有 パラメータ"

#: ../web/swat.c:1181
msgid "Important Note:"
msgstr "*注"

#: ../web/swat.c:1182
msgid "Printer names marked with [*] in the Choose Printer drop-down box "
msgstr "名前の先頭に [*] がついたプリンタ"

#: ../web/swat.c:1183
msgid "are autoloaded printers from "
msgstr "は"

#: ../web/swat.c:1184
msgid "Printcap Name"
msgstr "printcap name パラメータ"

#: ../web/swat.c:1185
msgid "Attempting to delete these printers from SWAT will have no effect."
msgstr "から自動設定されたものですから、削除することはできません。"

#: ../web/swat.c:1231
msgid "Choose Printer"
msgstr "印刷共有の選択"

#: ../web/swat.c:1250
msgid "Delete Printer"
msgstr "印刷共有の削除"

#: ../web/swat.c:1257
msgid "Create Printer"
msgstr "印刷共有の作成"

#: ../web/statuspage.c:123
msgid "RDONLY     "
msgstr "参照のみ   "

#: ../web/statuspage.c:124
msgid "WRONLY     "
msgstr "更新のみ   "

#: ../web/statuspage.c:125
msgid "RDWR       "
msgstr "参照/更新  "

#: ../web/statuspage.c:309
msgid "Server Status"
msgstr "サーバの状態"

#: ../web/statuspage.c:314
msgid "Auto Refresh"
msgstr "自動更新の開始"

#: ../web/statuspage.c:315 ../web/statuspage.c:320
msgid "Refresh Interval: "
msgstr "更新間隔: "

#: ../web/statuspage.c:319
msgid "Stop Refreshing"
msgstr "自動更新の停止"

#: ../web/statuspage.c:334
msgid "version:"
msgstr "バージョン"

#: ../web/statuspage.c:337
msgid "smbd:"
msgstr ""

#: ../web/statuspage.c:337 ../web/statuspage.c:350 ../web/statuspage.c:364
msgid "running"
msgstr "実行中"

#: ../web/statuspage.c:337 ../web/statuspage.c:350 ../web/statuspage.c:364
msgid "not running"
msgstr "停止中"

#: ../web/statuspage.c:341
msgid "Stop smbd"
msgstr "smbd の停止"

#: ../web/statuspage.c:343
msgid "Start smbd"
msgstr "smbd の起動"

#: ../web/statuspage.c:345
msgid "Restart smbd"
msgstr "smbd の再起動"

#: ../web/statuspage.c:350
msgid "nmbd:"
msgstr ""

#: ../web/statuspage.c:354
msgid "Stop nmbd"
msgstr "nmbd の停止"

#: ../web/statuspage.c:356
msgid "Start nmbd"
msgstr "nmbd の起動"

#: ../web/statuspage.c:358
msgid "Restart nmbd"
msgstr "nmbd の再起動"

#: ../web/statuspage.c:364
msgid "winbindd:"
msgstr ""

#: ../web/statuspage.c:368
msgid "Stop winbindd"
msgstr "winbindd の停止"

#: ../web/statuspage.c:370
msgid "Start winbindd"
msgstr "winbindd の起動"

#: ../web/statuspage.c:372
msgid "Restart winbindd"
msgstr "winbindd の再起動"

#. stop, restart all
#: ../web/statuspage.c:381
msgid "Stop All"
msgstr "すべて停止"

#: ../web/statuspage.c:382
msgid "Restart All"
msgstr "すべて再起動"

#. start all
#: ../web/statuspage.c:386
msgid "Start All"
msgstr "すべて起動"

#: ../web/statuspage.c:393
msgid "Active Connections"
msgstr "接続中のクライアント"

#: ../web/statuspage.c:395 ../web/statuspage.c:408 ../web/statuspage.c:416
msgid "PID"
msgstr ""

#: ../web/statuspage.c:395 ../web/statuspage.c:408
msgid "Client"
msgstr "クライアント"

#: ../web/statuspage.c:395
msgid "IP address"
msgstr "IPアドレス"

#: ../web/statuspage.c:395 ../web/statuspage.c:408 ../web/statuspage.c:416
msgid "Date"
msgstr "日付"

#: ../web/statuspage.c:397
msgid "Kill"
msgstr "切断"

#: ../web/statuspage.c:405
msgid "Active Shares"
msgstr "接続中の共有"

#: ../web/statuspage.c:408
msgid "Share"
msgstr "共有名"

#: ../web/statuspage.c:408
msgid "User"
msgstr "ユーザ"

#: ../web/statuspage.c:408
msgid "Group"
msgstr "グループ"

#: ../web/statuspage.c:414
msgid "Open Files"
msgstr "使用中のファイル"

#: ../web/statuspage.c:416
msgid "Sharing"
msgstr "排他モード"

#: ../web/statuspage.c:416
msgid "R/W"
msgstr "参照/更新"

#: ../web/statuspage.c:416
msgid "Oplock"
msgstr ""

#: ../web/statuspage.c:416
msgid "File"
msgstr "ファイル名"

#: ../web/statuspage.c:425
msgid "Show Client in col 1"
msgstr "クライアント名を先頭に表示"

#: ../web/statuspage.c:426
msgid "Show PID in col 1"
msgstr "PIDを先頭に表示"

#: ../param/loadparm.c:755
msgid "Base Options"
msgstr "基本 オプション"

#: ../param/loadparm.c:775
msgid "Security Options"
msgstr "セキュリティ オプション"

#: ../param/loadparm.c:859
msgid "Logging Options"
msgstr "ロギング オプション"

#: ../param/loadparm.c:874
msgid "Protocol Options"
msgstr "プロトコル オプション"

#: ../param/loadparm.c:911
msgid "Tuning Options"
msgstr "チューニング オプション"

#: ../param/loadparm.c:940
msgid "Printing Options"
msgstr "印刷 オプション"

#: ../param/loadparm.c:970
msgid "Filename Handling"
msgstr "ファイル名の取扱"

#: ../param/loadparm.c:996
msgid "Domain Options"
msgstr "ドメイン オプション"

#: ../param/loadparm.c:1000
msgid "Logon Options"
msgstr "ログオン オプション"

#: ../param/loadparm.c:1019
msgid "Browse Options"
msgstr "ブラウジング オプション"

#: ../param/loadparm.c:1033
msgid "WINS Options"
msgstr "WINS オプション"

#: ../param/loadparm.c:1043
msgid "Locking Options"
msgstr "ロッキング オプション"

#: ../param/loadparm.c:1061
msgid "Ldap Options"
msgstr "LDAP オプション"

#: ../param/loadparm.c:1078
msgid "Miscellaneous Options"
msgstr "その他のオプション"

#: ../param/loadparm.c:1138
msgid "VFS module options"
msgstr "VFS オプション"

#: ../param/loadparm.c:1148
msgid "Winbind options"
msgstr "Winbind オプション"
