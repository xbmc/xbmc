;; TiMidity interface for Emacs
;;
;;   TiMidity++ -- MIDI to WAVE converter and player
;;   Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
;;   Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>
;;
;;   This program is free software; you can redistribute it and/or modify
;;   it under the terms of the GNU General Public License as published by
;;   the Free Software Foundation; either version 2 of the License, or
;;   (at your option) any later version.
;;
;;   This program is distributed in the hope that it will be useful,
;;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;   GNU General Public License for more details.
;;
;;   You should have received a copy of the GNU General Public License
;;   along with this program; if not, write to the Free Software
;;   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;;
;;
;; Author: Masanao Izumo <mo@goice.co.jp>
;; Create: Tue Oct 7 1997
;; LastModified: Tue Dec 9 1997
;; Keywords: timidity
;;
;;; Commentary:
;;
;; You write follows in ~/.emacs
;; (autoload 'timidity "timidity" "TiMidity Interface" t)
;; (setq timidity-prog-path "/usr/local/bin/timidity")
;;
;; Then you type:
;; M-x timidity

;; Configuration parameters.
; Absolute path of timidity.
(defvar timidity-prog-path "/usr/local/bin/timidity")

; String list for timidity program options.
(defvar timidity-default-options nil)

;; Follows are shouldn't need to be changed.
(defvar timidity-drumchannels nil)
(defvar timidity-prev-files nil)
(defvar timidity-next-files nil)
(defvar timidity-cmd-buffer-name "*TiMidityCmd*")
(defvar timidity-cmd-buffer nil)
(defvar timidity-cmd-process nil)
(defvar timidity-main-buffer-name "*TiMidity*")
(defvar timidity-main-buffer nil)
(defvar timidity-msg-buffer-name "*TiMidityMsg*")
(defvar timidity-msg-buffer nil)
(defvar timidity-orig-window-configuration nil)
(defvar timidity-note-table-ofs nil)
(defvar timidity-error-flag nil)
(defvar timidity-total-time 0)
(defvar timidity-current-time-bar 0)
(defvar timidity-current-filename "")
(defvar timidity-main-volume 0)
(defvar timidity-reset-bufstr nil)
(defvar timidity-cmd-version nil)
(defvar timidity-playing-flag nil)
(defvar timidity-pre-byte-compiles
  '(timidity-cmd-filter
    timidity-CMSG timidity-CURT timidity-NOTE timidity-PROG timidity-VOL
    timidity-EXP timidity-PAN timidity-SUS timidity-PIT))
(defvar timidity-jump-event-flag nil)

