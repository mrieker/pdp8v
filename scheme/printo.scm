
; convert the given integer to octal string
(define printo
    (lambda (i)
        (cond
            ((< i) (string #\- (printo (- i))))
            ((= i) "0")
            (#T (string (printo (>> i 3)) (& i 7))))))

; convert the given integer to hex string
(define printx (lambda (i) (if (< i) (string "-0x" (hexdigits (- i))) (string "0x" (hexdigits i)))))

(define hexdigits
    (lambda (i)
        (cond
            ((<= i  9) (char (+ i 48)))
            ((<= i 15) (char (+ i 55)))
            (#T (string (hexdigits (>> i 4)) (hexdigits (& i 15)))))))

