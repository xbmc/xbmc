;;; autotest-mode.el --- autotest code editing commands for Emacs

;; Author: Akim Demaille (akim@freefriends.org)
;; Keywords: languages, faces, m4, Autotest

;; This file is part of Autoconf

;; Copyright (C) 2001, 2009, 2010 Free Software Foundation, Inc.
;;
;; This program is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;;; Commentary:

;; A major mode for editing autotest input (like testsuite.at).
;; Derived from autoconf-mode.el, by Martin Buchholz (martin@xemacs.org).

;;; Your should add the following to your Emacs configuration file:

;;   (autoload 'autotest-mode "autotest-mode"
;;             "Major mode for editing autotest files." t)
;;   (setq auto-mode-alist
;;         (cons '("\\.at\\'" . autotest-mode) auto-mode-alist))

;;; Code:

(defvar autotest-font-lock-keywords
  `(("\\bdnl\\b\\(.*\\)"  1 font-lock-comment-face t)
    ("\\$[0-9*#@]" . font-lock-variable-name-face)
    ("^\\(m4_define\\|m4_defun\\)(\\[*\\([A-Za-z0-9_]+\\)" 2 font-lock-function-name-face)
    ("^AT_SETUP(\\[+\\([^]]+\\)" 1 font-lock-function-name-face)
    ("^AT_DATA(\\[+\\([^]]+\\)" 1 font-lock-variable-name-face)
    ("\\b\\(_?m4_[_a-z0-9]*\\|_?A[ST]_[_A-Z0-9]+\\)\\b" . font-lock-keyword-face)
    "default font-lock-keywords")
)

(defvar autotest-mode-syntax-table nil
  "syntax table used in autotest mode")
(setq autotest-mode-syntax-table (make-syntax-table))
(modify-syntax-entry ?\" "\""  autotest-mode-syntax-table)
;;(modify-syntax-entry ?\' "\""  autotest-mode-syntax-table)
(modify-syntax-entry ?#  "<\n" autotest-mode-syntax-table)
(modify-syntax-entry ?\n ">#"  autotest-mode-syntax-table)
(modify-syntax-entry ?\( "()"   autotest-mode-syntax-table)
(modify-syntax-entry ?\) ")("   autotest-mode-syntax-table)
(modify-syntax-entry ?\[ "(]"  autotest-mode-syntax-table)
(modify-syntax-entry ?\] ")["  autotest-mode-syntax-table)
(modify-syntax-entry ?*  "."   autotest-mode-syntax-table)
(modify-syntax-entry ?_  "_"   autotest-mode-syntax-table)

(defvar autotest-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map '[(control c) (\;)] 'comment-region)
    map))

(defun autotest-current-defun ()
  "Autotest value for `add-log-current-defun-function'.
This tells add-log.el how to find the current test group/macro."
  (save-excursion
    (if (re-search-backward "^\\(m4_define\\|m4_defun\\|AT_SETUP\\)(\\[+\\([^]]+\\)" nil t)
	(buffer-substring (match-beginning 2)
			  (match-end 2))
      nil)))

;;;###autoload
(defun autotest-mode ()
  "A major-mode to edit Autotest files like testsuite.at.
\\{autotest-mode-map}
"
  (interactive)
  (kill-all-local-variables)
  (use-local-map autotest-mode-map)

  (make-local-variable 'add-log-current-defun-function)
  (setq add-log-current-defun-function 'autotest-current-defun)

  (make-local-variable 'comment-start)
  (setq comment-start "# ")
  (make-local-variable 'parse-sexp-ignore-comments)
  (setq parse-sexp-ignore-comments t)

  (make-local-variable	'font-lock-defaults)
  (setq major-mode 'autotest-mode)
  (setq mode-name "Autotest")
  (setq font-lock-defaults `(autotest-font-lock-keywords nil))
  (set-syntax-table autotest-mode-syntax-table)
  (run-hooks 'autotest-mode-hook))

(provide 'autotest-mode)

;;; autotest-mode.el ends here
