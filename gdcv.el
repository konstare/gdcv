(require 'gdcv-elisp)


(defvar ivy-more-chars-alist)

(defvar gdcv-index-path "~/.config/gdcv"
  "Path to index files formed by gdcv -i. Usually ~/.config/gdcv/")

(defvar gdcv-buffer "*gdcv*"
  "Buffer for translation")

(defvar gdcv-db nil
  "Current state of gdcv (handle pointer to database and dictionaries).")

(defvar gdcv-default-dictionary-list nil
  "If such dictionary exists, open it")


(defvar gdcv-image-regex-alist "\\(?:\\.\\(?:BMP\\|GIF\\|JP\\(?:E?G\\)\\|bmp\\|gif\\|jp\\(?:e?g\\)\\)\\)"
  "Regex to determine image files (\".jpg\" \".bmp\" ans so on)  ")
(defvar gdcv-media-regex-alist "\\(?:\\.\\(?:AVI\\|MP3\\|OGG\\|WAV\\|avi\\|mp3\\|ogg\\|wav\\)\\)"
  "Regex to determine audio and video files (\".wav\" \".avi\" ans so on)  " )
(defvar gdcv-media-temp-directory "/tmp/gdcv/"
  "Where save temporary files ")
(defvar gdcv-media-icon "🔊"
  "Icon for media files (\".wav\" \".avi\" ans so on)")

(defvar gdcv-play-media
  (lambda (file)
    (let ((process-connection-type nil))
      (start-process "" nil  "xdg-open"  file)))
  "Function to play all media files with click. The default is just xdg-open")

(defun gdcv--shutdown ()
  "Clean up resources used by the gdcv."
  (when gdcv-db
    (gdcv-database-close gdcv-db)
    (setq gdcv-db nil)))

(defun gdcv--handler ()
  "Clean up resources used by the gdcv."
  (unless gdcv-db
    (setq gdcv-db (gdcv-database-open   (file-truename gdcv-index-path)))))




(require 'subr-x)

(defun gdcv--look-word (word)
  (if (equal (substring word 0 1) "*")
      (gdcv-look-word gdcv-db (string-remove-prefix  "*" word) 0 )
    (gdcv-look-word gdcv-db  word 1 )))


(defun gdcv-choose-defs (word)
  (gdcv--handler)
  (let (articles)
    (gdcv-goto-gdcv)
    (setq buffer-read-only nil)
    (erase-buffer)

    (dolist (x (gdcv-word-defs gdcv-db word) articles)
      (if (member (car x) gdcv-default-dictionary-list)
	  (setq articles (cons x articles))
	(setq articles (nconc  articles (list x)))))

    (insert (concat "Word: " word))

    (dolist (article articles)
      (insert "\n")
      (let* (
	     (name (concat "* " (car article) "\n") )
	     (dic (nth 1 article))
	     (start (nth 2 article))
	     (end (nth 3 article))
	     (resources (nth 4 article)))
	(add-text-properties 0 (length name)
			     `(face font-lock-type-face Dic ,dic Start ,start End ,end Definition nil Folded nil Resources ,resources ) name)
	(insert name)))

    (insert "\n")
    (let ((name  "* Suggestions\n"))
      (add-text-properties 0 (length name)
    			   `(face font-lock-type-face Dic nil Start nil End nil Definition t Folded nil ) name)
      (insert name))
    (dolist (w (gdcv--look-word (concat "*" word)))
      (add-text-properties 0 (length w) (nconc
					 (list 'mouse-face 'highlight  'help-echo "Follow")
					 (list 'face `(:foreground "DeepSkyBlue"))
					 `(link t))  w)
      (insert (concat   w "\n")))
    
    (gdcv-mode)
    (goto-char (point-min))
    (outline-next-heading)
    (gdcv-show-entry)))

(defun gdcv-show-entry ()
  (interactive)
  (setq buffer-read-only nil)
  (outline-back-to-heading)
  (let ((p1 (point)) p2)
    (save-excursion (end-of-line) (setq p2 (point)))
    (if (get-text-property  p1 'Folded)
  	(progn (outline-hide-entry)
  	       (put-text-property p1 p2 'Folded nil))
      (outline-show-entry)
      (put-text-property p1 p2 'Folded t)
      (unless (get-text-property  p1 'Definition)
  	(put-text-property p1 p2 'Definition t)
  	(let* ((dic (get-text-property  p1 'Dic))
  	       (start (get-text-property  p1 'Start))
  	       (end (get-text-property  p1 'End))
	       (resources (get-text-property  p1 'Resources))
  	       (data (when dic (gdcv-word-fetch gdcv-db dic start end (file-name-as-directory gdcv-media-temp-directory) ))) 
	       (article  (car data))
	       )
	  (forward-line)
	  (dolist (property (car (cdr data)))
	    (let ((type (nth 0 property))
		  (S (nth 1 property))
		  (E (nth 2 property))
		  (P (nth 3 property)))
	      (cond
	       ((equal type 0)
		(add-face-text-property S E '(:weight bold) t article))
	       ((equal type 1)
		(add-face-text-property S E '(:slant italic) t article))
	       ((equal type 2)
		(add-face-text-property S E '(:underline t) t article))
	       ((or (equal type 3) (equal type 6) (equal type 7))
		(add-face-text-property S E '(:weight ultralight) t article))
	       ((or (equal type 8) (equal type 13) )
		(add-text-properties S E (nconc
					  (list 'mouse-face 'highlight  'help-echo "Follow")
					  (list 'face `(:foreground "DeepSkyBlue"))
					  `(link t))  article)
		)
	       ((equal type 10) 
		(add-face-text-property S E '(:foreground "green") t article))
	       ((or (equal type 14)  (equal type 15) )
		(add-face-text-property S E '(:height 0.6) t article))
	       ((equal type 16)
		(unless P (setq P "green"))
		(unless (or (equal P "green") (equal P "red") (setq P "green")))
		(add-face-text-property S E `(:foreground ,P) t article))
	       
	       ((equal type 11) 
		(add-face-text-property S E '(:slant italic :weight bold :foreground "red") t article))
	       ((and (equal type 17) P resources)

		(setq P (concat (file-name-as-directory gdcv-media-temp-directory) P))

		(when (string-match-p gdcv-image-regex-alist P)		  
		  (add-text-properties S E (list 'display (create-image P)) article))

		(when (string-match-p gdcv-media-regex-alist P)
		  (add-text-properties S E (nconc
					    (list 'mouse-face 'highlight  'help-echo "Play")
					    `(play-file ,P)
					    (list 'display gdcv-media-icon))
				       article)))
	       (t ))))
	  (insert article)  	  
	  )))
    (goto-char p1))
  (setq buffer-read-only t))



(defun gdcv-media-position (pos)
  (let ((file (get-text-property  pos 'play-file))
	(link (get-text-property  pos 'link))
	(Dic (get-text-property  pos 'Dic)))
    (when file
      (funcall gdcv-play-media file))
    (when link
      (let ((p1 (previous-property-change (1+ pos)))
	    (p2 (next-property-change pos )))
	(gdcv-search-word  (buffer-substring-no-properties p1 p2))))
    (when Dic
      (gdcv-show-entry))
    ))


(defun gdcv-click-media (event)
  (interactive "e")
  (let ((pos (posn-point (event-end event))))
    (gdcv-media-position pos)))

(defun gdcv-press-media ()
  (interactive)
  (gdcv-media-position (point)))


(defvar gdcv-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map [mouse-1]  'gdcv-click-media)
    (define-key map "q" 'gdcv-return-from-gdcv)
    (define-key map (kbd "<tab>")  'gdcv-show-entry)
    (define-key map (kbd "<return>")  'gdcv-press-media)
    (define-key map (kbd "SPC")  'scroll-up)
    (define-key map (kbd "d")  'gdcv-search-word)
    map)
  "Keymap for `gdcv-mode'.")


(define-derived-mode gdcv-mode nil "GDCV"
  "Major mode to look up word through gdcv."
  ;;(setq font-lock-defaults '(gdcv-mode-font-lock-keywords))
  (setq font-lock-mode nil)
  (outline-minor-mode)
  (outline-hide-body)
  (setq buffer-read-only t)
  )


(defvar gdcv-previous-window-conf nil
  "Window configuration before switching to sdcv buffer.")


(defun gdcv-get-buffer ()
  "Get the gdcv buffer. Create one if there's none."
  (let ((buffer (get-buffer-create gdcv-buffer)))
    (with-current-buffer buffer
      (unless (eq major-mode 'gdcv-mode)
        (gdcv-mode)))
    buffer))


(defun gdcv-goto-gdcv ()
  "Switch to gdcv buffer in other window."
  (interactive)
  (unless (eq (current-buffer)
	      (gdcv-get-buffer))
    (setq gdcv-previous-window-conf (current-window-configuration)))
  (let* ((buffer (gdcv-get-buffer))
         (window (get-buffer-window buffer)))
    (if (null window)
        (switch-to-buffer-other-window buffer)
      (select-window window))))

(defun gdcv-return-from-gdcv ()
  "Bury gdcv buffer and restore the previous window configuration."
  (interactive)
  (if (window-configuration-p gdcv-previous-window-conf)
      (progn
        (set-window-configuration gdcv-previous-window-conf)
        (setq gdcv-previous-window-conf nil)
        (bury-buffer (gdcv-get-buffer)))
    (bury-buffer)))



(setf (alist-get 'ivy-gdcv ivy-more-chars-alist) 1)




(defun ivy-gdcv ()
  "Use ivy-read to select your search history."
  (interactive)
  (gdcv--handler)
  
  (ivy-read "Search gdcv: " #'(lambda (word )
				(or
				 (ivy-more-chars)
				 (gdcv--look-word word)
				 ))
	    :dynamic-collection t
	    :action #'gdcv-choose-defs
	    :caller 'ivy-gdcv))


(defun gdcv-search-word (word)
  (interactive "P")
  (unless word
    (setq word (if (and transient-mark-mode mark-active)
		   (buffer-substring-no-properties (region-beginning) (region-end))
		 (current-word nil t)))
    (unless word

      ;;(setq word (read-string (format "Search the dictionary for: " )))
      (ivy-gdcv)
      ))
  (when word
    (gdcv-choose-defs word)))

(provide 'gdcv)
