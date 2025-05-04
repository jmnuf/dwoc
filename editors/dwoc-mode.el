;; This is basically just ripped from SimpC Mode: https://github.com/rexim/simpc-mode
;; With some few tweaks to abide to my likes and needs
;; Emacs 24.4+ needed I believe

(require 'subr-x)

(defvar dwoc-mode-syntax-table
  (let ((table (make-syntax-table)))
    ;; C style comments
	(modify-syntax-entry ?/ ". 124b" table)
	(modify-syntax-entry ?* ". 23" table)
	(modify-syntax-entry ?\n "> b" table)
    ;; Chars are the same as strings
    (modify-syntax-entry ?' "\"" table)
    ;; Treat <> as punctuation
    (modify-syntax-entry ?< "." table)
    (modify-syntax-entry ?> "." table)

    (modify-syntax-entry ?& "." table)
    (modify-syntax-entry ?% "." table)
    table))

(defun dwoc-types ()
  '())

(defun dwoc-keywords ()
  '("fn" "use" "let"))

(defun dwoc-font-lock-keywords ()
  (list
   `("use *\\(\\\".*\\\"\\);" . (1 font-lock-string-face))
   `(,(regexp-opt (dwoc-keywords) 'symbols) . font-lock-keyword-face)
   `(,"[0-9]+\\(?:\\.[0-9]+\\)?\\(e[+-]?[0-9]+\\)?" . 'font-lock-constant-face)
   `(,(regexp-opt (dwoc-types) 'symbols) . font-lock-type-face)))

(defun dwoc--previous-non-empty-line ()
  (save-excursion
    (forward-line -1)
    (while (and (not (bobp))
                (string-empty-p
                 (string-trim-right
                  (thing-at-point 'line t))))
      (forward-line -1))
    (thing-at-point 'line t)))

(defun dwoc--indentation-of-previous-non-empty-line ()
  (save-excursion
    (forward-line -1)
    (while (and (not (bobp))
                (string-empty-p
                 (string-trim-right
                  (thing-at-point 'line t))))
      (forward-line -1))
    (current-indentation)))

(defun dwoc--desired-indentation ()
  (let* ((cur-line (string-trim-right (thing-at-point 'line t)))
         (prev-line (string-trim-right (dwoc--previous-non-empty-line)))
         (indent-len 2)
         (prev-indent (dwoc--indentation-of-previous-non-empty-line)))
    (cond
     ((and (string-suffix-p "{" prev-line)
           (string-prefix-p "}" (string-trim-left cur-line)))
      prev-indent)
     ((string-suffix-p "{" prev-line)
      (+ prev-indent indent-len))
     ((string-prefix-p "}" (string-trim-left cur-line))
      (max (- prev-indent indent-len) 0))
     ((string-suffix-p ":" prev-line)
      (if (string-suffix-p ":" cur-line)
          prev-indent
        (+ prev-indent indent-len)))
     ((string-suffix-p ":" cur-line)
      (max (- prev-indent indent-len) 0))
     (t prev-indent))))

;;; TODO: customizable indentation (amount of spaces, tabs, etc)
(defun dwoc-indent-line ()
  (interactive)
  (when (not (bobp))
    (let* ((desired-indentation
            (dwoc--desired-indentation))
           (n (max (- (current-column) (current-indentation)) 0)))
      (indent-line-to desired-indentation)
      (forward-char n))))

(define-derived-mode dwoc-mode prog-mode "Dwo Code"
  "Simple major mode for editing C files."
  :syntax-table dwoc-mode-syntax-table
  (setq-local font-lock-defaults '(dwoc-font-lock-keywords))
  (setq-local indent-line-function 'dwoc-indent-line)
  (setq-local comment-start "// "))

(provide 'dwoc-mode)
