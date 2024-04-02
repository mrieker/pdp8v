; split string into list of words starting at given index
(define split
    (lambda (s i)
        (begin
            (define j (nspacepos s i))
            (define k (spacepos s j))
            (if (<= k j) empty
                (cons (substr s j k) (split s k))))))

; find next space char at or after the given index
; if not found, return string length
(define spacepos
    (lambda (s i)
        (if (= (strlen s) i) i
            (if (<= (int (charat i s)) 32) i
                (spacepos s (+ i 1))))))

; find next non-space char at or after the given index
; if not found, return string length
(define nspacepos
    (lambda (s i)
        (if (= (strlen s) i) i
            (if (> (int (charat i s)) 32) i
                (nspacepos s (+ i 1))))))

