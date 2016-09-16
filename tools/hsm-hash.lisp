(spark-init)
(require :ergolib)
(require :hashlib)
(require :basen)

(define-class serial-device path stream)

(define-print-method (serial-device path self.stream) "#<Serial-device ~A ~S>" path
  (if (and (streamp self.stream) (open-stream-p self.stream)) :open :closed))

(define-method (close (s two-way-stream) &key abort)
  (close (two-way-stream-input-stream s) :abort abort)
  (close (two-way-stream-output-stream s) :abort abort))

(define-method (close (sd serial-device stream) &key abort)
  (close stream :abort abort))

(define-method (reopen (sd serial-device path stream))
  (if (and (streamp stream) (open-stream-p stream)) (close stream))
  ; Have to use external-format :latin-1 instead of :element-type 'octet
  ; because CL doesn't have read-byte-no-hang
  ; TODO: Try using "ccl:library;serial-streams.lisp"
  (bb fin (open path :sharing :lock :external-format :latin-1)
      fout (open path :sharing :lock :external-format :latin-1
                 :direction :output :if-exists :overwrite)
      (setf stream (make-two-way-stream fin fout))
      sd))

(defun make-serial-device (path)
  (reopen (make-instance 'serial-device :path path)))

(defun read-byte-no-hang (stream)
  (aif (read-char-no-hang stream) (char-code it) nil))

(define-method (iterator (sd serial-device stream))
  (fn ()
    (or (read-byte-no-hang stream) (sleep 0.1) (read-byte-no-hang stream) (iterend))))

(define-method (out (sd serial-device stream) (s string))
  (write-sequence s stream)
  (force-output stream))

(define-method (out (sd serial-device) (v sequence))
  (out sd (bytes-to-string v :latin-1)))

(define-method (in (sd serial-device stream))
  (bytes-to-string (force sd) :latin-1))

(defv $hsm (make-serial-device (1st (directory "/dev/cu.usbmodem*"))))

#+NIL
(system (fmt "lsof ~A" (1st (directory "/dev/cu.usbmodem*"))))

(define-method (hsm-hash (hsm serial-device stream) (v sequence))
  (out hsm (fmt "h~A~%" (length v)))
  (for s in (slices 128 v) do
    (read-char stream)
    (out hsm s))
  (sleep 0.1)
  (bb h1 (b58 (sha512 (coerce v 'vector)))
      h2 (slice (in hsm) 0 (length h1))
      (if (equal h1 h2)
        (for s in (slices 21 h1) do
          (princ s)
          (terpri)
          t)
        (values nil h1 h2))))

(define-method (hsm-hash (hsm serial-device) (p pathname))
  (hsm-hash hsm (file-contents p :binary)))

#+NIL
(hsm-hash $hsm #P"~/devel/spark/sc4/sc4-hsm/tools/term")