(defvar timidity-mode-map nil)
(if timidity-mode-map
    nil
  (setq timidity-mode-map (make-sparse-keymap))
  (define-key timidity-mode-map "l" 'timidity-load-file)
  (define-key timidity-mode-map "L" 'timidity-load-file-list)
  (define-key timidity-mode-map "n" 'timidity-next-file)
  (define-key timidity-mode-map "p" 'timidity-prev-file)
  (define-key timidity-mode-map "q" 'timidity-quit)
  (define-key timidity-mode-map "d" 'timidity-select-drumchannel)
  (define-key timidity-mode-map "r" 'timidity-replay-cmd)
  (define-key timidity-mode-map "V" 'timidity-simple-cmd)
  (define-key timidity-mode-map "v" 'timidity-simple-cmd)
  (define-key timidity-mode-map "1" 'timidity-simple-cmd)
  (define-key timidity-mode-map "2" 'timidity-simple-cmd)
  (define-key timidity-mode-map "3" 'timidity-simple-cmd)
  (define-key timidity-mode-map "4" 'timidity-simple-cmd)
  (define-key timidity-mode-map "5" 'timidity-simple-cmd)
  (define-key timidity-mode-map "6" 'timidity-simple-cmd)
  (define-key timidity-mode-map "f" 'timidity-simple-cmd)
  (define-key timidity-mode-map "b" 'timidity-simple-cmd)
  (define-key timidity-mode-map " " 'timidity-simple-cmd)
  (define-key timidity-mode-map "+" 'timidity-simple-cmd)
  (define-key timidity-mode-map "-" 'timidity-simple-cmd)
  (define-key timidity-mode-map ">" 'timidity-simple-cmd)
  (define-key timidity-mode-map "<" 'timidity-simple-cmd)
  (define-key timidity-mode-map "O" 'timidity-simple-cmd)
  (define-key timidity-mode-map "o" 'timidity-simple-cmd)
  (define-key timidity-mode-map "g" 'timidity-simple-cmd)
  )

(defun timidity ()
  (interactive)
  (let (width height check)
    (if (timidity-check-run)
	(progn
	  (switch-to-buffer timidity-main-buffer-name)
	  (error "Already timidity is running.")))
    (setq timidity-orig-window-configuration (current-window-configuration))
    (setq timidity-main-buffer (switch-to-buffer timidity-main-buffer-name))
    (delete-other-windows)
    (setq width (window-width))
    (setq height (1- (window-height)))
    (if (or (< width 80) (< height 23))
	(error "Window is too small."))
    (kill-all-local-variables)
    (use-local-map timidity-mode-map)
    (setq mode-name "TiMidity")
    (setq major-mode 'timidity-mode)
    (run-hooks 'timidity-mode-hook)
    (timidity-make-windows)
    (timidity-demo)
    (timidity-run)))

(defun timidity-simple-modeline ()
  (setq mode-line-modified nil)
  (setq mc-verbose-code nil)
  (setq mc-flag nil)
  (make-local-variable 'line-number-mode)
  (setq line-number-mode nil)
  (setq mode-line-mc-status nil)
  (setq mode-line-buffer-identification '("  ---: %15b")))

(defun timidity-make-windows ()
  (split-window nil 20)
  (other-window 1)
  (setq timidity-cmd-buffer (switch-to-buffer timidity-cmd-buffer-name))
  (lisp-interaction-mode)
  (timidity-simple-modeline)
  (erase-buffer)
  (setq timidity-msg-buffer (switch-to-buffer timidity-msg-buffer-name))
  (other-window 1)
  (setq truncate-lines t)
  (setq buffer-read-only t)
  (timidity-simple-modeline))

(defun timidity-check-run ()
  (if (and timidity-cmd-process
	   (or (null (buffer-name (process-buffer timidity-cmd-process)))
	       (eq 'exit (process-status timidity-cmd-process))))
      (setq timidity-cmd-process nil))
  timidity-cmd-process)

(defun timidity-run ()
  (setq timidity-error-flag nil)
  (if (timidity-check-run)
      timidity-cmd-process
    (let ((type (if (boundp 'MULE) "mule" "emacs")))
      (setq timidity-cmd-process
	    (apply 'start-process
		   "TiMidity"
		   timidity-cmd-buffer
		   timidity-prog-path
		   (append timidity-default-options (list "-ietv" type))))
      (set-process-filter timidity-cmd-process
			  (symbol-function 'timidity-cmd-filter))
      (set-process-sentinel timidity-cmd-process 'timidity-cmd-sentinel)
      (process-kill-without-query timidity-cmd-process))))

(defun timidity-cmd-filter (process string)
  (or timidity-error-flag
      (save-excursion
	(set-buffer timidity-cmd-buffer)
	(goto-char (point-max))
	(insert string)
	(skip-chars-backward "^\n")
	(condition-case err
	    (progn
	      (eval-region (point-min) (point))
	      (delete-region (point-min) (point)))
	  (error
	   (setq timidity-error-flag t)
	   (timidity-quit)
	   (switch-to-buffer timidity-cmd-buffer-name)
	   (error "%s" err))))))

(defun timidity-cmd-sentinel (process event)
  (if timidity-error-flag
      (message "TiMidity Error")
    (message "TiMidity ... done"))
  (setq timidity-playing-flag nil)
  (setq timidity-cmd-process nil))

(defun timidity-VERSION (str)
  (setq timidity-cmd-version str))

(defun timidity-NEXT ()
  (if timidity-next-files
      (timidity-next-file)
    (setq timidity-playing-flag nil)
    (sit-for 1)
    (timidity-demo)))

(defun timidity-CMSG (type msg)
  (save-excursion
    (let (origw msgw)
      (set-buffer timidity-msg-buffer)
      (goto-char (point-max))
      (insert msg ?\n)
      (setq msgw (get-buffer-window timidity-msg-buffer))
      (if msgw
	  (progn
	    (setq origw (selected-window))
	    (if (eq msgw origw)
		nil
	      (select-window msgw)
	      (goto-char (point-max))
	      (select-window origw)))))))

(defun timidity-TIME (time)
  (setq timidity-total-time time))

(defun timidity-DRUMS (drums)
  (setq timidity-drumchannels drums))

(defun timidity-MVOL (mv)
  (setq timidity-main-volume mv))

(defun timidity-FILE (name)
  (setq timidity-current-filename name)
  (let ((f (timidity-expand-file-name name)))
    (or (member f timidity-next-files)
	(member f timidity-prev-files)
	(setq timidity-prev-files (cons name timidity-prev-files)))))

(defun timidity-CURT (secs v)
  (set-buffer timidity-main-buffer)
  (let ((bar (/ (* 48 secs) timidity-total-time))
	(buffer-read-only nil)
	p)
    (if (> bar 48) (setq bar 48))
    (if (= bar timidity-current-time-bar)
	nil
      (timidity-update-time-bar bar)
      (setq timidity-current-time-bar bar))
    (goto-char (point-min))
    (forward-line 2)
    (skip-chars-backward "^|")
    (delete-char 12)
    (insert (format "%2d:%02d /%2d:%02d"
		    (/ secs 60)
		    (% secs 60)
		    (/ timidity-total-time 60)
		    (% timidity-total-time 60)))))

(defun timidity-NOTE (ch note stat)
  (set-buffer timidity-main-buffer)
  (let ((buffer-read-only nil))
    (goto-char (+ timidity-note-table-ofs
		  (* ch 80)
		  3
		  (% note 48)))
    (delete-char 1)
    (insert (if (eq stat 1)
		(aref "cCdDefFgGaAb" (% note 12))
	      (aref "..+_," stat)))))

(defun timidity-PROG (ch val)
  (set-buffer timidity-main-buffer)
  (let ((buffer-read-only nil))
    (goto-char (+ timidity-note-table-ofs
		  (* ch 80)
		  53))
    (delete-char 3)
    (insert (format "%03d" val))))

(defun timidity-VOL (ch val)
  (set-buffer timidity-main-buffer)
  (let ((buffer-read-only nil))
    (goto-char (+ timidity-note-table-ofs
		  (* ch 80)
		  57))
    (delete-char 3)
    (insert (format "%3d" val))))

(defun timidity-EXP (ch val)
  (set-buffer timidity-main-buffer)
  (let ((buffer-read-only nil))
    (goto-char (+ timidity-note-table-ofs
		  (* ch 80)
		  61))
    (delete-char 3)
    (insert (format "%3d" val))))

(defun timidity-PAN (ch val)
  (set-buffer timidity-main-buffer)
  (let ((buffer-read-only nil))
    (goto-char (+ timidity-note-table-ofs
		  (* ch 80)
		  65))
    (delete-char 3)
    (cond ((= val -1)		(insert "   "))
	  ((< val 5)		(insert " L "))
	  ((> val 123)		(insert " R "))
	  ((and (> val 60) (< val 68)) (insert " C "))
	  (t
	   (setq val (/ (* 100 (- val 64)) 64))
	   (if (>= val 0)
	       (insert ?+)
	     (insert ?-)
	     (setq val (- val)))
	   (insert (format "%02d" val))))))

(defun timidity-SUS (ch val)
  (set-buffer timidity-main-buffer)
  (let ((buffer-read-only nil))
    (goto-char (+ timidity-note-table-ofs
		  (* ch 80)
		  69))
    (delete-char 1)
    (insert (if (zerop val) ?\  ?S))))

(defun timidity-PIT (ch val)
  (set-buffer timidity-main-buffer)
  (let ((buffer-read-only nil))
    (goto-char (+ timidity-note-table-ofs
		  (* ch 80)
		  71))
    (delete-char 1)
    (insert (cond ((= val -1) ?=)
		  ((> val 8192) ?>)
		  ((< val 8192) ?<)
		  (t ?\  )))))

(defun timidity-RESET ()
  (set-buffer timidity-main-buffer)
  (let ((buffer-read-only nil)
	i s s1)
    (sit-for 0) ; flush
    (erase-buffer)
    (if timidity-reset-bufstr
	(insert timidity-reset-bufstr)
      (insert ?\n)
      (insert "  |>                                               |            \n")
      (insert "Ch c d ef g a bc d ef g a bc d ef g a bc d ef g a b  Prg Vol Exp Pan S B\n")
      (setq s (make-string 48 ?.))
      (setq s1 (make-string 28 ?\ ))
      (setq i 0)
      (while (< i 16)
	(insert (format "%02d " (1+ i)) s s1 ?\n)
	(setq i (1+ i)))
      (setq timidity-reset-bufstr (buffer-string)))
    (setq timidity-current-time-bar 0)
    (goto-char (point-min))
    (insert "File: " timidity-current-filename)
    (forward-line 3)
    (setq timidity-note-table-ofs (point))
    (setq i 0)
    (while (< i 16)
      (if (timidity-is-drumchannel i)
	  (progn
	    (forward-char 2)
	    (delete-char 1)
	    (insert ?*)))
      (setq i (1+ i))
      (forward-line 1))))

(defun timidity-load-file (fname)
  (interactive "FMIDI file: ")
  (if fname
      (progn
	(setq timidity-playing-flag t)
	(timidity-run)
	(setq fname (timidity-expand-file-name fname))
	(garbage-collect)
	(process-send-string timidity-cmd-process
			     (format "L\nPLAY %s\n" fname))
	(message "MIDI File: %s" fname))
    (setq timidity-playing-flag nil)))

(defun timidity-load-file-list ()
  (interactive)
  (let ((f (concat (expand-file-name "") "/"))
	files)
    (while (not (string= "" (setq f (read-file-name
				     "MIDI File: "
				     (file-name-directory
				      (timidity-expand-file-name f))
				     ""))))
      (setq files (cons (timidity-expand-file-name f) files)))
    (setq timidity-next-files (append (reverse files) timidity-next-files))
    (timidity-next-file)))

(defun timidity-next-file ()
  (interactive)
  (if timidity-next-files
      (let ((f (car timidity-next-files)))
	(setq timidity-next-files (cdr timidity-next-files))
	(setq timidity-prev-files (cons f timidity-prev-files))
	(timidity-load-file f))))

(defun timidity-prev-file ()
  (interactive)
  (if timidity-playing-flag
      (if (cdr timidity-prev-files)
	  (let ((f (car timidity-prev-files)))
	    (setq timidity-prev-files (cdr timidity-prev-files))
	    (setq timidity-next-files (cons f timidity-next-files))
	    (timidity-load-file (car timidity-prev-files)))
	(timidity-load-file (car-safe timidity-prev-files)))
    (timidity-load-file (car-safe timidity-prev-files))))

(defun timidity-replay-cmd ()
  (interactive)
  (if timidity-playing-flag
      (timidity-simple-cmd 1 ?r)
    (timidity-load-file (car-safe timidity-prev-files))))

(defun timidity-quit ()
  (interactive)
  (if (and timidity-cmd-process
	   (or (null (buffer-name (process-buffer timidity-cmd-process)))
	       (eq 'exit (process-status timidity-cmd-process))))
      (setq timidity-cmd-process nil))
  (if timidity-cmd-process
      (progn
	(process-send-string timidity-cmd-process (format "QUIT\n"))
	(setq timidity-cmd-process nil)))
  (if timidity-orig-window-configuration
      (progn
	(set-window-configuration timidity-orig-window-configuration)
	(setq timidity-orig-window-configuration nil))))

(defun timidity-simple-cmd (&optional cnt cmd)
  (interactive "p")
  (process-send-string timidity-cmd-process
		       (format "%c%d\n"
			       (if cmd cmd last-command-char)
			       (if cnt cnt 1))))

(defun timidity-select-drumchannel (ch)
  (interactive "nChannel: ")
  (if (or (not (integerp ch))
	  (< ch 1)
	  (> ch 16))
      (error "Invalid channel: %s" ch))
  (process-send-string timidity-cmd-process (format "d%d\n" (1- ch))))

(defun timidity-is-drumchannel (ch)
  (not (zerop (logand timidity-drumchannels (lsh 1 ch)))))

(defun timidity-update-time-bar (bar)
  (let (diff)
    (setq diff (- bar timidity-current-time-bar))
    (goto-char (point-min))
    (forward-line 1)
    (forward-char 3)
    (if (< 0 diff)
	(progn
	  (insert-char ?= diff)
	  (skip-chars-forward "=")
	  (if (= 48 bar)
	      (delete-char diff)
	    (forward-char 1)
	    (delete-char diff)))
      (setq diff (- diff))
      (delete-char diff)
      (skip-chars-forward "=")
      (if (eq (following-char) ?|)
	  (progn
	    (insert-char ?> 1)
	    (setq diff (1- diff))))
      (forward-char 1)
      (insert-char ?\  diff))))

(defun timidity-expand-file-name (fname)
  (if (or (string-match "^http://" fname)
	  (string-match "^ftp://" fname)
	  (string-match "^file://" fname)
	  (string-match "^news://" fname)
	  (string-match "^mime:" fname))
      fname
    (expand-file-name fname)))

(or (fboundp 'member)
    (defun member (elt lis)
      "A member function for emacs 18."
      (let ((loop t))
	(while (and loop lis)
	  (if (equal elt (car lis))
	      (setq loop nil)
	    (setq lis (cdr lis))))
	lis)))

(defvar timidity-demo-string
'("#######   ###   #     #   ###   ######    ###   ####### #     #"
  "   #       #    ##   ##    #    #     #    #       #     #   # "
  "   #       #    # # # #    #    #     #    #       #      # #  "
  "   #       #    #  #  #    #    #     #    #       #       #   "
  "   #       #    #     #    #    #     #    #       #       #   "
  "   #       #    #     #    #    #     #    #       #       #   "
  "   #      ###   #     #   ###   ######    ###      #       #   "
  "              TiMidity emacs interface written by Masanao Izumo"
  "                                                               "
  "   l/L: Load & Play a MIDI file                                "
  "   n/p: Next/Prev file                                         "
  "   r  : Replay a MIDI file                                     "
  "   q: Quit                                                     "
))

(defun timidity-demo ()
  (interactive)
  (set-buffer timidity-main-buffer)
  (let ((buffer-read-only nil)
	(win-width  (- (window-width) 3))
	(win-height (window-height))
	(str-width  (length (car timidity-demo-string)))
	(str-height (length timidity-demo-string))
	start-point i j k evenp sline c)
    (erase-buffer)
    (if (<= win-height str-height)
	(setq win-height str-height))
    (if (<= win-width str-width)
	(setq win-width str-width))
    (insert-char ?\n win-width)
    (goto-line (/ (- win-height str-height) 2))

    (if (= (% (- win-width str-width) 2) 1)
	(setq win-width (1- win-width)))

    (setq start-point (point)
	  i 0
	  evenp nil)
    (while (< i str-height)
      (if evenp (insert-char ?\  win-width))
      (forward-line 1)
      (setq evenp (not evenp)
	    i (1+ i)))

    (setq j 0)
    (while (< j str-width)
      (goto-char start-point)
      (setq evenp nil
	    i 0
	    sline timidity-demo-string)
      (while (< i str-height)
	(if evenp
	    (progn
	      (delete-char 1)
	      (end-of-line)
	      (insert-char (aref (car sline) j) 1))
	  (insert-char (aref (car sline) (- str-width j 1)) 1))
	(setq i (1+ i)
	      evenp (not evenp)
	      sline (cdr sline))
	(forward-line 1))
      (setq j (1+ j))
      (sit-for 0.01))
    (setq j 0
	  k (/ (- win-width str-width) 2))
    (while (< j k)
      (goto-char start-point)
      (setq evenp nil
	    i 0
	    sline timidity-demo-string)
      (while (< i str-height)
	(if evenp
	    (delete-char 1)
	  (insert-char ?\  1))
	(setq i (1+ i)
	      evenp (not evenp)
	      sline (cdr sline))
	(forward-line 1))
      (setq j (1+ j))
      (sit-for 0.01))))

(mapcar (function (lambda (symbol)
		    (if (byte-code-function-p (symbol-function symbol))
			nil
		      (message "Byte compiling %s..." symbol)
		      (byte-compile symbol))))
	timidity-pre-byte-compiles)

(provide 'timidity)
